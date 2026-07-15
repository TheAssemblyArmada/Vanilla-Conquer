import { BlobReader, BlobWriter, TextWriter, ZipReader, type Entry, type FileEntry } from "@zip.js/zip.js";
import { ContentStore, type ContentManifest, validateContentManifest, validateContentPath } from "./ContentStore";
import { checkStorageReadiness } from "./helpers";

export interface PackageImportLimits {
  maxEntries: number;
  maxManifestBytes: number;
  maxFileBytes: number;
  maxTotalBytes: number;
  maxCompressionRatio: number;
}

export interface PackageImportExpectation {
  packageId: string;
  contentSha256: string;
  sourceProduct: ContentManifest["source"]["product"];
  sourceProvider: ContentManifest["source"]["provider"];
}

export const DEFAULT_PACKAGE_LIMITS: PackageImportLimits = {
  maxEntries: 100_000,
  maxManifestBytes: 4 * 1024 * 1024,
  // Import is deliberately bounded until streaming SHA-256 lands. This avoids
  // materializing multi-gigabyte archive entries on memory-constrained phones.
  maxFileBytes: 64 * 1024 * 1024,
  maxTotalBytes: 2 * 1024 * 1024 * 1024,
  maxCompressionRatio: 1_000,
};

function validateArchivePath(path: string): void {
  validateContentPath(path);
}

function entrySize(entry: Entry): number {
  const size = Number(entry.uncompressedSize);
  if (!Number.isSafeInteger(size) || size < 0) throw new Error(`Package entry has an unsupported size: ${entry.filename}`);
  return size;
}

export function packageInstallQuotaBytes(contentBytes: number, manifestBytes: number): number {
  if (!Number.isSafeInteger(contentBytes) || contentBytes < 0 || !Number.isSafeInteger(manifestBytes) || manifestBytes < 0) {
    throw new Error("Package install size is invalid");
  }
  const required = contentBytes + manifestBytes;
  if (!Number.isSafeInteger(required)) throw new Error("Package install size is unsupported");
  return required;
}

export async function importCncwebPackage(
  file: Blob,
  store: ContentStore,
  onProgress?: (completed: number, total: number) => void,
  limits = DEFAULT_PACKAGE_LIMITS,
  expectation?: PackageImportExpectation,
) {
  const reader = new ZipReader(new BlobReader(file));
  try {
    const entries = await reader.getEntries();
    if (entries.length > limits.maxEntries + 1) throw new Error("Content package contains too many entries");
    const files = new Map<string, FileEntry>();
    let totalBytes = 0;
    for (const entry of entries) {
      if (entry.directory) throw new Error(`Directory records are not allowed in content packages: ${entry.filename}`);
      validateArchivePath(entry.filename);
      let decodedName: string;
      try { decodedName = new TextDecoder("utf-8", { fatal: true }).decode(entry.rawFilename); } catch { throw new Error("Package entry name is not valid UTF-8"); }
      if (decodedName !== entry.filename || entry.encrypted) throw new Error(`Package entry must use unencrypted UTF-8 metadata: ${entry.filename}`);
      if (entry.filename.toLowerCase() === "manifest.json" && entry.filename !== "manifest.json") throw new Error("manifest.json must use its canonical lowercase name");
      if (entry.compressionMethod !== 0 && entry.compressionMethod !== 8) throw new Error(`Package entry uses unsupported compression: ${entry.filename}`);
      if (entry.unixMode !== undefined && ![0, 0o100000].includes(entry.unixMode & 0o170000)) throw new Error(`Package entry is not a regular file: ${entry.filename}`);
      if (files.has(entry.filename.toLowerCase())) throw new Error(`Duplicate or case-colliding package path: ${entry.filename}`);
      files.set(entry.filename.toLowerCase(), entry as FileEntry);
      const size = entrySize(entry);
      const compressedSize = Number(entry.compressedSize);
      if (size > limits.maxFileBytes) throw new Error(`Package entry exceeds the browser limit: ${entry.filename}`);
      if (size > 0 && (!Number.isFinite(compressedSize) || compressedSize <= 0 || size / compressedSize > limits.maxCompressionRatio)) throw new Error(`Package entry has an unsafe compression ratio: ${entry.filename}`);
      if (entry.filename !== "manifest.json") totalBytes += size;
      if (!Number.isSafeInteger(totalBytes) || totalBytes > limits.maxTotalBytes) throw new Error("Content package expands beyond the browser limit");
    }
    const manifestEntry = files.get("manifest.json");
    if (!manifestEntry || !manifestEntry.getData) throw new Error("Content package does not contain manifest.json");
    if (entrySize(manifestEntry) > limits.maxManifestBytes) throw new Error("Content manifest is too large");
    const manifestText = await manifestEntry.getData(new TextWriter(), {
      onprogress: (loaded) => { if (loaded > limits.maxManifestBytes) throw new Error("Content manifest expanded beyond its declared limit"); },
    });
    const manifest = await validateContentManifest(JSON.parse(manifestText) as ContentManifest);
    if (expectation) {
      if (manifest.package_id !== expectation.packageId) throw new Error("Content package ID does not match the bootstrap descriptor");
      if (manifest.content_sha256 !== expectation.contentSha256) throw new Error("Content package digest does not match the bootstrap descriptor");
      if (manifest.source.product !== expectation.sourceProduct || manifest.source.provider !== expectation.sourceProvider) {
        throw new Error("Content package provenance does not match the bootstrap descriptor");
      }
    }
    // The selected archive remains outside origin storage. ContentStore writes
    // one unique candidate root and commits its index last, so only the
    // expanded content plus its stored manifest consumes new OPFS quota.
    const readiness = await checkStorageReadiness(packageInstallQuotaBytes(totalBytes, entrySize(manifestEntry)));
    if (!readiness.supported || !readiness.enoughSpace) throw new Error("Not enough private browser storage to stage and install this content package");
    if (manifest.files.length !== entries.length - 1) throw new Error("Package entries do not match the content manifest");
    for (const descriptor of manifest.files) {
      const entry = files.get(descriptor.path.toLowerCase());
      if (!entry || entry.filename !== descriptor.path || entrySize(entry) !== descriptor.size) throw new Error(`Package entry does not match manifest: ${descriptor.path}`);
    }
    return await store.installFromLoader(
      manifest,
      async (path) => {
        const entry = files.get(path.toLowerCase());
        if (!entry?.getData) throw new Error(`Package entry is missing: ${path}`);
        return entry.getData(new BlobWriter(), {
          checkSignature: true,
          onprogress: (loaded) => { if (loaded > limits.maxFileBytes || loaded > entrySize(entry)) throw new Error(`Package entry expanded beyond its declared size: ${path}`); },
        });
      },
      onProgress,
    );
  } finally {
    await reader.close();
  }
}
