export interface BufferPoolStats {
  available: number;
  leased: number;
  capacity: number;
}

export const TRANSFER_BUFFER_BUCKET_BYTES = 64 * 1024;

function bucketBytes(value: number): number {
  const remainder = value % TRANSFER_BUFFER_BUCKET_BYTES;
  if (remainder === 0) return value;
  const rounded = value + TRANSFER_BUFFER_BUCKET_BYTES - remainder;
  if (!Number.isSafeInteger(rounded)) throw new RangeError("Buffer size exceeds the supported range");
  return rounded;
}

/** A bounded pool intended for ArrayBuffers transferred across a Worker boundary. */
export class TransferBufferPool {
  readonly capacity: number;
  private readonly minimumBytes: number;
  private available: ArrayBuffer[] = [];
  private leased = 0;

  constructor(capacity = 3, minimumBytes = 256 * 1024) {
    if (!Number.isInteger(capacity) || capacity < 1) throw new RangeError("capacity must be a positive integer");
    if (!Number.isInteger(minimumBytes) || minimumBytes < 1) throw new RangeError("minimumBytes must be positive");
    this.capacity = capacity;
    this.minimumBytes = bucketBytes(minimumBytes);
    for (let index = 0; index < capacity; index += 1) this.available.push(new ArrayBuffer(this.minimumBytes));
  }

  acquire(requiredBytes: number): ArrayBuffer | undefined {
    if (!Number.isSafeInteger(requiredBytes) || requiredBytes < 0) throw new RangeError("requiredBytes must be non-negative");
    const buffer = this.available.pop();
    if (!buffer) return undefined;
    this.leased += 1;
    const requiredCapacity = bucketBytes(Math.max(requiredBytes, this.minimumBytes));
    /* ArrayBuffer transfer sends its complete capacity, not the snapshot's
     * declared prefix. Replace an oversized baseline buffer when subsequent
     * dirty rectangles become small, otherwise pooling defeats delta traffic. */
    return buffer.byteLength === requiredCapacity ? buffer : new ArrayBuffer(requiredCapacity);
  }

  release(buffer: ArrayBuffer): void {
    if (!(buffer instanceof ArrayBuffer) || buffer.byteLength === 0) throw new Error("Cannot recycle a detached or invalid buffer");
    if (this.leased === 0) throw new Error("Buffer pool release has no matching lease");
    this.leased -= 1;
    this.available.push(buffer);
  }

  get stats(): BufferPoolStats {
    return { available: this.available.length, leased: this.leased, capacity: this.capacity };
  }

  get hasAvailable(): boolean {
    return this.available.length > 0;
  }
}
