import { describe, expect, it, vi } from "vitest";
import { WasmScratchBuffers } from "./WasmScratchBuffers";

describe("WasmScratchBuffers", () => {
  it("keeps allocator traffic constant across a sustained same-size workload", () => {
    let nextPointer = 8;
    const malloc = vi.fn((size: number) => {
      const pointer = nextPointer;
      nextPointer += size;
      return pointer;
    });
    const free = vi.fn();
    const scratch = new WasmScratchBuffers({ malloc, free });

    const scalar = scratch.outputU32("advance");
    const bytes = scratch.bytes(128 * 1024, "write snapshot");
    for (let index = 0; index < 10_000; index += 1) {
      expect(scratch.outputU32("advance")).toBe(scalar);
      expect(scratch.bytes(128 * 1024, "write snapshot")).toBe(bytes);
    }

    expect(malloc.mock.calls.map(([size]) => size)).toEqual([4, 128 * 1024]);
    expect(free).not.toHaveBeenCalled();
  });

  it("grows without discarding a usable buffer when allocation fails", () => {
    const free = vi.fn();
    const malloc = vi.fn()
      .mockReturnValueOnce(100)
      .mockReturnValueOnce(0)
      .mockReturnValueOnce(200);
    const scratch = new WasmScratchBuffers({ malloc, free });

    expect(scratch.bytes(64, "first")).toBe(100);
    expect(() => scratch.bytes(128, "grow")).toThrow(/allocation failed/i);
    expect(scratch.bytes(32, "reuse")).toBe(100);
    expect(scratch.bytes(128, "grow")).toBe(200);
    expect(free).toHaveBeenCalledWith(100);
  });

  it("releases retained memory exactly once", () => {
    let pointer = 4;
    const free = vi.fn();
    const scratch = new WasmScratchBuffers({ malloc: (size) => (pointer += size), free });
    const scalar = scratch.outputU32("advance");
    const bytes = scratch.bytes(32, "snapshot");

    scratch.release();
    scratch.release();

    expect(free.mock.calls.map(([value]) => value)).toEqual([scalar, bytes]);
    expect(() => scratch.outputU32("advance")).toThrow(/released/i);
    expect(() => scratch.bytes(32, "snapshot")).toThrow(/released/i);
  });
});
