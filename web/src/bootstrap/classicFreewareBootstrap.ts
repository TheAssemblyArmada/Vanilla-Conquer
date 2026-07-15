import { sha256, validateId } from "../storage/helpers";

export const CLASSIC_FREEWARE_DESCRIPTOR_FILENAME = "classic-freeware-v1.json";
export const CLASSIC_FREEWARE_DESCRIPTOR_FORMAT = "cncweb-classic-freeware";
export const CLASSIC_FREEWARE_DESCRIPTOR_VERSION = 1;
export const CLASSIC_FREEWARE_SOURCE_PRODUCT = "tiberian-dawn-freeware";
export const CLASSIC_FREEWARE_SOURCE_PROVIDER = "ea-freeware";

export const MAX_CLASSIC_FREEWARE_DESCRIPTOR_BYTES = 16 * 1024;
export const MAX_CLASSIC_FREEWARE_ARCHIVE_BYTES = 2 * 1024 * 1024 * 1024;

export interface ClassicFreewareSourceV1 {
  product: typeof CLASSIC_FREEWARE_SOURCE_PRODUCT;
  provider: typeof CLASSIC_FREEWARE_SOURCE_PROVIDER;
}

export interface ClassicFreewareArchiveV1 {
  url: URL;
  bytes: number;
  sha256: string;
}

export interface ClassicFreewarePackageV1 {
  id: string;
  contentSha256: string;
  source: ClassicFreewareSourceV1;
  archive: ClassicFreewareArchiveV1;
}

export interface ClassicFreewareDescriptorV1 {
  format: typeof CLASSIC_FREEWARE_DESCRIPTOR_FORMAT;
  version: typeof CLASSIC_FREEWARE_DESCRIPTOR_VERSION;
  package: ClassicFreewarePackageV1;
}

export interface ClassicFreewareManifestIdentity {
  package_id: string;
  content_sha256: string;
  source: {
    product: string;
    provider: string;
  };
}

export interface ClassicFreewareInstalledContent {
  id: string;
  manifest: ClassicFreewareManifestIdentity;
}

export type ClassicFreewareFetch = (input: RequestInfo | URL, init?: RequestInit) => Promise<Response>;

export interface ClassicFreewareDescriptorLocation {
  applicationUrl?: string | URL;
  descriptorUrl?: string | URL;
}

export interface FetchClassicFreewareDescriptorOptions extends ClassicFreewareDescriptorLocation {
  fetcher?: ClassicFreewareFetch;
}

export interface BootstrapClassicFreewareOptions extends FetchClassicFreewareDescriptorOptions {
  listInstalled: () => Promise<readonly ClassicFreewareInstalledContent[]>;
  importPackage: (archive: Blob, expected: ClassicFreewarePackageV1) => Promise<ClassicFreewareInstalledContent>;
}

export type ClassicFreewareBootstrapResult =
  | {
    status: "already-installed";
    descriptor: ClassicFreewareDescriptorV1;
    installed: ClassicFreewareInstalledContent;
  }
  | {
    status: "installed";
    descriptor: ClassicFreewareDescriptorV1;
    installed: ClassicFreewareInstalledContent;
  };

function record(value: unknown, label: string): Record<string, unknown> {
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error(`${label} must be an object`);
  return value as Record<string, unknown>;
}

function exactKeys(value: Record<string, unknown>, expected: readonly string[], label: string): void {
  const actual = Object.keys(value).sort();
  const wanted = [...expected].sort();
  if (actual.length !== wanted.length || actual.some((key, index) => key !== wanted[index])) {
    throw new Error(`${label} contains missing or unknown fields`);
  }
}

function lowercaseSha256(value: unknown, label: string): string {
  if (typeof value !== "string" || !/^[a-f0-9]{64}$/.test(value)) throw new Error(`${label} must be a lowercase SHA-256 digest`);
  return value;
}

function applicationUrl(value: string | URL | undefined): URL {
  if (value !== undefined) return new URL(value.toString());
  if (typeof document !== "undefined" && document.baseURI) return new URL(document.baseURI);
  if (typeof location !== "undefined" && location.href) return new URL(location.href);
  throw new Error("Classic freeware bootstrap requires an application URL outside a browser document");
}

function assertCleanHttpUrl(url: URL, label: string): void {
  if (url.protocol !== "http:" && url.protocol !== "https:") throw new Error(`${label} must use HTTP or HTTPS`);
  if (url.username || url.password || url.search || url.hash) throw new Error(`${label} must not contain credentials, a query, or a fragment`);
}

function assertApplicationUrl(url: URL): void {
  if (url.protocol !== "http:" && url.protocol !== "https:") throw new Error("Application URL must use HTTP or HTTPS");
  if (url.username || url.password) throw new Error("Application URL must not contain credentials");
}

function resolveDescriptorUrl(location: ClassicFreewareDescriptorLocation): { application: URL; descriptor: URL } {
  const application = applicationUrl(location.applicationUrl);
  // The application may legitimately use a query for an acceptance session or
  // deep link. URL resolution does not carry that query into the fixed
  // descriptor URL, while credentials remain forbidden.
  assertApplicationUrl(application);
  const descriptor = location.descriptorUrl === undefined
    ? new URL(CLASSIC_FREEWARE_DESCRIPTOR_FILENAME, application)
    : new URL(location.descriptorUrl.toString(), application);
  assertCleanHttpUrl(descriptor, "Classic freeware descriptor URL");
  if (descriptor.origin !== application.origin) throw new Error("Classic freeware descriptor URL must be same-origin");
  if (descriptor.pathname.split("/").at(-1) !== CLASSIC_FREEWARE_DESCRIPTOR_FILENAME) {
    throw new Error(`Classic freeware descriptor URL must end with ${CLASSIC_FREEWARE_DESCRIPTOR_FILENAME}`);
  }
  return { application, descriptor };
}

function parseArchive(value: unknown, descriptorUrl: URL, application: URL): ClassicFreewareArchiveV1 {
  const archive = record(value, "Classic freeware archive");
  exactKeys(archive, ["url", "bytes", "sha256"], "Classic freeware archive");
  if (typeof archive.url !== "string" || !archive.url || archive.url.length > 2048) throw new Error("Classic freeware archive URL is invalid");
  const url = new URL(archive.url, descriptorUrl);
  assertCleanHttpUrl(url, "Classic freeware archive URL");
  if (url.origin !== application.origin) throw new Error("Classic freeware archive URL must be same-origin");
  if (!url.pathname.toLowerCase().endsWith(".cncweb")) throw new Error("Classic freeware archive URL must identify a .cncweb package");
  if (!Number.isSafeInteger(archive.bytes) || Number(archive.bytes) <= 0 || Number(archive.bytes) > MAX_CLASSIC_FREEWARE_ARCHIVE_BYTES) {
    throw new Error(`Classic freeware archive bytes must be an integer between 1 and ${MAX_CLASSIC_FREEWARE_ARCHIVE_BYTES}`);
  }
  return {
    url,
    bytes: Number(archive.bytes),
    sha256: lowercaseSha256(archive.sha256, "Classic freeware archive sha256"),
  };
}

export function parseClassicFreewareDescriptor(
  value: unknown,
  location: ClassicFreewareDescriptorLocation = {},
): ClassicFreewareDescriptorV1 {
  const { application, descriptor: descriptorUrl } = resolveDescriptorUrl(location);
  const descriptor = record(value, "Classic freeware descriptor");
  exactKeys(descriptor, ["format", "version", "package"], "Classic freeware descriptor");
  if (descriptor.format !== CLASSIC_FREEWARE_DESCRIPTOR_FORMAT || descriptor.version !== CLASSIC_FREEWARE_DESCRIPTOR_VERSION) {
    throw new Error("Classic freeware descriptor format is unsupported");
  }

  const packageValue = record(descriptor.package, "Classic freeware package");
  exactKeys(packageValue, ["id", "contentSha256", "source", "archive"], "Classic freeware package");
  if (typeof packageValue.id !== "string") throw new Error("Classic freeware package ID is invalid");
  validateId(packageValue.id, "Classic freeware package ID");

  const source = record(packageValue.source, "Classic freeware source");
  exactKeys(source, ["product", "provider"], "Classic freeware source");
  if (source.product !== CLASSIC_FREEWARE_SOURCE_PRODUCT || source.provider !== CLASSIC_FREEWARE_SOURCE_PROVIDER) {
    throw new Error("Classic freeware source must identify the EA Tiberian Dawn freeware release");
  }

  return {
    format: CLASSIC_FREEWARE_DESCRIPTOR_FORMAT,
    version: CLASSIC_FREEWARE_DESCRIPTOR_VERSION,
    package: {
      id: packageValue.id,
      contentSha256: lowercaseSha256(packageValue.contentSha256, "Classic freeware contentSha256"),
      source: {
        product: CLASSIC_FREEWARE_SOURCE_PRODUCT,
        provider: CLASSIC_FREEWARE_SOURCE_PROVIDER,
      },
      archive: parseArchive(packageValue.archive, descriptorUrl, application),
    },
  };
}

function fetchImplementation(fetcher: ClassicFreewareFetch | undefined): ClassicFreewareFetch {
  if (fetcher) return fetcher;
  if (typeof globalThis.fetch !== "function") throw new Error("Classic freeware bootstrap requires Fetch API support");
  return globalThis.fetch.bind(globalThis);
}

function assertResponseUrl(response: Response, expected: URL, label: string): void {
  if (!response.url) return;
  const actual = new URL(response.url);
  if (actual.href !== expected.href) throw new Error(`${label} resolved to an unexpected URL`);
}

export async function fetchClassicFreewareDescriptor(
  options: FetchClassicFreewareDescriptorOptions = {},
): Promise<ClassicFreewareDescriptorV1> {
  const location = resolveDescriptorUrl(options);
  const response = await fetchImplementation(options.fetcher)(location.descriptor, {
    cache: "no-store",
    credentials: "same-origin",
    headers: { Accept: "application/json" },
    redirect: "error",
  });
  if (!response.ok) throw new Error(`Classic freeware descriptor request failed with status ${response.status}`);
  assertResponseUrl(response, location.descriptor, "Classic freeware descriptor request");
  const contentType = response.headers.get("content-type")?.split(";", 1)[0].trim().toLowerCase();
  if (contentType !== "application/json" && !contentType?.endsWith("+json")) throw new Error("Classic freeware descriptor response must be JSON");
  const bytes = new Uint8Array(await response.arrayBuffer());
  if (!bytes.byteLength || bytes.byteLength > MAX_CLASSIC_FREEWARE_DESCRIPTOR_BYTES) {
    throw new Error(`Classic freeware descriptor must contain 1 to ${MAX_CLASSIC_FREEWARE_DESCRIPTOR_BYTES} bytes`);
  }
  let value: unknown;
  try {
    value = JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(bytes));
  } catch (cause) {
    throw new Error("Classic freeware descriptor is not valid UTF-8 JSON", { cause });
  }
  return parseClassicFreewareDescriptor(value, {
    applicationUrl: location.application,
    descriptorUrl: location.descriptor,
  });
}

function matchesExpectedPackage(installed: ClassicFreewareInstalledContent, expected: ClassicFreewarePackageV1): boolean {
  return installed.id === expected.id
    && installed.manifest.package_id === expected.id
    && installed.manifest.content_sha256 === expected.contentSha256
    && installed.manifest.source.product === expected.source.product
    && installed.manifest.source.provider === expected.source.provider;
}

function assertExpectedPackage(installed: ClassicFreewareInstalledContent, expected: ClassicFreewarePackageV1): void {
  if (!matchesExpectedPackage(installed, expected)) {
    throw new Error("Imported classic freeware package does not match the bootstrap descriptor");
  }
}

async function fetchVerifiedArchive(
  expected: ClassicFreewareArchiveV1,
  fetcher: ClassicFreewareFetch,
): Promise<Blob> {
  const response = await fetcher(expected.url, {
    cache: "no-store",
    credentials: "same-origin",
    headers: { Accept: "application/zip, application/octet-stream" },
    redirect: "error",
  });
  if (!response.ok) throw new Error(`Classic freeware archive request failed with status ${response.status}`);
  assertResponseUrl(response, expected.url, "Classic freeware archive request");
  const archive = await response.blob();
  if (archive.size !== expected.bytes) {
    throw new Error(`Classic freeware archive byte length mismatch: expected ${expected.bytes}, received ${archive.size}`);
  }
  const digest = await sha256(new Uint8Array(await archive.arrayBuffer()));
  if (digest !== expected.sha256) throw new Error("Classic freeware archive SHA-256 mismatch");
  return archive;
}

export async function bootstrapClassicFreeware(
  options: BootstrapClassicFreewareOptions,
): Promise<ClassicFreewareBootstrapResult> {
  const descriptor = await fetchClassicFreewareDescriptor(options);
  const expected = descriptor.package;
  const installed = (await options.listInstalled()).find((candidate) => matchesExpectedPackage(candidate, expected));
  if (installed) return { status: "already-installed", descriptor, installed };

  const archive = await fetchVerifiedArchive(expected.archive, fetchImplementation(options.fetcher));
  const imported = await options.importPackage(archive, expected);
  assertExpectedPackage(imported, expected);
  return { status: "installed", descriptor, installed: imported };
}
