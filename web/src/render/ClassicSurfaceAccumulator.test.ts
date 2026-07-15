import { describe, expect, it } from "vitest";
import type { SnapshotClassicSurface } from "../simulation/snapshot";
import { ClassicSurfaceAccumulator } from "./ClassicSurfaceAccumulator";

function baseline(pixels = new Uint8Array([1, 2, 3, 4, 5, 6])): SnapshotClassicSurface {
  return { format: 1, width: 3, height: 2, rectX: 0, rectY: 0, rectWidth: 3, rectHeight: 2, pixels };
}

function dirty(overrides: Partial<SnapshotClassicSurface> = {}): SnapshotClassicSurface {
  return { format: 2, width: 3, height: 2, rectX: 1, rectY: 0, rectWidth: 2, rectHeight: 2, pixels: new Uint8Array([7, 8, 9, 10]), ...overrides };
}

describe("ClassicSurfaceAccumulator", () => {
  it("copies a baseline and patches dirty rows into a complete frame", () => {
    const accumulator = new ClassicSurfaceAccumulator();
    const source = new Uint8Array([1, 2, 3, 4, 5, 6]);
    const first = accumulator.apply(baseline(source));
    source.fill(99);
    expect([...first!.pixels]).toEqual([1, 2, 3, 4, 5, 6]);

    const updated = accumulator.apply(dirty());
    expect(updated).toBe(first);
    expect([...updated!.pixels]).toEqual([1, 7, 8, 4, 9, 10]);
  });

  it("treats a zero rectangle and an omitted update as unchanged", () => {
    const accumulator = new ClassicSurfaceAccumulator();
    const first = accumulator.apply(baseline());
    expect(accumulator.current()).toBe(first);
    const unchanged = accumulator.apply(dirty({ rectX: 3, rectY: 2, rectWidth: 0, rectHeight: 0, pixels: new Uint8Array(0) }));
    expect(unchanged).toBe(first);
    expect(accumulator.apply(undefined)).toBe(first);
  });

  it("does not invent a baseline when a delta arrives first", () => {
    const accumulator = new ClassicSurfaceAccumulator();
    expect(accumulator.apply(dirty())).toBeUndefined();
    expect(accumulator.apply(dirty({ rectWidth: 0, rectHeight: 0, rectX: 0, rectY: 0, pixels: new Uint8Array(0) }))).toBeUndefined();
  });

  it("invalidates an old baseline when delta dimensions change", () => {
    const accumulator = new ClassicSurfaceAccumulator();
    accumulator.apply(baseline());
    expect(accumulator.apply(dirty({ width: 4, rectX: 2 }))).toBeUndefined();
    expect(accumulator.apply(dirty())).toBeUndefined();
    expect(accumulator.apply(baseline())).toBeDefined();
    accumulator.reset();
    expect(accumulator.current()).toBeUndefined();
    expect(accumulator.apply(undefined)).toBeUndefined();
  });

  it("rejects malformed direct updates instead of writing outside the frame", () => {
    const accumulator = new ClassicSurfaceAccumulator();
    expect(() => accumulator.apply(dirty({ rectX: 2 }))).toThrow(/layout/i);
    expect(() => accumulator.apply(dirty({ rectWidth: 0 }))).toThrow(/layout/i);
    expect(() => accumulator.apply(baseline(new Uint8Array(5)))).toThrow(/baseline/i);
  });

  it("keeps one bounded CPU baseline across a sustained dirty-update run", () => {
    const width = 1536;
    const height = 1536;
    const accumulator = new ClassicSurfaceAccumulator();
    const frame = accumulator.apply({
      format: 1,
      width,
      height,
      rectX: 0,
      rectY: 0,
      rectWidth: width,
      rectHeight: height,
      pixels: new Uint8Array(width * height),
    });
    const pixel = new Uint8Array(1);
    const update: SnapshotClassicSurface = {
      format: 2,
      width,
      height,
      rectX: 0,
      rectY: 0,
      rectWidth: 1,
      rectHeight: 1,
      pixels: pixel,
    };

    for (let index = 0; index < 10_000; index += 1) {
      update.rectX = index % width;
      update.rectY = Math.floor(index / width);
      pixel[0] = index & 0xff;
      expect(accumulator.apply(update)).toBe(frame);
    }

    expect(accumulator.current()?.pixels).toHaveLength(width * height);
    expect(accumulator.current()?.pixels[9_999]).toBe(9_999 & 0xff);
  });
});
