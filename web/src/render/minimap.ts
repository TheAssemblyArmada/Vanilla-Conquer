export interface MinimapImage {
  width: number;
  height: number;
  rgba: Uint8ClampedArray;
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
