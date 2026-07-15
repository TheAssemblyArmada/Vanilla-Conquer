export function encodeJson(value: unknown): Uint8Array {
  return new TextEncoder().encode(JSON.stringify(value));
}

export function decodeJson<T>(data: Uint8Array): T {
  return JSON.parse(new TextDecoder().decode(data)) as T;
}

export async function sha256(data: Uint8Array): Promise<string> {
  const input: Uint8Array<ArrayBuffer> = data.buffer instanceof ArrayBuffer
    ? new Uint8Array(data.buffer, data.byteOffset, data.byteLength)
    : new Uint8Array(data);
  const digest = await crypto.subtle.digest("SHA-256", input);
  return [...new Uint8Array(digest)].map((byte) => byte.toString(16).padStart(2, "0")).join("");
}

export function validateId(value: string, label = "ID"): void {
  if (!/^[a-z0-9][a-z0-9._-]{0,127}$/i.test(value)) throw new Error(`${label} contains unsupported characters`);
}

export interface StorageReadiness {
  supported: boolean;
  persisted: boolean;
  quota?: number;
  usage?: number;
  available?: number;
  enoughSpace: boolean;
}

export async function checkStorageReadiness(requiredBytes = 0): Promise<StorageReadiness> {
  if (!navigator.storage?.getDirectory) return { supported: false, persisted: false, enoughSpace: false };
  const estimate: StorageEstimate = await navigator.storage.estimate().catch(() => ({}));
  // Requesting persistence can display permission UI and remains pending in
  // some browsers/headless environments. It is an enhancement, not a
  // prerequisite for OPFS, so startup only observes the current status.
  const persisted = typeof navigator.storage.persisted === "function"
    ? await navigator.storage.persisted().catch(() => false)
    : false;
  const available = estimate.quota === undefined ? undefined : Math.max(0, estimate.quota - (estimate.usage ?? 0));
  return {
    supported: true,
    persisted,
    quota: estimate.quota,
    usage: estimate.usage,
    available,
    enoughSpace: available === undefined || available >= requiredBytes,
  };
}
