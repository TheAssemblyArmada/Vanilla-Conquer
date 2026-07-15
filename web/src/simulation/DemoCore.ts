import type { DecodedCommandBatch, SimulationEvent, StartConfiguration } from "./protocol";
import { SpriteFlags, snapshotByteLength, writeSnapshot, type SnapshotSprite } from "./snapshot";
import type { SimulationCore } from "./core";

const WIDTH = 320;
const HEIGHT = 200;
const SPRITE_COUNT = 5;

interface SavedDemoState {
  version: 1;
  tick: number;
  cameraX: number;
  cameraY: number;
  zoom: number;
  seed: number;
}

function xorshift32(value: number): number {
  let next = value | 0;
  next ^= next << 13;
  next ^= next >>> 17;
  next ^= next << 5;
  return next >>> 0;
}

export class DemoCore implements SimulationCore {
  readonly supportsSaves = true;
  private tick = 0;
  private cameraX = 0;
  private cameraY = 0;
  private zoom = 1;
  private seed = 1;
  private readonly pixels = new Uint8Array(WIDTH * HEIGHT);
  private readonly palette = new Uint8Array(256 * 4);

  constructor() {
    for (let index = 0; index < 256; index += 1) {
      const offset = index * 4;
      this.palette[offset] = Math.min(255, index * 0.82);
      this.palette[offset + 1] = Math.min(255, 18 + index * 0.92);
      this.palette[offset + 2] = Math.min(255, 12 + index * 0.52);
      this.palette[offset + 3] = 255;
    }
    this.palette.set([8, 12, 10, 255], 0);
  }

  start(configuration: StartConfiguration): void {
    this.tick = 0;
    this.cameraX = 0;
    this.cameraY = 0;
    this.zoom = 1;
    this.seed = configuration.seed >>> 0 || 1;
    this.drawClassicSurface();
  }

  submitCommands(batch: DecodedCommandBatch): void {
    // The synthetic core deliberately accepts the production command wire
    // without attempting to emulate engine gameplay semantics.
    this.seed ^= batch.commands.length + Number(batch.playerId & 0xffff_ffffn);
  }

  currentTick(): number { return this.tick; }

  advance(): boolean {
    this.tick = (this.tick + 1) >>> 0;
    this.seed = xorshift32(this.seed);
    this.drawClassicSurface();
    return true;
  }

  snapshotSize(): number {
    return snapshotByteLength(WIDTH, HEIGHT, SPRITE_COUNT);
  }

  writeSnapshot(target: ArrayBuffer): number {
    const phase = this.tick / 15;
    const sprites: SnapshotSprite[] = [
      this.sprite(50 + Math.sin(phase * 0.7) * 22, 57, 25, 20, 10, 0xffc98554, SpriteFlags.Selected),
      this.sprite(135 + Math.cos(phase * 0.45) * 38, 92, 30, 18, 20, 0xff6ea468),
      this.sprite(215, 42 + Math.sin(phase) * 20, 18, 27, 30, 0xff7aa7ca),
      this.sprite(248, 130, 36, 34, 40, 0xffb5915b, SpriteFlags.Shadow),
      this.sprite(92, 145 + Math.cos(phase * 0.9) * 9, 16, 16, 50, 0xffd1c06a),
    ];
    return writeSnapshot(target, {
      tick: this.tick,
      worldWidth: WIDTH,
      worldHeight: HEIGHT,
      classicWidth: WIDTH,
      classicHeight: HEIGHT,
      classicPixels: this.pixels,
      palette: this.palette,
      sprites,
      cameraX: this.cameraX,
      cameraY: this.cameraY,
      zoom: this.zoom,
    });
  }

  save(): Uint8Array {
    const state: SavedDemoState = {
      version: 1,
      tick: this.tick,
      cameraX: this.cameraX,
      cameraY: this.cameraY,
      zoom: this.zoom,
      seed: this.seed,
    };
    return new TextEncoder().encode(JSON.stringify(state));
  }

  load(data: Uint8Array): void {
    const value: unknown = JSON.parse(new TextDecoder().decode(data));
    if (!value || typeof value !== "object" || (value as Partial<SavedDemoState>).version !== 1) {
      throw new Error("Demo save is incompatible");
    }
    const state = value as SavedDemoState;
    for (const field of [state.tick, state.cameraX, state.cameraY, state.zoom, state.seed]) {
      if (!Number.isFinite(field)) throw new Error("Demo save contains invalid values");
    }
    this.tick = state.tick >>> 0;
    this.cameraX = state.cameraX;
    this.cameraY = state.cameraY;
    this.zoom = Math.min(2.5, Math.max(0.6, state.zoom));
    this.seed = state.seed >>> 0;
    this.drawClassicSurface();
  }

  drainEvents(): SimulationEvent[] {
    return [];
  }

  destroy(): void {}

  private sprite(x: number, y: number, width: number, height: number, sortKey: number, tint: number, flags = 0): SnapshotSprite {
    return {
      x,
      y,
      width,
      height,
      u0: 0,
      v0: 0,
      u1: 1,
      v1: 1,
      atlasPage: 0,
      flags,
      sortKey,
      tint,
      teamColor: tint,
    };
  }

  private drawClassicSurface(): void {
    const time = this.tick / 15;
    for (let y = 0; y < HEIGHT; y += 1) {
      for (let x = 0; x < WIDTH; x += 1) {
        const grid = ((x >> 4) + (y >> 4)) & 1;
        const ridge = Math.sin(x * 0.045 + time * 0.24) * 9 + Math.cos(y * 0.06 - time * 0.13) * 7;
        this.pixels[y * WIDTH + x] = Math.max(8, Math.min(110, 49 + grid * 9 + ridge)) | 0;
      }
    }
    const units = [
      [50 + Math.sin(time * 0.7) * 22, 57, 12, 172],
      [135 + Math.cos(time * 0.45) * 38, 92, 14, 148],
      [215, 42 + Math.sin(time) * 20, 10, 190],
    ];
    for (const [centerX, centerY, radius, color] of units) {
      const minY = Math.max(0, Math.floor(centerY - radius));
      const maxY = Math.min(HEIGHT - 1, Math.ceil(centerY + radius));
      const minX = Math.max(0, Math.floor(centerX - radius));
      const maxX = Math.min(WIDTH - 1, Math.ceil(centerX + radius));
      for (let y = minY; y <= maxY; y += 1) {
        for (let x = minX; x <= maxX; x += 1) {
          if ((x - centerX) ** 2 + (y - centerY) ** 2 <= radius ** 2) this.pixels[y * WIDTH + x] = color;
        }
      }
    }
  }
}
