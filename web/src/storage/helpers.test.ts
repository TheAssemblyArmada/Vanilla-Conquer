import { afterEach, describe, expect, it, vi } from "vitest";
import { checkStorageReadiness, sha256 } from "./helpers";

afterEach(() => vi.unstubAllGlobals());

describe("sha256", () => {
  it("returns the standard SHA-256 digest", async () => {
    await expect(sha256(new TextEncoder().encode("abc"))).resolves.toBe(
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
    );
  });

  it("hashes only the selected Uint8Array subview without modifying its buffer", async () => {
    const bytes = Uint8Array.from([0xff, 0x61, 0x62, 0x63, 0xee]);
    const before = bytes.slice();
    await expect(sha256(bytes.subarray(1, 4))).resolves.toBe(
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
    );
    expect(bytes).toEqual(before);
  });
});

describe("storage readiness", () => {
  it("observes persistence without making a blocking permission request", async () => {
    const persist = vi.fn(() => new Promise<boolean>(() => undefined));
    vi.stubGlobal("navigator", {
      storage: {
        getDirectory: vi.fn(),
        estimate: vi.fn().mockResolvedValue({ quota: 1_000, usage: 250 }),
        persisted: vi.fn().mockResolvedValue(true),
        persist,
      },
    });

    await expect(checkStorageReadiness(500)).resolves.toEqual({
      supported: true,
      persisted: true,
      quota: 1_000,
      usage: 250,
      available: 750,
      enoughSpace: true,
    });
    expect(persist).not.toHaveBeenCalled();
  });

  it("keeps OPFS usable when persistence status is unavailable", async () => {
    vi.stubGlobal("navigator", {
      storage: {
        getDirectory: vi.fn(),
        estimate: vi.fn().mockResolvedValue({}),
      },
    });

    await expect(checkStorageReadiness()).resolves.toMatchObject({
      supported: true,
      persisted: false,
      enoughSpace: true,
    });
  });
});
