// @vitest-environment node
import { webcrypto } from "node:crypto";
import { beforeAll, describe, expect, it } from "vitest";
import { MemoryBinaryStore } from "./BinaryStore";
import { SaveStore, type SaveMetadata } from "./SaveStore";

beforeAll(() => {
  Object.defineProperty(globalThis, "crypto", { value: webcrypto, configurable: true });
});

describe("SaveStore", () => {
  it("commits, lists, validates, and deletes save data", async () => {
    const binary = new MemoryBinaryStore();
    const store = new SaveStore(binary);
    const timestamp = new Date(1_700_000_000_000).toISOString();
    await store.write(
      { id: "manual-1", name: "Synthetic save", game: "demo", scenario: "test", kind: "manual", tick: 15, createdAt: timestamp, updatedAt: timestamp, presentation: { cameraX: 120, cameraY: -24, zoom: 1.5, graphicsMode: "remastered" } },
      new Uint8Array([4, 8, 15, 16, 23, 42]),
    );
    expect((await store.list())[0]).toMatchObject({ id: "manual-1", tick: 15, byteLength: 6, presentation: { cameraX: 120, cameraY: -24, zoom: 1.5, graphicsMode: "remastered" } });
    expect([...(await store.read("manual-1")).data]).toEqual([4, 8, 15, 16, 23, 42]);
    await store.write(
      { id: "manual-1", name: "Synthetic save", game: "demo", scenario: "test", kind: "manual", tick: 16, createdAt: timestamp, updatedAt: new Date(1_700_000_001_000).toISOString() },
      new Uint8Array([1, 2, 3]),
    );
    expect(await binary.list("save-data/manual-1")).toHaveLength(1);
    await store.remove("manual-1");
    expect(await store.list()).toEqual([]);
  });

  it("detects corruption behind a committed index", async () => {
    const binary = new MemoryBinaryStore();
    const store = new SaveStore(binary);
    const now = new Date().toISOString();
    const metadata = await store.write({ id: "auto", name: "Auto", game: "demo", scenario: "test", kind: "autosave", tick: 2, createdAt: now, updatedAt: now }, new Uint8Array([1, 2]));
    await binary.write(`save-data/auto/${metadata.revision}.bin`, new Uint8Array([9, 9]));
    await expect(store.read("auto")).rejects.toThrow(/validation/i);
  });

  it("lists valid saves independently and quarantines malformed active indexes", async () => {
    const binary = new MemoryBinaryStore();
    const store = new SaveStore(binary);
    const now = new Date().toISOString();
    await store.write({ id: "valid-save", name: "Valid", game: "demo", scenario: "test", kind: "manual", tick: 3, createdAt: now, updatedAt: now }, new Uint8Array([1, 2, 3]));
    await binary.write("save-index/torn-save.json", new TextEncoder().encode("{torn"));

    const listing = await store.listWithIssues();

    expect(listing.saves.map((save) => save.id)).toEqual(["valid-save"]);
    expect(listing.issues).toEqual([expect.objectContaining({ id: "torn-save", indexPath: "save-index/torn-save.json", quarantined: true })]);
    expect(await binary.list("save-index")).toEqual(["save-index/valid-save.json"]);
    expect(await binary.list("save-quarantine/torn-save")).toHaveLength(1);
    expect(await store.listWithIssues()).toMatchObject({ saves: [expect.objectContaining({ id: "valid-save" })], issues: [] });
  });

  it("tries resume candidates newest-first, quarantines corrupt payloads, and falls back", async () => {
    const binary = new MemoryBinaryStore();
    const store = new SaveStore(binary);
    const createdAt = new Date(1_700_000_000_000).toISOString();
    const older = await store.write({ id: "older-save", name: "Older", game: "demo", scenario: "test", kind: "manual", tick: 4, createdAt, updatedAt: new Date(1_700_000_001_000).toISOString() }, new Uint8Array([1, 2, 3]));
    const newest = await store.write({ id: "newest-save", name: "Newest", game: "demo", scenario: "test", kind: "autosave", tick: 8, createdAt, updatedAt: new Date(1_700_000_002_000).toISOString() }, new Uint8Array([4, 5, 6]));
    await binary.write(`save-data/newest-save/${newest.revision}.bin`, new Uint8Array([9, 9, 9]));

    const selection = await store.readNewestValid([newest, older].reverse());

    expect(selection.save?.metadata.id).toBe("older-save");
    expect(selection.issues).toEqual([expect.objectContaining({ id: "newest-save", revision: newest.revision, quarantined: true })]);
    expect(await binary.list("save-index")).toEqual(["save-index/older-save.json"]);
    expect(await binary.list("save-quarantine/newest-save")).toHaveLength(1);
    await expect(store.readNewestValid(await store.list())).resolves.toMatchObject({ save: { metadata: { id: "older-save" } }, issues: [] });
  });

  it("rejects missing, unknown, and invalid metadata fields exactly", async () => {
    const store = new SaveStore(new MemoryBinaryStore());
    const now = new Date().toISOString();
    const metadata = { id: "strict-save", name: "Strict", game: "demo" as const, scenario: "test", kind: "manual" as const, tick: 1, createdAt: now, updatedAt: now };
    await expect(store.write({ ...metadata, unexpected: true } as unknown as SaveMetadata, new Uint8Array([1]))).rejects.toThrow("missing or unknown fields");
    await expect(store.write({ ...metadata, game: "unsupported" } as unknown as SaveMetadata, new Uint8Array([1]))).rejects.toThrow("Save game is invalid");
    await expect(store.write({ ...metadata, createdAt: "2024-01-01" }, new Uint8Array([1]))).rejects.toThrow("Save timestamp is invalid");
  });

  it("persists a complete content identity and rejects partial identities", async () => {
    const store = new SaveStore(new MemoryBinaryStore());
    const now = new Date().toISOString();
    const identity = { contentPackageId: "owned-pack", contentRevision: "ab".repeat(32), missionId: "gdi-01-east-a" };
    await store.write({ id: "mission-auto", name: "Mission", game: "tiberian-dawn", scenario: "gdi-01-east-a", kind: "autosave", tick: 1, createdAt: now, updatedAt: now, runId: "campaign-run-1", ...identity }, new Uint8Array([1]));
    expect((await store.list())[0]).toMatchObject({ ...identity, runId: "campaign-run-1" });
    await expect(store.write({ id: "partial", name: "Bad", game: "tiberian-dawn", scenario: "gdi-01-east-a", kind: "manual", tick: 1, createdAt: now, updatedAt: now, contentPackageId: "owned-pack" }, new Uint8Array([1]))).rejects.toThrow("complete");
    await expect(store.write({ id: "mismatch", name: "Bad", game: "tiberian-dawn", scenario: "gdi-02", kind: "manual", tick: 1, createdAt: now, updatedAt: now, ...identity }, new Uint8Array([1]))).rejects.toThrow("does not match");
    await expect(store.write({ id: "bad-run", name: "Bad", game: "tiberian-dawn", scenario: "gdi-01-east-a", kind: "manual", tick: 1, createdAt: now, updatedAt: now, runId: "../run", ...identity }, new Uint8Array([1]))).rejects.toThrow("run ID");
  });

  it("rejects presentation state outside the renderer contract", async () => {
    const store = new SaveStore(new MemoryBinaryStore());
    const now = new Date().toISOString();
    const metadata = { id: "bad-view", name: "Bad view", game: "demo" as const, scenario: "test", kind: "manual" as const, tick: 1, createdAt: now, updatedAt: now };
    await expect(store.write({ ...metadata, presentation: { cameraX: 0, cameraY: 0, zoom: 3, graphicsMode: "classic" } }, new Uint8Array([1]))).rejects.toThrow("zoom");
  });

  it("serializes concurrent updates to one save index and cleans superseded data", async () => {
    const binary = new MemoryBinaryStore();
    const first = new SaveStore(binary);
    const second = new SaveStore(binary);
    const now = new Date().toISOString();
    const metadata = { id: "shared-auto", name: "Shared", game: "demo" as const, scenario: "test", kind: "autosave" as const, tick: 3, createdAt: now, updatedAt: now };
    await Promise.all([
      first.write(metadata, new Uint8Array([1, 2, 3])),
      second.write({ ...metadata, tick: 4 }, new Uint8Array([4, 5, 6, 7])),
    ]);
    expect(await binary.list("save-data/shared-auto")).toHaveLength(1);
    const committed = await first.read("shared-auto");
    expect([[1, 2, 3], [4, 5, 6, 7]]).toContainEqual([...committed.data]);
  });
});
