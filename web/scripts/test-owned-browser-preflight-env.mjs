import assert from "node:assert/strict";
import { mkdir, mkdtemp, rm, symlink, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { loadOwnedBrowserPreflightEnvironment, OWNED_PREFLIGHT_ENV as names } from "./owned-browser-preflight-env.mjs";
import OwnedBrowserPreflightReporter from "./owned-browser-preflight-reporter.mjs";

async function fixture() {
  const root = await mkdtemp(join(tmpdir(), "cncweb-owned-preflight-config-"));
  const repo = join(root, "repo");
  const privateRoot = join(root, "private");
  const reportDir = join(privateRoot, "reports");
  const packagePath = join(privateRoot, "owned.cncweb");
  await mkdir(repo);
  await mkdir(reportDir, { recursive: true });
  await writeFile(packagePath, new Uint8Array());
  const environment = {
    [names.enabled]: "1",
    [names.packagePath]: packagePath,
    [names.reportDir]: reportDir,
    [names.baseUrl]: "http://127.0.0.1:48173",
    [names.acceptanceUrl]: `http://127.0.0.1:48173/?acceptance=${"01".repeat(16)}`,
    [names.packageId]: "local-owned-pack",
    [names.packageRevision]: "ab".repeat(32),
    [names.missionId]: "gdi-01-east-a",
    [names.buildId]: "0123456789abcdef",
    [names.acceptanceSession]: "01".repeat(16),
  };
  return { root, repo, privateRoot, reportDir, packagePath, environment };
}

test("is inert without the explicit opt-in", () => {
  const value = loadOwnedBrowserPreflightEnvironment({}, { temporaryRoot: "/tmp" });
  assert.equal(value.enabled, false);
  assert.match(value.reason, /CNCWEB_OWNED_BROWSER_PREFLIGHT=1/);
  assert.match(value.outputDir, /^\/tmp\//);
});

test("rejects ambiguous opt-in values and incomplete private configuration", async () => {
  const value = await fixture();
  try {
    assert.throws(() => loadOwnedBrowserPreflightEnvironment({ [names.enabled]: "yes" }, { repoRoot: value.repo }), /exactly 0 or 1/);
    assert.throws(() => loadOwnedBrowserPreflightEnvironment({ [names.enabled]: "1" }, { repoRoot: value.repo }), new RegExp(names.packagePath));
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("accepts only an external package, report directory, exact URL, and content-safe identities", async () => {
  const value = await fixture();
  try {
    const parsed = loadOwnedBrowserPreflightEnvironment(value.environment, { repoRoot: value.repo });
    assert.equal(parsed.enabled, true);
    assert.equal(parsed.packagePath, value.packagePath);
    assert.equal(parsed.reportDir, value.reportDir);
    assert.equal(parsed.acceptanceURL, value.environment[names.acceptanceUrl]);
    assert.equal(parsed.packageRevision, "ab".repeat(32));
    assert.equal(parsed.missionId, "gdi-01-east-a");
    assert.equal(parsed.buildId, "0123456789abcdef");
    assert.equal(parsed.reportPath, join(value.reportDir, "owned-browser-preflight.json"));
    assert.ok(parsed.outputDir.startsWith(`${value.reportDir}/`));
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("rejects repository paths, including symlinks, without reading package content", async () => {
  const value = await fixture();
  try {
    const repositoryPackage = join(value.repo, "forbidden.cncweb");
    await writeFile(repositoryPackage, new Uint8Array());
    assert.throws(() => loadOwnedBrowserPreflightEnvironment({ ...value.environment, [names.packagePath]: repositoryPackage }, { repoRoot: value.repo }), /outside the repository/);

    const reportLink = join(value.privateRoot, "repo-link");
    await symlink(value.repo, reportLink, "dir");
    assert.throws(() => loadOwnedBrowserPreflightEnvironment({ ...value.environment, [names.reportDir]: reportLink }, { repoRoot: value.repo }), /outside the repository/);
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("rejects a different origin, path, query, or acceptance identity", async () => {
  const value = await fixture();
  try {
    for (const acceptanceURL of [
      `http://localhost:48173/?acceptance=${"01".repeat(16)}`,
      `http://127.0.0.1:48173/other?acceptance=${"01".repeat(16)}`,
      `http://127.0.0.1:48173/?acceptance=${"02".repeat(16)}`,
      `http://127.0.0.1:48173/?acceptance=${"01".repeat(16)}&extra=1`,
    ]) {
      assert.throws(() => loadOwnedBrowserPreflightEnvironment({ ...value.environment, [names.acceptanceUrl]: acceptanceURL }, { repoRoot: value.repo }), /acceptance URL/);
    }
  } finally {
    await rm(value.root, { recursive: true, force: true });
  }
});

test("the private reporter suppresses arbitrary browser error details", () => {
  const original = process.stderr.write;
  let output = "";
  process.stderr.write = ((chunk) => { output += String(chunk); return true; });
  try {
    new OwnedBrowserPreflightReporter().onError(new Error("RETAIL_TEXT_SENTINEL"));
  } finally {
    process.stderr.write = original;
  }
  assert.match(output, /details suppressed/);
  assert.doesNotMatch(output, /RETAIL_TEXT_SENTINEL/);
});
