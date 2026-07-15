import { SpriteFlags, SnapshotView, type SnapshotClassicSurface } from "../simulation/snapshot";
import { runtimePerformanceMetrics } from "../performance/runtimeMetrics";
import type { AccumulatedClassicSurface } from "./ClassicSurfaceAccumulator";
import { fitViewport, visibleWorldRect } from "./viewport";

export type GraphicsMode = "classic" | "remastered";

export interface RendererOptions {
  maximumDevicePixelRatio?: number;
  resolutionScale?: number;
  onContextLost?: () => void;
  onContextRestored?: () => void;
}

export interface AtlasPageUpload {
  id: number;
  source: TexImageSource;
  nearest?: boolean;
  premultiplyAlpha?: boolean;
}

export interface CameraTransform {
  x: number;
  y: number;
  zoom: number;
}

interface ProgramBundle {
  program: WebGLProgram;
  uniforms: Record<string, WebGLUniformLocation>;
}

interface RenderResources {
  classic: ProgramBundle;
  sprite: ProgramBundle;
  quad: WebGLBuffer;
  instances: WebGLBuffer;
  classicTexture: WebGLTexture;
  paletteTexture: WebGLTexture;
  fallbackTexture: WebGLTexture;
}

interface PreparedSpriteBatch {
  pageId: number;
  start: number;
  count: number;
}

const CLASSIC_VERTEX = `#version 300 es
layout(location = 0) in vec2 aCorner;
uniform vec4 uUvRect;
out vec2 vUv;
void main() {
  vUv = mix(uUvRect.xy, uUvRect.zw, vec2(aCorner.x, 1.0 - aCorner.y));
  gl_Position = vec4(aCorner * 2.0 - 1.0, 0.0, 1.0);
}`;

const CLASSIC_FRAGMENT = `#version 300 es
precision highp float;
precision highp int;
uniform sampler2D uIndexed;
uniform sampler2D uPalette;
in vec2 vUv;
out vec4 outColor;
void main() {
  float sampleValue = texture(uIndexed, vUv).r;
  int paletteIndex = int(floor(sampleValue * 255.0 + 0.5));
  outColor = texelFetch(uPalette, ivec2(paletteIndex, 0), 0);
}`;

const SPRITE_VERTEX = `#version 300 es
precision highp float;
layout(location = 0) in vec2 aCorner;
layout(location = 1) in vec4 aRect;
layout(location = 2) in vec4 aUvRect;
layout(location = 3) in vec4 aTint;
layout(location = 4) in float aFlags;
uniform vec2 uWorldSize;
uniform vec2 uCamera;
uniform float uZoom;
out vec2 vUv;
out vec4 vTint;
flat out int vFlags;
void main() {
  vec2 world = aRect.xy + aCorner * aRect.zw;
  vec2 screen = (world - uCamera) * uZoom;
  vec2 clip = vec2(screen.x / uWorldSize.x * 2.0 - 1.0, 1.0 - screen.y / uWorldSize.y * 2.0);
  gl_Position = vec4(clip, 0.0, 1.0);
  vUv = mix(aUvRect.xy, aUvRect.zw, aCorner);
  vTint = aTint;
  vFlags = int(aFlags + 0.5);
}`;

const SPRITE_FRAGMENT = `#version 300 es
precision highp float;
precision highp int;
uniform sampler2D uAtlas;
in vec2 vUv;
in vec4 vTint;
flat in int vFlags;
out vec4 outColor;
void main() {
  vec2 uv = vUv;
  if ((vFlags & ${SpriteFlags.FlipX}) != 0) uv.x = 1.0 - uv.x;
  if ((vFlags & ${SpriteFlags.FlipY}) != 0) uv.y = 1.0 - uv.y;
  vec4 color = texture(uAtlas, uv) * vTint;
  if ((vFlags & ${SpriteFlags.Shadow}) != 0) color.rgb *= 0.35;
  if ((vFlags & ${SpriteFlags.Translucent}) != 0) color.a *= 0.55;
  if ((vFlags & ${SpriteFlags.Selected}) != 0) {
    vec2 edgeDistance = min(uv, 1.0 - uv);
    if (min(edgeDistance.x, edgeDistance.y) < 0.055) color.rgb = vec3(0.72, 1.0, 0.42);
  }
  if (color.a < 0.01) discard;
  outColor = color;
}`;

function shader(gl: WebGL2RenderingContext, type: number, source: string): WebGLShader {
  const result = gl.createShader(type);
  if (!result) throw new Error("Could not create WebGL shader");
  gl.shaderSource(result, source);
  gl.compileShader(result);
  if (!gl.getShaderParameter(result, gl.COMPILE_STATUS)) {
    const log = gl.getShaderInfoLog(result) || "unknown compile error";
    gl.deleteShader(result);
    throw new Error(`WebGL shader compilation failed: ${log}`);
  }
  return result;
}

function program(gl: WebGL2RenderingContext, vertexSource: string, fragmentSource: string, uniforms: readonly string[]): ProgramBundle {
  const vertex = shader(gl, gl.VERTEX_SHADER, vertexSource);
  const fragment = shader(gl, gl.FRAGMENT_SHADER, fragmentSource);
  const result = gl.createProgram();
  if (!result) throw new Error("Could not create WebGL program");
  gl.attachShader(result, vertex);
  gl.attachShader(result, fragment);
  gl.linkProgram(result);
  gl.deleteShader(vertex);
  gl.deleteShader(fragment);
  if (!gl.getProgramParameter(result, gl.LINK_STATUS)) {
    const log = gl.getProgramInfoLog(result) || "unknown link error";
    gl.deleteProgram(result);
    throw new Error(`WebGL program linking failed: ${log}`);
  }
  const locations: Record<string, WebGLUniformLocation> = {};
  for (const name of uniforms) {
    const location = gl.getUniformLocation(result, name);
    if (!location) {
      gl.deleteProgram(result);
      throw new Error(`WebGL program is missing uniform ${name}`);
    }
    locations[name] = location;
  }
  return { program: result, uniforms: locations };
}

function texture(gl: WebGL2RenderingContext): WebGLTexture {
  const result = gl.createTexture();
  if (!result) throw new Error("Could not create WebGL texture");
  return result;
}

function unpackColor(value: number): [number, number, number, number] {
  return [(value & 0xff) / 255, ((value >>> 8) & 0xff) / 255, ((value >>> 16) & 0xff) / 255, ((value >>> 24) & 0xff) / 255];
}

/** Owns the baseline requirement and upload decisions for the indexed WebGL texture. */
export class ClassicSurfaceTextureUploader {
  private width = 0;
  private height = 0;
  private hasBaseline = false;
  private snapshotBuffer?: ArrayBuffer;

  upload(
    gl: WebGL2RenderingContext,
    update: SnapshotClassicSurface,
    snapshotBuffer: ArrayBuffer,
    recoveredBaseline?: AccumulatedClassicSurface,
  ): boolean {
    if (this.snapshotBuffer === snapshotBuffer) return this.hasBaseline && this.width === update.width && this.height === update.height;
    if (update.format === 1) {
      this.uploadBaseline(gl, update.width, update.height, update.pixels, snapshotBuffer);
      return true;
    }
    if (!this.hasBaseline || this.width !== update.width || this.height !== update.height) {
      this.width = 0;
      this.height = 0;
      this.hasBaseline = false;
      this.snapshotBuffer = undefined;
      if (recoveredBaseline && recoveredBaseline.width === update.width && recoveredBaseline.height === update.height) {
        this.uploadBaseline(gl, recoveredBaseline.width, recoveredBaseline.height, recoveredBaseline.pixels, snapshotBuffer);
        return true;
      }
      return false;
    }
    if (update.rectWidth > 0) {
      gl.texSubImage2D(
        gl.TEXTURE_2D,
        0,
        update.rectX,
        update.rectY,
        update.rectWidth,
        update.rectHeight,
        gl.RED,
        gl.UNSIGNED_BYTE,
        update.pixels,
      );
    }
    this.snapshotBuffer = snapshotBuffer;
    return true;
  }

  isReady(width: number, height: number): boolean {
    return this.hasBaseline && this.width === width && this.height === height;
  }

  hasIngested(snapshotBuffer: ArrayBuffer): boolean {
    return this.snapshotBuffer === snapshotBuffer;
  }

  reset(): void {
    this.width = 0;
    this.height = 0;
    this.hasBaseline = false;
    this.snapshotBuffer = undefined;
  }

  private uploadBaseline(gl: WebGL2RenderingContext, width: number, height: number, pixels: Uint8Array, snapshotBuffer: ArrayBuffer): void {
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.R8, width, height, 0, gl.RED, gl.UNSIGNED_BYTE, pixels);
    this.width = width;
    this.height = height;
    this.hasBaseline = true;
    this.snapshotBuffer = snapshotBuffer;
  }
}

export class WebGLRenderer {
  private readonly canvas: HTMLCanvasElement;
  private readonly gl: WebGL2RenderingContext;
  private readonly options: Required<Pick<RendererOptions, "maximumDevicePixelRatio" | "resolutionScale">> & RendererOptions;
  private resources!: RenderResources;
  private readonly atlasPages = new Map<number, WebGLTexture>();
  private readonly classicTextureUploader = new ClassicSurfaceTextureUploader();
  private uploadedPalette?: Uint8Array;
  private paletteSnapshotBuffer?: ArrayBuffer;
  private spriteSnapshotBuffer?: ArrayBuffer;
  private preparedSpriteBatches: PreparedSpriteBatch[] = [];
  private renderedSnapshotBuffer?: ArrayBuffer;
  private renderedMode?: GraphicsMode;
  private renderedCameraX = Number.NaN;
  private renderedCameraY = Number.NaN;
  private renderedCameraZoom = Number.NaN;
  private contextLost = false;
  private destroyed = false;

  constructor(canvas: HTMLCanvasElement, options: RendererOptions = {}) {
    this.canvas = canvas;
    this.options = {
      maximumDevicePixelRatio: options.maximumDevicePixelRatio ?? 2,
      resolutionScale: options.resolutionScale ?? 1,
      ...options,
    };
    const gl = canvas.getContext("webgl2", {
      alpha: false,
      antialias: false,
      depth: false,
      stencil: false,
      powerPreference: "high-performance",
      preserveDrawingBuffer: false,
    });
    if (!gl) throw new Error("WebGL 2 is required to play");
    this.gl = gl;
    this.canvas.addEventListener("webglcontextlost", this.onContextLost);
    this.canvas.addEventListener("webglcontextrestored", this.onContextRestored);
    this.initializeResources();
  }

  resize(): boolean {
    const dpr = Math.min(window.devicePixelRatio || 1, this.options.maximumDevicePixelRatio);
    const width = Math.max(1, Math.round(this.canvas.clientWidth * dpr * this.options.resolutionScale));
    const height = Math.max(1, Math.round(this.canvas.clientHeight * dpr * this.options.resolutionScale));
    if (this.canvas.width === width && this.canvas.height === height) return false;
    this.canvas.width = width;
    this.canvas.height = height;
    return true;
  }

  uploadAtlasPage(upload: AtlasPageUpload): void {
    if (!Number.isInteger(upload.id) || upload.id < 0 || upload.id > 0xffff) throw new RangeError("Atlas page ID must be an unsigned 16-bit integer");
    const gl = this.gl;
    const existing = this.atlasPages.get(upload.id);
    const page = existing ?? texture(gl);
    gl.bindTexture(gl.TEXTURE_2D, page);
    gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, upload.premultiplyAlpha ? 1 : 0);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, upload.source);
    const filter = upload.nearest ? gl.NEAREST : gl.LINEAR;
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, filter);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, filter);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, 0);
    this.atlasPages.set(upload.id, page);
  }

  removeAtlasPage(id: number): void {
    const page = this.atlasPages.get(id);
    if (!page) return;
    this.gl.deleteTexture(page);
    this.atlasPages.delete(id);
  }

  render(snapshot: SnapshotView, mode: GraphicsMode, camera: CameraTransform = { x: 0, y: 0, zoom: 1 }): void {
    if (this.contextLost || this.destroyed) return;
    // Keep render() useful for standalone callers. App ingests in onSnapshot so
    // intermediate deltas are not lost when requestAnimationFrame skips a draw.
    this.ingestClassicSurface(snapshot);
    const resized = this.resize();
    if (!resized
      && this.renderedSnapshotBuffer === snapshot.buffer
      && this.renderedMode === mode
      && this.renderedCameraX === camera.x
      && this.renderedCameraY === camera.y
      && this.renderedCameraZoom === camera.zoom) return;
    const gl = this.gl;
    gl.viewport(0, 0, this.canvas.width, this.canvas.height);
    gl.clear(gl.COLOR_BUFFER_BIT);
    if (mode === "classic") this.renderClassic(snapshot, camera);
    else this.renderSprites(snapshot, camera);
    this.renderedSnapshotBuffer = snapshot.buffer;
    this.renderedMode = mode;
    this.renderedCameraX = camera.x;
    this.renderedCameraY = camera.y;
    this.renderedCameraZoom = camera.zoom;
  }

  ingestClassicSurface(snapshot: SnapshotView, recoveredBaseline?: AccumulatedClassicSurface): void {
    if (this.contextLost || this.destroyed) return;
    // App ingests each worker delivery synchronously, while render() may run
    // several times for the same snapshot. Avoid rebinding GL state on those
    // duplicate animation frames without compromising ordered delta uploads.
    if (this.classicTextureUploader.hasIngested(snapshot.buffer)) return;
    const update = snapshot.classicSurface;
    if (!update) return;
    const gl = this.gl;
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, this.resources.classicTexture);
    const hadBaseline = this.classicTextureUploader.isReady(update.width, update.height);
    if (!this.classicTextureUploader.upload(gl, update, snapshot.buffer, recoveredBaseline)) return;
    if (update.format === 1 || (!hadBaseline && recoveredBaseline)) {
      runtimePerformanceMetrics.recordClassicUpload("baseline", update.width * update.height);
    } else if (update.rectWidth === 0) {
      runtimePerformanceMetrics.recordClassicUpload("unchanged", 0);
    } else {
      runtimePerformanceMetrics.recordClassicUpload("delta", update.rectWidth * update.rectHeight);
    }
  }

  destroy(): void {
    if (this.destroyed) return;
    this.destroyed = true;
    this.canvas.removeEventListener("webglcontextlost", this.onContextLost);
    this.canvas.removeEventListener("webglcontextrestored", this.onContextRestored);
    for (const page of this.atlasPages.values()) this.gl.deleteTexture(page);
    this.atlasPages.clear();
    this.deleteResources();
  }

  private initializeResources(): void {
    const gl = this.gl;
    const quad = gl.createBuffer();
    const instances = gl.createBuffer();
    if (!quad || !instances) throw new Error("Could not allocate WebGL buffers");
    gl.bindBuffer(gl.ARRAY_BUFFER, quad);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1]), gl.STATIC_DRAW);
    const fallbackTexture = texture(gl);
    gl.bindTexture(gl.TEXTURE_2D, fallbackTexture);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 1, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array([255, 255, 255, 255]));
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    this.resources = {
      classic: program(gl, CLASSIC_VERTEX, CLASSIC_FRAGMENT, ["uIndexed", "uPalette", "uUvRect"]),
      sprite: program(gl, SPRITE_VERTEX, SPRITE_FRAGMENT, ["uAtlas", "uWorldSize", "uCamera", "uZoom"]),
      quad,
      instances,
      classicTexture: texture(gl),
      paletteTexture: texture(gl),
      fallbackTexture,
    };
    gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);
    gl.disable(gl.DEPTH_TEST);
    gl.disable(gl.CULL_FACE);
    gl.clearColor(0.025, 0.04, 0.032, 1);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, this.resources.classicTexture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.bindTexture(gl.TEXTURE_2D, this.resources.paletteTexture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    this.classicTextureUploader.reset();
    this.uploadedPalette = undefined;
    this.paletteSnapshotBuffer = undefined;
    this.spriteSnapshotBuffer = undefined;
    this.preparedSpriteBatches = [];
    this.renderedSnapshotBuffer = undefined;
    this.renderedMode = undefined;
    this.renderedCameraX = Number.NaN;
    this.renderedCameraY = Number.NaN;
    this.renderedCameraZoom = Number.NaN;
  }

  private deleteResources(): void {
    const gl = this.gl;
    if (!this.resources) return;
    gl.deleteProgram(this.resources.classic.program);
    gl.deleteProgram(this.resources.sprite.program);
    gl.deleteBuffer(this.resources.quad);
    gl.deleteBuffer(this.resources.instances);
    gl.deleteTexture(this.resources.classicTexture);
    gl.deleteTexture(this.resources.paletteTexture);
    gl.deleteTexture(this.resources.fallbackTexture);
  }

  private bindQuad(): void {
    const gl = this.gl;
    gl.bindBuffer(gl.ARRAY_BUFFER, this.resources.quad);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 8, 0);
    gl.vertexAttribDivisor(0, 0);
  }

  private renderClassic(snapshot: SnapshotView, camera: CameraTransform): void {
    const surface = snapshot.classicSurface;
    const palette = snapshot.palette;
    if (!surface || !palette || !this.classicTextureUploader.isReady(surface.width, surface.height)) return;
    const gl = this.gl;
    const { classic } = this.resources;
    gl.useProgram(classic.program);
    this.bindQuad();
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, this.resources.classicTexture);
    gl.uniform1i(classic.uniforms.uIndexed, 0);
    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_2D, this.resources.paletteTexture);
    if (this.paletteSnapshotBuffer !== snapshot.buffer) {
      const paletteChanged = !this.uploadedPalette || this.uploadedPalette.some((value, index) => value !== palette[index]);
      if (paletteChanged) {
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, 256, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, palette);
        this.uploadedPalette = palette.slice();
      }
      this.paletteSnapshotBuffer = snapshot.buffer;
    }
    gl.uniform1i(classic.uniforms.uPalette, 1);

    const worldView = visibleWorldRect(snapshot, "classic", camera);
    const viewWidth = worldView.width;
    const viewHeight = worldView.height;
    const viewX = worldView.x - snapshot.classicOriginX;
    const viewY = worldView.y - snapshot.classicOriginY;
    gl.uniform4f(
      classic.uniforms.uUvRect,
      viewX / snapshot.classicWidth,
      viewY / snapshot.classicHeight,
      (viewX + viewWidth) / snapshot.classicWidth,
      (viewY + viewHeight) / snapshot.classicHeight,
    );

    const viewport = fitViewport(this.canvas.width, this.canvas.height, viewWidth, viewHeight);
    gl.viewport(Math.floor(viewport.x), Math.floor(viewport.y), Math.round(viewport.width), Math.round(viewport.height));
    gl.drawArrays(gl.TRIANGLES, 0, 6);
    gl.viewport(0, 0, this.canvas.width, this.canvas.height);
  }

  private renderSprites(snapshot: SnapshotView, camera: CameraTransform): void {
    if (snapshot.spriteCount === 0) return;
    const gl = this.gl;
    const { sprite } = this.resources;
    gl.useProgram(sprite.program);
    this.bindQuad();
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    gl.uniform1i(sprite.uniforms.uAtlas, 0);
    gl.uniform2f(sprite.uniforms.uWorldSize, snapshot.worldWidth, snapshot.worldHeight);
    gl.uniform2f(sprite.uniforms.uCamera, snapshot.cameraX + camera.x, snapshot.cameraY + camera.y);
    gl.uniform1f(sprite.uniforms.uZoom, snapshot.zoom * camera.zoom);

    const stride = 13 * 4;
    gl.bindBuffer(gl.ARRAY_BUFFER, this.resources.instances);
    if (this.spriteSnapshotBuffer !== snapshot.buffer) {
      const sprites = snapshot.sprites().sort((left, right) => left.sortKey - right.sortKey);
      const data = new Float32Array(sprites.length * 13);
      const batches: PreparedSpriteBatch[] = [];
      sprites.forEach((entry, index) => {
        const offset = index * 13;
        const color = unpackColor(entry.tint);
        data.set([entry.x, entry.y, entry.width, entry.height, entry.u0, entry.v0, entry.u1, entry.v1, ...color, entry.flags], offset);
        const last = batches.at(-1);
        if (last?.pageId === entry.atlasPage) last.count += 1;
        else batches.push({ pageId: entry.atlasPage, start: index, count: 1 });
      });
      gl.bufferData(gl.ARRAY_BUFFER, data, gl.DYNAMIC_DRAW);
      this.spriteSnapshotBuffer = snapshot.buffer;
      this.preparedSpriteBatches = batches;
    }
    for (const batch of this.preparedSpriteBatches) {
      const base = batch.start * stride;
      gl.enableVertexAttribArray(1);
      gl.vertexAttribPointer(1, 4, gl.FLOAT, false, stride, base);
      gl.vertexAttribDivisor(1, 1);
      gl.enableVertexAttribArray(2);
      gl.vertexAttribPointer(2, 4, gl.FLOAT, false, stride, base + 16);
      gl.vertexAttribDivisor(2, 1);
      gl.enableVertexAttribArray(3);
      gl.vertexAttribPointer(3, 4, gl.FLOAT, false, stride, base + 32);
      gl.vertexAttribDivisor(3, 1);
      gl.enableVertexAttribArray(4);
      gl.vertexAttribPointer(4, 1, gl.FLOAT, false, stride, base + 48);
      gl.vertexAttribDivisor(4, 1);
      gl.activeTexture(gl.TEXTURE0);
      gl.bindTexture(gl.TEXTURE_2D, this.atlasPages.get(batch.pageId) ?? this.resources.fallbackTexture);
      gl.drawArraysInstanced(gl.TRIANGLES, 0, 6, batch.count);
    }
    gl.disable(gl.BLEND);
  }

  private readonly onContextLost = (event: Event): void => {
    event.preventDefault();
    this.contextLost = true;
    this.options.onContextLost?.();
  };

  private readonly onContextRestored = (): void => {
    this.contextLost = false;
    this.atlasPages.clear();
    this.initializeResources();
    this.options.onContextRestored?.();
  };
}
