import { validateStoragePath, type BinaryStore } from "./BinaryStore";
import { retryTransientOpfsNotFound } from "./opfsRetry";

interface DirectoryEntryIterable extends FileSystemDirectoryHandle {
  entries(): AsyncIterableIterator<[string, FileSystemHandle]>;
}

export class OpfsBinaryStore implements BinaryStore {
  private readonly root: Promise<FileSystemDirectoryHandle>;

  constructor(root?: FileSystemDirectoryHandle | Promise<FileSystemDirectoryHandle>) {
    if (root) this.root = Promise.resolve(root);
    else {
      if (!navigator.storage?.getDirectory) throw new Error("Origin-private file storage is unavailable in this browser");
      this.root = navigator.storage.getDirectory();
    }
  }

  async write(path: string, data: Uint8Array): Promise<void> {
    const segments = validateStoragePath(path);
    const filename = segments.pop()!;
    const directory = await this.directory(segments, true);
    const handle = await directory.getFileHandle(filename, { create: true });
    const writable = await handle.createWritable({ keepExistingData: false });
    try {
      await writable.write(new Uint8Array(data));
      await writable.close();
    } catch (error) {
      await writable.abort().catch(() => undefined);
      throw error;
    }
  }

  async read(path: string): Promise<Uint8Array> {
    const segments = validateStoragePath(path);
    const filename = segments.pop()!;
    return retryTransientOpfsNotFound(async () => {
      const directory = await this.directory(segments, false);
      const handle = await directory.getFileHandle(filename);
      return new Uint8Array(await (await handle.getFile()).arrayBuffer());
    });
  }

  async remove(path: string): Promise<void> {
    const segments = validateStoragePath(path);
    const filename = segments.pop()!;
    const directory = await this.directory(segments, false);
    await directory.removeEntry(filename).catch((error: unknown) => {
      if (!(error instanceof DOMException && error.name === "NotFoundError")) throw error;
    });
  }

  async removeTree(path: string): Promise<void> {
    const segments = validateStoragePath(path);
    const name = segments.pop()!;
    const directory = await this.directory(segments, false);
    await directory.removeEntry(name, { recursive: true }).catch((error: unknown) => {
      if (!(error instanceof DOMException && error.name === "NotFoundError")) throw error;
    });
  }

  async list(prefix: string): Promise<string[]> {
    const segments = prefix ? validateStoragePath(prefix) : [];
    let start: FileSystemDirectoryHandle;
    try {
      start = await this.directory(segments, false);
    } catch (error) {
      if (error instanceof DOMException && error.name === "NotFoundError") return [];
      throw error;
    }
    const output: string[] = [];
    await this.walk(start, prefix, output);
    return output.sort();
  }

  private async directory(segments: readonly string[], create: boolean): Promise<FileSystemDirectoryHandle> {
    let directory = await this.root;
    for (const segment of segments) directory = await directory.getDirectoryHandle(segment, { create });
    return directory;
  }

  private async walk(directory: FileSystemDirectoryHandle, prefix: string, output: string[]): Promise<void> {
    for await (const [name, handle] of (directory as DirectoryEntryIterable).entries()) {
      const path = prefix ? `${prefix}/${name}` : name;
      if (handle.kind === "file") output.push(path);
      else await this.walk(handle as FileSystemDirectoryHandle, path, output);
    }
  }
}
