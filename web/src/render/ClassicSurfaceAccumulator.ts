import type { SnapshotClassicSurface } from "../simulation/snapshot";

export interface AccumulatedClassicSurface {
  width: number;
  height: number;
  /** Complete, tightly packed indexed-color surface. */
  pixels: Uint8Array;
}

/** Reconstructs complete CPU frames from an initial baseline and later dirty rectangles. */
export class ClassicSurfaceAccumulator {
  private frame?: AccumulatedClassicSurface;

  current(): AccumulatedClassicSurface | undefined {
    return this.frame;
  }

  apply(update: SnapshotClassicSurface | undefined): AccumulatedClassicSurface | undefined {
    if (!update) return this.frame;
    this.validate(update);
    if (update.format === 1) {
      this.frame = { width: update.width, height: update.height, pixels: update.pixels.slice() };
      return this.frame;
    }
    if (!this.frame || this.frame.width !== update.width || this.frame.height !== update.height) {
      this.frame = undefined;
      return undefined;
    }
    if (update.rectWidth === 0) return this.frame;
    for (let row = 0; row < update.rectHeight; row += 1) {
      const sourceOffset = row * update.rectWidth;
      const destinationOffset = (update.rectY + row) * update.width + update.rectX;
      this.frame.pixels.set(update.pixels.subarray(sourceOffset, sourceOffset + update.rectWidth), destinationOffset);
    }
    return this.frame;
  }

  reset(): void {
    this.frame = undefined;
  }

  private validate(update: SnapshotClassicSurface): void {
    if (!Number.isInteger(update.width) || !Number.isInteger(update.height) || update.width < 1 || update.height < 1) {
      throw new Error("Classic surface dimensions are invalid");
    }
    if (update.format === 1) {
      if (update.rectX !== 0 || update.rectY !== 0 || update.rectWidth !== update.width || update.rectHeight !== update.height
        || update.pixels.byteLength !== update.width * update.height) throw new Error("Classic baseline layout is invalid");
      return;
    }
    const empty = update.rectWidth === 0 && update.rectHeight === 0;
    if ((!empty && (update.rectWidth < 1 || update.rectHeight < 1))
      || !Number.isInteger(update.rectX) || !Number.isInteger(update.rectY)
      || !Number.isInteger(update.rectWidth) || !Number.isInteger(update.rectHeight)
      || update.rectX < 0 || update.rectY < 0
      || update.rectX + update.rectWidth > update.width || update.rectY + update.rectHeight > update.height
      || update.pixels.byteLength !== update.rectWidth * update.rectHeight) throw new Error("Classic dirty rectangle layout is invalid");
  }
}
