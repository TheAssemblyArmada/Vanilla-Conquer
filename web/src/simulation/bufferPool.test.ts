import { describe, expect, it } from "vitest";
import { TRANSFER_BUFFER_BUCKET_BYTES, TransferBufferPool } from "./bufferPool";

describe("TransferBufferPool", () => {
  it("bounds leases and grows returned buffers in 64 KiB buckets", () => {
    const pool = new TransferBufferPool(2, 1);
    const first = pool.acquire(TRANSFER_BUFFER_BUCKET_BYTES + 1)!;
    const second = pool.acquire(4)!;
    expect(first.byteLength).toBe(2 * TRANSFER_BUFFER_BUCKET_BYTES);
    expect(second.byteLength).toBe(TRANSFER_BUFFER_BUCKET_BYTES);
    expect(pool.acquire(1)).toBeUndefined();
    expect(pool.hasAvailable).toBe(false);
    expect(pool.stats).toEqual({ available: 0, leased: 2, capacity: 2 });
    pool.release(first);
    expect(pool.hasAvailable).toBe(true);
    expect(pool.acquire(TRANSFER_BUFFER_BUCKET_BYTES + 24)?.byteLength).toBe(2 * TRANSFER_BUFFER_BUCKET_BYTES);
  });

  it("does not grow a buffer at an exact bucket boundary", () => {
    const pool = new TransferBufferPool(1, TRANSFER_BUFFER_BUCKET_BYTES);
    expect(pool.acquire(TRANSFER_BUFFER_BUCKET_BYTES)?.byteLength).toBe(TRANSFER_BUFFER_BUCKET_BYTES);
  });

  it("shrinks a recycled baseline buffer before transferring a small delta", () => {
    const pool = new TransferBufferPool(1, TRANSFER_BUFFER_BUCKET_BYTES);
    const baseline = pool.acquire(3 * TRANSFER_BUFFER_BUCKET_BYTES)!;
    expect(baseline.byteLength).toBe(3 * TRANSFER_BUFFER_BUCKET_BYTES);
    pool.release(baseline);
    expect(pool.acquire(128)?.byteLength).toBe(TRANSFER_BUFFER_BUCKET_BYTES);
  });

  it("rejects unmatched releases", () => {
    const pool = new TransferBufferPool(1, 8);
    expect(() => pool.release(new ArrayBuffer(8))).toThrow(/matching lease/i);
  });
});
