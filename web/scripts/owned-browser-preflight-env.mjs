import { existsSync, realpathSync, statSync } from "node:fs";
import { basename, dirname, isAbsolute, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

export const OWNED_PREFLIGHT_ENV = Object.freeze({
  enabled: "CNCWEB_OWNED_BROWSER_PREFLIGHT",
  packagePath: "CNCWEB_OWNED_PREFLIGHT_PACKAGE_PATH",
  reportDir: "CNCWEB_OWNED_PREFLIGHT_REPORT_DIR",
  baseUrl: "CNCWEB_OWNED_PREFLIGHT_BASE_URL",
  acceptanceUrl: "CNCWEB_OWNED_PREFLIGHT_ACCEPTANCE_URL",
  packageId: "CNCWEB_OWNED_PREFLIGHT_PACKAGE_ID",
  packageRevision: "CNCWEB_OWNED_PREFLIGHT_PACKAGE_REVISION",
  missionId: "CNCWEB_OWNED_PREFLIGHT_MISSION_ID",
  buildId: "CNCWEB_OWNED_PREFLIGHT_BUILD_ID",
  acceptanceSession: "CNCWEB_OWNED_PREFLIGHT_ACCEPTANCE_SESSION",
});

function required(environment, name) {
  const value = environment[name];
  if (!value) throw new Error(`Owned-browser preflight requires ${name}`);
  return value;
}

function canonicalFuturePath(path) {
  let cursor = resolve(path);
  const missing = [];
  while (!existsSync(cursor)) {
    const parent = dirname(cursor);
    if (parent === cursor) return resolve(path);
    missing.unshift(basename(cursor));
    cursor = parent;
  }
  return resolve(realpathSync(cursor), ...missing);
}

function isInside(candidate, root) {
  const child = relative(root, candidate);
  return child === "" || (!child.startsWith(`..${process.platform === "win32" ? "\\" : "/"}`) && child !== ".." && !isAbsolute(child));
}

function localUrl(value, label) {
  let url;
  try { url = new URL(value); }
  catch { throw new Error(`${label} must be a valid URL`); }
  if (url.protocol !== "http:" || url.hostname !== "127.0.0.1" || url.username || url.password || url.hash) {
    throw new Error(`${label} must be an uncredentialed http://127.0.0.1 URL`);
  }
  return url;
}

export function loadOwnedBrowserPreflightEnvironment(environment = process.env, options = {}) {
  const enabled = environment[OWNED_PREFLIGHT_ENV.enabled];
  if (!enabled || enabled === "0") {
    return Object.freeze({
      enabled: false,
      reason: `Set ${OWNED_PREFLIGHT_ENV.enabled}=1 with the private harness environment to run this preflight`,
      baseURL: "http://127.0.0.1:9/",
      outputDir: join(options.temporaryRoot ?? "/tmp", "cncweb-owned-browser-preflight-disabled"),
    });
  }
  if (enabled !== "1") throw new Error(`${OWNED_PREFLIGHT_ENV.enabled} must be exactly 0 or 1`);

  const defaultRepoRoot = fileURLToPath(new URL("../..", import.meta.url));
  const repoRoot = realpathSync(options.repoRoot ?? defaultRepoRoot);
  const packageInput = required(environment, OWNED_PREFLIGHT_ENV.packagePath);
  const reportInput = required(environment, OWNED_PREFLIGHT_ENV.reportDir);
  if (!isAbsolute(packageInput) || !isAbsolute(reportInput)) throw new Error("Owned-browser package and report paths must be absolute");
  if (!existsSync(packageInput) || !statSync(packageInput).isFile() || !packageInput.endsWith(".cncweb")) {
    throw new Error("Owned-browser package path must name an existing .cncweb file");
  }
  if (!existsSync(reportInput) || !statSync(reportInput).isDirectory()) throw new Error("Owned-browser report directory must already exist");
  const packagePath = realpathSync(packageInput);
  const reportDir = canonicalFuturePath(reportInput);
  if (isInside(packagePath, repoRoot) || isInside(reportDir, repoRoot)) throw new Error("Owned-browser package and report paths must remain outside the repository");

  const base = localUrl(required(environment, OWNED_PREFLIGHT_ENV.baseUrl), "Owned-browser base URL");
  if (base.pathname !== "/" || base.search) throw new Error("Owned-browser base URL must identify the preview origin root");
  const acceptance = localUrl(required(environment, OWNED_PREFLIGHT_ENV.acceptanceUrl), "Owned-browser acceptance URL");
  const acceptanceSession = required(environment, OWNED_PREFLIGHT_ENV.acceptanceSession);
  if (!/^[a-f0-9]{32,64}$/.test(acceptanceSession)) throw new Error("Owned-browser acceptance session is invalid");
  if (acceptance.origin !== base.origin || acceptance.pathname !== "/" || acceptance.searchParams.size !== 1 || acceptance.searchParams.get("acceptance") !== acceptanceSession) {
    throw new Error("Owned-browser acceptance URL must be the exact preview URL for its acceptance session");
  }

  const packageId = required(environment, OWNED_PREFLIGHT_ENV.packageId);
  const packageRevision = required(environment, OWNED_PREFLIGHT_ENV.packageRevision);
  const missionId = required(environment, OWNED_PREFLIGHT_ENV.missionId);
  const buildId = required(environment, OWNED_PREFLIGHT_ENV.buildId);
  if (!/^[a-z0-9][a-z0-9._-]{0,127}$/i.test(packageId)) throw new Error("Owned-browser package ID is invalid");
  if (!/^[a-f0-9]{64}$/.test(packageRevision)) throw new Error("Owned-browser package revision is invalid");
  if (!/^[a-z0-9][a-z0-9._-]{0,127}$/.test(missionId)) throw new Error("Owned-browser mission ID is invalid");
  if (!/^[a-f0-9]{16}$/.test(buildId)) throw new Error("Owned-browser build ID is invalid");

  return Object.freeze({
    enabled: true,
    packagePath,
    reportDir,
    reportPath: join(reportDir, "owned-browser-preflight.json"),
    outputDir: join(reportDir, "playwright-output"),
    baseURL: base.href,
    acceptanceURL: acceptance.href,
    packageId,
    packageRevision,
    missionId,
    buildId,
    acceptanceSession,
  });
}
