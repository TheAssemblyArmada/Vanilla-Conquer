// @vitest-environment node
import { webcrypto } from "node:crypto";
import { readFile } from "node:fs/promises";
import { beforeAll, describe, expect, it, vi } from "vitest";
import { MemoryBinaryStore } from "./BinaryStore";
import { ContentStore, type ContentFileDescriptor, type ContentManifest } from "./ContentStore";
import { sha256 } from "./helpers";

beforeAll(() => {
  Object.defineProperty(globalThis, "crypto", { value: webcrypto, configurable: true });
});

function hexBytes(value: string): Uint8Array {
  return Uint8Array.from(value.match(/../g) ?? [], (part) => Number.parseInt(part, 16));
}

async function contentDigest(files: ContentFileDescriptor[]): Promise<string> {
  const encoder = new TextEncoder();
  const chunks: Uint8Array[] = [encoder.encode("CNCWEB-CONTENT-MANIFEST-V1\0")];
  for (const file of files) {
    const path = encoder.encode(file.path);
    const metadata = new Uint8Array(16);
    const view = new DataView(metadata.buffer);
    view.setBigUint64(0, BigInt(path.byteLength), true);
    view.setBigUint64(8, BigInt(file.size), true);
    const role = encoder.encode(file.role);
    const roleLength = new Uint8Array(8);
    new DataView(roleLength.buffer).setBigUint64(0, BigInt(role.byteLength), true);
    chunks.push(metadata.subarray(0, 8), path, metadata.subarray(8), hexBytes(file.sha256), roleLength, role);
  }
  const total = chunks.reduce((sum, chunk) => sum + chunk.byteLength, 0);
  const data = new Uint8Array(total);
  let offset = 0;
  for (const chunk of chunks) { data.set(chunk, offset); offset += chunk.byteLength; }
  return sha256(data);
}

async function fixture(payload = "synthetic audio payload"): Promise<{ manifest: ContentManifest; data: Uint8Array }> {
  const data = new TextEncoder().encode(payload);
  const files: ContentFileDescriptor[] = [{ path: "audio/test.bin", size: data.byteLength, sha256: await sha256(data), role: "audio" }];
  return {
    data,
    manifest: {
      format: "cncweb-content",
      version: 1,
      package_id: "synthetic-slice",
      created_at_unix_ms: 1_700_000_000_000,
      source: { product: "cnc-remastered-collection", provider: "copied-installation", install_fingerprint_sha256: "09".repeat(32) },
      content: { games: ["tiberian-dawn"], locales: ["en-US"] },
      content_sha256: await contentDigest(files),
      files,
    },
  };
}

describe("ContentStore", () => {
  it("accepts the canonical Rust packer ManifestV1 fixture", async () => {
    const fixtureUrl = new URL("../../../tools/content-packer/fixtures/manifest-v1.example.json", import.meta.url);
    const manifest = JSON.parse(await readFile(fixtureUrl, "utf8")) as ContentManifest;
    const data = new TextEncoder().encode("{}\n");
    const store = new ContentStore(new MemoryBinaryStore());
    const installed = await store.install(manifest, new Map([["config/example.json", data]]));
    expect(installed.id).toBe("synthetic-contract-example");
  });

  it("accepts only the supported freeware product/provider provenance pair", async () => {
    const freeware = await fixture("freeware payload");
    freeware.manifest.source = {
      ...freeware.manifest.source,
      product: "tiberian-dawn-freeware",
      provider: "ea-freeware",
    };
    const store = new ContentStore(new MemoryBinaryStore());
    await expect(store.install(freeware.manifest, new Map([["audio/test.bin", freeware.data]]))).resolves.toMatchObject({ id: "synthetic-slice" });

    const crossed = await fixture("crossed provenance");
    crossed.manifest.source = { ...crossed.manifest.source, provider: "ea-freeware" };
    await expect(new ContentStore(new MemoryBinaryStore()).install(
      crossed.manifest,
      new Map([["audio/test.bin", crossed.data]]),
    )).rejects.toThrow(/product and provider/i);
  });

  it("transactionally installs and verifies a packer-compatible manifest", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const { manifest, data } = await fixture();
    const installed = await store.install(manifest, new Map([["audio/test.bin", data]]));
    expect(installed.id).toBe("synthetic-slice");
    expect(await store.readFile(installed.id, "audio/test.bin")).toEqual(data);
    const selected = await store.getRevisionDescriptor(installed.id);
    expect(selected.storageKey).toContain(selected.revision);
    expect(await store.readRevisionFile(selected, "audio/test.bin")).toEqual(data);
    expect((await store.list()).map((entry) => entry.id)).toEqual(["synthetic-slice"]);
  });

  it("writes one unreachable candidate and commits its index last", async () => {
    class RecordingStore extends MemoryBinaryStore {
      readonly writes: string[] = [];
      override async write(path: string, data: Uint8Array): Promise<void> {
        this.writes.push(path);
        await super.write(path, data);
      }
    }
    const binary = new RecordingStore();
    const store = new ContentStore(binary);
    const { manifest, data } = await fixture("single candidate payload");

    await store.install(manifest, new Map([["audio/test.bin", data]]));

    const contentWrites = binary.writes.filter((path) => path.includes("/files/audio/test.bin"));
    expect(contentWrites).toHaveLength(1);
    expect(contentWrites[0]).toMatch(/^content\/synthetic-slice\//);
    expect(binary.writes.some((path) => path.startsWith("content-staging/"))).toBe(false);
    expect(binary.writes.at(-1)).toBe("content-index/synthetic-slice.json");
  });

  it("does not commit an index when file verification fails", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const { manifest } = await fixture();
    await expect(store.install(manifest, new Map([["audio/test.bin", new Uint8Array([1, 2, 3])]]))).rejects.toThrow(/size mismatch/i);
    expect(await binary.list("content-index")).toEqual([]);
    expect(await binary.list("content")).toEqual([]);
  });

  it("preserves a committed same-revision install and garbage-collects replaced revisions", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const first = await fixture("first payload");
    const installed = await store.install(first.manifest, new Map([["audio/test.bin", first.data]]));
    await store.install(first.manifest, new Map([["audio/test.bin", new Uint8Array([9])]]));
    expect(await store.readFile(installed.id, "audio/test.bin")).toEqual(first.data);

    const second = await fixture("second payload");
    const replacement = await store.install(second.manifest, new Map([["audio/test.bin", second.data]]));
    const contentPaths = await binary.list("content/synthetic-slice");
    expect(contentPaths.every((path) => path.includes(replacement.revision))).toBe(true);
    expect(await binary.list("content-staging")).toEqual([]);
  });

  it("serializes concurrent same-revision installers across store instances", async () => {
    const binary = new MemoryBinaryStore();
    const firstStore = new ContentStore(binary);
    const secondStore = new ContentStore(binary);
    const { manifest, data } = await fixture("concurrent payload");
    let releaseFirst!: () => void;
    const firstMayFinish = new Promise<void>((resolve) => { releaseFirst = resolve; });
    let firstEntered!: () => void;
    const firstDidEnter = new Promise<void>((resolve) => { firstEntered = resolve; });
    const firstLoader = vi.fn(async () => {
      firstEntered();
      await firstMayFinish;
      return data;
    });
    const secondLoader = vi.fn(async () => new Uint8Array([9]));

    const firstInstall = firstStore.installFromLoader(manifest, firstLoader);
    await firstDidEnter;
    const secondInstall = secondStore.installFromLoader(manifest, secondLoader);
    await new Promise((resolve) => setTimeout(resolve, 0));
    expect(secondLoader).not.toHaveBeenCalled();
    releaseFirst();

    const [first, second] = await Promise.all([firstInstall, secondInstall]);
    expect(second.revision).toBe(first.revision);
    expect(secondLoader).not.toHaveBeenCalled();
    expect(await secondStore.readFile(first.id, "audio/test.bin")).toEqual(data);
    expect(await binary.list("content-staging")).toEqual([]);
  });

  it("repairs a corrupted same-revision root without exposing a partial replacement", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const { manifest, data } = await fixture("repair payload");
    await store.install(manifest, new Map([["audio/test.bin", data]]));
    const oldFile = (await binary.list("content/synthetic-slice")).find((path) => path.endsWith("/files/audio/test.bin"))!;
    await binary.write(oldFile, new Uint8Array(data.byteLength).fill(0xff));
    const loader = vi.fn(async () => data);

    await store.installFromLoader(manifest, loader);
    expect(loader).toHaveBeenCalledOnce();
    expect(await store.readFile("synthetic-slice", "audio/test.bin")).toEqual(data);
    const newFiles = (await binary.list("content/synthetic-slice")).filter((path) => path.endsWith("/files/audio/test.bin"));
    expect(newFiles).toHaveLength(1);
    expect(newFiles[0]).not.toBe(oldFile);
    const index = JSON.parse(new TextDecoder().decode(await binary.read("content-index/synthetic-slice.json"))) as { schemaVersion: number; storageKey: string };
    expect(index.schemaVersion).toBe(2);
    expect(index.storageKey).toContain("-");
  });

  it("repairs an unreadable index by committing a verified candidate last", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const { manifest, data } = await fixture("index repair payload");
    await store.install(manifest, new Map([["audio/test.bin", data]]));
    await binary.write("content-index/synthetic-slice.json", new TextEncoder().encode("{torn"));
    const loader = vi.fn(async () => data);

    const repaired = await store.installFromLoader(manifest, loader);

    expect(loader).toHaveBeenCalledOnce();
    expect(await store.readFile(repaired.id, "audio/test.bin")).toEqual(data);
    const index = JSON.parse(new TextDecoder().decode(await binary.read("content-index/synthetic-slice.json"))) as { schemaVersion: number; revision: string; storageKey: string };
    expect(index).toMatchObject({ schemaVersion: 2, revision: repaired.revision });
    expect(index.storageKey).toContain(`${repaired.revision}-`);
  });

  it("isolates broken package indexes while preserving valid installs and issue identity", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const { manifest, data } = await fixture("valid package beside corrupt indexes");
    await store.install(manifest, new Map([["audio/test.bin", data]]));
    const missingRevision = "ab".repeat(32);
    await binary.write("content-index/missing-pack.json", new TextEncoder().encode(JSON.stringify({
      schemaVersion: 2,
      id: "missing-pack",
      revision: missingRevision,
      storageKey: `${missingRevision}-deadbeef`,
      installedAt: new Date(0).toISOString(),
    })));
    await binary.write("content-index/torn-pack.json", new TextEncoder().encode("{torn"));

    const listing = await store.listWithIssues();

    expect(listing.installed.map((entry) => entry.id)).toEqual(["synthetic-slice"]);
    expect(listing.issues).toEqual(expect.arrayContaining([
      expect.objectContaining({ indexPath: "content-index/missing-pack.json", id: "missing-pack", revision: missingRevision }),
      expect.objectContaining({ indexPath: "content-index/torn-pack.json", id: "torn-pack" }),
    ]));
    expect((await store.list()).map((entry) => entry.id)).toEqual(["synthetic-slice"]);
  });

  it("keeps the previously committed root and index when repair fails", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const { manifest, data } = await fixture("failed repair payload");
    await store.install(manifest, new Map([["audio/test.bin", data]]));
    const oldFile = (await binary.list("content/synthetic-slice")).find((path) => path.endsWith("/files/audio/test.bin"))!;
    await binary.write(oldFile, new Uint8Array(data.byteLength).fill(0xee));
    const oldPaths = await binary.list("content/synthetic-slice");
    const oldIndex = await binary.read("content-index/synthetic-slice.json");

    await expect(store.install(manifest, new Map([["audio/test.bin", new Uint8Array([1])]]))).rejects.toThrow(/size mismatch/i);
    expect(await binary.list("content/synthetic-slice")).toEqual(oldPaths);
    expect(await binary.read("content-index/synthetic-slice.json")).toEqual(oldIndex);
    expect(await binary.list("content-staging")).toEqual([]);
  });

  it("keeps an older committed revision when a replacement import fails", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const first = await fixture("stable payload");
    await store.install(first.manifest, new Map([["audio/test.bin", first.data]]));
    const before = await binary.list("content/synthetic-slice");
    const replacement = await fixture("replacement payload");
    await expect(store.install(replacement.manifest, new Map([["audio/test.bin", new Uint8Array([1])]]))).rejects.toThrow(/size mismatch/i);
    expect(await binary.list("content/synthetic-slice")).toEqual(before);
    expect(await store.readFile("synthetic-slice", "audio/test.bin")).toEqual(first.data);
  });

  it("removes a complete candidate and preserves the old revision when the index commit fails", async () => {
    class FailingIndexStore extends MemoryBinaryStore {
      failNextIndexWrite = false;
      override async write(path: string, data: Uint8Array): Promise<void> {
        if (this.failNextIndexWrite && path === "content-index/synthetic-slice.json") {
          this.failNextIndexWrite = false;
          throw new DOMException("synthetic quota failure", "QuotaExceededError");
        }
        await super.write(path, data);
      }
    }
    const binary = new FailingIndexStore();
    const store = new ContentStore(binary);
    const first = await fixture("committed before index failure");
    await store.install(first.manifest, new Map([["audio/test.bin", first.data]]));
    const beforePaths = await binary.list("content/synthetic-slice");
    const beforeIndex = await binary.read("content-index/synthetic-slice.json");
    const replacement = await fixture("candidate whose index fails");
    binary.failNextIndexWrite = true;

    await expect(store.install(replacement.manifest, new Map([["audio/test.bin", replacement.data]]))).rejects.toThrow("quota failure");

    expect(await binary.list("content/synthetic-slice")).toEqual(beforePaths);
    expect(await binary.read("content-index/synthetic-slice.json")).toEqual(beforeIndex);
    expect(await store.readFile("synthetic-slice", "audio/test.bin")).toEqual(first.data);
  });

  it("uses shared Web Locks for immutable reads and exclusive locks for mutations", async () => {
    const originalNavigator = Object.getOwnPropertyDescriptor(globalThis, "navigator");
    const request = vi.fn(async (...args: unknown[]) => {
      const callback = args.at(-1) as (lock: Lock) => Promise<unknown>;
      const options = args[1] as LockOptions;
      return callback({ name: String(args[0]), mode: options.mode } as Lock);
    });
    Object.defineProperty(globalThis, "navigator", { value: { locks: { request } }, configurable: true });
    try {
      const binary = new MemoryBinaryStore();
      const { manifest, data } = await fixture("web lock payload");
      const store = new ContentStore(binary);
      await store.install(manifest, new Map([["audio/test.bin", data]]));
      const revision = await store.getRevisionDescriptor(manifest.package_id);
      await store.get(manifest.package_id);
      await store.listWithIssues();
      await store.readFile(manifest.package_id, "audio/test.bin");
      await store.readRevisionFile(revision, "audio/test.bin");
      await store.uninstall(manifest.package_id);

      expect(request.mock.calls.map(([name]) => name)).toEqual(Array(7).fill("theater-content:synthetic-slice"));
      expect(request.mock.calls.map(([, options]) => options)).toEqual([
        { mode: "exclusive" },
        { mode: "shared" },
        { mode: "shared" },
        { mode: "shared" },
        { mode: "shared" },
        { mode: "shared" },
        { mode: "exclusive" },
      ]);
    } finally {
      if (originalNavigator) Object.defineProperty(globalThis, "navigator", originalNavigator);
      else Reflect.deleteProperty(globalThis, "navigator");
    }
  });

  it("lets an immutable audio read join a held shared lease while replacement waits", async () => {
    type PendingLock = {
      mode: LockMode;
      callback: (lock: Lock) => Promise<unknown> | unknown;
      resolve: (value: unknown) => void;
      reject: (error: unknown) => void;
    };
    const queue: PendingLock[] = [];
    let activeShared = 0;
    let activeExclusive = false;
    const requestedModes: LockMode[] = [];
    const drain = (): void => {
      if (activeExclusive || queue.length === 0) return;
      const next = queue[0];
      if (next.mode === "exclusive") {
        if (activeShared !== 0) return;
        queue.shift();
        activeExclusive = true;
        void Promise.resolve(next.callback({ name: "theater-content:synthetic-slice", mode: "exclusive" } as Lock))
          .then(next.resolve, next.reject)
          .finally(() => { activeExclusive = false; drain(); });
        return;
      }
      while (queue[0]?.mode === "shared" && !activeExclusive) {
        const shared = queue.shift()!;
        activeShared += 1;
        void Promise.resolve(shared.callback({ name: "theater-content:synthetic-slice", mode: "shared" } as Lock))
          .then(shared.resolve, shared.reject)
          .finally(() => { activeShared -= 1; drain(); });
      }
    };
    const request = vi.fn((name: string, options: LockOptions, callback: (lock: Lock) => Promise<unknown> | unknown) => {
      expect(name).toBe("theater-content:synthetic-slice");
      const mode = options.mode ?? "exclusive";
      requestedModes.push(mode);
      return new Promise<unknown>((resolve, reject) => {
        queue.push({ mode, callback, resolve, reject });
        drain();
      });
    });

    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const first = await fixture("audio bytes readable during an engine lease");
    await store.install(first.manifest, new Map([["audio/test.bin", first.data]]));
    const revision = await store.getRevisionDescriptor(first.manifest.package_id);
    const replacement = await fixture("replacement waits for the engine lease");
    const replacementLoader = vi.fn(async () => replacement.data);
    const originalNavigator = Object.getOwnPropertyDescriptor(globalThis, "navigator");
    Object.defineProperty(globalThis, "navigator", { value: { locks: { request } }, configurable: true });
    let releaseLease!: () => void;
    const leaseLifetime = new Promise<void>((resolve) => { releaseLease = resolve; });
    let leaseEntered!: () => void;
    const entered = new Promise<void>((resolve) => { leaseEntered = resolve; });
    try {
      const lease = request("theater-content:synthetic-slice", { mode: "shared" }, async () => {
        leaseEntered();
        await leaseLifetime;
      });
      await entered;

      await expect(store.readRevisionFile(revision, "audio/test.bin")).resolves.toEqual(first.data);
      expect(requestedModes).toEqual(["shared", "shared"]);

      let replacementSettled = false;
      const install = store.installFromLoader(replacement.manifest, replacementLoader)
        .finally(() => { replacementSettled = true; });
      await vi.waitFor(() => expect(requestedModes).toEqual(["shared", "shared", "exclusive"]));
      expect(replacementLoader).not.toHaveBeenCalled();
      expect(replacementSettled).toBe(false);

      releaseLease();
      await lease;
      await install;
      expect(replacementLoader).toHaveBeenCalledOnce();
    } finally {
      releaseLease();
      if (originalNavigator) Object.defineProperty(globalThis, "navigator", originalNavigator);
      else Reflect.deleteProperty(globalThis, "navigator");
    }
  });

  it("verifies and migrates a legacy deterministic-root index without reloading sources", async () => {
    const binary = new MemoryBinaryStore();
    const store = new ContentStore(binary);
    const { manifest, data } = await fixture("legacy index payload");
    const installed = await store.install(manifest, new Map([["audio/test.bin", data]]));
    const currentPaths = await binary.list("content/synthetic-slice");
    const manifestPath = currentPaths.find((path) => path.endsWith("/manifest.json"))!;
    const filePath = currentPaths.find((path) => path.endsWith("/files/audio/test.bin"))!;
    const manifestBytes = await binary.read(manifestPath);
    const fileBytes = await binary.read(filePath);
    await binary.removeTree("content/synthetic-slice");
    await binary.write(`content/synthetic-slice/${installed.revision}/manifest.json`, manifestBytes);
    await binary.write(`content/synthetic-slice/${installed.revision}/files/audio/test.bin`, fileBytes);
    await binary.write("content-index/synthetic-slice.json", new TextEncoder().encode(JSON.stringify({ schemaVersion: 1, id: "synthetic-slice", revision: installed.revision, installedAt: installed.installedAt })));
    const loader = vi.fn(async () => new Uint8Array([0]));

    await store.installFromLoader(manifest, loader);
    expect(loader).not.toHaveBeenCalled();
    const migrated = JSON.parse(new TextDecoder().decode(await binary.read("content-index/synthetic-slice.json"))) as { schemaVersion: number; storageKey: string };
    expect(migrated).toMatchObject({ schemaVersion: 2, storageKey: installed.revision });
    expect(await store.readFile("synthetic-slice", "audio/test.bin")).toEqual(data);
  });

  it("rejects traversal paths before touching storage", async () => {
    const binary = new MemoryBinaryStore();
    const { manifest } = await fixture();
    manifest.files[0].path = "../escape.bin";
    const store = new ContentStore(binary);
    await expect(store.install(manifest, new Map())).rejects.toThrow(/invalid content path/i);
    expect(await binary.list("")).toEqual([]);
  });

  it("rejects package IDs and locales outside the shared schema", async () => {
    const { manifest } = await fixture();
    manifest.package_id = ".hidden";
    await expect(new ContentStore(new MemoryBinaryStore()).install(manifest, new Map())).rejects.toThrow(/Package ID/i);

    const second = await fixture();
    second.manifest.content.locales = ["en--US"];
    await expect(new ContentStore(new MemoryBinaryStore()).install(second.manifest, new Map())).rejects.toThrow(/locale/i);
  });
});
