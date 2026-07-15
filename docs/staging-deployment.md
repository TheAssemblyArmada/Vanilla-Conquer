# Static-host staging contract

This contract applies to a classic-freeware release candidate served by a
generic static host. It does not select a provider or publish anything. An
external deployment still requires an operator-controlled hosting target,
credentials, DNS/TLS, and a final deployment URL; none is configured or
published from this checkout.

## Deployment directory and artifacts

Build the integrated release and sidecar into one clean directory as described
in [classic-freeware deployment](classic-freeware.md). The deployment URL is
the HTTPS directory containing `index.html`, not merely an origin. For example:

```text
https://staging.example.test/releases/<release-id>/
```

The trailing slash is significant for relative URL resolution. The automated
gate normalizes a missing slash, but the hosted canonical URL should include
it. It must not contain credentials, a query, or a fragment. A release may be
hosted at an origin subpath; origin-root hosting is not required.

Keep every runtime artifact under that directory and same-origin. In
particular, `classic-freeware-v1.json` and
`classic-freeware-gdi-v1.cncweb` sit beside `index.html`, while
`tiberiandawn.js` and `tiberiandawn.wasm` remain under `engine/`. Do not rewrite
relative asset URLs to another origin. Descriptor and archive requests must
return directly without redirects.

## Atomic publication and rollback

Never update a live release directory file by file. Upload the complete output
to a new versioned directory that is not yet routed to users, verify its file
inventory and digests, and only then expose or promote that directory with the
host's atomic routing/alias operation. Object stores without rename support
should upload behind an unpublished release key before switching the route.

Retain the preceding immutable directory for rollback and for already-open
tabs. Roll back by switching routing to that complete directory; do not repair
or overwrite a partially published release in place. The exact candidate URL
used for acceptance must continue to identify the same bytes after it passes.

## HTTP response contract

Use HTTPS for every non-loopback deployment. HTTP is accepted only for local
`localhost`, `127.0.0.0/8`, or `[::1]` verification. Serve the directory URL
itself as `index.html` and configure these MIME types explicitly while
`X-Content-Type-Options: nosniff` is active:

| Files | Required `Content-Type` |
| --- | --- |
| `*.html` | `text/html` |
| `*.js` | `text/javascript` or `application/javascript` |
| `*.css` | `text/css` |
| `*.wasm` | `application/wasm` |
| `*.json` | `application/json` |
| `*.webmanifest` | `application/manifest+json` or `application/json` |
| `*.cncweb` | `application/zip` or `application/octet-stream` |
| `*.svg` | `image/svg+xml` |

Precompressed responses may use Brotli or gzip only with the correct
`Content-Encoding`; their decoded bytes and MIME type must still match the
artifact. The `.cncweb` archive is the exception: serve it without
`Content-Encoding`, with a valid `Content-Length` exactly equal to the
descriptor's declared bytes, and do not transform it.

Apply the following policy to HTML and other responses in the deployment
directory:

```text
Content-Security-Policy: default-src 'self'; base-uri 'none'; connect-src 'self'; font-src 'self' data:; form-action 'none'; frame-ancestors 'none'; frame-src 'none'; img-src 'self' blob: data:; manifest-src 'self'; media-src 'self' blob:; object-src 'none'; script-src 'self' 'wasm-unsafe-eval'; script-src-attr 'none'; style-src 'self' 'unsafe-inline'; worker-src 'self' blob:
Cross-Origin-Resource-Policy: same-origin
Permissions-Policy: camera=(), geolocation=(), microphone=(), payment=(), usb=()
Referrer-Policy: no-referrer
X-Content-Type-Options: nosniff
X-Frame-Options: DENY
```

`'wasm-unsafe-eval'` is required for WebAssembly compilation. The `blob:`
worker source keeps ZIP decompression off the UI thread. Do not replace the
Wasm source with the broader `'unsafe-eval'`. Cross-origin isolation is not
required. On a dedicated HTTPS hostname, also enable HSTS according to the
hosting organization's domain policy; use `includeSubDomains` only when every
covered subdomain is HTTPS.

## Cache rules

Atomic, versioned directories prevent mixed releases. Use these response-cache
rules in addition to the PWA's own shell cache:

- Serve every fixed or otherwise unclassified response—including the directory
  response, `index.html`, `sw.js`, `build-v1.json`,
  `classic-freeware-v1.json`, `manifest.webmanifest`, `icon.svg`, and
  `legal.html`—with `Cache-Control: no-cache, max-age=0, must-revalidate`.
- Serve content-hashed files under `assets/` with `Cache-Control: public,
  max-age=31536000, immutable`.
- Serve the engine pair and digest-pinned `.cncweb` archive with
  `Cache-Control: public, max-age=31536000, immutable, no-transform`. This is
  safe only because the release directory is immutable; never reuse that URL
  for different bytes.
- Do not add the `.cncweb` archive to the service-worker precache. The app
  verifies it and stores the expanded package in origin-private storage.

If a stable alias points at release directories, keep the alias response
revalidating and switch it atomically. Acceptance and rollback records should
name the immutable deployment-directory URL, not just the alias.

## Verify a candidate

First run the local release acceptance documented in
[classic-freeware deployment](classic-freeware.md). After uploading, give the
host-independent verifier the exact deployment-directory URL:

```sh
cd web
export CNCWEB_CLASSIC_FREEWARE_BASE_URL="https://<host>/<release-directory>/"
corepack pnpm verify:classic-freeware:deployment \
  "$CNCWEB_CLASSIC_FREEWARE_BASE_URL" --dist dist --json
```

The verifier fails on redirects, missing or out-of-directory files,
byte/digest drift from `dist`, incorrect MIME, unsafe headers or CSP, mixed
cache classes, service-worker content caching, malformed descriptors, and any
outer or inner package-integrity mismatch. The bundled Vite preview implements
the full contract. For a different loopback-only server that cannot express
the production cache classes, `--allow-loopback-cache-relaxation` skips only
that cache check; it is rejected for non-loopback URLs and never relaxes
security or integrity checks.

Run the combined staging gate before promotion:

```sh
corepack pnpm test:classic-freeware:staging
```

It reruns the strict deployment verifier with local `dist` parity and starts
the remote desktop-browser matrix only if verification succeeds. The matrix
normalizes the URL's trailing slash, disables Playwright's local preview
server, and navigates relative to the supplied directory so subpath staging is
exercised.

Chromium and Firefox should boot the real Wasm mission, save, and reload the
same package revision. The Playwright Linux WebKit project records its explicit
demo fallback when that port lacks OPFS; it is not a substitute for the open
physical Safari device gate. Preserve the verifier output, Playwright report,
exact deployment URL, release/build IDs, and deployment inventory as the
staging acceptance record before promotion.
