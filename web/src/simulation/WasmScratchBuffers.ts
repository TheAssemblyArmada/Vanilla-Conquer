export interface WasmLinearAllocator {
  malloc(size: number): number;
  free(pointer: number): void;
}

/**
 * Reuses the small scalar output and largest byte buffer needed to cross the
 * Emscripten ABI. Wasm linear memory does not shrink after free(), so retaining
 * these allocations avoids allocator churn without increasing the long-run
 * heap high-water mark.
 *
 * Callers must use the buffers synchronously. This matches SimulationCore's
 * synchronous ABI methods and the dedicated worker that owns each core.
 */
export class WasmScratchBuffers {
  private readonly allocator: WasmLinearAllocator;
  private scalarPointer = 0;
  private bytesPointer = 0;
  private bytesCapacity = 0;
  private released = false;

  constructor(allocator: WasmLinearAllocator) {
    this.allocator = allocator;
  }

  outputU32(operation: string): number {
    this.assertActive();
    if (this.scalarPointer) return this.scalarPointer;
    const pointer = this.allocator.malloc(4);
    if (!pointer) throw new Error(`Wasm allocation failed during ${operation}`);
    this.scalarPointer = pointer;
    return pointer;
  }

  bytes(requiredBytes: number, operation: string): number {
    this.assertActive();
    if (!Number.isSafeInteger(requiredBytes) || requiredBytes < 1) {
      throw new RangeError(`${operation} requires a positive safe byte length`);
    }
    if (requiredBytes <= this.bytesCapacity) return this.bytesPointer;

    const pointer = this.allocator.malloc(requiredBytes);
    if (!pointer) throw new Error(`Wasm allocation failed during ${operation}`);
    if (this.bytesPointer) this.allocator.free(this.bytesPointer);
    this.bytesPointer = pointer;
    this.bytesCapacity = requiredBytes;
    return pointer;
  }

  release(): void {
    if (this.released) return;
    this.released = true;
    if (this.scalarPointer) this.allocator.free(this.scalarPointer);
    if (this.bytesPointer) this.allocator.free(this.bytesPointer);
    this.scalarPointer = 0;
    this.bytesPointer = 0;
    this.bytesCapacity = 0;
  }

  private assertActive(): void {
    if (this.released) throw new Error("Wasm scratch buffers have been released");
  }
}
