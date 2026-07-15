import { validateContentPath } from "../storage/ContentStore";
import { sha256, validateId } from "../storage/helpers";
import { isOpfsNotFoundError, retryTransientOpfsNotFound } from "../storage/opfsRetry";
import type { ContentMountProgress, ContentMountRequest } from "./protocol";
import { contentMountRoot } from "./runtimeCatalog";

export const MAX_MOUNT_FILE_BYTES = 64 * 1024 * 1024;
export const MAX_MOUNT_TOTAL_BYTES = 2 * 1024 * 1024 * 1024;
export const MAX_MOUNT_FILES = 100_000;

export interface PreparedContentMount {
  root: string;
  blobs: Array<{ name: string; data: Blob }>;
  totalBytes: number;
}

export interface ContentMountLease {
  prepared: PreparedContentMount;
  release(): Promise<void>;
}

export interface EmscriptenWorkerFs {
  filesystems?: { WORKERFS?: unknown };
  mkdirTree(path: string): void;
  mount(type: unknown, options: { blobs: Array<{ name: string; data: Blob }> }, path: string): void;
}

export interface EmscriptenModuleWithFs extends Record<string, unknown> {
  FS?: EmscriptenWorkerFs;
}

function progress(request: ContentMountRequest, phase: ContentMountProgress["phase"], completedFiles: number, completedBytes: number, currentPath?: string): ContentMountProgress {
  return {
    phase,
    packageId: request.packageId,
    completedFiles,
    totalFiles: request.files.length,
    completedBytes,
    totalBytes: request.files.reduce((sum, file) => sum + file.size, 0),
    currentPath,
  };
}

function validateContentMountRequest(request: ContentMountRequest): void {
  validateId(request.packageId, "Content ID");
  if (!/^[a-f0-9]{64}$/.test(request.revision)) throw new Error("Content mount revision is invalid");
  if (request.storageKey !== request.revision && (!request.storageKey.startsWith(`${request.revision}-`) || !/^[a-f0-9-]{65,128}$/.test(request.storageKey))) throw new Error("Content mount storage key is invalid");
  if (!Array.isArray(request.files) || request.files.length === 0 || request.files.length > MAX_MOUNT_FILES) throw new Error("Content mount file count is outside the browser profile");
  const seen = new Set<string>();
  let totalBytes = 0;
  for (const file of request.files) {
    validateContentPath(file.path);
    if (seen.has(file.path.toLowerCase())) throw new Error(`Content mount path is duplicated: ${file.path}`);
    seen.add(file.path.toLowerCase());
    if (!Number.isSafeInteger(file.size) || file.size < 0 || file.size > MAX_MOUNT_FILE_BYTES) throw new Error(`Content mount file exceeds the browser profile: ${file.path}`);
    if (!/^[a-f0-9]{64}$/.test(file.sha256)) throw new Error(`Content mount checksum is invalid: ${file.path}`);
    totalBytes += file.size;
    if (!Number.isSafeInteger(totalBytes) || totalBytes > MAX_MOUNT_TOTAL_BYTES) throw new Error("Content mount exceeds the browser total-size profile");
  }
}

export async function prepareContentMount(request: ContentMountRequest, openFile: (logicalPath: string) => Promise<Blob>, onProgress?: (progress: ContentMountProgress) => void): Promise<PreparedContentMount> {
  validateContentMountRequest(request);
  const totalBytes = request.files.reduce((sum, file) => sum + file.size, 0);

  const blobs: Array<{ name: string; data: Blob }> = [];
  let completedBytes = 0;
  onProgress?.(progress(request, "opening", 0, 0));
  for (let index = 0; index < request.files.length; index += 1) {
    const descriptor = request.files[index];
    const file = await openFile(descriptor.path);
    if (file.size !== descriptor.size) throw new Error(`Mounted content size mismatch: ${descriptor.path}`);
    onProgress?.(progress(request, "verifying", index, completedBytes, descriptor.path));
    const digest = await sha256(new Uint8Array(await file.arrayBuffer()));
    if (digest !== descriptor.sha256) throw new Error(`Mounted content checksum mismatch: ${descriptor.path}`);
    blobs.push({ name: descriptor.path, data: file });
    completedBytes += descriptor.size;
    onProgress?.(progress(request, "verifying", index + 1, completedBytes, descriptor.path));
  }
  return { root: contentMountRoot(request.revision), blobs, totalBytes };
}

export function mountPreparedContent(module: EmscriptenModuleWithFs, prepared: PreparedContentMount): void {
  const fs = module.FS;
  const workerFs = fs?.filesystems?.WORKERFS;
  if (!fs || !workerFs) throw new Error("Emscripten engine does not expose the required WORKERFS filesystem");
  fs.mkdirTree(prepared.root);
  fs.mount(workerFs, { blobs: prepared.blobs }, prepared.root);
}

interface StoredIndexV1 { schemaVersion: 1; id: string; revision: string; installedAt: string }
interface StoredIndexV2 { schemaVersion: 2; id: string; revision: string; storageKey: string; installedAt: string }

async function fileAt(root: FileSystemDirectoryHandle, path: string): Promise<File> {
  const segments = path.split("/");
  const filename = segments.pop();
  if (!filename) throw new Error(`OPFS path has no filename: ${path}`);
  try {
    return await retryTransientOpfsNotFound(async () => {
      let directory = root;
      for (const segment of segments) directory = await directory.getDirectoryHandle(segment);
      return (await directory.getFileHandle(filename)).getFile();
    });
  } catch (error) {
    if (isOpfsNotFoundError(error)) throw new Error(`Installed content path remained unavailable after retry: ${path}`, { cause: error });
    throw error;
  }
}

async function assertSelectedOpfsRevision(root: FileSystemDirectoryHandle, request: ContentMountRequest, phase: "before" | "after"): Promise<void> {
  const indexFile = await fileAt(root, `content-index/${request.packageId}.json`);
  const stored = JSON.parse(await indexFile.text()) as StoredIndexV1 | StoredIndexV2;
  if (!stored || (stored.schemaVersion !== 1 && stored.schemaVersion !== 2) || stored.id !== request.packageId || stored.revision !== request.revision) {
    throw new Error(phase === "before" ? "Selected content index changed before mounting" : "Selected content index changed while mounting");
  }
  const storageKey = stored.schemaVersion === 1 ? stored.revision : stored.storageKey;
  if (storageKey !== request.storageKey) {
    throw new Error(phase === "before" ? "Selected physical content revision changed before mounting" : "Selected physical content revision changed while mounting");
  }
}

async function prepareFromOpfsUnlocked(request: ContentMountRequest, onProgress?: (progress: ContentMountProgress) => void): Promise<PreparedContentMount> {
  // Validate every path component before using request values to traverse
  // OPFS. prepareContentMount repeats this at the generic loader boundary.
  validateContentMountRequest(request);
  if (!navigator.storage?.getDirectory) throw new Error("Origin-private file storage is unavailable in this worker");
  const root = await navigator.storage.getDirectory();
  await assertSelectedOpfsRevision(root, request, "before");
  const prefix = `content/${request.packageId}/${request.storageKey}/files`;
  const prepared = await prepareContentMount(request, (logicalPath) => fileAt(root, `${prefix}/${logicalPath}`), onProgress);
  // Web Locks makes this invariant atomic on supporting browsers. Rechecking
  // also detects a cross-context replacement on implementations without the
  // Locks API before an inconsistent mount is exposed to Emscripten.
  await assertSelectedOpfsRevision(root, request, "after");
  return prepared;
}

export async function acquireContentMountLeaseFromOpfs(request: ContentMountRequest, onProgress?: (progress: ContentMountProgress) => void): Promise<ContentMountLease> {
  // Validate before interpolating the package ID into a lock name or using it
  // to traverse OPFS. prepareFromOpfsUnlocked repeats this at the file-loader
  // boundary so neither entry point relies on call ordering for safety.
  validateContentMountRequest(request);
  const operation = () => prepareFromOpfsUnlocked(request, onProgress);
  if (!navigator.locks) {
    return { prepared: await operation(), release: async () => undefined };
  }

  let resolvePrepared!: (prepared: PreparedContentMount) => void;
  let rejectPrepared!: (error: unknown) => void;
  const preparedPromise = new Promise<PreparedContentMount>((resolve, reject) => {
    resolvePrepared = resolve;
    rejectPrepared = reject;
  });
  let releaseLock!: () => void;
  const lockLifetime = new Promise<void>((resolve) => { releaseLock = resolve; });

  // A Web Lock request resolves only after its callback returns. Publish the
  // prepared Files through a separate promise, then keep the callback pending
  // until the runtime explicitly releases its lease. This prevents a package
  // replacement from deleting OPFS files still mounted by WORKERFS.
  const lockDone = navigator.locks.request(`theater-content:${request.packageId}`, { mode: "shared" }, async () => {
    try {
      resolvePrepared(await operation());
      await lockLifetime;
    } catch (error) {
      rejectPrepared(error);
    }
  }).catch((error: unknown) => { rejectPrepared(error); });

  try {
    const prepared = await preparedPromise;
    let released = false;
    return {
      prepared,
      async release(): Promise<void> {
        if (!released) {
          released = true;
          releaseLock();
        }
        await lockDone;
      },
    };
  } catch (error) {
    // Preparation failed while the callback owned the lock. Let it unwind and
    // wait for the lock request to settle before exposing the failure.
    releaseLock();
    await lockDone;
    throw error;
  }
}
