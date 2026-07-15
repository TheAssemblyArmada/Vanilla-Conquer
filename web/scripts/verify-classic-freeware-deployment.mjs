#!/usr/bin/env node

import { createHash } from "node:crypto";
import {
  existsSync,
  lstatSync,
  readFileSync,
  realpathSync,
} from "node:fs";
import { basename, resolve, sep } from "node:path";
import { pathToFileURL } from "node:url";

import { Uint8ArrayReader, Uint8ArrayWriter, ZipReader } from "@zip.js/zip.js";

import {
  CLASSIC_FREEWARE_BASE_URL_ENV,
  resolveClassicFreewareBaseURL,
} from "./classic-freeware-base-url.mjs";

const MAXIMUM_TEXT_ASSET_BYTES = 16 * 1024 * 1024;
const MAXIMUM_ARCHIVE_BYTES = 2 * 1024 * 1024 * 1024;
const MAXIMUM_PACKAGE_FILE_BYTES = 64 * 1024 * 1024;
const MAXIMUM_PACKAGE_EXPANDED_BYTES = 2 * 1024 * 1024 * 1024;
const MAXIMUM_HTTP_REQUEST_MS = 2 * 60 * 1000;
const FIXED_ASSETS = Object.freeze({
  index: "index.html",
  manifest: "manifest.webmanifest",
  serviceWorker: "sw.js",
  build: "build-v1.json",
  engineJavaScript: "engine/tiberiandawn.js",
  engineWasm: "engine/tiberiandawn.wasm",
  freewareDescriptor: "classic-freeware-v1.json",
});

const MIME = Object.freeze({
  html: ["text/html"],
  javascript: ["text/javascript", "application/javascript"],
  css: ["text/css"],
  json: ["application/json"],
  manifest: ["application/manifest+json", "application/json"],
  svg: ["image/svg+xml"],
  wasm: ["application/wasm"],
  archive: ["application/octet-stream", "application/zip"],
});

export const REQUIRED_CONTENT_SECURITY_POLICY = [
  "base-uri 'none'",
  "connect-src 'self'",
  "default-src 'self'",
  "font-src 'self' data:",
  "form-action 'none'",
  "frame-ancestors 'none'",
  "frame-src 'none'",
  "img-src 'self' blob: data:",
  "manifest-src 'self'",
  "media-src 'self' blob:",
  "object-src 'none'",
  "script-src 'self' 'wasm-unsafe-eval'",
  "script-src-attr 'none'",
  "style-src 'self' 'unsafe-inline'",
  "worker-src 'self' blob:",
].join("; ");
export const REQUIRED_PERMISSIONS_POLICY = "camera=(), geolocation=(), microphone=(), payment=(), usb=()";

function verificationError(message) {
  return new Error(`Classic-freeware deployment verification failed: ${message}`);
}

function sha256(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

function exactKeys(value, expected, label) {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    throw verificationError(`${label} must be an object`);
  }
  const actual = Object.keys(value).sort();
  const wanted = [...expected].sort();
  if (actual.length !== wanted.length || actual.some((key, index) => key !== wanted[index])) {
    throw verificationError(`${label} contains missing or unknown fields`);
  }
}

function scanJsonForDuplicateKeys(text, label) {
  let offset = 0;
  const fail = () => { throw verificationError(`${label} is not strict JSON`); };
  const whitespace = () => {
    while (offset < text.length && /[\t\n\r ]/.test(text[offset])) offset += 1;
  };
  const string = () => {
    if (text[offset] !== '"') fail();
    const start = offset;
    offset += 1;
    while (offset < text.length) {
      const character = text[offset];
      if (character === '"') {
        offset += 1;
        try { return JSON.parse(text.slice(start, offset)); } catch { fail(); }
      }
      if (character === "\\") {
        offset += 1;
        if (offset >= text.length) fail();
        if (text[offset] === "u") {
          if (!/^[a-f\d]{4}$/i.test(text.slice(offset + 1, offset + 5))) fail();
          offset += 5;
        } else {
          if (!/["\\/bfnrt]/.test(text[offset])) fail();
          offset += 1;
        }
      } else {
        if (character.charCodeAt(0) < 0x20) fail();
        offset += 1;
      }
    }
    fail();
  };
  const value = () => {
    whitespace();
    if (text[offset] === "{") {
      offset += 1;
      whitespace();
      const keys = new Set();
      if (text[offset] === "}") { offset += 1; return; }
      while (offset < text.length) {
        const key = string();
        if (keys.has(key)) throw verificationError(`${label} contains duplicate key ${JSON.stringify(key)}`);
        keys.add(key);
        whitespace();
        if (text[offset] !== ":") fail();
        offset += 1;
        value();
        whitespace();
        if (text[offset] === "}") { offset += 1; return; }
        if (text[offset] !== ",") fail();
        offset += 1;
        whitespace();
      }
      fail();
    }
    if (text[offset] === "[") {
      offset += 1;
      whitespace();
      if (text[offset] === "]") { offset += 1; return; }
      while (offset < text.length) {
        value();
        whitespace();
        if (text[offset] === "]") { offset += 1; return; }
        if (text[offset] !== ",") fail();
        offset += 1;
      }
      fail();
    }
    if (text[offset] === '"') { string(); return; }
    for (const literal of ["true", "false", "null"]) {
      if (text.startsWith(literal, offset)) { offset += literal.length; return; }
    }
    const number = /^-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?/.exec(text.slice(offset));
    if (!number) fail();
    offset += number[0].length;
  };
  value();
  whitespace();
  if (offset !== text.length) fail();
}

function parseJson(bytes, label) {
  let text;
  try { text = new TextDecoder("utf-8", { fatal: true }).decode(bytes); } catch {
    throw verificationError(`${label} is not valid UTF-8`);
  }
  scanJsonForDuplicateKeys(text, label);
  try { return JSON.parse(text); } catch { throw verificationError(`${label} is not valid JSON`); }
}

function decodePathname(pathname, label) {
  if (/%2f|%5c/i.test(pathname)) throw verificationError(`${label} contains an encoded path separator`);
  try { return decodeURIComponent(pathname); } catch { throw verificationError(`${label} contains invalid URL encoding`); }
}

export function resolveDeploymentAssetURL(reference, baseURL, label = "Asset URL", resolutionBase = baseURL) {
  if (typeof reference !== "string" || reference.length === 0 || reference !== reference.trim()) {
    throw verificationError(`${label} must be a non-empty URL without surrounding whitespace`);
  }
  if (reference.includes("?") || reference.includes("#")) {
    throw verificationError(`${label} must not contain a query or fragment`);
  }
  let asset;
  try { asset = new URL(reference, resolutionBase); } catch { throw verificationError(`${label} is invalid`); }
  const base = baseURL instanceof URL ? baseURL : new URL(baseURL);
  if (asset.protocol !== base.protocol || asset.origin !== base.origin) {
    throw verificationError(`${label} must remain on the deployment origin`);
  }
  if (asset.username || asset.password || asset.search || asset.hash) {
    throw verificationError(`${label} must not contain credentials, a query, or a fragment`);
  }
  const basePath = decodePathname(base.pathname, "Deployment URL");
  const assetPath = decodePathname(asset.pathname, label);
  if (!assetPath.startsWith(basePath)) throw verificationError(`${label} escapes the deployment directory`);
  return asset;
}

function expectedMimes(url) {
  const pathname = url.pathname.toLowerCase();
  if (pathname.endsWith(".html") || pathname.endsWith("/")) return MIME.html;
  if (pathname.endsWith(".webmanifest")) return MIME.manifest;
  if (pathname.endsWith(".json")) return MIME.json;
  if (pathname.endsWith(".js")) return MIME.javascript;
  if (pathname.endsWith(".css")) return MIME.css;
  if (pathname.endsWith(".wasm")) return MIME.wasm;
  if (pathname.endsWith(".svg")) return MIME.svg;
  if (pathname.endsWith(".cncweb")) return MIME.archive;
  throw verificationError(`No MIME policy exists for ${url.href}`);
}

function normalizedMime(response) {
  return (response.headers.get("content-type") ?? "").split(";", 1)[0].trim().toLowerCase();
}

function canonicalCsp(value) {
  const directives = value.split(";").map((directive) => directive.trim()).filter(Boolean);
  const parsed = new Map();
  for (const directive of directives) {
    const [name, ...tokens] = directive.split(/\s+/);
    if (!name || parsed.has(name)) throw verificationError("Content-Security-Policy contains a duplicate or empty directive");
    parsed.set(name, tokens);
  }
  return [...parsed.entries()]
    .sort(([left], [right]) => left.localeCompare(right))
    .map(([name, tokens]) => `${name}${tokens.length ? ` ${tokens.join(" ")}` : ""}`)
    .join("; ");
}

function validateSecurityHeaders(response, label) {
  const csp = response.headers.get("content-security-policy") ?? "";
  if (canonicalCsp(csp) !== canonicalCsp(REQUIRED_CONTENT_SECURITY_POLICY)) {
    throw verificationError(`${label} Content-Security-Policy does not match the release contract`);
  }
  if (/(?:^|\s)'unsafe-eval'(?:\s|$)/.test(csp)) {
    throw verificationError(`${label} Content-Security-Policy must not allow unsafe-eval`);
  }
  const exact = [
    ["x-content-type-options", "nosniff"],
    ["referrer-policy", "no-referrer"],
    ["x-frame-options", "DENY"],
    ["cross-origin-resource-policy", "same-origin"],
  ];
  for (const [name, expected] of exact) {
    if ((response.headers.get(name) ?? "").trim() !== expected) {
      throw verificationError(`${label} ${name} header must be ${expected}`);
    }
  }
  const permissions = (response.headers.get("permissions-policy") ?? "")
    .split(",")
    .map((directive) => directive.trim())
    .filter(Boolean)
    .sort()
    .join(", ");
  if (permissions !== REQUIRED_PERMISSIONS_POLICY.split(",").map((value) => value.trim()).sort().join(", ")) {
    throw verificationError(`${label} Permissions-Policy does not match the release contract`);
  }
}

function cacheDirectives(response, label) {
  const value = response.headers.get("cache-control") ?? "";
  const directives = new Map();
  for (const raw of value.split(",")) {
    const [rawName, rawArgument] = raw.trim().split("=", 2);
    const name = rawName.toLowerCase();
    if (!name || directives.has(name)) throw verificationError(`${label} Cache-Control is missing or malformed`);
    directives.set(name, rawArgument?.replace(/^"|"$/g, ""));
  }
  return directives;
}

function validateCacheClass(response, label, cacheClass, relaxed) {
  if (relaxed) return;
  const directives = cacheDirectives(response, label);
  if (cacheClass === "revalidate") {
    const explicitRevalidation = directives.has("no-cache")
      && directives.get("max-age") === "0"
      && directives.has("must-revalidate");
    if (!explicitRevalidation || directives.has("immutable") || directives.has("no-store") || directives.has("private")) {
      throw verificationError(`${label} must use a revalidating Cache-Control policy`);
    }
    return;
  }
  const maximumAge = directives.get("max-age");
  if (
    !directives.has("public") || !directives.has("immutable")
    || maximumAge === undefined || !/^\d+$/.test(maximumAge) || Number(maximumAge) < 31_536_000
    || directives.has("no-cache") || directives.has("no-store") || directives.has("private")
  ) {
    throw verificationError(`${label} must use a public one-year immutable Cache-Control policy`);
  }
  if (cacheClass === "immutable-no-transform" && !directives.has("no-transform")) {
    throw verificationError(`${label} immutable Cache-Control policy must include no-transform`);
  }
}

function isLoopback(hostname) {
  return hostname === "localhost" || hostname === "[::1]" || /^127(?:\.\d{1,3}){3}$/.test(hostname);
}

function responseLength(response, label) {
  const raw = response.headers.get("content-length");
  if (raw === null || !/^(?:0|[1-9]\d*)$/.test(raw)) {
    throw verificationError(`${label} must provide a valid Content-Length`);
  }
  const value = Number(raw);
  if (!Number.isSafeInteger(value)) throw verificationError(`${label} Content-Length is unsupported`);
  return value;
}

function text(bytes, label) {
  try { return new TextDecoder("utf-8", { fatal: true }).decode(bytes); } catch {
    throw verificationError(`${label} is not valid UTF-8`);
  }
}

function localRelativePath(url, baseURL) {
  if (url.href === baseURL.href) return FIXED_ASSETS.index;
  const relative = decodePathname(url.pathname, "Deployment asset").slice(
    decodePathname(baseURL.pathname, "Deployment URL").length,
  );
  if (!relative || relative.endsWith("/")) throw verificationError(`Cannot map ${url.href} to a distribution file`);
  return relative;
}

function cacheClassForURL(url, baseURL) {
  const relative = localRelativePath(url, baseURL);
  if (/^(?:engine\/tiberiandawn\.(?:js|wasm)|classic-freeware-gdi-v1\.cncweb)$/.test(relative)) {
    return "immutable-no-transform";
  }
  if (/^assets\/[A-Za-z0-9_.-]+-[A-Za-z0-9_-]{6,}\.(?:js|css|wasm)$/.test(relative)) return "immutable";
  return "revalidate";
}

function verifyLocalParity(records, baseURL, distDirectory) {
  const root = realpathSync(resolve(distDirectory));
  const prefix = root.endsWith(sep) ? root : `${root}${sep}`;
  for (const record of records.values()) {
    const relative = localRelativePath(record.url, baseURL);
    const candidate = resolve(root, relative);
    if (!candidate.startsWith(prefix) || !existsSync(candidate) || !lstatSync(candidate).isFile()) {
      throw verificationError(`Distribution parity file is missing: ${relative}`);
    }
    const real = realpathSync(candidate);
    if (!real.startsWith(prefix)) throw verificationError(`Distribution parity path escapes through a symlink: ${relative}`);
    const local = readFileSync(real);
    if (!local.equals(record.bytes)) throw verificationError(`Remote bytes differ from --dist for ${relative}`);
  }
}

function parseHtmlReferences(html, baseURL) {
  if (/<base\b/i.test(html)) throw verificationError("index.html must not contain a base element");
  const assignments = [...html.matchAll(/\b(?:src|href)\s*=/gi)];
  const references = [...html.matchAll(/\b(src|href)\s*=\s*(?:"([^"]*)"|'([^']*)')/gi)]
    .map((match, index) => resolveDeploymentAssetURL(match[2] ?? match[3], baseURL, `index.html asset ${index + 1}`));
  if (references.length !== assignments.length) throw verificationError("index.html contains an unquoted or malformed asset URL");
  if (new Set(references.map(({ href }) => href)).size !== references.length) {
    throw verificationError("index.html contains a duplicate asset URL");
  }
  const paths = references.map(({ pathname }) => pathname);
  if (!paths.some((path) => path.endsWith(".js")) || !paths.some((path) => path.endsWith(".css"))) {
    throw verificationError("index.html must reference application JavaScript and CSS");
  }
  if (!paths.some((path) => path.endsWith(`/${FIXED_ASSETS.manifest}`)) || !paths.some((path) => path.endsWith("/icon.svg"))) {
    throw verificationError("index.html must reference the deployment manifest and icon");
  }
  return references;
}

function validateWebManifest(value, manifestURL, baseURL) {
  exactKeys(value, [
    "name", "short_name", "description", "start_url", "scope", "display", "orientation",
    "background_color", "theme_color", "icons",
  ], "Web app manifest");
  if (typeof value.name !== "string" || typeof value.short_name !== "string" || typeof value.description !== "string") {
    throw verificationError("Web app manifest names and description are invalid");
  }
  const start = resolveDeploymentAssetURL(value.start_url, baseURL, "Web app start_url", manifestURL);
  const scope = resolveDeploymentAssetURL(value.scope, baseURL, "Web app scope", manifestURL);
  if (start.href !== baseURL.href || scope.href !== baseURL.href) {
    throw verificationError("Web app start_url and scope must equal the deployment directory");
  }
  if (!Array.isArray(value.icons) || value.icons.length === 0) throw verificationError("Web app manifest icons are missing");
  return value.icons.map((icon, index) => {
    exactKeys(icon, ["src", "sizes", "type", "purpose"], `Web app icon ${index + 1}`);
    if (icon.type !== "image/svg+xml" || icon.sizes !== "any" || typeof icon.purpose !== "string") {
      throw verificationError(`Web app icon ${index + 1} is invalid`);
    }
    return resolveDeploymentAssetURL(icon.src, baseURL, `Web app icon ${index + 1}`, manifestURL);
  });
}

function extractServiceWorkerArray(source, name) {
  const match = new RegExp(`\\bconst\\s+${name}\\s*=\\s*(\\[[\\s\\S]*?\\]);`).exec(source);
  if (!match) throw verificationError(`Service worker ${name} declaration is missing`);
  const parsed = parseJson(new TextEncoder().encode(match[1]), `Service worker ${name}`);
  if (!Array.isArray(parsed) || parsed.some((value) => typeof value !== "string") || new Set(parsed).size !== parsed.length) {
    throw verificationError(`Service worker ${name} must be a unique string array`);
  }
  return parsed;
}

function validateServiceWorker(source, baseURL, buildId, htmlReferences) {
  if (/\.cncweb|classic-freeware/i.test(source)) {
    throw verificationError("Service worker must not name or cache classic-freeware payloads");
  }
  if (/\bcache\.add(?:All)?\s*\(/.test(source)) {
    throw verificationError("Service worker must use its reviewed cache.put allowlist path");
  }
  const version = /\bconst\s+CACHE_VERSION\s*=\s*["']([a-f0-9]{16})["']\s*;/.exec(source)?.[1];
  if (!version || version !== buildId) throw verificationError("Service worker cache version does not match build-v1.json");
  if (!/!isVersionedRuntimeAsset\(url\)\s*&&\s*!isSourceDerivedEngineAsset\(url\)\s*&&\s*!isStaticShellAsset\(url\)\)\s*return\s*;/.test(source)) {
    throw verificationError("Service worker fetch caching is not constrained to the reviewed shell allowlists");
  }

  const staticShell = extractServiceWorkerArray(source, "STATIC_SHELL");
  const optionalEngine = extractServiceWorkerArray(source, "OPTIONAL_ENGINE");
  const buildAssets = extractServiceWorkerArray(source, "BUILD_ASSETS");
  const expectedStatic = ["./manifest.webmanifest", "./icon.svg", "./legal.html", "./build-v1.json"];
  const expectedEngine = ["./engine/tiberiandawn.js", "./engine/tiberiandawn.wasm"];
  if (JSON.stringify(staticShell) !== JSON.stringify(expectedStatic)) {
    throw verificationError("Service worker static shell allowlist is unexpected");
  }
  if (JSON.stringify(optionalEngine) !== JSON.stringify(expectedEngine)) {
    throw verificationError("Service worker optional engine allowlist is unexpected");
  }
  if (buildAssets.filter((asset) => /^assets\/simulation\.worker-[A-Za-z0-9_-]{6,}\.js$/.test(asset)).length !== 1) {
    throw verificationError("Service worker must name exactly one versioned simulation worker");
  }
  for (const required of ["engine/tiberiandawn.js", "engine/tiberiandawn.wasm"]) {
    if (!buildAssets.includes(required)) throw verificationError(`Service worker BUILD_ASSETS is missing ${required}`);
  }
  const htmlRuntimeAssets = htmlReferences
    .map((url) => localRelativePath(url, baseURL))
    .filter((path) => /^assets\/.+\.(?:js|css)$/.test(path));
  for (const required of htmlRuntimeAssets) {
    if (!buildAssets.includes(required)) throw verificationError(`Service worker BUILD_ASSETS is missing ${required}`);
  }
  for (const asset of buildAssets) {
    if (!/^(?:assets\/[A-Za-z0-9_.-]+-[A-Za-z0-9_-]{6,}\.(?:js|css|wasm)|engine\/tiberiandawn\.(?:js|wasm))$/.test(asset)) {
      throw verificationError(`Service worker BUILD_ASSETS contains an unsupported path: ${asset}`);
    }
  }
  return [...staticShell, ...optionalEngine, ...buildAssets]
    .map((reference, index) => resolveDeploymentAssetURL(reference, baseURL, `Service worker asset ${index + 1}`));
}

function validateBuildDescriptor(value) {
  exactKeys(value, ["format", "version", "id"], "Build descriptor");
  if (value.format !== "cncweb-build" || value.version !== 1 || !/^[a-f0-9]{16}$/.test(value.id)) {
    throw verificationError("Build descriptor is invalid");
  }
  return value;
}

function validateFreewareDescriptor(value) {
  exactKeys(value, ["format", "version", "package"], "Classic-freeware descriptor");
  exactKeys(value.package, ["id", "contentSha256", "source", "archive"], "Classic-freeware package descriptor");
  exactKeys(value.package.source, ["product", "provider"], "Classic-freeware source descriptor");
  exactKeys(value.package.archive, ["url", "bytes", "sha256"], "Classic-freeware archive descriptor");
  if (value.format !== "cncweb-classic-freeware" || value.version !== 1 || value.package.id !== "classic-freeware-gdi-v1") {
    throw verificationError("Classic-freeware descriptor identity is invalid");
  }
  if (value.package.source.product !== "tiberian-dawn-freeware" || value.package.source.provider !== "ea-freeware") {
    throw verificationError("Classic-freeware descriptor provenance is invalid");
  }
  if (!/^[a-f0-9]{64}$/.test(value.package.contentSha256) || !/^[a-f0-9]{64}$/.test(value.package.archive.sha256)) {
    throw verificationError("Classic-freeware descriptor hashes must be lowercase SHA-256 values");
  }
  if (!Number.isSafeInteger(value.package.archive.bytes) || value.package.archive.bytes <= 0 || value.package.archive.bytes > MAXIMUM_ARCHIVE_BYTES) {
    throw verificationError("Classic-freeware archive size is invalid");
  }
  if (value.package.archive.url !== "./classic-freeware-gdi-v1.cncweb") {
    throw verificationError("Classic-freeware archive URL is not canonical");
  }
  return value;
}

function manifestContentDigest(files) {
  const hash = createHash("sha256");
  hash.update("CNCWEB-CONTENT-MANIFEST-V1\0");
  for (const file of files) {
    const path = Buffer.from(file.path, "utf8");
    const role = Buffer.from(file.role, "utf8");
    const pathLength = Buffer.alloc(8);
    pathLength.writeBigUInt64LE(BigInt(path.length));
    const size = Buffer.alloc(8);
    size.writeBigUInt64LE(BigInt(file.size));
    const roleLength = Buffer.alloc(8);
    roleLength.writeBigUInt64LE(BigInt(role.length));
    hash.update(pathLength);
    hash.update(path);
    hash.update(size);
    hash.update(Buffer.from(file.sha256, "hex"));
    hash.update(roleLength);
    hash.update(role);
  }
  return hash.digest("hex");
}

export function calculateContentDigest(files) {
  return manifestContentDigest(files);
}

async function verifyPackageArchive(bytes, descriptor) {
  const reader = new ZipReader(new Uint8ArrayReader(bytes));
  try {
    const entries = await reader.getEntries();
    const names = new Set();
    for (const entry of entries) {
      if (entry.directory || entry.encrypted || !entry.getData || names.has(entry.filename.toLowerCase())) {
        throw verificationError(`Classic-freeware archive entry is unsafe or duplicated: ${entry.filename}`);
      }
      names.add(entry.filename.toLowerCase());
    }
    const manifestEntry = entries.find((entry) => entry.filename === "manifest.json");
    if (!manifestEntry?.getData) throw verificationError("Classic-freeware archive manifest.json is missing");
    if (Number(manifestEntry.uncompressedSize) > 4 * 1024 * 1024) throw verificationError("Classic-freeware package manifest is too large");
    const manifestBytes = await manifestEntry.getData(new Uint8ArrayWriter(), { checkSignature: true });
    const manifest = parseJson(manifestBytes, "Classic-freeware package manifest");
    exactKeys(manifest, [
      "format", "version", "package_id", "created_at_unix_ms", "source", "content", "content_sha256", "files",
    ], "Classic-freeware package manifest");
    exactKeys(manifest.source, ["product", "provider", "install_fingerprint_sha256"], "Classic-freeware package source");
    exactKeys(manifest.content, ["games", "locales"], "Classic-freeware package content");
    if (
      manifest.format !== "cncweb-content" || manifest.version !== 1
      || manifest.package_id !== descriptor.package.id
      || manifest.content_sha256 !== descriptor.package.contentSha256
      || manifest.source.product !== descriptor.package.source.product
      || manifest.source.provider !== descriptor.package.source.provider
      || !/^[a-f0-9]{64}$/.test(manifest.source.install_fingerprint_sha256)
      || JSON.stringify(manifest.content.games) !== JSON.stringify(["tiberian-dawn"])
      || !Array.isArray(manifest.content.locales) || manifest.content.locales.length === 0
    ) {
      throw verificationError("Classic-freeware package manifest identity or provenance is invalid");
    }
    if (!Array.isArray(manifest.files) || manifest.files.length !== entries.length - 1) {
      throw verificationError("Classic-freeware package file inventory is invalid");
    }

    const entryByName = new Map(entries.map((entry) => [entry.filename, entry]));
    let previous = "";
    let expandedBytes = 0;
    for (const file of manifest.files) {
      exactKeys(file, ["path", "size", "sha256", "role"], `Classic-freeware package file ${file?.path ?? "unknown"}`);
      if (
        typeof file.path !== "string" || !file.path || file.path.startsWith("/") || file.path.includes("\\")
        || file.path.split("/").some((part) => !part || part === "." || part === "..")
        || previous >= file.path || !Number.isSafeInteger(file.size) || file.size < 0
        || file.size > MAXIMUM_PACKAGE_FILE_BYTES || !/^[a-f0-9]{64}$/.test(file.sha256)
        || !["engine-data", "texture-atlas", "audio", "map", "configuration", "other"].includes(file.role)
      ) {
        throw verificationError(`Classic-freeware package file descriptor is invalid: ${file.path}`);
      }
      previous = file.path;
      const lower = file.path.toLowerCase();
      if (file.role === "video" || /(?:^|\/)(?:music|movies)(?:\/|$)/.test(lower) || /(?:scores|movies)\.mix$|\.vqa$/.test(lower)) {
        throw verificationError(`Classic-freeware package contains excluded music or movie content: ${file.path}`);
      }
      const entry = entryByName.get(file.path);
      if (!entry?.getData || Number(entry.uncompressedSize) !== file.size) {
        throw verificationError(`Classic-freeware package entry does not match its manifest: ${file.path}`);
      }
      expandedBytes += file.size;
      if (!Number.isSafeInteger(expandedBytes) || expandedBytes > MAXIMUM_PACKAGE_EXPANDED_BYTES) {
        throw verificationError("Classic-freeware package expands beyond the deployment limit");
      }
      const payload = await entry.getData(new Uint8ArrayWriter(), { checkSignature: true });
      if (payload.byteLength !== file.size || sha256(payload) !== file.sha256) {
        throw verificationError(`Classic-freeware package checksum failed: ${file.path}`);
      }
    }
    if (manifestContentDigest(manifest.files) !== manifest.content_sha256) {
      throw verificationError("Classic-freeware package aggregate content digest is invalid");
    }
    return { files: manifest.files.length, expandedBytes, fingerprint: manifest.source.install_fingerprint_sha256 };
  } finally {
    await reader.close();
  }
}

function validateEngine(javaScript, wasm) {
  const source = text(javaScript, "Tiberian Dawn engine JavaScript");
  if (!source.includes("WebAssembly") || !source.includes("tiberiandawn.wasm")) {
    throw verificationError("Tiberian Dawn engine JavaScript does not reference its Wasm module");
  }
  if (wasm.byteLength < 8 || !wasm.subarray(0, 8).every((byte, index) => byte === [0, 97, 115, 109, 1, 0, 0, 0][index])) {
    throw verificationError("Tiberian Dawn engine Wasm header is invalid");
  }
}

export async function verifyClassicFreewareDeployment({
  baseURL: inputBaseURL,
  distDirectory,
  fetchImpl = globalThis.fetch,
  allowLoopbackCacheRelaxation = false,
} = {}) {
  const normalized = resolveClassicFreewareBaseURL(inputBaseURL);
  if (!normalized) throw verificationError(`A deployment URL or ${CLASSIC_FREEWARE_BASE_URL_ENV} is required`);
  if (typeof fetchImpl !== "function") throw verificationError("A Fetch-compatible implementation is required");
  const baseURL = new URL(normalized);
  decodePathname(baseURL.pathname, "Deployment URL");
  if (allowLoopbackCacheRelaxation && !isLoopback(baseURL.hostname)) {
    throw verificationError("Cache-policy relaxation is permitted only for an explicit loopback deployment");
  }
  const records = new Map();

  const fetchAsset = async (
    url,
    label,
    mimes = expectedMimes(url),
    maximumBytes = MAXIMUM_TEXT_ASSET_BYTES,
    requireLength = false,
  ) => {
    const cacheClass = cacheClassForURL(url, baseURL);
    const existing = records.get(url.href);
    if (existing) {
      if (!mimes.includes(existing.mime)) throw verificationError(`${label} has an unexpected MIME type: ${existing.mime || "missing"}`);
      return existing;
    }
    let response;
    try {
      response = await fetchImpl(url, {
        redirect: "manual",
        cache: "no-store",
        credentials: "omit",
        signal: AbortSignal.timeout(MAXIMUM_HTTP_REQUEST_MS),
      });
    } catch (error) {
      throw verificationError(`${label} could not be fetched: ${error instanceof Error ? error.message : String(error)}`);
    }
    if (response.status !== 200 || response.redirected) throw verificationError(`${label} returned status ${response.status} or redirected`);
    if (!response.url || new URL(response.url).href !== url.href) throw verificationError(`${label} response URL changed unexpectedly`);
    const mime = normalizedMime(response);
    if (!mimes.includes(mime)) throw verificationError(`${label} has an unexpected MIME type: ${mime || "missing"}`);
    validateSecurityHeaders(response, label);
    validateCacheClass(response, label, cacheClass, allowLoopbackCacheRelaxation);
    const rawDeclaredLength = response.headers.get("content-length");
    const declaredLength = rawDeclaredLength === null
      ? (requireLength ? responseLength(response, label) : undefined)
      : responseLength(response, label);
    const contentEncoding = response.headers.get("content-encoding");
    const transferIsEncoded = contentEncoding !== null && contentEncoding !== "identity";
    if (declaredLength !== undefined && !transferIsEncoded && declaredLength > maximumBytes) {
      throw verificationError(`${label} declared Content-Length exceeds its byte limit`);
    }
    if (requireLength && transferIsEncoded) {
      throw verificationError(`${label} must not use transfer content encoding`);
    }
    const bytes = Buffer.from(await response.arrayBuffer());
    if (bytes.byteLength > maximumBytes) throw verificationError(`${label} exceeds its byte limit`);
    if (declaredLength !== undefined && !transferIsEncoded && declaredLength !== bytes.byteLength) {
      throw verificationError(`${label} Content-Length does not match its response body`);
    }
    const record = { url, mime, bytes, sha256: sha256(bytes) };
    records.set(url.href, record);
    return record;
  };

  const rootDocument = await fetchAsset(baseURL, "Deployment directory document", MIME.html);
  const indexURL = resolveDeploymentAssetURL(FIXED_ASSETS.index, baseURL, "index.html URL");
  const index = await fetchAsset(indexURL, "index.html", MIME.html);
  if (!rootDocument.bytes.equals(index.bytes)) throw verificationError("Deployment directory and index.html return different bytes");
  const htmlReferences = parseHtmlReferences(text(index.bytes, "index.html"), baseURL);
  for (const [index_, url] of htmlReferences.entries()) await fetchAsset(url, `index.html asset ${index_ + 1}`);

  const manifestURL = resolveDeploymentAssetURL(FIXED_ASSETS.manifest, baseURL, "Web app manifest URL");
  const serviceWorkerURL = resolveDeploymentAssetURL(FIXED_ASSETS.serviceWorker, baseURL, "Service worker URL");
  const buildURL = resolveDeploymentAssetURL(FIXED_ASSETS.build, baseURL, "Build descriptor URL");
  const engineJavaScriptURL = resolveDeploymentAssetURL(FIXED_ASSETS.engineJavaScript, baseURL, "Engine JavaScript URL");
  const engineWasmURL = resolveDeploymentAssetURL(FIXED_ASSETS.engineWasm, baseURL, "Engine Wasm URL");
  const descriptorURL = resolveDeploymentAssetURL(FIXED_ASSETS.freewareDescriptor, baseURL, "Classic-freeware descriptor URL");

  const manifestRecord = await fetchAsset(manifestURL, "Web app manifest", MIME.manifest);
  const iconURLs = validateWebManifest(parseJson(manifestRecord.bytes, "Web app manifest"), manifestURL, baseURL);
  for (const [index_, url] of iconURLs.entries()) await fetchAsset(url, `Web app icon ${index_ + 1}`, MIME.svg);

  const buildRecord = await fetchAsset(buildURL, "Build descriptor", MIME.json);
  const build = validateBuildDescriptor(parseJson(buildRecord.bytes, "Build descriptor"));
  const serviceWorker = await fetchAsset(serviceWorkerURL, "Service worker", MIME.javascript);
  const serviceWorkerAssets = validateServiceWorker(text(serviceWorker.bytes, "Service worker"), baseURL, build.id, htmlReferences);
  for (const [index_, url] of serviceWorkerAssets.entries()) await fetchAsset(url, `Service worker asset ${index_ + 1}`);

  const engineJavaScript = await fetchAsset(engineJavaScriptURL, "Tiberian Dawn engine JavaScript", MIME.javascript);
  const engineWasm = await fetchAsset(engineWasmURL, "Tiberian Dawn engine Wasm", MIME.wasm, MAXIMUM_TEXT_ASSET_BYTES);
  validateEngine(engineJavaScript.bytes, engineWasm.bytes);

  const descriptorRecord = await fetchAsset(descriptorURL, "Classic-freeware descriptor", MIME.json);
  const descriptor = validateFreewareDescriptor(parseJson(descriptorRecord.bytes, "Classic-freeware descriptor"));
  const archiveURL = resolveDeploymentAssetURL(
    descriptor.package.archive.url,
    baseURL,
    "Classic-freeware archive URL",
    descriptorURL,
  );
  const archive = await fetchAsset(
    archiveURL,
    "Classic-freeware archive",
    MIME.archive,
    descriptor.package.archive.bytes,
    true,
  );
  if (archive.bytes.byteLength !== descriptor.package.archive.bytes || archive.sha256 !== descriptor.package.archive.sha256) {
    throw verificationError("Classic-freeware archive size or SHA-256 does not match its descriptor");
  }
  const packageInspection = await verifyPackageArchive(archive.bytes, descriptor);

  if (distDirectory !== undefined) verifyLocalParity(records, baseURL, distDirectory);
  return {
    format: "cncweb-classic-freeware-deployment-verification",
    version: 1,
    baseURL: baseURL.href,
    buildId: build.id,
    package: {
      id: descriptor.package.id,
      contentSha256: descriptor.package.contentSha256,
      archiveBytes: archive.bytes.byteLength,
      archiveSha256: archive.sha256,
      files: packageInspection.files,
      expandedBytes: packageInspection.expandedBytes,
      sourceFingerprintSha256: packageInspection.fingerprint,
    },
    distParity: distDirectory !== undefined,
    assets: [...records.values()]
      .map((record) => ({
        path: localRelativePath(record.url, baseURL),
        bytes: record.bytes.byteLength,
        sha256: record.sha256,
        mime: record.mime,
      }))
      .sort((left, right) => left.path.localeCompare(right.path)),
  };
}

function usage() {
  console.log(`Usage: node ${basename(process.argv[1])} [URL] [--dist DIRECTORY] [--json] [--allow-loopback-cache-relaxation]\n\nURL may instead be supplied through ${CLASSIC_FREEWARE_BASE_URL_ENV}.`);
}

function parseArguments(argv) {
  const options = {
    baseURL: undefined,
    distDirectory: undefined,
    json: false,
    allowLoopbackCacheRelaxation: false,
  };
  for (let index = 0; index < argv.length; index += 1) {
    const argument = argv[index];
    if (argument === "--dist") {
      index += 1;
      if (index >= argv.length) throw verificationError("--dist requires a directory");
      options.distDirectory = argv[index];
    } else if (argument === "--json") options.json = true;
    else if (argument === "--allow-loopback-cache-relaxation") options.allowLoopbackCacheRelaxation = true;
    else if (argument === "-h" || argument === "--help") { usage(); return null; }
    else if (argument.startsWith("-")) throw verificationError(`Unknown argument: ${argument}`);
    else if (options.baseURL !== undefined) throw verificationError("Only one deployment URL may be supplied");
    else options.baseURL = argument;
  }
  options.baseURL ??= process.env[CLASSIC_FREEWARE_BASE_URL_ENV];
  return options;
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  if (!options) return;
  const result = await verifyClassicFreewareDeployment(options);
  if (options.json) console.log(JSON.stringify(result, null, 2));
  else {
    console.log(`Classic-freeware deployment verified: ${result.baseURL}`);
    console.log(`  build ${result.buildId}`);
    console.log(`  ${result.package.id}: ${result.package.archiveBytes} archive bytes, ${result.package.files} files`);
    console.log(`  ${result.assets.length} HTTP assets${result.distParity ? ", byte-identical to --dist" : ""}`);
  }
}

if (process.argv[1] && pathToFileURL(process.argv[1]).href === import.meta.url) {
  main().catch((error) => {
    console.error(error instanceof Error ? error.message : String(error));
    process.exitCode = 1;
  });
}
