import { describe, expect, it, vi } from "vitest";
import { TransferBufferPool } from "./bufferPool";
import { SnapshotTransferPump, type SnapshotSource } from "./snapshotTransferPump";

const SNAPSHOT_BYTES = 64;

function sourceState() {
  let tick = 0;
  const source: SnapshotSource = {
    snapshotSize: vi.fn(() => SNAPSHOT_BYTES),
    writeSnapshot: vi.fn((target: ArrayBuffer) => {
      const view = new DataView(target);
      view.setUint32(8, SNAPSHOT_BYTES, true);
      view.setUint32(16, tick, true);
      return SNAPSHOT_BYTES;
    }),
  };
  return { source, setTick: (value: number) => { tick = value; } };
}

describe("SnapshotTransferPump", () => {
  it("coalesces a sustained saturated run without materializing dropped deltas", () => {
    const delivered: ArrayBuffer[] = [];
    const state = sourceState();
    const pump = new SnapshotTransferPump(new TransferBufferPool(3, SNAPSHOT_BYTES), (buffer) => delivered.push(buffer));

    for (let tick = 1; tick <= 10_000; tick += 1) {
      state.setTick(tick);
      pump.request(state.source);
    }

    expect(delivered).toHaveLength(3);
    expect(state.source.snapshotSize).toHaveBeenCalledTimes(3);
    expect(state.source.writeSnapshot).toHaveBeenCalledTimes(3);
    expect(pump.hasPending).toBe(true);

    pump.recycle(delivered.shift()!, state.source);
    expect(delivered).toHaveLength(3);
    expect(new DataView(delivered.at(-1)!).getUint32(16, true)).toBe(10_000);
    expect(state.source.snapshotSize).toHaveBeenCalledTimes(4);
    expect(pump.hasPending).toBe(false);
  });

  it("delivers a pending terminal or paused frame as soon as one lease returns", () => {
    const delivered: ArrayBuffer[] = [];
    const state = sourceState();
    const pump = new SnapshotTransferPump(new TransferBufferPool(1, SNAPSHOT_BYTES), (buffer) => delivered.push(buffer));
    state.setTick(4);
    expect(pump.request(state.source)).toBe(4);
    state.setTick(9);
    expect(pump.request(state.source)).toBeUndefined();

    expect(pump.recycle(delivered.shift()!, state.source)).toBe(9);
    expect(new DataView(delivered[0]).getUint32(16, true)).toBe(9);
  });

  it("clears a pending request after a malformed write instead of retrying an unsafe delta", () => {
    const pool = new TransferBufferPool(1, SNAPSHOT_BYTES);
    const pump = new SnapshotTransferPump(pool, vi.fn());
    const source: SnapshotSource = {
      snapshotSize: () => SNAPSHOT_BYTES,
      writeSnapshot: () => SNAPSHOT_BYTES - 1,
    };

    expect(() => pump.request(source)).toThrow(/length does not match/i);
    expect(pump.hasPending).toBe(false);
    expect(pool.stats).toEqual({ available: 1, leased: 0, capacity: 1 });
  });

  it("does not publish after snapshot materialization throws", () => {
    const delivered: ArrayBuffer[] = [];
    const state = sourceState();
    const pump = new SnapshotTransferPump(new TransferBufferPool(2, SNAPSHOT_BYTES), (buffer) => delivered.push(buffer));
    pump.request(state.source);
    const materializationError: SnapshotSource = {
      snapshotSize: () => { throw new Error("materialization failed"); },
      writeSnapshot: vi.fn(),
    };

    expect(() => pump.request(materializationError)).toThrow("materialization failed");
    expect(pump.hasPending).toBe(false);
    pump.recycle(delivered.shift()!, state.source);
    expect(delivered).toHaveLength(0);
    expect(materializationError.writeSnapshot).not.toHaveBeenCalled();
  });
});
