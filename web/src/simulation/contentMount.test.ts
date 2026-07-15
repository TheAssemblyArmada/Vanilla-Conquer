// @vitest-environment node
import { webcrypto } from "node:crypto";
import { beforeAll, describe, expect, it, vi } from "vitest";
import { sha256 } from "../storage/helpers";
import { acquireContentMountLeaseFromOpfs, mountPreparedContent, prepareContentMount, type EmscriptenModuleWithFs } from "./contentMount";
import type { ContentMountRequest } from "./protocol";

beforeAll(() => { Object.defineProperty(globalThis, "crypto", { value: webcrypto, configurable: true }); });

describe("immutable content mounts", () => {
  it("verifies blobs before exposing the canonical WORKERFS mount", async () => {
    const bytes = new TextEncoder().encode("owned synthetic MIX");
    const revision = "ab".repeat(32);
    const request: ContentMountRequest = { packageId: "owned-pack", revision, storageKey: revision, files: [{ path: "engine/td/GENERAL.MIX", size: bytes.byteLength, sha256: await sha256(bytes) }] };
    const progress = vi.fn();
    const prepared = await prepareContentMount(request, async () => new Blob([bytes]), progress);
    expect(prepared.root).toBe(`/cnc-content/${revision.slice(0, 16)}`);
    expect(prepared.blobs[0]).toMatchObject({ name: "engine/td/GENERAL.MIX" });
    expect(progress.mock.calls.at(-1)?.[0]).toMatchObject({ phase: "verifying", completedFiles: 1, completedBytes: bytes.byteLength });

    const workerFs = {};
    const mkdirTree = vi.fn();
    const mount = vi.fn();
    mountPreparedContent({ FS: { filesystems: { WORKERFS: workerFs }, mkdirTree, mount } } as EmscriptenModuleWithFs, prepared);
    expect(mkdirTree).toHaveBeenCalledWith(prepared.root);
    expect(mount).toHaveBeenCalledWith(workerFs, { blobs: prepared.blobs }, prepared.root);
  });

  it("rejects a changed OPFS blob before Wasm is loaded", async () => {
    const expected = new TextEncoder().encode("expected");
    const revision = "cd".repeat(32);
    const request: ContentMountRequest = { packageId: "owned-pack", revision, storageKey: revision, files: [{ path: "engine/td/GENERAL.MIX", size: expected.byteLength, sha256: await sha256(expected) }] };
    await expect(prepareContentMount(request, async () => new Blob([new TextEncoder().encode("tampered")]))).rejects.toThrow(/checksum mismatch/i);
  });

  it("rejects a physical revision replaced while an unlocked OPFS mount is prepared", async () => {
    const bytes = new TextEncoder().encode("stable engine bytes");
    const revision = "ef".repeat(32);
    const request: ContentMountRequest = { packageId: "owned-pack", revision, storageKey: revision, files: [{ path: "engine/td/GENERAL.MIX", size: bytes.byteLength, sha256: await sha256(bytes) }] };
    let indexReads = 0;
    const files = new Map<string, Blob>([[`content/owned-pack/${revision}/files/engine/td/GENERAL.MIX`, new Blob([bytes])]]);
    const root = {
      path: "",
      async getDirectoryHandle(this: { path: string }, name: string) { return Object.assign(Object.create(this), { path: this.path ? `${this.path}/${name}` : name }); },
      async getFileHandle(this: { path: string }, name: string) {
        const path = this.path ? `${this.path}/${name}` : name;
        return {
          getFile: async () => {
            if (path === "content-index/owned-pack.json") {
              indexReads += 1;
              const storageKey = indexReads === 1 ? revision : `${revision}-replacement`;
              return new Blob([JSON.stringify({ schemaVersion: 2, id: "owned-pack", revision, storageKey, installedAt: new Date(0).toISOString() })]);
            }
            const file = files.get(path);
            if (!file) throw new DOMException(`Missing ${path}`, "NotFoundError");
            return file;
          },
        };
      },
    } as unknown as FileSystemDirectoryHandle;
    const originalNavigator = Object.getOwnPropertyDescriptor(globalThis, "navigator");
    Object.defineProperty(globalThis, "navigator", { value: { storage: { getDirectory: async () => root } }, configurable: true });
    try {
      await expect(acquireContentMountLeaseFromOpfs(request)).rejects.toThrow("physical content revision changed while mounting");
      expect(indexReads).toBe(2);
    } finally {
      if (originalNavigator) Object.defineProperty(globalThis, "navigator", originalNavigator);
      else Reflect.deleteProperty(globalThis, "navigator");
    }
  });

  it("recovers when the first OPFS traversal transiently reports NotFound", async () => {
    const bytes = new TextEncoder().encode("stable engine bytes");
    const revision = "34".repeat(32);
    const request: ContentMountRequest = { packageId: "owned-pack", revision, storageKey: revision, files: [{ path: "engine/td/GENERAL.MIX", size: bytes.byteLength, sha256: await sha256(bytes) }] };
    const files = new Map<string, Blob>([
      ["content-index/owned-pack.json", new Blob([JSON.stringify({ schemaVersion: 2, id: request.packageId, revision, storageKey: revision, installedAt: new Date(0).toISOString() })])],
      [`content/owned-pack/${revision}/files/engine/td/GENERAL.MIX`, new Blob([bytes])],
    ]);
    let transientFailures = 0;
    const root = {
      path: "",
      async getDirectoryHandle(this: { path: string }, name: string) {
        if (!this.path && name === "content-index" && transientFailures === 0) {
          transientFailures += 1;
          throw new DOMException("transient traversal", "NotFoundError");
        }
        return Object.assign(Object.create(this), { path: this.path ? `${this.path}/${name}` : name });
      },
      async getFileHandle(this: { path: string }, name: string) {
        const path = this.path ? `${this.path}/${name}` : name;
        return { getFile: async () => {
          const file = files.get(path);
          if (!file) throw new DOMException(`Missing ${path}`, "NotFoundError");
          return file;
        } };
      },
    } as unknown as FileSystemDirectoryHandle;
    const originalNavigator = Object.getOwnPropertyDescriptor(globalThis, "navigator");
    Object.defineProperty(globalThis, "navigator", { value: { storage: { getDirectory: async () => root } }, configurable: true });
    try {
      const lease = await acquireContentMountLeaseFromOpfs(request);
      expect(transientFailures).toBe(1);
      expect(lease.prepared.blobs[0]).toMatchObject({ name: "engine/td/GENERAL.MIX" });
      await lease.release();
    } finally {
      if (originalNavigator) Object.defineProperty(globalThis, "navigator", originalNavigator);
      else Reflect.deleteProperty(globalThis, "navigator");
    }
  });

  it("rejects request paths before opening OPFS", async () => {
    const getDirectory = vi.fn();
    const originalNavigator = Object.getOwnPropertyDescriptor(globalThis, "navigator");
    Object.defineProperty(globalThis, "navigator", { value: { storage: { getDirectory } }, configurable: true });
    try {
      await expect(acquireContentMountLeaseFromOpfs({
        packageId: "../escape",
        revision: "ab".repeat(32),
        storageKey: "ab".repeat(32),
        files: [{ path: "engine/td/GENERAL.MIX", size: 6, sha256: "cd".repeat(32) }],
      })).rejects.toThrow("Content ID");
      expect(getDirectory).not.toHaveBeenCalled();
    } finally {
      if (originalNavigator) Object.defineProperty(globalThis, "navigator", originalNavigator);
      else Reflect.deleteProperty(globalThis, "navigator");
    }
  });

  it("holds the shared package lock until the mounted runtime releases its lease", async () => {
    const bytes = new TextEncoder().encode("leased engine bytes");
    const revision = "12".repeat(32);
    const request: ContentMountRequest = { packageId: "owned-pack", revision, storageKey: revision, files: [{ path: "engine/td/GENERAL.MIX", size: bytes.byteLength, sha256: await sha256(bytes) }] };
    const index = new Blob([JSON.stringify({ schemaVersion: 2, id: request.packageId, revision, storageKey: revision, installedAt: new Date(0).toISOString() })]);
    const files = new Map<string, Blob>([
      ["content-index/owned-pack.json", index],
      [`content/owned-pack/${revision}/files/engine/td/GENERAL.MIX`, new Blob([bytes])],
    ]);
    const root = {
      path: "",
      async getDirectoryHandle(this: { path: string }, name: string) { return Object.assign(Object.create(this), { path: this.path ? `${this.path}/${name}` : name }); },
      async getFileHandle(this: { path: string }, name: string) {
        const path = this.path ? `${this.path}/${name}` : name;
        return { getFile: async () => {
          const file = files.get(path);
          if (!file) throw new DOMException(`Missing ${path}`, "NotFoundError");
          return file;
        } };
      },
    } as unknown as FileSystemDirectoryHandle;
    let lockHeld = false;
    let lockCompleted = false;
    const lockRequest = vi.fn(async (...args: unknown[]) => {
      const callback = args.at(-1) as (lock: Lock) => Promise<void>;
      lockHeld = true;
      try { await callback({ name: String(args[0]), mode: "shared" } as Lock); }
      finally { lockHeld = false; lockCompleted = true; }
    });
    const originalNavigator = Object.getOwnPropertyDescriptor(globalThis, "navigator");
    Object.defineProperty(globalThis, "navigator", { value: { storage: { getDirectory: async () => root }, locks: { request: lockRequest } }, configurable: true });
    try {
      const lease = await acquireContentMountLeaseFromOpfs(request);
      expect(lease.prepared.blobs[0]).toMatchObject({ name: "engine/td/GENERAL.MIX" });
      expect(lockRequest).toHaveBeenCalledWith("theater-content:owned-pack", { mode: "shared" }, expect.any(Function));
      expect(lockHeld).toBe(true);
      expect(lockCompleted).toBe(false);

      await lease.release();
      expect(lockHeld).toBe(false);
      expect(lockCompleted).toBe(true);
      await expect(lease.release()).resolves.toBeUndefined();
    } finally {
      if (originalNavigator) Object.defineProperty(globalThis, "navigator", originalNavigator);
      else Reflect.deleteProperty(globalThis, "navigator");
    }
  });
});
