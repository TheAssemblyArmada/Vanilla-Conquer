import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { createServer } from "node:http";
import { tmpdir } from "node:os";
import { dirname, join } from "node:path";
import test from "node:test";

import { TextReader, Uint8ArrayReader, Uint8ArrayWriter, ZipWriter } from "@zip.js/zip.js";

import {
  REQUIRED_CONTENT_SECURITY_POLICY,
  REQUIRED_PERMISSIONS_POLICY,
  calculateContentDigest,
  resolveDeploymentAssetURL,
  verifyClassicFreewareDeployment,
} from "./verify-classic-freeware-deployment.mjs";

const deploymentPath = "/releases/candidate/";
const buildId = "0123456789abcdef";
const appJavaScript = "assets/index-ABCDEF12.js";
const appCss = "assets/index-ABCDEF12.css";
const worker = "assets/simulation.worker-ABCDEF12.js";

function hash(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

function json(value) {
  return Buffer.from(`${JSON.stringify(value)}\n`);
}

async function packageArchive() {
  const path = "engine/td/CONQUER.MIX";
  const payload = Buffer.from("synthetic engine data");
  const files = [{ path, size: payload.byteLength, sha256: hash(payload), role: "engine-data" }];
  const manifest = {
    format: "cncweb-content",
    version: 1,
    package_id: "classic-freeware-gdi-v1",
    created_at_unix_ms: 0,
    source: {
      product: "tiberian-dawn-freeware",
      provider: "ea-freeware",
      install_fingerprint_sha256: "11".repeat(32),
    },
    content: { games: ["tiberian-dawn"], locales: ["en-US"] },
    content_sha256: calculateContentDigest(files),
    files,
  };
  const writer = new ZipWriter(new Uint8ArrayWriter());
  await writer.add("manifest.json", new TextReader(JSON.stringify(manifest)));
  await writer.add(path, new Uint8ArrayReader(payload));
  return { bytes: Buffer.from(await writer.close()), manifest };
}

async function fixture() {
  const archive = await packageArchive();
  const descriptor = {
    format: "cncweb-classic-freeware",
    version: 1,
    package: {
      id: "classic-freeware-gdi-v1",
      contentSha256: archive.manifest.content_sha256,
      source: { product: "tiberian-dawn-freeware", provider: "ea-freeware" },
      archive: {
        url: "./classic-freeware-gdi-v1.cncweb",
        bytes: archive.bytes.byteLength,
        sha256: hash(archive.bytes),
      },
    },
  };
  const webManifest = {
    name: "Theater Runtime",
    short_name: "Theater",
    description: "Synthetic release fixture",
    start_url: "./",
    scope: "./",
    display: "standalone",
    orientation: "landscape",
    background_color: "#000000",
    theme_color: "#111111",
    icons: [{ src: "./icon.svg", sizes: "any", type: "image/svg+xml", purpose: "any maskable" }],
  };
  const serviceWorker = `
const CACHE_PREFIX = "theater-shell-";
const CACHE_VERSION = "${buildId}";
const SHELL_CACHE = \`${"${CACHE_PREFIX}"}${"${CACHE_VERSION}"}\`;
const STATIC_SHELL = ["./manifest.webmanifest", "./icon.svg", "./legal.html", "./build-v1.json"];
const OPTIONAL_ENGINE = ["./engine/tiberiandawn.js", "./engine/tiberiandawn.wasm"];
const BUILD_ASSETS = ["${appJavaScript}", "${appCss}", "${worker}", "engine/tiberiandawn.js", "engine/tiberiandawn.wasm"];
function isVersionedRuntimeAsset(url) { return url.pathname.includes("/assets/"); }
function isSourceDerivedEngineAsset(url) { return url.pathname.includes("/engine/"); }
function isStaticShellAsset(url) { return STATIC_SHELL.includes(url.pathname); }
self.addEventListener("fetch", (event) => {
  const url = new URL(event.request.url);
  if (!isVersionedRuntimeAsset(url) && !isSourceDerivedEngineAsset(url) && !isStaticShellAsset(url)) return;
  event.respondWith(caches.open(SHELL_CACHE).then(async (cache) => {
    const response = await fetch(event.request);
    if (response.ok) event.waitUntil(cache.put(event.request, response.clone()));
    return response;
  }));
});
`;
  const files = new Map([
    ["index.html", Buffer.from(`<!doctype html><html><head>
      <link rel="manifest" href="./manifest.webmanifest">
      <link rel="icon" href="./icon.svg">
      <script type="module" src="./${appJavaScript}"></script>
      <link rel="stylesheet" href="./${appCss}">
    </head><body><div id="root"></div></body></html>`)],
    ["manifest.webmanifest", json(webManifest)],
    ["icon.svg", Buffer.from("<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n")],
    ["legal.html", Buffer.from("<!doctype html><title>Legal</title>\n")],
    ["sw.js", Buffer.from(serviceWorker)],
    ["build-v1.json", json({ format: "cncweb-build", version: 1, id: buildId })],
    [appJavaScript, Buffer.from("console.log('fixture');\n")],
    [appCss, Buffer.from("body { color: white; }\n")],
    [worker, Buffer.from("self.onmessage = () => {};\n")],
    ["engine/tiberiandawn.js", Buffer.from("const engine = WebAssembly; const wasm = 'tiberiandawn.wasm'; export default engine;\n")],
    ["engine/tiberiandawn.wasm", Buffer.from([0, 97, 115, 109, 1, 0, 0, 0])],
    ["classic-freeware-v1.json", json(descriptor)],
    ["classic-freeware-gdi-v1.cncweb", archive.bytes],
  ]);
  const mimes = new Map([
    ["index.html", "text/html; charset=utf-8"],
    ["manifest.webmanifest", "application/manifest+json"],
    ["icon.svg", "image/svg+xml"],
    ["legal.html", "text/html; charset=utf-8"],
    ["sw.js", "text/javascript; charset=utf-8"],
    ["build-v1.json", "application/json"],
    [appJavaScript, "text/javascript"],
    [appCss, "text/css"],
    [worker, "text/javascript"],
    ["engine/tiberiandawn.js", "text/javascript"],
    ["engine/tiberiandawn.wasm", "application/wasm"],
    ["classic-freeware-v1.json", "application/json"],
    ["classic-freeware-gdi-v1.cncweb", "application/zip"],
  ]);
  return {
    files,
    mimes,
    statuses: new Map(),
    redirects: new Map(),
    headerOverrides: new Map(),
    descriptor,
    webManifest,
  };
}

async function withServer(value, callback) {
  const server = createServer((request, response) => {
    const url = new URL(request.url ?? "/", "http://fixture.invalid");
    if (!url.pathname.startsWith(deploymentPath)) {
      response.writeHead(404).end();
      return;
    }
    const requested = url.pathname.slice(deploymentPath.length);
    const path = requested === "" ? "index.html" : requested;
    const redirect = value.redirects.get(path);
    if (redirect) {
      response.writeHead(302, { location: redirect }).end();
      return;
    }
    const body = value.files.get(path);
    const status = value.statuses.get(path) ?? (body ? 200 : 404);
    if (!body || status !== 200) {
      response.writeHead(status, { "content-type": "text/plain" }).end("missing");
      return;
    }
    const immutable = /^(?:assets\/|engine\/|classic-freeware-gdi-v1\.cncweb$)/.test(path);
    const pinnedBinary = /^(?:engine\/|classic-freeware-gdi-v1\.cncweb$)/.test(path);
    const headers = {
      "content-type": value.mimes.get(path) ?? "application/octet-stream",
      "content-length": body.byteLength,
      "cache-control": immutable
        ? `public, max-age=31536000, immutable${pinnedBinary ? ", no-transform" : ""}`
        : "no-cache, max-age=0, must-revalidate",
      "content-security-policy": REQUIRED_CONTENT_SECURITY_POLICY,
      "x-content-type-options": "nosniff",
      "referrer-policy": "no-referrer",
      "x-frame-options": "DENY",
      "cross-origin-resource-policy": "same-origin",
      "permissions-policy": REQUIRED_PERMISSIONS_POLICY,
      ...(value.headerOverrides.get(path) ?? {}),
    };
    for (const [name, header] of Object.entries(headers)) {
      if (header === null) delete headers[name];
    }
    response.writeHead(200, headers);
    response.end(body);
  });
  await new Promise((resolve_) => server.listen(0, "127.0.0.1", resolve_));
  const address = server.address();
  if (!address || typeof address === "string") throw new Error("Fixture server did not bind a TCP port");
  const baseURL = `http://127.0.0.1:${address.port}${deploymentPath}`;
  try { return await callback(baseURL); } finally { await new Promise((resolve_) => server.close(resolve_)); }
}

function writeDist(value) {
  const root = mkdtempSync(join(tmpdir(), "cncweb-deployment-"));
  for (const [path, body] of value.files) {
    const target = join(root, path);
    mkdirSync(dirname(target), { recursive: true });
    writeFileSync(target, body);
  }
  return root;
}

test("verifies a header-correct subpath deployment and optional dist parity", async () => {
  const value = await fixture();
  const dist = writeDist(value);
  try {
    await withServer(value, async (baseURL) => {
      const result = await verifyClassicFreewareDeployment({ baseURL: baseURL.slice(0, -1), distDirectory: dist });
      assert.equal(result.baseURL, baseURL);
      assert.equal(result.buildId, buildId);
      assert.equal(result.package.id, "classic-freeware-gdi-v1");
      assert.equal(result.package.files, 1);
      assert.equal(result.distParity, true);
      assert.ok(result.assets.some(({ path, mime }) => path === "classic-freeware-gdi-v1.cncweb" && mime === "application/zip"));
    });
  } finally {
    rmSync(dist, { recursive: true, force: true });
  }
});

test("rejects unsafe base and asset URLs, including deployment path escapes", async () => {
  await assert.rejects(
    verifyClassicFreewareDeployment({ baseURL: "http://staging.example.test/release/" }),
    /HTTPS for non-loopback/,
  );
  for (const input of [
    "https://user@example.test/release/",
    "https://example.test/release/?token=x",
    "https://example.test/release/#candidate",
  ]) {
    await assert.rejects(verifyClassicFreewareDeployment({ baseURL: input }));
  }
  const base = new URL("https://example.test/releases/candidate/");
  assert.throws(() => resolveDeploymentAssetURL("../outside.js", base), /escapes the deployment directory/);
  assert.throws(() => resolveDeploymentAssetURL("https://cdn.example.test/app.js", base), /deployment origin/);
});

test("rejects redirects and cross-origin HTML assets", async () => {
  const redirected = await fixture();
  redirected.redirects.set("index.html", `${deploymentPath}other.html`);
  await withServer(redirected, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /status 302 or redirected/,
  ));

  const crossOrigin = await fixture();
  crossOrigin.files.set("index.html", Buffer.from(`<!doctype html><link rel="manifest" href="./manifest.webmanifest"><link rel="icon" href="./icon.svg"><link rel="stylesheet" href="./${appCss}"><script src="https://cdn.example.test/app.js"></script>`));
  await withServer(crossOrigin, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /deployment origin/,
  ));
});

test("rejects bad status, MIME, archive length, and archive hash", async (context) => {
  await context.test("status", async () => {
    const value = await fixture();
    value.statuses.set("engine/tiberiandawn.wasm", 503);
    await withServer(value, (baseURL) => assert.rejects(verifyClassicFreewareDeployment({ baseURL }), /status 503/));
  });
  await context.test("MIME", async () => {
    const value = await fixture();
    value.mimes.set("classic-freeware-gdi-v1.cncweb", "text/plain");
    await withServer(value, (baseURL) => assert.rejects(verifyClassicFreewareDeployment({ baseURL }), /unexpected MIME type/));
  });
  await context.test("length", async () => {
    const value = await fixture();
    value.descriptor.package.archive.bytes += 1;
    value.files.set("classic-freeware-v1.json", json(value.descriptor));
    await withServer(value, (baseURL) => assert.rejects(verifyClassicFreewareDeployment({ baseURL }), /Content-Length does not match|size or SHA-256/));
  });
  await context.test("hash", async () => {
    const value = await fixture();
    value.descriptor.package.archive.sha256 = "ff".repeat(32);
    value.files.set("classic-freeware-v1.json", json(value.descriptor));
    await withServer(value, (baseURL) => assert.rejects(verifyClassicFreewareDeployment({ baseURL }), /size or SHA-256/));
  });
  await context.test("declared oversize before body allocation", async () => {
    const value = await fixture();
    value.headerOverrides.set("classic-freeware-v1.json", { "content-length": String(17 * 1024 * 1024) });
    await withServer(value, (baseURL) => assert.rejects(
      verifyClassicFreewareDeployment({ baseURL }),
      /declared Content-Length exceeds its byte limit/,
    ));
  });
});

test("rejects build drift and any service-worker reference to freeware payloads", async () => {
  const drift = await fixture();
  drift.files.set("build-v1.json", json({ format: "cncweb-build", version: 1, id: "fedcba9876543210" }));
  await withServer(drift, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /cache version does not match/,
  ));

  const cachedPayload = await fixture();
  cachedPayload.files.set("sw.js", Buffer.concat([
    cachedPayload.files.get("sw.js"),
    Buffer.from("\nconst forbiddenPayload = './classic-freeware-v1.json';\n"),
  ]));
  await withServer(cachedPayload, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /must not name or cache classic-freeware payloads/,
  ));
});

test("rejects missing or broadened response security headers", async () => {
  const missing = await fixture();
  missing.headerOverrides.set("index.html", { "x-content-type-options": null });
  await withServer(missing, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /x-content-type-options header must be nosniff/,
  ));

  const broadened = await fixture();
  broadened.headerOverrides.set("index.html", {
    "content-security-policy": REQUIRED_CONTENT_SECURITY_POLICY.replace(
      "script-src 'self' 'wasm-unsafe-eval'",
      "script-src 'self' 'wasm-unsafe-eval' 'unsafe-eval'",
    ),
  });
  await withServer(broadened, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /Content-Security-Policy/,
  ));

  const workerPolicy = await fixture();
  workerPolicy.headerOverrides.set(worker, { "content-security-policy": null });
  await withServer(workerPolicy, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /Content-Security-Policy/,
  ));
});

test("enforces cache classes with an explicit loopback-only relaxation", async () => {
  const value = await fixture();
  value.headerOverrides.set(appJavaScript, { "cache-control": "no-cache" });
  await withServer(value, async (baseURL) => {
    await assert.rejects(
      verifyClassicFreewareDeployment({ baseURL }),
      /one-year immutable Cache-Control policy/,
    );
    const relaxed = await verifyClassicFreewareDeployment({ baseURL, allowLoopbackCacheRelaxation: true });
    assert.equal(relaxed.package.id, "classic-freeware-gdi-v1");
  });

  const transformedEngine = await fixture();
  transformedEngine.headerOverrides.set("engine/tiberiandawn.wasm", {
    "cache-control": "public, max-age=31536000, immutable",
  });
  await withServer(transformedEngine, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /must include no-transform/,
  ));

  const staleDescriptor = await fixture();
  staleDescriptor.headerOverrides.set("classic-freeware-v1.json", { "cache-control": "no-cache" });
  await withServer(staleDescriptor, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL }),
    /revalidating Cache-Control policy/,
  ));

  await assert.rejects(
    verifyClassicFreewareDeployment({
      baseURL: "https://staging.example.test/release/",
      allowLoopbackCacheRelaxation: true,
    }),
    /permitted only for an explicit loopback/,
  );

  const insecure = await fixture();
  insecure.headerOverrides.set("index.html", { "permissions-policy": null });
  await withServer(insecure, (baseURL) => assert.rejects(
    verifyClassicFreewareDeployment({ baseURL, allowLoopbackCacheRelaxation: true }),
    /Permissions-Policy/,
  ));
});

test("rejects strict JSON duplicates, escaped PWA scope, and --dist byte drift", async (context) => {
  await context.test("duplicate descriptor key", async () => {
    const value = await fixture();
    const original = value.files.get("classic-freeware-v1.json").toString("utf8").trim();
    value.files.set("classic-freeware-v1.json", Buffer.from(original.replace('"version":1', '"version":1,"version":1')));
    await withServer(value, (baseURL) => assert.rejects(verifyClassicFreewareDeployment({ baseURL }), /duplicate key/));
  });
  await context.test("escaped scope", async () => {
    const value = await fixture();
    value.webManifest.scope = "../";
    value.files.set("manifest.webmanifest", json(value.webManifest));
    await withServer(value, (baseURL) => assert.rejects(verifyClassicFreewareDeployment({ baseURL }), /escapes the deployment directory/));
  });
  await context.test("dist drift", async () => {
    const value = await fixture();
    const dist = writeDist(value);
    writeFileSync(join(dist, appCss), "body { color: red; }\n");
    try {
      await withServer(value, (baseURL) => assert.rejects(
        verifyClassicFreewareDeployment({ baseURL, distDirectory: dist }),
        /Remote bytes differ from --dist/,
      ));
    } finally {
      rmSync(dist, { recursive: true, force: true });
    }
  });
});
