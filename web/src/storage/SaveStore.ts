import type { BinaryStore } from "./BinaryStore";
import { decodeJson, encodeJson, sha256, validateId } from "./helpers";

const fallbackSaveLocks = new Map<string, Promise<void>>();

async function withFallbackSaveLock<T>(name: string, operation: () => Promise<T>): Promise<T> {
  const previous = fallbackSaveLocks.get(name) ?? Promise.resolve();
  let release!: () => void;
  const held = new Promise<void>((resolve) => { release = resolve; });
  const tail = previous.catch(() => undefined).then(() => held);
  fallbackSaveLocks.set(name, tail);
  await previous.catch(() => undefined);
  try { return await operation(); }
  finally {
    release();
    if (fallbackSaveLocks.get(name) === tail) fallbackSaveLocks.delete(name);
  }
}

async function withSaveLock<T>(id: string, operation: () => Promise<T>): Promise<T> {
  const name = `theater-save:${id}`;
  const locks = typeof navigator === "undefined" ? undefined : navigator.locks;
  if (locks) return locks.request(name, { mode: "exclusive" }, operation);
  return withFallbackSaveLock(name, operation);
}

export type SaveKind = "manual" | "autosave" | "checkpoint";

export interface SavePresentation {
  cameraX: number;
  cameraY: number;
  zoom: number;
  graphicsMode: "classic" | "remastered";
}

export interface SaveMetadata {
  id: string;
  name: string;
  game: "tiberian-dawn" | "red-alert" | "demo";
  scenario: string;
  kind: SaveKind;
  tick: number;
  createdAt: string;
  updatedAt: string;
  contentPackageId?: string;
  contentRevision?: string;
  missionId?: string;
  /** Isolates separate campaign attempts that revisit the same mission. */
  runId?: string;
  presentation?: SavePresentation;
}

export interface StoredSave extends SaveMetadata {
  schemaVersion: 1;
  checksum: string;
  byteLength: number;
  revision: string;
}

export interface SaveIndexIssue {
  indexPath: string;
  id?: string;
  revision?: string;
  reason: string;
  quarantined: boolean;
}

export interface SaveListResult {
  saves: StoredSave[];
  issues: SaveIndexIssue[];
}

export interface ReadableSaveSelection {
  save?: { metadata: StoredSave; data: Uint8Array };
  issues: SaveIndexIssue[];
}

class CorruptSaveError extends Error {}

function isNotFound(error: unknown): boolean {
  return error instanceof DOMException && error.name === "NotFoundError";
}

function message(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

export class SaveStore {
  constructor(private readonly store: BinaryStore) {}

  async write(metadata: SaveMetadata, dataInput: ArrayBuffer | Uint8Array): Promise<StoredSave> {
    const data = dataInput instanceof Uint8Array ? new Uint8Array(dataInput) : new Uint8Array(dataInput);
    const checksum = await sha256(data);
    const revision = `${Date.now().toString(36)}-${checksum.slice(0, 16)}`;
    const stored = this.validate({ ...metadata, schemaVersion: 1, checksum, byteLength: data.byteLength, revision } as StoredSave);
    return withSaveLock(metadata.id, async () => {
      const dataPath = `save-data/${metadata.id}/${revision}.bin`;
      let previous: StoredSave | undefined;
      let previousData: Uint8Array | undefined;
      try {
        previousData = await this.store.read(`save-index/${metadata.id}.json`);
        previous = this.validate(decodeJson<StoredSave>(previousData));
        if (previous.id !== metadata.id) throw new Error("Save index ID does not match its path");
      } catch (error) {
        if (!isNotFound(error)) {
          if (!previousData) throw error;
          await this.quarantineIndexDataLocked(metadata.id, previousData);
        }
      }
      await this.store.write(dataPath, data);
      try {
        await this.store.write(`save-index/${metadata.id}.json`, encodeJson(stored));
      } catch (error) {
        await this.store.remove(dataPath).catch(() => undefined);
        throw error;
      }
      if (previous && previous.revision !== revision) await this.store.remove(`save-data/${metadata.id}/${previous.revision}.bin`).catch(() => undefined);
      return stored;
    });
  }

  async list(): Promise<StoredSave[]> {
    return (await this.listWithIssues()).saves;
  }

  async listWithIssues(): Promise<SaveListResult> {
    const paths = await this.store.list("save-index");
    type Entry = { save: StoredSave } | { issue: SaveIndexIssue } | undefined;
    const entries: Entry[] = await Promise.all(paths.filter((path) => path.endsWith(".json")).map(async (path): Promise<Entry> => {
      const id = path.slice("save-index/".length, -".json".length);
      try {
        validateId(id, "Save ID");
      } catch (error) {
        return { issue: { indexPath: path, reason: message(error), quarantined: false } };
      }
      return withSaveLock(id, async () => {
        let data: Uint8Array;
        try {
          data = await this.store.read(path);
        } catch (error) {
          if (isNotFound(error)) return undefined;
          return { issue: { indexPath: path, id, reason: message(error), quarantined: false } };
        }
        let decoded: unknown;
        try {
          decoded = decodeJson<unknown>(data);
          const save = this.validate(decoded as StoredSave);
          if (save.id !== id) throw new Error("Save index ID does not match its path");
          return { save };
        } catch (error) {
          const revision = decoded && typeof decoded === "object" && "revision" in decoded && typeof decoded.revision === "string" && /^[a-z0-9]+-[a-f0-9]{16}$/.test(decoded.revision)
            ? decoded.revision
            : undefined;
          try {
            await this.quarantineIndexDataLocked(id, data);
            return { issue: { indexPath: path, id, revision, reason: message(error), quarantined: true } };
          } catch (quarantineError) {
            return { issue: { indexPath: path, id, revision, reason: `${message(error)}; quarantine failed: ${message(quarantineError)}`, quarantined: false } };
          }
        }
      });
    }));
    const saves = entries.flatMap((entry) => entry && "save" in entry ? [entry.save] : []);
    const issues = entries.flatMap((entry) => entry && "issue" in entry ? [entry.issue] : []);
    saves.sort((left, right) => Date.parse(right.updatedAt) - Date.parse(left.updatedAt) || left.id.localeCompare(right.id));
    issues.sort((left, right) => left.indexPath.localeCompare(right.indexPath));
    return { saves, issues };
  }

  async read(id: string): Promise<{ metadata: StoredSave; data: Uint8Array }> {
    validateId(id, "Save ID");
    return withSaveLock(id, async () => {
      let metadata: StoredSave;
      try {
        metadata = this.validate(decodeJson<StoredSave>(await this.store.read(`save-index/${id}.json`)));
        if (metadata.id !== id) throw new Error("Save index ID does not match its path");
      } catch (error) {
        if (isNotFound(error)) throw error;
        throw new CorruptSaveError(`Save index failed validation: ${message(error)}`);
      }
      let data: Uint8Array;
      try {
        data = await this.store.read(`save-data/${id}/${metadata.revision}.bin`);
      } catch (error) {
        if (isNotFound(error)) throw new CorruptSaveError("Save data failed validation: payload is missing");
        throw error;
      }
      if (data.byteLength !== metadata.byteLength || (await sha256(data)) !== metadata.checksum) throw new CorruptSaveError("Save data failed validation");
      return { metadata, data };
    });
  }

  async readNewestValid(candidates: readonly StoredSave[]): Promise<ReadableSaveSelection> {
    const sorted = [...candidates].sort((left, right) => Date.parse(right.updatedAt) - Date.parse(left.updatedAt) || left.id.localeCompare(right.id));
    const issues: SaveIndexIssue[] = [];
    for (const candidate of sorted) {
      try {
        this.validate(candidate);
        const save = await this.read(candidate.id);
        if (save.metadata.revision !== candidate.revision) {
          issues.push({ indexPath: `save-index/${candidate.id}.json`, id: candidate.id, revision: candidate.revision, reason: "Save changed while selecting a resume point", quarantined: false });
          continue;
        }
        return { save, issues };
      } catch (error) {
        let quarantined = false;
        if (error instanceof CorruptSaveError) {
          try { quarantined = await this.quarantine(candidate.id, candidate.revision); }
          catch (quarantineError) {
            issues.push({ indexPath: `save-index/${candidate.id}.json`, id: candidate.id, revision: candidate.revision, reason: `${message(error)}; quarantine failed: ${message(quarantineError)}`, quarantined: false });
            continue;
          }
        }
        issues.push({ indexPath: `save-index/${candidate.id}.json`, id: candidate.id, revision: candidate.revision, reason: message(error), quarantined });
      }
    }
    return { issues };
  }

  async quarantine(id: string, expectedRevision?: string): Promise<boolean> {
    validateId(id, "Save ID");
    return withSaveLock(id, async () => {
      let data: Uint8Array;
      try { data = await this.store.read(`save-index/${id}.json`); }
      catch (error) {
        if (isNotFound(error)) return false;
        throw error;
      }
      if (expectedRevision) {
        try {
          const current = this.validate(decodeJson<StoredSave>(data));
          if (current.id === id && current.revision !== expectedRevision) return false;
        } catch {
          // A malformed current index has no trustworthy revision to compare.
        }
      }
      await this.quarantineIndexDataLocked(id, data);
      return true;
    });
  }

  async remove(id: string): Promise<void> {
    validateId(id, "Save ID");
    await withSaveLock(id, async () => {
      await this.store.remove(`save-index/${id}.json`);
      await this.store.removeTree(`save-data/${id}`);
      await this.store.removeTree(`save-quarantine/${id}`).catch(() => undefined);
    });
  }

  private validate(value: StoredSave): StoredSave {
    if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error("Save index is not an object");
    const required = ["schemaVersion", "id", "name", "game", "scenario", "kind", "tick", "createdAt", "updatedAt", "checksum", "byteLength", "revision"];
    const optional = new Set(["contentPackageId", "contentRevision", "missionId", "runId", "presentation"]);
    const keys = Object.keys(value);
    if (required.some((key) => !Object.hasOwn(value, key)) || keys.some((key) => !required.includes(key) && !optional.has(key))) throw new Error("Save index contains missing or unknown fields");
    if (value.schemaVersion !== 1) throw new Error("Unsupported save index schema");
    validateId(value.id, "Save ID");
    if (!/^[a-f0-9]{64}$/.test(value.checksum) || !/^[a-z0-9]+-[a-f0-9]{16}$/.test(value.revision)) throw new Error("Save index is invalid");
    if (typeof value.name !== "string" || !value.name.trim() || new TextEncoder().encode(value.name).byteLength > 256) throw new Error("Save name is invalid");
    if (value.game !== "tiberian-dawn" && value.game !== "red-alert" && value.game !== "demo") throw new Error("Save game is invalid");
    if (typeof value.scenario !== "string") throw new Error("Save scenario is invalid");
    validateId(value.scenario, "Save scenario");
    if (value.kind !== "manual" && value.kind !== "autosave" && value.kind !== "checkpoint") throw new Error("Save kind is invalid");
    if (!Number.isSafeInteger(value.tick) || value.tick < 0) throw new Error("Save tick is invalid");
    if (typeof value.createdAt !== "string" || typeof value.updatedAt !== "string" || !Number.isFinite(Date.parse(value.createdAt)) || !Number.isFinite(Date.parse(value.updatedAt)) || new Date(value.createdAt).toISOString() !== value.createdAt || new Date(value.updatedAt).toISOString() !== value.updatedAt) throw new Error("Save timestamp is invalid");
    if (!Number.isSafeInteger(value.byteLength) || value.byteLength < 0) throw new Error("Save byte length is invalid");
    this.validateContentIdentity(value);
    if (value.runId !== undefined) {
      if (typeof value.runId !== "string") throw new Error("Save run ID is invalid");
      validateId(value.runId, "Save run ID");
    }
    this.validatePresentation(value.presentation);
    return value;
  }

  private validatePresentation(value: SavePresentation | undefined): void {
    if (value === undefined) return;
    if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error("Save presentation state is invalid");
    const keys = Object.keys(value).sort();
    const expected = ["cameraX", "cameraY", "graphicsMode", "zoom"];
    if (keys.length !== expected.length || keys.some((key, index) => key !== expected[index])) throw new Error("Save presentation state contains missing or unknown fields");
    if (!Number.isFinite(value.cameraX) || !Number.isFinite(value.cameraY) || Math.abs(value.cameraX) > 1_000_000 || Math.abs(value.cameraY) > 1_000_000) {
      throw new Error("Save camera position is invalid");
    }
    if (!Number.isFinite(value.zoom) || value.zoom < 0.6 || value.zoom > 2.5) throw new Error("Save camera zoom is invalid");
    if (value.graphicsMode !== "classic" && value.graphicsMode !== "remastered") throw new Error("Save graphics mode is invalid");
  }

  private validateContentIdentity(value: Pick<SaveMetadata, "game" | "scenario" | "contentPackageId" | "contentRevision" | "missionId">): void {
    const fields = [value.contentPackageId, value.contentRevision, value.missionId];
    if (value.game === "demo") {
      if (fields.some((field) => field !== undefined)) throw new Error("Demo saves cannot reference retail content");
      return;
    }
    if (fields.some((field) => field === undefined) || fields.some((field) => typeof field !== "string")) throw new Error("Save content identity must be complete for a retail mission");
    validateId(value.contentPackageId!, "Save content package ID");
    if (!/^[a-f0-9]{64}$/.test(value.contentRevision!)) throw new Error("Save content revision is invalid");
    validateId(value.missionId!, "Save mission ID");
    if (value.missionId !== value.scenario) throw new Error("Save mission ID does not match its scenario");
  }

  private async quarantineIndexDataLocked(id: string, data: Uint8Array): Promise<void> {
    const token = `${Date.now().toString(36)}-${crypto.randomUUID().toLowerCase()}`;
    await this.store.write(`save-quarantine/${id}/${token}.json`, data);
    await this.store.remove(`save-index/${id}.json`);
  }
}
