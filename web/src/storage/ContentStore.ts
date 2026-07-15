import type { BinaryStore } from "./BinaryStore";
import { decodeJson, encodeJson, sha256, validateId } from "./helpers";

export interface ContentFileDescriptor {
  path: string;
  size: number;
  sha256: string;
  role: "engine-data" | "texture-atlas" | "audio" | "video" | "map" | "configuration" | "other";
}

export interface ContentManifest {
  format: "cncweb-content";
  version: 1;
  package_id: string;
  created_at_unix_ms: number;
  source: {
    product: "cnc-remastered-collection" | "tiberian-dawn-freeware";
    provider: "steam" | "ea-app" | "copied-installation" | "ea-freeware" | "unknown";
    install_fingerprint_sha256: string;
  };
  content: {
    games: ("tiberian-dawn" | "red-alert")[];
    locales: string[];
  };
  content_sha256: string;
  files: ContentFileDescriptor[];
}

export interface InstalledContent {
  id: string;
  revision: string;
  manifest: ContentManifest;
  installedAt: string;
}

export interface ContentRevisionDescriptor extends InstalledContent {
  storageKey: string;
}

export interface ContentIndexIssue {
  indexPath: string;
  id?: string;
  revision?: string;
  reason: string;
}

export interface ContentListResult {
  installed: InstalledContent[];
  issues: ContentIndexIssue[];
}

export type ContentFileSource = Blob | ArrayBuffer | Uint8Array;

interface ContentIndexV1 {
  schemaVersion: 1;
  id: string;
  revision: string;
  installedAt: string;
}

interface ContentIndexV2 {
  schemaVersion: 2;
  id: string;
  revision: string;
  storageKey: string;
  installedAt: string;
}

type StoredContentIndex = ContentIndexV1 | ContentIndexV2;

interface ContentIndex {
  schemaVersion: 1 | 2;
  id: string;
  revision: string;
  storageKey: string;
  installedAt: string;
}

const fallbackLocks = new Map<string, Promise<void>>();

async function withFallbackLock<T>(name: string, operation: () => Promise<T>): Promise<T> {
  const previous = fallbackLocks.get(name) ?? Promise.resolve();
  let release!: () => void;
  const held = new Promise<void>((resolve) => { release = resolve; });
  const tail = previous.catch(() => undefined).then(() => held);
  fallbackLocks.set(name, tail);
  await previous.catch(() => undefined);
  try {
    return await operation();
  } finally {
    release();
    if (fallbackLocks.get(name) === tail) fallbackLocks.delete(name);
  }
}

type ContentLockMode = "shared" | "exclusive";

async function withContentLock<T>(packageId: string, mode: ContentLockMode, operation: () => Promise<T>): Promise<T> {
  const name = `theater-content:${packageId}`;
  const locks = typeof navigator === "undefined" ? undefined : navigator.locks;
  if (locks) return locks.request(name, { mode }, operation);
  // The in-process fallback cannot coordinate shared ownership across browser
  // contexts, so conservatively serialize both read and write operations.
  return withFallbackLock(name, operation);
}

export function validateContentPath(path: string): void {
  const encoder = new TextEncoder();
  if (!path || encoder.encode(path).byteLength > 1024 || path.startsWith("/") || path.includes("\\") || path.includes("\0")) throw new Error(`Invalid content path: ${path}`);
  for (const segment of path.split("/")) {
    const stem = segment.split(".")[0].toUpperCase();
    const device = ["CON", "PRN", "AUX", "NUL"].includes(stem) || /^(?:COM|LPT)[1-9]$/.test(stem);
    if (!segment || segment === "." || segment === ".." || encoder.encode(segment).byteLength > 255 || /[ .]$/.test(segment) || /[<>:"|?*]/.test(segment) || [...segment].some((character) => /\p{Cc}/u.test(character)) || device) {
      throw new Error(`Invalid content path: ${path}`);
    }
  }
}

const CONTENT_ROLES = new Set<ContentFileDescriptor["role"]>(["engine-data", "texture-atlas", "audio", "video", "map", "configuration", "other"]);

async function calculateContentDigest(files: readonly ContentFileDescriptor[]): Promise<string> {
  const encoder = new TextEncoder();
  const chunks: Uint8Array[] = [encoder.encode("CNCWEB-CONTENT-MANIFEST-V1\0")];
  let total = chunks[0].byteLength;
  for (const file of files) {
    const path = encoder.encode(file.path);
    const hash = Uint8Array.from(file.sha256.match(/../g) ?? [], (value) => Number.parseInt(value, 16));
    const metadata = new Uint8Array(16);
    const view = new DataView(metadata.buffer);
    view.setBigUint64(0, BigInt(path.byteLength), true);
    view.setBigUint64(8, BigInt(file.size), true);
    const role = encoder.encode(file.role);
    const roleLength = new Uint8Array(8);
    new DataView(roleLength.buffer).setBigUint64(0, BigInt(role.byteLength), true);
    chunks.push(metadata.subarray(0, 8), path, metadata.subarray(8), hash, roleLength, role);
    total += 24 + path.byteLength + hash.byteLength + role.byteLength;
  }
  const payload = new Uint8Array(total);
  let offset = 0;
  for (const chunk of chunks) {
    payload.set(chunk, offset);
    offset += chunk.byteLength;
  }
  return sha256(payload);
}

async function bytes(source: ContentFileSource): Promise<Uint8Array> {
  if (source instanceof Blob) return new Uint8Array(await source.arrayBuffer());
  if (source instanceof ArrayBuffer) return new Uint8Array(source);
  return new Uint8Array(source);
}

function exactKeys(value: object, expected: readonly string[], label: string): void {
  const actual = Object.keys(value).sort();
  const wanted = [...expected].sort();
  if (actual.length !== wanted.length || actual.some((key, index) => key !== wanted[index])) throw new Error(`${label} contains missing or unknown fields`);
}

export async function validateContentManifest(manifest: ContentManifest): Promise<ContentManifest> {
  if (!manifest || typeof manifest !== "object") throw new Error("Content manifest is not an object");
  exactKeys(manifest, ["format", "version", "package_id", "created_at_unix_ms", "source", "content", "content_sha256", "files"], "Content manifest");
  validateId(manifest.package_id, "Package ID");
  if (manifest.format !== "cncweb-content" || manifest.version !== 1) throw new Error("Unsupported content manifest schema");
  if (!Number.isSafeInteger(manifest.created_at_unix_ms) || manifest.created_at_unix_ms < 0) throw new Error("Content manifest creation date is invalid");
  if (!manifest.source || typeof manifest.source !== "object") throw new Error("Content manifest source is invalid");
  exactKeys(manifest.source, ["product", "provider", "install_fingerprint_sha256"], "Content source");
  const remasteredSource = manifest.source.product === "cnc-remastered-collection"
    && ["steam", "ea-app", "copied-installation", "unknown"].includes(manifest.source.provider);
  const freewareSource = manifest.source.product === "tiberian-dawn-freeware"
    && manifest.source.provider === "ea-freeware";
  if (!remasteredSource && !freewareSource) throw new Error("Content source product and provider are unsupported");
  if (!/^[a-f0-9]{64}$/i.test(manifest.source?.install_fingerprint_sha256 ?? "")) throw new Error("Install fingerprint is invalid");
  if (!manifest.content?.games.length || manifest.content.games.some((game) => game !== "tiberian-dawn" && game !== "red-alert") || new Set(manifest.content.games).size !== manifest.content.games.length) throw new Error("Content manifest game list is invalid");
  exactKeys(manifest.content, ["games", "locales"], "Content descriptor");
  if (!manifest.content.locales.length || new Set(manifest.content.locales.map((value) => value.toLowerCase())).size !== manifest.content.locales.length) throw new Error("Content manifest locale list is invalid");
  if (manifest.content.locales.some((value) => value.length < 2 || value.length > 35 || !/^[a-z0-9]+(?:-[a-z0-9]+)*$/i.test(value))) throw new Error("Content manifest contains an invalid locale");
  const seen = new Set<string>();
  const collisions = new Set<string>();
  let previous = "";
  const files = manifest.files.map((file) => {
    if (!file || typeof file !== "object") throw new Error("Content manifest file record is invalid");
    exactKeys(file, ["path", "size", "sha256", "role"], "Content file record");
    validateContentPath(file.path);
    if (file.path.toLowerCase() === "manifest.json") throw new Error("manifest.json is reserved and cannot be content");
    if (previous && previous >= file.path) throw new Error("Content files must be strictly sorted by path");
    previous = file.path;
    if (seen.has(file.path)) throw new Error(`Duplicate content file: ${file.path}`);
    seen.add(file.path);
    const collision = file.path.toLowerCase();
    if (collisions.has(collision)) throw new Error(`Case-colliding content file: ${file.path}`);
    collisions.add(collision);
    if (!Number.isSafeInteger(file.size) || file.size < 0) throw new Error(`Invalid file size for ${file.path}`);
    if (!/^[a-f0-9]{64}$/i.test(file.sha256)) throw new Error(`Invalid SHA-256 for ${file.path}`);
    if (!CONTENT_ROLES.has(file.role)) throw new Error(`Invalid content role for ${file.path}`);
    return { ...file, sha256: file.sha256.toLowerCase() };
  });
  const normalized: ContentManifest = {
    ...manifest,
    source: { ...manifest.source, install_fingerprint_sha256: manifest.source.install_fingerprint_sha256.toLowerCase() },
    content_sha256: manifest.content_sha256.toLowerCase(),
    files,
  };
  if (!/^[a-f0-9]{64}$/.test(normalized.content_sha256) || (await calculateContentDigest(files)) !== normalized.content_sha256) {
    throw new Error("Content manifest aggregate checksum is invalid");
  }
  return normalized;
}

export class ContentStore {
  constructor(private readonly store: BinaryStore) {}

  async install(manifestInput: ContentManifest, sources: ReadonlyMap<string, ContentFileSource>, onProgress?: (completed: number, total: number) => void): Promise<InstalledContent> {
    return this.installFromLoader(manifestInput, async (path) => {
      const source = sources.get(path);
      if (!source) throw new Error(`Content package is missing ${path}`);
      return source;
    }, onProgress);
  }

  async installFromLoader(manifestInput: ContentManifest, load: (path: string) => Promise<ContentFileSource>, onProgress?: (completed: number, total: number) => void): Promise<InstalledContent> {
    const manifest = await validateContentManifest(manifestInput);
    const manifestBytes = encodeJson(manifest);
    const revision = await sha256(manifestBytes);
    return withContentLock(manifest.package_id, "exclusive", () => this.installLocked(manifest, manifestBytes, revision, load, onProgress));
  }

  private async installLocked(manifest: ContentManifest, manifestBytes: Uint8Array, revision: string, load: (path: string) => Promise<ContentFileSource>, onProgress?: (completed: number, total: number) => void): Promise<InstalledContent> {
    const previous = await this.tryReadIndex(manifest.package_id);
    if (previous?.revision === revision) {
      try {
        const installed = await this.readByIndex(previous, true);
        if (previous.schemaVersion === 1) await this.writeIndex({ schemaVersion: 2, id: previous.id, revision, storageKey: previous.storageKey, installedAt: previous.installedAt });
        return installed;
      } catch {
        // A same-revision index is only a fast path after every stored file
        // verifies. Corruption is repaired into a new, unreferenced root.
      }
    }

    const transactionId = crypto.randomUUID().toLowerCase();
    const storageKey = `${revision}-${transactionId}`;
    const candidate = `content/${manifest.package_id}/${storageKey}`;
    let committed = false;
    try {
      let completed = 0;
      for (const descriptor of manifest.files) {
        const source = await load(descriptor.path);
        const data = await bytes(source);
        if (data.byteLength !== descriptor.size) throw new Error(`Content size mismatch for ${descriptor.path}`);
        if ((await sha256(data)) !== descriptor.sha256) throw new Error(`Content checksum mismatch for ${descriptor.path}`);
        // The candidate root is unique and remains unreachable until the
        // index-last commit below. Writing it directly preserves the same
        // transactional boundary without duplicating the entire package in
        // OPFS during import.
        await this.store.write(`${candidate}/files/${descriptor.path}`, data);
        completed += 1;
        onProgress?.(completed, manifest.files.length);
      }
      await this.store.write(`${candidate}/manifest.json`, manifestBytes);
      await this.verifyRoot(manifest, revision, candidate);
      const installedAt = new Date().toISOString();
      const index: ContentIndexV2 = { schemaVersion: 2, id: manifest.package_id, revision, storageKey, installedAt };
      await this.writeIndex(index);
      committed = true;
      if (previous && previous.storageKey !== storageKey) await this.store.removeTree(this.rootFor(previous)).catch(() => undefined);
      return { id: manifest.package_id, revision, manifest, installedAt };
    } catch (error) {
      if (!committed) await this.store.removeTree(candidate).catch(() => undefined);
      throw error;
    }
  }

  async list(): Promise<InstalledContent[]> {
    return (await this.listWithIssues()).installed;
  }

  async listWithIssues(): Promise<ContentListResult> {
    const paths = await this.store.list("content-index");
    type Entry = { installed: InstalledContent } | { issue: ContentIndexIssue };
    const entries: Entry[] = await Promise.all(
      paths.filter((path) => path.endsWith(".json")).map(async (path): Promise<Entry> => {
        const id = path.slice("content-index/".length, -".json".length);
        try {
          validateId(id, "Content ID");
        } catch (error) {
          return { issue: { indexPath: path, reason: error instanceof Error ? error.message : String(error) } };
        }
        let revision: string | undefined;
        try {
          const installed = await withContentLock(id, "shared", async () => {
            const index = await this.readIndex(id);
            revision = index.revision;
            return this.readByIndex(index);
          });
          return { installed };
        } catch (error) {
          return { issue: { indexPath: path, id, revision, reason: error instanceof Error ? error.message : String(error) } };
        }
      }),
    );
    const installed = entries.flatMap((entry) => "installed" in entry ? [entry.installed] : []);
    const issues = entries.flatMap((entry) => "issue" in entry ? [entry.issue] : []);
    issues.sort((left, right) => left.indexPath.localeCompare(right.indexPath));
    installed.sort((left, right) => left.id.localeCompare(right.id));
    return { installed, issues };
  }

  async get(id: string): Promise<InstalledContent> {
    validateId(id, "Content ID");
    return withContentLock(id, "shared", async () => this.readByIndex(await this.readIndex(id)));
  }

  async getRevisionDescriptor(id: string): Promise<ContentRevisionDescriptor> {
    validateId(id, "Content ID");
    return withContentLock(id, "shared", async () => {
      const index = await this.readIndex(id);
      const installed = await this.readByIndex(index);
      return { ...installed, storageKey: index.storageKey };
    });
  }

  async readRevisionFile(revision: Pick<ContentRevisionDescriptor, "id" | "revision" | "storageKey">, path: string): Promise<Uint8Array> {
    validateId(revision.id, "Content ID");
    validateContentPath(path);
    return withContentLock(revision.id, "shared", async () => {
      const current = await this.readIndex(revision.id);
      if (current.revision !== revision.revision || current.storageKey !== revision.storageKey) throw new Error("Selected content revision changed; select the package again");
      const installed = await this.readByIndex(current);
      const descriptor = installed.manifest.files.find((file) => file.path === path);
      if (!descriptor) throw new Error(`Content manifest does not contain ${path}`);
      const data = await this.store.read(`${this.rootFor(current)}/files/${path}`);
      if (data.byteLength !== descriptor.size || (await sha256(data)) !== descriptor.sha256) throw new Error(`Stored content failed validation: ${path}`);
      return data;
    });
  }

  async readFile(id: string, path: string): Promise<Uint8Array> {
    validateId(id, "Content ID");
    validateContentPath(path);
    return withContentLock(id, "shared", async () => {
      const index = await this.readIndex(id);
      const installed = await this.readByIndex(index);
      const descriptor = installed.manifest.files.find((file) => file.path === path);
      if (!descriptor) throw new Error(`Content manifest does not contain ${path}`);
      const data = await this.store.read(`${this.rootFor(index)}/files/${path}`);
      if (data.byteLength !== descriptor.size || (await sha256(data)) !== descriptor.sha256) throw new Error(`Stored content failed validation: ${path}`);
      return data;
    });
  }

  async uninstall(id: string): Promise<void> {
    validateId(id, "Content ID");
    await withContentLock(id, "exclusive", async () => {
      await this.store.remove(`content-index/${id}.json`);
      await this.store.removeTree(`content/${id}`).catch(() => undefined);
      await this.store.removeTree(`content-staging/${id}`).catch(() => undefined);
    });
  }

  private normalizeIndex(stored: StoredContentIndex): ContentIndex {
    if (stored.schemaVersion !== 1 && stored.schemaVersion !== 2) throw new Error("Unsupported installed-content index");
    validateId(stored.id, "Content ID");
    if (!/^[a-f0-9]{64}$/.test(stored.revision)) throw new Error("Installed-content revision is invalid");
    const storageKey = stored.schemaVersion === 1 ? stored.revision : stored.storageKey;
    if (storageKey !== stored.revision && (!storageKey.startsWith(`${stored.revision}-`) || !/^[a-f0-9-]{65,128}$/.test(storageKey))) throw new Error("Installed-content storage key is invalid");
    if (!Number.isFinite(Date.parse(stored.installedAt))) throw new Error("Installed-content timestamp is invalid");
    return { schemaVersion: stored.schemaVersion, id: stored.id, revision: stored.revision, storageKey, installedAt: stored.installedAt };
  }

  private rootFor(index: ContentIndex): string {
    return `content/${index.id}/${index.storageKey}`;
  }

  private async tryReadIndex(id: string): Promise<ContentIndex | undefined> {
    let data: Uint8Array;
    try {
      data = await this.store.read(`content-index/${id}.json`);
    } catch (error) {
      if (error instanceof DOMException && error.name === "NotFoundError") return undefined;
      throw error;
    }
    try {
      return this.decodeIndex(id, data);
    } catch {
      // A torn or obsolete index must not permanently prevent a fresh,
      // independently verified import of the same package ID. The unique
      // candidate remains unreferenced until writeIndex succeeds.
      return undefined;
    }
  }

  private async readIndex(id: string): Promise<ContentIndex> {
    return this.decodeIndex(id, await this.store.read(`content-index/${id}.json`));
  }

  private decodeIndex(id: string, data: Uint8Array): ContentIndex {
    const index = this.normalizeIndex(decodeJson<StoredContentIndex>(data));
    if (index.id !== id) throw new Error("Installed-content index ID does not match its path");
    return index;
  }

  private async writeIndex(index: ContentIndexV2): Promise<void> {
    await this.store.write(`content-index/${index.id}.json`, encodeJson(index));
  }

  private async readByIndex(index: ContentIndex, verifyFiles = false): Promise<InstalledContent> {
    const manifestBytes = await this.store.read(`${this.rootFor(index)}/manifest.json`);
    if ((await sha256(manifestBytes)) !== index.revision) throw new Error("Installed-content manifest revision is invalid");
    const manifest = await validateContentManifest(decodeJson<ContentManifest>(manifestBytes));
    if (manifest.package_id !== index.id) throw new Error("Installed-content ID does not match its manifest");
    if (verifyFiles) await this.verifyFiles(manifest, this.rootFor(index));
    return { id: index.id, revision: index.revision, manifest, installedAt: index.installedAt };
  }

  private async verifyRoot(manifest: ContentManifest, revision: string, root: string): Promise<void> {
    const storedManifest = await this.store.read(`${root}/manifest.json`);
    if ((await sha256(storedManifest)) !== revision) throw new Error("Candidate content manifest revision is invalid");
    const parsed = await validateContentManifest(decodeJson<ContentManifest>(storedManifest));
    if (parsed.package_id !== manifest.package_id || parsed.content_sha256 !== manifest.content_sha256) throw new Error("Candidate content manifest does not match the requested package");
    await this.verifyFiles(parsed, root);
  }

  private async verifyFiles(manifest: ContentManifest, root: string): Promise<void> {
    for (const descriptor of manifest.files) {
      const data = await this.store.read(`${root}/files/${descriptor.path}`);
      if (data.byteLength !== descriptor.size || (await sha256(data)) !== descriptor.sha256) throw new Error(`Stored content failed validation: ${descriptor.path}`);
    }
  }
}
