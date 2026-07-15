const CACHE_PREFIX = "theater-shell-";
const CACHE_VERSION = "__THEATER_BUILD_ID__";
const SHELL_CACHE = `${CACHE_PREFIX}${CACHE_VERSION}`;
const STATIC_SHELL = ["./manifest.webmanifest", "./icon.svg", "./legal.html", "./build-v1.json"];
const OPTIONAL_ENGINE = ["./engine/tiberiandawn.js", "./engine/tiberiandawn.wasm"];
const BUILD_ASSETS = /* __THEATER_PRECACHE__ */ [];

async function discoverApplicationShell() {
  const indexUrl = new URL("./index.html", self.registration.scope).href;
  const response = await fetch(indexUrl, { cache: "no-store" });
  if (!response.ok) throw new Error(`Application shell returned ${response.status}`);
  const html = await response.clone().text();
  const urls = new Set();
  for (const match of html.matchAll(/(?:src|href)=["']([^"']+)["']/g)) {
    const url = new URL(match[1], indexUrl);
    if (url.origin === self.location.origin) urls.add(url.href);
  }
  const cache = await caches.open(SHELL_CACHE);
  await cache.put(indexUrl, response);
  for (const staticUrl of STATIC_SHELL) urls.add(new URL(staticUrl, self.registration.scope).href);
  for (const assetUrl of BUILD_ASSETS) urls.add(new URL(assetUrl, self.registration.scope).href);
  const fetched = new Set();
  while (urls.size) {
    const assetUrl = urls.values().next().value;
    urls.delete(assetUrl);
    if (fetched.has(assetUrl)) continue;
    fetched.add(assetUrl);
    const assetResponse = await fetch(assetUrl, { cache: "no-store" });
    if (!assetResponse.ok) throw new Error(`Application asset returned ${assetResponse.status}`);
    await cache.put(assetUrl, assetResponse.clone());
    if (new URL(assetUrl).pathname.endsWith(".js")) {
      const source = await assetResponse.text();
      for (const match of source.matchAll(/["'`]([^"'`]+\.(?:js|css|wasm))["'`]/g)) {
        const nested = new URL(match[1], assetUrl);
        if (nested.origin === self.location.origin && isVersionedRuntimeAsset(nested)) urls.add(nested.href);
      }
    }
  }
  for (const engineUrl of OPTIONAL_ENGINE) {
    const url = new URL(engineUrl, self.registration.scope).href;
    try {
      const response = await fetch(url, { cache: "no-store" });
      const expectedType = engineUrl.endsWith(".wasm") ? "application/wasm" : "javascript";
      if (response.ok && (response.headers.get("content-type") || "").includes(expectedType)) await cache.put(url, response);
    } catch {
      // Demo-only builds intentionally omit the optional production engine.
    }
  }
}

self.addEventListener("install", (event) => {
  event.waitUntil(
    discoverApplicationShell().catch(async (error) => {
      // A failed install must not leave a partial build cache that an older
      // controller can discover while serving another open tab.
      await caches.delete(SHELL_CACHE);
      throw error;
    }),
  );
});

self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches
      .keys()
      .then((keys) => {
        // Keep the immediately previous immutable bundle so another open tab
        // can finish lazy-loading its hashed modules after this update wins.
        const previous = keys.filter((key) => key.startsWith(CACHE_PREFIX) && key !== SHELL_CACHE);
        const obsolete = previous.slice(0, -1);
        return Promise.all(obsolete.map((key) => caches.delete(key)));
      })
      .then(() => self.clients.claim()),
  );
});

self.addEventListener("message", (event) => {
  if (event.data?.type === "SKIP_WAITING") self.skipWaiting();
  if (event.data?.type === "GET_BUILD_ID") event.ports[0]?.postMessage({ type: "BUILD_ID", buildId: CACHE_VERSION });
});

function isVersionedRuntimeAsset(url) {
  const scope = new URL(self.registration.scope);
  const relative = url.pathname.slice(scope.pathname.length);
  return /^assets\/[a-z0-9_.-]+-[a-zA-Z0-9_-]{6,}\.(?:js|css|wasm)$/.test(relative);
}

function isSourceDerivedEngineAsset(url) {
  const scope = new URL(self.registration.scope);
  const relative = url.pathname.slice(scope.pathname.length);
  return /^engine\/tiberiandawn\.(?:js|wasm)$/.test(relative);
}

function isStaticShellAsset(url) {
  return STATIC_SHELL.some((relative) => new URL(relative, self.registration.scope).href === url.href);
}

async function matchVersionedAssetAcrossBuilds(request) {
  const keys = await caches.keys();
  const buildCaches = [SHELL_CACHE, ...keys.filter((key) => key.startsWith(CACHE_PREFIX) && key !== SHELL_CACHE).reverse()];
  for (const key of buildCaches) {
    const cached = await (await caches.open(key)).match(request, { ignoreVary: true });
    if (cached) return cached;
  }
  return undefined;
}

self.addEventListener("fetch", (event) => {
  const request = event.request;
  if (request.method !== "GET") return;
  const url = new URL(request.url);
  if (url.origin !== self.location.origin || !url.href.startsWith(self.registration.scope)) return;

  if (request.mode === "navigate") {
    event.respondWith(
      caches.open(SHELL_CACHE).then(async (cache) => {
        const cached = (await cache.match(request)) || (await cache.match(new URL("./index.html", self.registration.scope).href));
        if (cached) return cached;
        const response = await fetch(request);
        if (response.ok) event.waitUntil(cache.put(request, response.clone()));
        return response;
      }).catch(() => Response.error()),
    );
    return;
  }

  if (!isVersionedRuntimeAsset(url) && !isSourceDerivedEngineAsset(url) && !isStaticShellAsset(url)) return;
  event.respondWith(
    caches.open(SHELL_CACHE).then(async (cache) => {
      const cached = isVersionedRuntimeAsset(url)
        ? await matchVersionedAssetAcrossBuilds(request)
        : await cache.match(request, { ignoreVary: true });
      if (cached) return cached;
      const response = await fetch(request);
      if (response.ok) event.waitUntil(cache.put(request, response.clone()));
      return response;
    }),
  );
});
