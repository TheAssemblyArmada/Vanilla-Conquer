import { describe, expect, it, vi } from "vitest";
import type { SnapshotClassicSurface } from "../simulation/snapshot";
import { ClassicSurfaceTextureUploader } from "./WebGLRenderer";

function fakeGl() {
  return {
    TEXTURE_2D: 0x0de1,
    R8: 0x8229,
    RED: 0x1903,
    UNSIGNED_BYTE: 0x1401,
    texImage2D: vi.fn(),
    texSubImage2D: vi.fn(),
  } as unknown as WebGL2RenderingContext;
}

function baseline(): SnapshotClassicSurface {
  return { format: 1, width: 4, height: 3, rectX: 0, rectY: 0, rectWidth: 4, rectHeight: 3, pixels: new Uint8Array(12) };
}

function dirty(overrides: Partial<SnapshotClassicSurface> = {}): SnapshotClassicSurface {
  return { format: 2, width: 4, height: 3, rectX: 1, rectY: 1, rectWidth: 2, rectHeight: 1, pixels: new Uint8Array([7, 8]), ...overrides };
}

describe("ClassicSurfaceTextureUploader", () => {
  it("allocates a complete baseline with texImage2D", () => {
    const gl = fakeGl();
    const uploader = new ClassicSurfaceTextureUploader();
    const update = baseline();
    const buffer = new ArrayBuffer(1);
    expect(uploader.upload(gl, update, buffer)).toBe(true);
    expect(gl.texImage2D).toHaveBeenCalledWith(gl.TEXTURE_2D, 0, gl.R8, 4, 3, 0, gl.RED, gl.UNSIGNED_BYTE, update.pixels);
    expect(gl.texSubImage2D).not.toHaveBeenCalled();
    expect(uploader.upload(gl, update, buffer)).toBe(true);
    expect(gl.texImage2D).toHaveBeenCalledTimes(1);
    expect(uploader.hasIngested(buffer)).toBe(true);
    expect(uploader.hasIngested(new ArrayBuffer(1))).toBe(false);
  });

  it("uploads only the dirty rectangle after a baseline", () => {
    const gl = fakeGl();
    const uploader = new ClassicSurfaceTextureUploader();
    uploader.upload(gl, baseline(), new ArrayBuffer(1));
    const update = dirty();
    expect(uploader.upload(gl, update, new ArrayBuffer(1))).toBe(true);
    expect(gl.texSubImage2D).toHaveBeenCalledWith(gl.TEXTURE_2D, 0, 1, 1, 2, 1, gl.RED, gl.UNSIGNED_BYTE, update.pixels);
    expect(gl.texImage2D).toHaveBeenCalledTimes(1);
  });

  it("ingests every intermediate delta in order when drawing is skipped", () => {
    const gl = fakeGl();
    const uploader = new ClassicSurfaceTextureUploader();
    uploader.upload(gl, baseline(), new ArrayBuffer(1));
    const first = dirty({ rectX: 0, rectY: 0, pixels: new Uint8Array([1, 2]) });
    const second = dirty({ rectX: 2, rectY: 2, pixels: new Uint8Array([3, 4]) });
    uploader.upload(gl, first, new ArrayBuffer(1));
    uploader.upload(gl, second, new ArrayBuffer(1));
    expect(vi.mocked(gl.texSubImage2D).mock.calls.map((call) => [call[2], call[3], call[8]])).toEqual([
      [0, 0, first.pixels],
      [2, 2, second.pixels],
    ]);
  });

  it("draws an unchanged zero rectangle without another texture upload", () => {
    const gl = fakeGl();
    const uploader = new ClassicSurfaceTextureUploader();
    uploader.upload(gl, baseline(), new ArrayBuffer(1));
    const unchanged = dirty({ rectX: 4, rectY: 3, rectWidth: 0, rectHeight: 0, pixels: new Uint8Array(0) });
    expect(uploader.upload(gl, unchanged, new ArrayBuffer(1))).toBe(true);
    expect(gl.texSubImage2D).not.toHaveBeenCalled();
    expect(gl.texImage2D).toHaveBeenCalledTimes(1);
  });

  it("refuses a delta before a matching baseline", () => {
    const gl = fakeGl();
    const uploader = new ClassicSurfaceTextureUploader();
    expect(uploader.upload(gl, dirty(), new ArrayBuffer(1))).toBe(false);
    expect(gl.texImage2D).not.toHaveBeenCalled();
    expect(gl.texSubImage2D).not.toHaveBeenCalled();

    uploader.upload(gl, baseline(), new ArrayBuffer(1));
    expect(uploader.upload(gl, dirty({ width: 5 }), new ArrayBuffer(1))).toBe(false);
    expect(uploader.upload(gl, dirty(), new ArrayBuffer(1))).toBe(false);
    expect(gl.texSubImage2D).not.toHaveBeenCalled();
  });

  it("requires a new baseline after reset", () => {
    const gl = fakeGl();
    const uploader = new ClassicSurfaceTextureUploader();
    const buffer = new ArrayBuffer(1);
    uploader.upload(gl, baseline(), buffer);
    expect(uploader.hasIngested(buffer)).toBe(true);
    uploader.reset();
    expect(uploader.hasIngested(buffer)).toBe(false);
    expect(uploader.upload(gl, dirty(), new ArrayBuffer(1))).toBe(false);
  });

  it("immediately restores a lost texture from the accumulated CPU baseline", () => {
    const gl = fakeGl();
    const uploader = new ClassicSurfaceTextureUploader();
    uploader.upload(gl, baseline(), new ArrayBuffer(1));
    uploader.reset();
    vi.mocked(gl.texImage2D).mockClear();
    const recovered = { width: 4, height: 3, pixels: new Uint8Array(12).fill(6) };
    expect(uploader.upload(gl, dirty(), new ArrayBuffer(1), recovered)).toBe(true);
    expect(gl.texImage2D).toHaveBeenCalledWith(gl.TEXTURE_2D, 0, gl.R8, 4, 3, 0, gl.RED, gl.UNSIGNED_BYTE, recovered.pixels);
    expect(gl.texSubImage2D).not.toHaveBeenCalled();
    expect(uploader.upload(gl, dirty(), new ArrayBuffer(1))).toBe(true);
    expect(gl.texSubImage2D).toHaveBeenCalledTimes(1);
  });
});
