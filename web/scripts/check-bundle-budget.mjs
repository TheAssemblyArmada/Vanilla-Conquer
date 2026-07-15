#!/usr/bin/env node

import { gzipSync } from "node:zlib";
import { existsSync, readFileSync, readdirSync } from "node:fs";
import { basename, join, relative, resolve, sep } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const artifactDefinitions = [
  {
    id: "appJavaScript",
    label: "app JavaScript",
    pattern: /^assets\/(?!simulation\.worker-)[A-Za-z0-9_.-]+\.js$/,
    engine: false,
    singleton: false,
  },
  {
    id: "appCss",
    label: "app CSS",
    pattern: /^assets\/[A-Za-z0-9_.-]+\.css$/,
    engine: false,
    singleton: false,
  },
  {
    id: "simulationWorker",
    label: "simulation worker",
    pattern: /^assets\/simulation\.worker-[A-Za-z0-9_-]+\.js$/,
    engine: false,
    singleton: true,
  },
  {
    id: "engineJavaScript",
    label: "engine JavaScript",
    pattern: /^engine\/tiberiandawn\.js$/,
    engine: true,
    singleton: true,
  },
  {
    id: "engineWasm",
    label: "engine Wasm",
    pattern: /^engine\/tiberiandawn\.wasm$/,
    engine: true,
    singleton: true,
  },
];

function fail(message) {
  throw new Error(`Bundle budget configuration error: ${message}`);
}

function validateLimit(value, label) {
  if (!Number.isSafeInteger(value) || value <= 0) fail(`${label} must be a positive integer`);
}

export function readPerformanceBudgets(
  file = fileURLToPath(new URL("../performance-budgets.json", import.meta.url)),
) {
  const value = JSON.parse(readFileSync(file, "utf8"));
  if (
    value?.format !== "cncweb-performance-budgets"
    || value?.version !== 1
    || !value.bundle
    || !value.assetFreeDemo
  ) {
    fail("unsupported performance budget document");
  }
  for (const definition of artifactDefinitions) {
    const limit = value.bundle[definition.id];
    if (!limit || Object.keys(limit).sort().join(",") !== "gzipBytes,rawBytes") {
      fail(`missing or malformed ${definition.id} limits`);
    }
    validateLimit(limit.rawBytes, `${definition.id}.rawBytes`);
    validateLimit(limit.gzipBytes, `${definition.id}.gzipBytes`);
  }
  return value;
}

function inventoryFiles(root, directory = root) {
  const files = [];
  for (const entry of readdirSync(directory, { withFileTypes: true })) {
    const path = join(directory, entry.name);
    if (entry.isDirectory()) files.push(...inventoryFiles(root, path));
    else if (entry.isFile()) files.push(relative(root, path).split(sep).join("/"));
  }
  return files.sort();
}

function formatBytes(value) {
  return `${(value / 1024).toFixed(1)} KiB`;
}

export function inspectBundle({ distDirectory, budgets, requireEngine = false }) {
  const root = resolve(distDirectory);
  if (!existsSync(root)) throw new Error(`Production bundle directory does not exist: ${root}`);
  const files = inventoryFiles(root);
  const engineMatches = artifactDefinitions
    .filter((definition) => definition.engine)
    .map((definition) => files.filter((file) => definition.pattern.test(file)).length);
  const hasAnyEngine = engineMatches.some((count) => count > 0);
  if (hasAnyEngine && engineMatches.some((count) => count === 0)) {
    throw new Error("Production bundle contains an incomplete engine pair");
  }

  const artifacts = [];
  for (const definition of artifactDefinitions) {
    const matches = files.filter((file) => definition.pattern.test(file));
    const required = !definition.engine || requireEngine || hasAnyEngine;
    if (!required && matches.length === 0) {
      artifacts.push({
        id: definition.id,
        label: definition.label,
        status: "omitted",
        paths: [],
        rawBytes: null,
        gzipBytes: null,
        limits: budgets.bundle[definition.id],
        violations: [],
      });
      continue;
    }
    if (matches.length === 0 || (definition.singleton && matches.length !== 1)) {
      throw new Error(
        `Expected ${definition.singleton ? "exactly one" : "at least one"} ${definition.label} artifact, found ${matches.length}`,
      );
    }
    const contents = matches.map((match) => readFileSync(join(root, match)));
    const rawBytes = contents.reduce((sum, content) => sum + content.byteLength, 0);
    const gzipBytes = contents.reduce(
      (sum, content) => sum + gzipSync(content, { level: 9, mtime: 0 }).byteLength,
      0,
    );
    const limits = budgets.bundle[definition.id];
    const violations = [];
    if (rawBytes > limits.rawBytes) violations.push(`raw ${rawBytes} > ${limits.rawBytes}`);
    if (gzipBytes > limits.gzipBytes) violations.push(`gzip ${gzipBytes} > ${limits.gzipBytes}`);
    artifacts.push({
      id: definition.id,
      label: definition.label,
      status: violations.length ? "over-budget" : "within-budget",
      paths: matches,
      rawBytes,
      gzipBytes,
      limits,
      violations,
    });
  }
  return {
    format: "cncweb-bundle-budget-result",
    version: 1,
    profile: requireEngine ? "integrated" : hasAnyEngine ? "source-plus-local-engine" : "source-only",
    distDirectory: root,
    artifacts,
    passed: artifacts.every((artifact) => artifact.violations.length === 0),
  };
}

export function printBundleReport(result) {
  console.log(`Bundle budget profile: ${result.profile}`);
  for (const artifact of result.artifacts) {
    if (artifact.status === "omitted") {
      console.log(`  ${artifact.label.padEnd(20)} omitted (allowed for source-only build)`);
      continue;
    }
    const marker = artifact.violations.length ? "FAIL" : "PASS";
    console.log(
      `  ${marker} ${artifact.label.padEnd(17)} raw ${formatBytes(artifact.rawBytes)}`
      + ` / ${formatBytes(artifact.limits.rawBytes)}, gzip ${formatBytes(artifact.gzipBytes)}`
      + ` / ${formatBytes(artifact.limits.gzipBytes)} (${artifact.paths.join(", ")})`,
    );
  }
  if (!result.passed) {
    for (const artifact of result.artifacts) {
      for (const violation of artifact.violations) {
        console.error(`  ${artifact.label}: ${violation}`);
      }
    }
  }
}

function usage() {
  console.log(`Usage: node ${basename(process.argv[1])} [--dist DIRECTORY] [--require-engine] [--json]\n`);
}

function parseArguments(argv) {
  const options = { distDirectory: "dist", requireEngine: false, json: false };
  for (let index = 0; index < argv.length; index += 1) {
    const argument = argv[index];
    if (argument === "--require-engine") options.requireEngine = true;
    else if (argument === "--json") options.json = true;
    else if (argument === "--dist") {
      index += 1;
      if (index >= argv.length) throw new Error("--dist requires a directory");
      options.distDirectory = argv[index];
    } else if (argument === "-h" || argument === "--help") {
      usage();
      return null;
    } else throw new Error(`Unknown argument: ${argument}`);
  }
  return options;
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  if (!options) return;
  const budgets = readPerformanceBudgets();
  const result = inspectBundle({ ...options, budgets });
  if (options.json) console.log(JSON.stringify(result, null, 2));
  else printBundleReport(result);
  if (!result.passed) process.exitCode = 1;
}

if (pathToFileURL(process.argv[1]).href === import.meta.url) {
  main().catch((error) => {
    console.error(error instanceof Error ? error.message : String(error));
    process.exitCode = 1;
  });
}
