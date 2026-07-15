import { describe, expect, it } from "vitest";
import { clampCameraTransform, fitViewport, focusCameraTransform, pointToWorld, presentationViewport, visibleWorldRect, type ViewportSnapshot } from "./viewport";

const snapshot: ViewportSnapshot = {
  cameraX: 200,
  cameraY: 120,
  worldWidth: 320,
  worldHeight: 200,
  zoom: 1,
  classicWidth: 640,
  classicHeight: 400,
  classicOriginX: 100,
  classicOriginY: 40,
};

describe("presentation viewport", () => {
  it("fits a source aspect ratio and identifies its black bars", () => {
    const viewport = fitViewport(1000, 500, 4, 3);
    expect(viewport.x).toBeCloseTo(500 / 3);
    expect(viewport.y).toBe(0);
    expect(viewport.width).toBeCloseTo(2000 / 3);
    expect(viewport.height).toBe(500);
    expect(pointToWorld({ x: 100, y: 250 }, viewport, { x: 0, y: 0, width: 400, height: 300 })).toBeUndefined();
    expect(pointToWorld({ x: 500, y: 250 }, viewport, { x: 0, y: 0, width: 400, height: 300 })).toEqual({ x: 200, y: 150 });
  });

  it("matches the classic renderer's cropped and clamped world view", () => {
    const world = visibleWorldRect(snapshot, "classic", { x: 20, y: 10, zoom: 2 });
    expect(world).toEqual({ x: 220, y: 130, width: 160, height: 100 });
    const viewport = presentationViewport(1000, 500, world, "classic");
    expect(viewport).toEqual({ x: 100, y: 0, width: 800, height: 500 });
    expect(pointToWorld({ x: 500, y: 250 }, viewport, world)).toEqual({ x: 300, y: 180 });
    expect(pointToWorld({ x: 950, y: 250 }, viewport, world)).toBeUndefined();
  });

  it("bounds presentation camera offsets and zoom to the exposed classic map", () => {
    expect(clampCameraTransform(snapshot, "classic", { x: -999, y: 999, zoom: 9 })).toEqual({ x: -100, y: 240, zoom: 2.5 });
    expect(clampCameraTransform(snapshot, "classic", { x: Number.NaN, y: Number.POSITIVE_INFINITY, zoom: Number.NaN })).toEqual({ x: 0, y: 0, zoom: 1 });
  });

  it("zooms a narrow real scenario surface inside the megamap-sized engine view", () => {
    const realEngineShape: ViewportSnapshot = {
      cameraX: 0,
      cameraY: 0,
      worldWidth: 3072,
      worldHeight: 3072,
      zoom: 1,
      classicWidth: 512,
      classicHeight: 384,
      classicOriginX: 0,
      classicOriginY: 0,
    };
    expect(visibleWorldRect(realEngineShape, "classic", { x: 0, y: 0, zoom: 1 }))
      .toEqual({ x: 0, y: 0, width: 512, height: 384 });
    expect(visibleWorldRect(realEngineShape, "classic", { x: 128, y: 96, zoom: 2 }))
      .toEqual({ x: 128, y: 96, width: 256, height: 192 });
    expect(clampCameraTransform(realEngineShape, "classic", { x: 999, y: 999, zoom: 2 }))
      .toEqual({ x: 256, y: 192, zoom: 2 });
  });

  it("focuses absolute engine points against the snapshot that carries the camera event", () => {
    expect(focusCameraTransform(snapshot, "classic", { x: 0, y: 0, zoom: 1 }, { x: 400, y: 240 }))
      .toEqual({ x: 40, y: 20, zoom: 1 });
    const engineAlreadyCentered = { ...snapshot, cameraX: 240, cameraY: 140 };
    expect(focusCameraTransform(engineAlreadyCentered, "classic", { x: 0, y: 0, zoom: 1 }, { x: 400, y: 240 }))
      .toEqual({ x: 0, y: 0, zoom: 1 });
  });

  it("uses the full canvas for the enhanced world view", () => {
    const world = visibleWorldRect(snapshot, "remastered", { x: 10, y: -20, zoom: 2 });
    expect(world).toEqual({ x: 210, y: 100, width: 160, height: 100 });
    const viewport = presentationViewport(900, 600, world, "remastered");
    expect(viewport).toEqual({ x: 0, y: 0, width: 900, height: 600 });
  });
});
