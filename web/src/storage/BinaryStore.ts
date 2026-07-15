export interface BinaryStore {
  write(path: string, data: Uint8Array): Promise<void>;
  read(path: string): Promise<Uint8Array>;
  remove(path: string): Promise<void>;
  removeTree(path: string): Promise<void>;
  list(prefix: string): Promise<string[]>;
}

export function validateStoragePath(path: string): string[] {
  if (!path || path.startsWith("/") || path.endsWith("/") || path.includes("\\") || path.includes("\0")) {
    throw new Error(`Invalid storage path: ${path}`);
  }
  const segments = path.split("/");
  if (segments.some((segment) => !segment || segment === "." || segment === "..")) throw new Error(`Invalid storage path: ${path}`);
  return segments;
}

export class MemoryBinaryStore implements BinaryStore {
  private readonly files = new Map<string, Uint8Array>();

  async write(path: string, data: Uint8Array): Promise<void> {
    validateStoragePath(path);
    this.files.set(path, data.slice());
  }

  async read(path: string): Promise<Uint8Array> {
    validateStoragePath(path);
    const data = this.files.get(path);
    if (!data) throw new DOMException(`File not found: ${path}`, "NotFoundError");
    return data.slice();
  }

  async remove(path: string): Promise<void> {
    validateStoragePath(path);
    this.files.delete(path);
  }

  async removeTree(path: string): Promise<void> {
    validateStoragePath(path);
    const prefix = `${path}/`;
    for (const key of this.files.keys()) if (key === path || key.startsWith(prefix)) this.files.delete(key);
  }

  async list(prefix: string): Promise<string[]> {
    if (prefix) validateStoragePath(prefix);
    const normalized = prefix ? `${prefix}/` : "";
    return [...this.files.keys()].filter((key) => key.startsWith(normalized)).sort();
  }
}
