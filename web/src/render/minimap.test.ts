import { describe, expect, it } from "vitest";
import { buildMinimapImage, minimapContentRect, minimapPointerToWorld } from "./minimap";

describe("minimapPointerToWorld", () => {
  it("maps the center of a letterboxed radar image to the classic-map center", () => {
    const content = minimapContentRect(200, 100, 100, 100);
    expect(content).toEqual({ x: 50, y: 0, width: 100, height: 100 });
    expect(minimapPointerToWorld({
      offsetX: 100,
      offsetY: 50,
      clientWidth: 200,
      clientHeight: 100,
      bitmapWidth: 100,
      bitmapHeight: 100,
      sourceWidth: 640,
      sourceHeight: 400,
      classicOriginX: 24,
      classicOriginY: 16,
    })).toEqual({ x: 24 + 320, y: 16 + 200 });
  });

  it("ignores clicks in the letterbox gutter", () => {
    expect(minimapPointerToWorld({
      offsetX: 10,
      offsetY: 50,
      clientWidth: 200,
      clientHeight: 100,
      bitmapWidth: 100,
      bitmapHeight: 100,
      sourceWidth: 640,
      sourceHeight: 400,
      classicOriginX: 0,
      classicOriginY: 0,
    })).toBeUndefined();
  });
});

describe("buildMinimapImage", () => {
  it("preserves a small indexed surface through its RGBA palette", () => {
    const palette = new Uint8Array(256 * 4);
    palette.set([10, 20, 30, 255], 4);
    palette.set([40, 50, 60, 255], 8);
    const image = buildMinimapImage(new Uint8Array([1, 2]), palette, 2, 1);
    expect(image).toMatchObject({ width: 2, height: 1 });
    expect([...image.rgba]).toEqual([10, 20, 30, 255, 40, 50, 60, 255]);
  });

  it("bounds a large map while retaining its aspect ratio", () => {
    const image = buildMinimapImage(new Uint8Array(648 * 552), new Uint8Array(256 * 4), 648, 552);
    expect(image.width).toBeLessThanOrEqual(220);
    expect(image.height).toBeLessThanOrEqual(152);
    expect(image.width / image.height).toBeCloseTo(648 / 552, 1);
  });

  it("rejects inconsistent source dimensions", () => {
    expect(() => buildMinimapImage(new Uint8Array(3), new Uint8Array(256 * 4), 2, 2)).toThrow(/layout/);
  });
});
