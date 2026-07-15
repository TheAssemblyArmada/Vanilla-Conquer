import { configDefaults, defineConfig } from "vitest/config";
import react from "@vitejs/plugin-react";
import { createHash } from "node:crypto";
import { existsSync, readFileSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import type { Plugin } from "vite";

const PREVIEW_SECURITY_HEADERS = Object.freeze({
  "Content-Security-Policy": [
    "default-src 'self'",
    "base-uri 'none'",
    "connect-src 'self'",
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
    // zip.js performs package decompression in a generated worker so import
    // stays off the UI thread; script execution remains limited to self.
    "worker-src 'self' blob:",
  ].join("; "),
  "Cross-Origin-Resource-Policy": "same-origin",
  "Permissions-Policy": "camera=(), geolocation=(), microphone=(), payment=(), usb=()",
  "Referrer-Policy": "no-referrer",
  "X-Content-Type-Options": "nosniff",
  "X-Frame-Options": "DENY",
});

function includeBrowserEngine(): Plugin {
  const artifacts = ["tiberiandawn.js", "tiberiandawn.wasm"].map((name) => ({
    name,
    path: new URL(`../build/web-td/${name}`, import.meta.url),
  }));
  const required = process.env.REQUIRE_BROWSER_ENGINE === "1";
  let building = false;

  const availableArtifacts = () => artifacts.filter(({ path }) => existsSync(path));

  return {
    name: "theater-browser-engine",
    configResolved(config) {
      building = config.command === "build";
    },
    configureServer(server) {
      server.middlewares.use((request, response, next) => {
        const pathname = new URL(request.url ?? "/", "http://vite.local").pathname;
        const artifact = artifacts.find(({ name }) => pathname === `/engine/${name}`);
        if (!artifact) { next(); return; }
        const available = availableArtifacts();
        if (available.length !== artifacts.length) {
          response.statusCode = 503;
          response.setHeader("Content-Type", "text/plain; charset=utf-8");
          response.end("Browser engine is not built. Run the web engine CMake target, then refresh this page.\n");
          return;
        }
        response.statusCode = 200;
        response.setHeader("Content-Type", artifact.name.endsWith(".wasm") ? "application/wasm" : "text/javascript; charset=utf-8");
        response.setHeader("Cache-Control", "no-store");
        response.end(readFileSync(artifact.path));
      });
    },
    configurePreviewServer(server) {
      server.middlewares.use((request, response, next) => {
        const pathname = new URL(request.url ?? "/", "http://vite.local").pathname;
        const immutable = /^\/(?:assets\/[A-Za-z0-9_.-]+-[A-Za-z0-9_-]{6,}\.(?:js|css|wasm)|engine\/tiberiandawn\.(?:js|wasm)|classic-freeware-gdi-v1\.cncweb)$/.test(pathname);
        response.setHeader(
          "Cache-Control",
          immutable
            ? "public, max-age=31536000, immutable, no-transform"
            : "no-cache, max-age=0, must-revalidate",
        );
        if (pathname.toLowerCase().endsWith(".cncweb")) {
          response.setHeader("Content-Type", "application/octet-stream");
        }
        // Route the directory URL through the static asset middleware so its
        // headers and bytes are identical to the explicit index document.
        if (pathname === "/") request.url = "/index.html";
        next();
      });
    },
    buildStart() {
      if (!building) return;
      const available = availableArtifacts();
      if (available.length === 0 && !required) return;
      if (available.length !== artifacts.length) throw new Error("Browser engine build is incomplete; expected tiberiandawn.js and tiberiandawn.wasm");
      for (const artifact of artifacts) {
        this.emitFile({ type: "asset", fileName: `engine/${artifact.name}`, source: readFileSync(artifact.path) });
      }
    },
    writeBundle(options, bundle) {
      if (!building) return;
      const outputDirectory = options.dir;
      if (!outputDirectory) throw new Error("Theater's service-worker build requires a directory output");
      const hash = createHash("sha256");
      for (const output of Object.values(bundle).sort((left, right) => left.fileName.localeCompare(right.fileName))) {
        hash.update(output.fileName);
        hash.update(output.type === "chunk" ? output.code : typeof output.source === "string" ? output.source : output.source);
      }
      for (const name of ["sw.js", "manifest.webmanifest", "icon.svg", "legal.html"]) {
        hash.update(name);
        hash.update(readFileSync(new URL(`./public/${name}`, import.meta.url)));
      }
      const buildId = hash.digest("hex").slice(0, 16);
      const runtimeAssets = Object.values(bundle)
        .map((output) => output.fileName)
        .filter((fileName) => /\.(?:js|css|wasm)$/.test(fileName))
        .sort();
      const serviceWorkerPath = join(outputDirectory, "sw.js");
      const source = readFileSync(serviceWorkerPath, "utf8");
      if (!source.includes("__THEATER_BUILD_ID__")) throw new Error("Service-worker build ID placeholder is missing");
      if (!source.includes("/* __THEATER_PRECACHE__ */ []")) throw new Error("Service-worker precache placeholder is missing");
      writeFileSync(
        serviceWorkerPath,
        source
          .replaceAll("__THEATER_BUILD_ID__", buildId)
          .replace("/* __THEATER_PRECACHE__ */ []", JSON.stringify(runtimeAssets)),
      );
      writeFileSync(
        join(outputDirectory, "build-v1.json"),
        `${JSON.stringify({ format: "cncweb-build", version: 1, id: buildId })}\n`,
      );
    },
  };
}

export default defineConfig({
  base: "./",
  plugins: [react(), includeBrowserEngine()],
  preview: {
    // Workspace previews are exposed through per-session subdomains. Keep the
    // allowlist scoped to that forwarding domain instead of accepting any host.
    allowedHosts: [".consoleapp.sh"],
    // Keep local release acceptance under the same restrictive policy expected
    // from staging. Static hosts still need to configure these response headers.
    headers: PREVIEW_SECURITY_HEADERS,
  },
  build: {
    target: "es2022",
    sourcemap: true,
    assetsInlineLimit: 0,
  },
  worker: {
    format: "es",
  },
  test: {
    environment: "jsdom",
    exclude: [...configDefaults.exclude, "e2e/**"],
    restoreMocks: true,
    coverage: {
      reporter: ["text", "html"],
    },
  },
});
