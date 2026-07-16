export interface MinimapImage {
  width: number;
  height: number;
  rgba: Uint8ClampedArray;
}

export interface MinimapContentRect {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface MinimapWorldPoint {
  x: number;
  y: number;
}

/** Letterboxed image rect for an object-fit: contain minimap canvas. */
export function minimapContentRect(
  clientWidth: number,
  clientHeight: number,
  bitmapWidth: number,
  bitmapHeight: number,
): MinimapContentRect | undefined {
  if (!(clientWidth > 0) || !(clientHeight > 0) || !(bitmapWidth > 0) || !(bitmapHeight > 0)) return undefined;
  const scale = Math.min(clientWidth / bitmapWidth, clientHeight / bitmapHeight);
  const width = bitmapWidth * scale;
  const height = bitmapHeight * scale;
  return {
    x: (clientWidth - width) / 2,
    y: (clientHeight - height) / 2,
    width,
    height,
  };
}

/** Maps a pointer inside the displayed radar image to an engine world point. */
export function minimapPointerToWorld(options: {
  offsetX: number;
  offsetY: number;
  clientWidth: number;
  clientHeight: number;
  bitmapWidth: number;
  bitmapHeight: number;
  sourceWidth: number;
  sourceHeight: number;
  classicOriginX: number;
  classicOriginY: number;
}): MinimapWorldPoint | undefined {
  const {
    offsetX,
    offsetY,
    clientWidth,
    clientHeight,
    bitmapWidth,
    bitmapHeight,
    sourceWidth,
    sourceHeight,
    classicOriginX,
    classicOriginY,
  } = options;
  if (!(sourceWidth > 0) || !(sourceHeight > 0)) return undefined;
  const content = minimapContentRect(clientWidth, clientHeight, bitmapWidth, bitmapHeight);
  if (!content) return undefined;
  if (offsetX < content.x || offsetY < content.y || offsetX > content.x + content.width || offsetY > content.y + content.height) {
    return undefined;
  }
  const bitmapX = ((offsetX - content.x) / content.width) * bitmapWidth;
  const bitmapY = ((offsetY - content.y) / content.height) * bitmapHeight;
  const sourceX = Math.min(sourceWidth, Math.max(0, (bitmapX / bitmapWidth) * sourceWidth));
  const sourceY = Math.min(sourceHeight, Math.max(0, (bitmapY / bitmapHeight) * sourceHeight));
  return {
    x: classicOriginX + sourceX,
    y: classicOriginY + sourceY,
  };
}

/** Downsamples an indexed classic surface into a small RGBA radar image. */
export function buildMinimapImage(
  indexed: Uint8Array,
  palette: Uint8Array,
  sourceWidth: number,
  sourceHeight: number,
  maximumWidth = 220,
  maximumHeight = 152,
): MinimapImage {
  if (!Number.isInteger(sourceWidth) || !Number.isInteger(sourceHeight) || sourceWidth < 1 || sourceHeight < 1
    || indexed.byteLength !== sourceWidth * sourceHeight || palette.byteLength !== 256 * 4) throw new Error("Minimap source layout is invalid");
  if (!Number.isInteger(maximumWidth) || !Number.isInteger(maximumHeight) || maximumWidth < 1 || maximumHeight < 1) throw new Error("Minimap bounds are invalid");
  const scale = Math.min(1, maximumWidth / sourceWidth, maximumHeight / sourceHeight);
  const width = Math.max(1, Math.round(sourceWidth * scale));
  const height = Math.max(1, Math.round(sourceHeight * scale));
  const rgba = new Uint8ClampedArray(width * height * 4);
  // Horizontal sampling is identical for every output row. Precomputing it
  // avoids tens of thousands of divisions per minimap refresh on large maps.
  const sourceXs = new Uint32Array(width);
  for (let x = 0; x < width; x += 1) {
    sourceXs[x] = Math.min(sourceWidth - 1, Math.floor(((x + 0.5) * sourceWidth) / width));
  }
  for (let y = 0; y < height; y += 1) {
    const sourceY = Math.min(sourceHeight - 1, Math.floor(((y + 0.5) * sourceHeight) / height));
    const sourceRow = sourceY * sourceWidth;
    for (let x = 0; x < width; x += 1) {
      const paletteOffset = indexed[sourceRow + sourceXs[x]] * 4;
      const outputOffset = (y * width + x) * 4;
      rgba[outputOffset] = palette[paletteOffset];
      rgba[outputOffset + 1] = palette[paletteOffset + 1];
      rgba[outputOffset + 2] = palette[paletteOffset + 2];
      rgba[outputOffset + 3] = 255;
    }
  }
  return { width, height, rgba };
}
