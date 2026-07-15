import type { CameraTransform, GraphicsMode } from "./WebGLRenderer";

export interface ViewportRect {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface WorldRect {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface ViewportSnapshot {
  cameraX: number;
  cameraY: number;
  worldWidth: number;
  worldHeight: number;
  zoom: number;
  classicWidth: number;
  classicHeight: number;
  classicOriginX: number;
  classicOriginY: number;
}

export interface ScreenPointLike {
  x: number;
  y: number;
}

function classicViewSize(snapshot: ViewportSnapshot, zoom: number): { width: number; height: number } {
  const baseWidth = Math.min(snapshot.classicWidth, snapshot.worldWidth);
  const baseHeight = Math.min(snapshot.classicHeight, snapshot.worldHeight);
  return {
    width: Math.min(snapshot.classicWidth, baseWidth / zoom),
    height: Math.min(snapshot.classicHeight, baseHeight / zoom),
  };
}

export function fitViewport(containerWidth: number, containerHeight: number, contentWidth: number, contentHeight: number): ViewportRect {
  if (containerWidth <= 0 || containerHeight <= 0 || contentWidth <= 0 || contentHeight <= 0) return { x: 0, y: 0, width: 0, height: 0 };
  const scale = Math.min(containerWidth / contentWidth, containerHeight / contentHeight);
  const width = contentWidth * scale;
  const height = contentHeight * scale;
  return { x: (containerWidth - width) / 2, y: (containerHeight - height) / 2, width, height };
}

export function visibleWorldRect(snapshot: ViewportSnapshot, mode: GraphicsMode, camera: CameraTransform): WorldRect {
  if (mode === "classic") {
    const { width, height } = classicViewSize(snapshot, camera.zoom);
    const requestedX = snapshot.cameraX + camera.x - snapshot.classicOriginX;
    const requestedY = snapshot.cameraY + camera.y - snapshot.classicOriginY;
    const offsetX = Math.max(0, Math.min(snapshot.classicWidth - width, requestedX));
    const offsetY = Math.max(0, Math.min(snapshot.classicHeight - height, requestedY));
    return { x: snapshot.classicOriginX + offsetX, y: snapshot.classicOriginY + offsetY, width, height };
  }
  return {
    x: snapshot.cameraX + camera.x,
    y: snapshot.cameraY + camera.y,
    width: snapshot.worldWidth / (snapshot.zoom * camera.zoom),
    height: snapshot.worldHeight / (snapshot.zoom * camera.zoom),
  };
}

export function clampCameraTransform(snapshot: ViewportSnapshot, mode: GraphicsMode, camera: CameraTransform): CameraTransform {
  const zoom = Math.min(2.5, Math.max(0.6, Number.isFinite(camera.zoom) ? camera.zoom : 1));
  if (mode !== "classic") {
    return {
      x: Number.isFinite(camera.x) ? camera.x : 0,
      y: Number.isFinite(camera.y) ? camera.y : 0,
      zoom,
    };
  }
  const { width, height } = classicViewSize(snapshot, zoom);
  const minimumX = snapshot.classicOriginX - snapshot.cameraX;
  const minimumY = snapshot.classicOriginY - snapshot.cameraY;
  const maximumX = minimumX + Math.max(0, snapshot.classicWidth - width);
  const maximumY = minimumY + Math.max(0, snapshot.classicHeight - height);
  const x = Number.isFinite(camera.x) ? camera.x : 0;
  const y = Number.isFinite(camera.y) ? camera.y : 0;
  return {
    x: Math.min(maximumX, Math.max(minimumX, x)),
    y: Math.min(maximumY, Math.max(minimumY, y)),
    zoom,
  };
}

/** Centers a host-side camera on an absolute engine world point. */
export function focusCameraTransform(snapshot: ViewportSnapshot, mode: GraphicsMode, camera: CameraTransform, point: ScreenPointLike): CameraTransform {
  const world = visibleWorldRect(snapshot, mode, camera);
  return clampCameraTransform(snapshot, mode, {
    ...camera,
    x: camera.x + point.x - (world.x + world.width / 2),
    y: camera.y + point.y - (world.y + world.height / 2),
  });
}

export function presentationViewport(containerWidth: number, containerHeight: number, world: WorldRect, mode: GraphicsMode): ViewportRect {
  return mode === "classic" ? fitViewport(containerWidth, containerHeight, world.width, world.height) : { x: 0, y: 0, width: containerWidth, height: containerHeight };
}

export function pointToWorld(point: ScreenPointLike, viewport: ViewportRect, world: WorldRect): ScreenPointLike | undefined {
  if (viewport.width <= 0 || viewport.height <= 0 || point.x < viewport.x || point.y < viewport.y || point.x > viewport.x + viewport.width || point.y > viewport.y + viewport.height) return undefined;
  return {
    x: world.x + ((point.x - viewport.x) / viewport.width) * world.width,
    y: world.y + ((point.y - viewport.y) / viewport.height) * world.height,
  };
}
