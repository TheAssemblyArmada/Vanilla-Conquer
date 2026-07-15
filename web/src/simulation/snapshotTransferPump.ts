import { TransferBufferPool } from "./bufferPool";
import { SNAPSHOT_HEADER_BYTES } from "./snapshot";

export interface SnapshotSource {
  snapshotSize(): number;
  writeSnapshot(target: ArrayBuffer): number;
}

export type SnapshotDelivery = (buffer: ArrayBuffer) => void;

/**
 * Publishes at most the pool's bounded number of in-flight snapshots. Requests
 * made under backpressure are coalesced into one newest-state delivery when a
 * buffer returns. Crucially, snapshotSize() is never called without available
 * pool capacity: the Wasm backend advances its dirty-rectangle baseline while
 * materializing.
 */
export class SnapshotTransferPump {
  private readonly pool: TransferBufferPool;
  private readonly deliver: SnapshotDelivery;
  private pending = false;

  constructor(pool: TransferBufferPool, deliver: SnapshotDelivery) {
    this.pool = pool;
    this.deliver = deliver;
  }

  request(source: SnapshotSource): number | undefined {
    this.pending = true;
    if (!this.pool.hasAvailable) return undefined;

    let buffer: ArrayBuffer | undefined;
    try {
      buffer = this.pool.acquire(source.snapshotSize());
      if (!buffer) return undefined;
      const written = source.writeSnapshot(buffer);
      if (!Number.isSafeInteger(written) || written < SNAPSHOT_HEADER_BYTES || written > buffer.byteLength) {
        throw new Error("Simulation core wrote an invalid snapshot length");
      }
      const view = new DataView(buffer, 0, written);
      if (view.getUint32(8, true) !== written) throw new Error("Simulation core snapshot length does not match its header");
      const tick = view.getUint32(16, true);
      this.deliver(buffer);
      this.pending = false;
      return tick;
    } catch (error) {
      // Once materialization or writing fails, the backend's delta baseline is
      // not safe to retry as a deferred frame. The worker treats this path as
      // fatal; do not let an unrelated later recycle publish it.
      this.pending = false;
      if (buffer) this.pool.release(buffer);
      throw error;
    }
  }

  recycle(buffer: ArrayBuffer, source: SnapshotSource | undefined): number | undefined {
    this.pool.release(buffer);
    return this.pending && source ? this.request(source) : undefined;
  }

  clearPending(): void {
    this.pending = false;
  }

  get hasPending(): boolean {
    return this.pending;
  }
}
