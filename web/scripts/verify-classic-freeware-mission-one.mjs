#!/usr/bin/env node

import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { existsSync, readFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

import { TextWriter, Uint8ArrayReader, Uint8ArrayWriter, ZipReader } from "@zip.js/zip.js";

const scriptDirectory = dirname(fileURLToPath(import.meta.url));
const defaultModulePath = resolve(scriptDirectory, "../dist/engine/tiberiandawn.js");
const builtPackagePath = resolve(scriptDirectory, "../dist/classic-freeware-gdi-v1.cncweb");
const cachedPackagePath = resolve(scriptDirectory, "../../.cache/classic-freeware/classic-freeware-gdi-v1.cncweb");
const defaultPackagePath = existsSync(builtPackagePath) ? builtPackagePath : cachedPackagePath;

const missionNumber = Number.parseInt(process.env.CNCWEB_VERIFY_MISSION ?? "1", 10);
const missionVariant = process.env.CNCWEB_VERIFY_MISSION_VARIANT ?? "west-a";
const missions = new Map([
  [1, {
    number: 1,
    id: "gdi-01-east-a",
    scenarioRoot: "SCG01EA",
    scenario: 1,
    variation: 0,
    direction: 0,
    buildLevel: 1,
    maxTicks: 6_000,
  }],
  [2, {
    number: 2,
    id: "gdi-02-east-a",
    scenarioRoot: "SCG02EA",
    scenario: 2,
    variation: 0,
    direction: 0,
    buildLevel: 2,
    maxTicks: 30_000,
  }],
  [3, {
    number: 3,
    id: "gdi-03-east-a",
    scenarioRoot: "SCG03EA",
    scenario: 3,
    variation: 0,
    direction: 0,
    buildLevel: 3,
    maxTicks: 90_000,
  }],
]);
const missionFourVariants = new Map([
  ["west-a", {
    number: 4,
    variant: "west-a",
    id: "gdi-04-west-a",
    scenarioRoot: "SCG04WA",
    scenario: 4,
    variation: 0,
    direction: 1,
    buildLevel: 4,
    maxTicks: 30_000,
    objective: "extract",
    runner: "apc",
    threatRadius: 0,
    route: [
      { cellX: 18, cellY: 47 },
      { cellX: 11, cellY: 42 },
      { cellX: 12, cellY: 28 },
      { cellX: 13, cellY: 19 },
      { cellX: 14, cellY: 13 },
    ],
  }],
  ["west-b", {
    number: 4,
    variant: "west-b",
    id: "gdi-04-west-b",
    scenarioRoot: "SCG04WB",
    scenario: 4,
    variation: 1,
    direction: 1,
    buildLevel: 4,
    maxTicks: 45_000,
    objective: "eliminate",
    threatRadius: 4,
    route: [
      { cellX: 31, cellY: 55 },
      { cellX: 42, cellY: 55 },
      { cellX: 52, cellY: 55 },
      { cellX: 55, cellY: 35 },
      { cellX: 50, cellY: 22 },
      { cellX: 42, cellY: 17 },
      { cellX: 35, cellY: 30 },
      { cellX: 29, cellY: 30 },
      { cellX: 29, cellY: 49 },
      { cellX: 20, cellY: 55 },
      { cellX: 19, cellY: 44 },
    ],
  }],
  ["east-a", {
    number: 4,
    variant: "east-a",
    id: "gdi-04-east-a",
    scenarioRoot: "SCG04EA",
    scenario: 4,
    variation: 0,
    direction: 0,
    buildLevel: 4,
    maxTicks: 30_000,
    objective: "extract",
    runner: "apc-one",
    threatRadius: 0,
    route: [
      { cellX: 16, cellY: 39 },
      { cellX: 17, cellY: 25 },
      { cellX: 40, cellY: 26 },
      { cellX: 52, cellY: 38 },
      { cellX: 57, cellY: 43 },
      { cellX: 57, cellY: 52 },
      { cellX: 25, cellY: 58 },
    ],
  }],
]);
const missionFiveVariants = new Map([
  ["east-a", {
    number: 5,
    variant: "east-a",
    id: "gdi-05-east-a",
    scenarioRoot: "SCG05EA",
    scenario: 5,
    variation: 0,
    direction: 0,
    buildLevel: 5,
    maxTicks: 120_000,
    earliestAssaultTick: 9_000,
    reliefRoute: [
      { cellX: 27, cellY: 55 },
      { cellX: 38, cellY: 55 },
      { cellX: 40, cellY: 55 },
      { cellX: 47, cellY: 55 },
    ],
    crate: { cellX: 44, cellY: 40 },
    home: { cellX: 50, cellY: 54 },
    guardPosts: [
      { cellX: 37, cellY: 54 },
      { cellX: 52, cellY: 46 },
    ],
    homeGuardSize: 10,
    assaultTargetStage: 5,
    precisionRouteStage: 4,
    huntSites: [
      { typeName: "AFLD", cellX: 11, cellY: 25 },
      { typeName: "FACT", cellX: 10, cellY: 29 },
      { typeName: "PROC", cellX: 22, cellY: 23 },
      { typeName: "HAND", cellX: 15, cellY: 23 },
    ],
    samSites: [
      { site: { cellX: 13, cellY: 30 }, approach: { cellX: 15, cellY: 30 } },
      { site: { cellX: 24, cellY: 31 }, approach: { cellX: 25, cellY: 32 } },
      { site: { cellX: 41, cellY: 34 }, approach: { cellX: 43, cellY: 34 } },
      { site: { cellX: 21, cellY: 39 }, approach: { cellX: 23, cellY: 41 } },
    ],
    samSweepSites: [
      { site: { cellX: 21, cellY: 39 }, approach: { cellX: 23, cellY: 41 } },
      { site: { cellX: 24, cellY: 31 }, approach: { cellX: 26, cellY: 33 } },
      { site: { typeName: "GUN", cellX: 24, cellY: 28 }, approach: { cellX: 26, cellY: 30 } },
    ],
    postSweepRouteStage: 3,
    preSweepRouteStage: 2,
    cleanupSites: [
      { site: { cellX: 9, cellY: 22 }, approach: { cellX: 10, cellY: 25 } },
      { site: { cellX: 24, cellY: 31 }, approach: { cellX: 25, cellY: 32 } },
      { site: { cellX: 21, cellY: 39 }, approach: { cellX: 23, cellY: 41 } },
      { site: { cellX: 41, cellY: 34 }, approach: { cellX: 43, cellY: 34 } },
      { site: { cellX: 13, cellY: 30 }, approach: { cellX: 15, cellY: 30 } },
      { site: { cellX: 11, cellY: 22 }, approach: { cellX: 10, cellY: 25 } },
      { site: { cellX: 13, cellY: 22 }, approach: { cellX: 15, cellY: 25 } },
    ],
    assaultRoute: [
      { cellX: 37, cellY: 54 },
      { cellX: 28, cellY: 45 },
      { cellX: 30, cellY: 31 },
      { cellX: 17, cellY: 28 },
      { cellX: 13, cellY: 29 },
      { cellX: 10, cellY: 25 },
      { cellX: 15, cellY: 25 },
      { cellX: 27, cellY: 33 },
      { cellX: 21, cellY: 42 },
      { cellX: 10, cellY: 34 },
      { cellX: 28, cellY: 45 },
      { cellX: 26, cellY: 54 },
      { cellX: 37, cellY: 54 },
      { cellX: 52, cellY: 46 },
      { cellX: 53, cellY: 31 },
      { cellX: 42, cellY: 29 },
      { cellX: 36, cellY: 28 },
    ],
  }],
  ["west-a", {
    number: 5,
    variant: "west-a",
    id: "gdi-05-west-a",
    scenarioRoot: "SCG05WA",
    scenario: 5,
    variation: 0,
    direction: 1,
    buildLevel: 5,
    maxTicks: 120_000,
    relaunchForce: 25,
    reliefRoute: [
      { cellX: 27, cellY: 55 },
      { cellX: 38, cellY: 55 },
      { cellX: 40, cellY: 55 },
      { cellX: 47, cellY: 55 },
    ],
    home: { cellX: 50, cellY: 54 },
    huntSites: [
      { typeName: "AFLD", cellX: 11, cellY: 25 },
      { typeName: "FACT", cellX: 10, cellY: 29 },
      { typeName: "PROC", cellX: 22, cellY: 23 },
      { typeName: "HAND", cellX: 15, cellY: 23 },
    ],
    samSites: [
      { site: { cellX: 13, cellY: 30 }, approach: { cellX: 15, cellY: 30 } },
      { site: { cellX: 24, cellY: 31 }, approach: { cellX: 25, cellY: 32 } },
      { site: { cellX: 41, cellY: 34 }, approach: { cellX: 43, cellY: 34 } },
      { site: { cellX: 21, cellY: 39 }, approach: { cellX: 23, cellY: 41 } },
    ],
    cleanupSites: [
      { site: { cellX: 9, cellY: 22 }, approach: { cellX: 10, cellY: 25 } },
      { site: { cellX: 24, cellY: 31 }, approach: { cellX: 25, cellY: 32 } },
      { site: { cellX: 21, cellY: 39 }, approach: { cellX: 23, cellY: 41 } },
      { site: { cellX: 41, cellY: 34 }, approach: { cellX: 43, cellY: 34 } },
      { site: { cellX: 13, cellY: 30 }, approach: { cellX: 15, cellY: 30 } },
      { site: { cellX: 11, cellY: 22 }, approach: { cellX: 10, cellY: 25 } },
      { site: { cellX: 13, cellY: 22 }, approach: { cellX: 15, cellY: 25 } },
    ],
    assaultRoute: [
      { cellX: 37, cellY: 54 },
      { cellX: 28, cellY: 45 },
      { cellX: 30, cellY: 31 },
      { cellX: 31, cellY: 25 },
      { cellX: 17, cellY: 28 },
      { cellX: 13, cellY: 29 },
      { cellX: 10, cellY: 25 },
      { cellX: 15, cellY: 25 },
      { cellX: 27, cellY: 33 },
      { cellX: 21, cellY: 42 },
      { cellX: 10, cellY: 34 },
      { cellX: 28, cellY: 45 },
      { cellX: 26, cellY: 54 },
      { cellX: 37, cellY: 54 },
      { cellX: 52, cellY: 46 },
      { cellX: 53, cellY: 31 },
      { cellX: 42, cellY: 29 },
      { cellX: 36, cellY: 28 },
    ],
  }],
  ["west-b", {
    number: 5,
    variant: "west-b",
    id: "gdi-05-west-b",
    scenarioRoot: "SCG05WB",
    scenario: 5,
    variation: 1,
    direction: 1,
    buildLevel: 5,
    maxTicks: 120_000,
    earliestAssaultTick: 7_200,
    relaunchForce: 3,
    reliefRoute: [
      { cellX: 12, cellY: 30 },
      { cellX: 12, cellY: 45 },
      { cellX: 12, cellY: 58 },
      { cellX: 23, cellY: 58 },
      { cellX: 25, cellY: 58 },
      { cellX: 30, cellY: 54 },
    ],
    crate: { cellX: 26, cellY: 42 },
    home: { cellX: 31, cellY: 53 },
    guardPosts: [
      { cellX: 20, cellY: 57 },
      { cellX: 42, cellY: 54 },
    ],
    homeGuardSize: 0,
    assaultTargetStage: 4,
    precisionRouteStage: 4,
    coreRouteHolds: [
      { routeStage: 4, sites: [{ typeName: "FACT", cellX: 52, cellY: 17 }] },
      { routeStage: 5, sites: [
        { typeName: "HAND", cellX: 41, cellY: 22 },
        { typeName: "AFLD", cellX: 42, cellY: 18 },
      ] },
      { routeStage: 6, sites: [
        { typeName: "PROC", cellX: 47, cellY: 22 },
        { typeName: "NUKE", cellX: 47, cellY: 18 },
      ] },
    ],
    huntSites: [
      { typeName: "PROC", cellX: 47, cellY: 22 },
      { typeName: "AFLD", cellX: 42, cellY: 18 },
      { typeName: "NUKE", cellX: 47, cellY: 18 },
      { typeName: "NUKE", cellX: 49, cellY: 17 },
      { typeName: "FACT", cellX: 52, cellY: 17 },
    ],
    samSites: [
      { site: { cellX: 40, cellY: 17 }, approach: { cellX: 38, cellY: 19 } },
      { site: { cellX: 38, cellY: 25 }, approach: { cellX: 36, cellY: 27 } },
      { site: { cellX: 52, cellY: 25 }, approach: { cellX: 54, cellY: 27 } },
      { site: { cellX: 26, cellY: 37 }, approach: { cellX: 24, cellY: 39 } },
    ],
    assaultRoute: [
      { cellX: 42, cellY: 54 },
      { cellX: 53, cellY: 53 },
      { cellX: 53, cellY: 42 },
      { cellX: 56, cellY: 29 },
      { cellX: 53, cellY: 19 },
      { cellX: 42, cellY: 20 },
      { cellX: 48, cellY: 20 },
      { cellX: 53, cellY: 42 },
      { cellX: 53, cellY: 53 },
      { cellX: 42, cellY: 54 },
      { cellX: 20, cellY: 57 },
      { cellX: 11, cellY: 51 },
      { cellX: 12, cellY: 30 },
      { cellX: 12, cellY: 21 },
      { cellX: 34, cellY: 20 },
      { cellX: 34, cellY: 29 },
    ],
  }],
]);
const mission = missionNumber === 4
  ? missionFourVariants.get(missionVariant)
  : missionNumber === 5
    ? missionFiveVariants.get(missionVariant)
    : missions.get(missionNumber);
if (!mission) {
  if (missionNumber === 4 || missionNumber === 5) {
    console.error("CNCWEB_VERIFY_MISSION_VARIANT must be west-a, west-b, or east-a");
  } else console.error("CNCWEB_VERIFY_MISSION must be 1, 2, 3, 4, or 5");
  process.exit(2);
}
const trace = process.env.CNCWEB_VERIFY_TRACE === "1";
const missionFiveWestBStrategy = mission.number === 5 && mission.variant === "west-b";
const missionTwoAssaultTick = Number.parseInt(process.env.CNCWEB_VERIFY_ASSAULT_TICK ?? "12000", 10);
if (mission.number === 2 && (!Number.isSafeInteger(missionTwoAssaultTick) || missionTwoAssaultTick < 0 || missionTwoAssaultTick > mission.maxTicks)) {
  console.error("CNCWEB_VERIFY_ASSAULT_TICK must be an integer within the mission tick budget");
  process.exit(2);
}
const missionThreeAssaultTick = Number.parseInt(process.env.CNCWEB_VERIFY_MISSION_THREE_ASSAULT_TICK ?? "24000", 10);
if (mission.number === 3 && (!Number.isSafeInteger(missionThreeAssaultTick)
  || missionThreeAssaultTick < 0 || missionThreeAssaultTick > mission.maxTicks)) {
  console.error("CNCWEB_VERIFY_MISSION_THREE_ASSAULT_TICK must be an integer within the mission tick budget");
  process.exit(2);
}
const missionFiveAssaultTick = Number.parseInt(
  process.env.CNCWEB_VERIFY_MISSION_FIVE_ASSAULT_TICK
    ?? (missionFiveWestBStrategy ? "12000" : "48000"),
  10,
);
const missionFiveAssaultForce = Number.parseInt(
  process.env.CNCWEB_VERIFY_MISSION_FIVE_ASSAULT_FORCE
    ?? (missionFiveWestBStrategy ? "25" : mission.variant === "east-a" ? "35" : "45"),
  10,
);
if (mission.number === 5 && (!Number.isSafeInteger(missionFiveAssaultTick)
  || missionFiveAssaultTick < 0 || missionFiveAssaultTick > mission.maxTicks
  || !Number.isSafeInteger(missionFiveAssaultForce) || missionFiveAssaultForce < 1 || missionFiveAssaultForce > 500)) {
  console.error("Mission 5 assault tuning must use a tick within the mission budget and a force from 1 to 500");
  process.exit(2);
}

const arguments_ = process.argv.slice(2);
if (arguments_.length !== 0 && arguments_.length !== 3 && arguments_.length !== 4) {
  console.error("usage: verify-classic-freeware-mission-one.mjs [ENGINE.js ENGINE_ASSET_BASE PACKAGE.cncweb [PACKAGE_REVISION]]");
  process.exit(2);
}

const modulePath = resolve(arguments_[0] ?? defaultModulePath);
const packagePath = resolve(arguments_[2] ?? defaultPackagePath);
const expectedRevision = arguments_[3];
if (expectedRevision !== undefined && !/^[a-f0-9]{64}$/.test(expectedRevision)) {
  console.error("PACKAGE_REVISION must be a lowercase SHA-256 digest");
  process.exit(2);
}

function directoryUrl(value) {
  if (value === undefined) return pathToFileURL(`${dirname(modulePath)}/`);
  let url;
  try {
    url = new URL(value);
  } catch {
    url = pathToFileURL(`${resolve(value)}/`);
  }
  if (!url.pathname.endsWith("/")) url.pathname += "/";
  return url;
}

const assetBase = directoryUrl(arguments_[1]);
const packageBytes = new Uint8Array(readFileSync(packagePath));
const archive = new ZipReader(new Uint8ArrayReader(packageBytes));
let manifest;
let packageRevision;
let engineFiles;
try {
  const entries = await archive.getEntries();
  const byPath = new Map(entries.filter((entry) => !entry.directory).map((entry) => [entry.filename, entry]));
  const manifestEntry = byPath.get("manifest.json");
  assert.ok(manifestEntry?.getData, "classic-freeware package has no canonical manifest");
  manifest = JSON.parse(await manifestEntry.getData(new TextWriter()));
  assert.equal(manifest.format, "cncweb-content", "package manifest format is not supported");
  assert.equal(manifest.version, 1, "package manifest version is not supported");
  assert.equal(manifest.package_id, "classic-freeware-gdi-v1", "package is not the classic-freeware GDI campaign");
  assert.equal(manifest.source?.product, "tiberian-dawn-freeware", "package source product is not Tiberian Dawn freeware");
  assert.equal(manifest.source?.provider, "ea-freeware", "package source provider is not EA freeware");
  assert.match(manifest.source.install_fingerprint_sha256, /^[a-f0-9]{64}$/, "package source fingerprint is invalid");

  packageRevision = createHash("sha256").update(JSON.stringify(manifest), "utf8").digest("hex");
  if (expectedRevision !== undefined) {
    assert.equal(packageRevision, expectedRevision, "package revision differs from the requested browser ContentStore identity");
  }

  const catalogEntry = byPath.get("runtime/catalog-v1.json");
  assert.ok(catalogEntry?.getData, "classic-freeware package has no runtime catalog");
  const catalog = JSON.parse(await catalogEntry.getData(new TextWriter()));
  assert.equal(catalog.format, "cncweb-runtime", "runtime catalog format is not supported");
  assert.equal(catalog.version, 1, "runtime catalog version is not supported");
  assert.equal(catalog.engine, "tiberian-dawn", "runtime catalog does not target Tiberian Dawn");
  assert.equal(catalog.engineRoot, "engine/td", "runtime catalog engine root is unexpected");
  assert.ok(catalog.missions?.some((candidate) => (
    candidate.id === mission.id
    && candidate.scenarioRoot === mission.scenarioRoot
    && candidate.scenario === mission.scenario
    && candidate.variation === mission.variation
    && candidate.direction === mission.direction
    && candidate.buildLevel === mission.buildLevel
    && candidate.faction === "gdi"
  )), `runtime catalog does not contain canonical GDI Mission ${mission.number}`);

  const descriptors = new Map(manifest.files.map((file) => [file.path, file]));
  engineFiles = [];
  for (const entry of entries) {
    if (entry.directory || !entry.filename.startsWith("engine/td/") || !entry.getData) continue;
    const descriptor = descriptors.get(entry.filename);
    assert.ok(descriptor, `${entry.filename} is absent from the package manifest`);
    const data = await entry.getData(new Uint8ArrayWriter());
    assert.equal(data.byteLength, descriptor.size, `${entry.filename} size differs from its manifest`);
    assert.equal(createHash("sha256").update(data).digest("hex"), descriptor.sha256, `${entry.filename} hash differs from its manifest`);
    engineFiles.push({ path: entry.filename, data });
  }
  assert.ok(engineFiles.some((file) => file.path === `engine/td/${mission.scenarioRoot}.INI`), `GDI Mission ${mission.number} scenario INI is missing`);
  assert.ok(engineFiles.some((file) => file.path === `engine/td/${mission.scenarioRoot}.BIN`), `GDI Mission ${mission.number} scenario map is missing`);
  assert.ok(engineFiles.length >= 15, "classic-freeware package has too few engine files");
} finally {
  await archive.close();
}

const { default: createModule } = await import(pathToFileURL(modulePath).href);
let moduleOptions;
if (assetBase.protocol === "file:") {
  const compiledWasm = new WebAssembly.Module(readFileSync(fileURLToPath(new URL("tiberiandawn.wasm", assetBase))));
  moduleOptions = {
    instantiateWasm(imports, receiveInstance) {
      const instance = new WebAssembly.Instance(compiledWasm, imports);
      receiveInstance(instance);
      return instance.exports;
    },
  };
} else {
  moduleOptions = { locateFile: (path) => new URL(path, assetBase).href };
}
const engine = await createModule(moduleOptions);
assert.equal(engine._cnc_web_abi_version(), 2, "unexpected browser ABI version");

const MAGIC = 0x57434e43;
const STATUS_OK = 0;
const MESSAGE_START = 1;
const MESSAGE_COMMAND = 2;
const MESSAGE_SNAPSHOT = 3;
const EVENT_GAME_OVER = 3;
const EVENT_MOVIE = 5;
const EVENT_DIAGNOSTIC = 14;
const EVENT_CAMPAIGN_OUTCOME = 15;
const DIAGNOSTIC_START_READY = 6;
const COMMAND_INPUT = 1;
const COMMAND_GAME = 7;
const COMMAND_STRUCTURE = 2;
const COMMAND_UNIT = 3;
const COMMAND_CLEAR_SELECTION = 8;
const COMMAND_SELECT_OBJECT = 9;
const COMMAND_SIDEBAR = 4;
const COMMAND_SUPERWEAPON = 5;
const SIDEBAR_START_CONSTRUCTION = 0;
const SIDEBAR_START_PLACEMENT = 3;
const SIDEBAR_PLACE = 4;
const SUPERWEAPON_PLACE = 0;
const STRUCTURE_REPAIR_START = 1;
const STRUCTURE_REPAIR = 2;
const STRUCTURE_SELL = 4;
const GAME_MOVIE_DONE = 0;
const INPUT_COMMAND_AT_POSITION = 9;
const INPUT_SPECIAL_KEYS = 10;
const UNIT_REQUEST_STOP = 5;
const UNIT_SCATTER = 1;
const MODIFIER_CTRL = 1 << 0;
const MODIFIER_ALT = 1 << 1;
const SECTION_STATIC_MAP = 1;
const SECTION_OBJECTS = 3;
const SECTION_SIDEBAR = 4;
const SECTION_PLACEMENT = 5;
const SECTION_SHROUD = 6;
const SNAPSHOT_TERMINAL = 1;
const OBJECT_RECORD_BYTES = 472;
const SIDEBAR_FIXED_BYTES = 60;
const SIDEBAR_RECORD_BYTES = 128;
const TICK_HZ = 15;
const TICKS_PER_ORDER = 30;
const MAX_TICKS = mission.maxTicks;
const CELL_PIXELS = 24;
const HOUSE_GDI = 0;
const HOUSE_NOD = 1;
const HOUSE_NEUTRAL = 2;
const ROOT_COMBAT_TYPES = new Set([1, 2, 3, 4]);

const mountRoot = `/cnc-content/${packageRevision.slice(0, 16)}`;
for (const file of engineFiles) {
  const destination = `${mountRoot}/${file.path}`;
  engine.FS.mkdirTree(dirname(destination));
  engine.FS.writeFile(destination, file.data);
}

function withAllocation(size, operation) {
  const pointer = engine._malloc(size);
  assert.notEqual(pointer, 0, `failed to allocate ${operation}`);
  return pointer;
}

function writeInput(bytes, callback) {
  const pointer = withAllocation(bytes.byteLength, "input buffer");
  try {
    engine.HEAPU8.set(bytes, pointer);
    return callback(pointer, bytes.byteLength);
  } finally {
    engine._free(pointer);
  }
}

function outputU32(callback, operation) {
  const pointer = withAllocation(4, operation);
  try {
    assert.equal(callback(pointer), STATUS_OK, `${operation} failed`);
    return new DataView(engine.HEAPU8.buffer).getUint32(pointer, true);
  } finally {
    engine._free(pointer);
  }
}

function startMessage() {
  const content = new TextEncoder().encode(`${mountRoot}/engine/td`);
  const bytes = new Uint8Array(72 + content.byteLength);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, MAGIC, true);
  view.setUint16(4, 1, true);
  view.setUint16(6, MESSAGE_START, true);
  view.setUint32(8, bytes.byteLength, true);
  view.setUint32(12, 1, true);
  view.setUint32(16, 0x1a2b3c4d, true);
  view.setInt32(20, mission.scenario, true);
  view.setInt32(24, mission.variation, true);
  view.setInt32(28, mission.direction, true);
  view.setInt32(32, mission.buildLevel, true);
  view.setInt32(36, -1, true);
  view.setUint32(40, 1, true);
  view.setUint32(44, 1, true);
  view.setBigUint64(48, 0n, true);
  view.setUint32(56, content.byteLength, true);
  view.setUint32(60, 0, true);
  const contentHash = BigInt(`0x${packageRevision.slice(0, 16)}`);
  assert.notEqual(contentHash, 0n, "package revision produced an invalid content identity");
  view.setBigUint64(64, contentHash, true);
  bytes.set(content, 72);
  return bytes;
}

function commandBatch(targetTick, commands) {
  assert.ok(commands.length > 0 && commands.length <= 4096, "command count is outside the protocol limit");
  const bytes = new Uint8Array(32 + commands.length * 32);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, MAGIC, true);
  view.setUint16(4, 1, true);
  view.setUint16(6, MESSAGE_COMMAND, true);
  view.setUint32(8, bytes.byteLength, true);
  view.setUint32(12, commands.length, true);
  view.setUint32(16, targetTick, true);
  view.setUint16(20, 32, true);
  view.setUint16(22, 0, true);
  view.setBigUint64(24, 0n, true);
  commands.forEach((command, index) => {
    assert.equal(command.args.length, 7, "normalized commands require seven arguments");
    const offset = 32 + index * 32;
    view.setUint16(offset, command.type, true);
    view.setUint16(offset + 2, command.flags ?? 0, true);
    command.args.forEach((value, argument) => view.setInt32(offset + 4 + argument * 4, value, true));
  });
  return bytes;
}

function submitCommands(handle, targetTick, commands) {
  const bytes = commandBatch(targetTick, commands);
  assert.equal(
    writeInput(bytes, (pointer, length) => engine._cnc_web_submit_commands(handle, pointer, length)),
    STATUS_OK,
    `command batch for tick ${targetTick} failed`,
  );
}

function readOutput(handle, sizeFunction, writeFunction, label) {
  const size = outputU32((output) => sizeFunction(handle, output), `${label}-size query`);
  assert.ok(size > 0 && size <= 64 * 1024 * 1024, `${label} size is invalid`);
  const dataPointer = withAllocation(size, `${label} buffer`);
  const writtenPointer = withAllocation(4, `${label} written output`);
  try {
    assert.equal(writeFunction(handle, dataPointer, size, writtenPointer), STATUS_OK, `${label} write failed`);
    const written = new DataView(engine.HEAPU8.buffer).getUint32(writtenPointer, true);
    assert.equal(written, size, `${label} size changed while writing`);
    return new Uint8Array(engine.HEAPU8.buffer, dataPointer, size).slice();
  } finally {
    engine._free(writtenPointer);
    engine._free(dataPointer);
  }
}

const events = [];
let currentTick = 0;
let movieAcknowledgements = 0;

function drainEvents(handle) {
  for (let index = 0; index < 4096; index += 1) {
    const size = outputU32((output) => engine._cnc_web_event_size(handle, output), "event-size query");
    if (size === 0) return;
    assert.ok(size >= 64 && size <= 1024 * 1024, "engine emitted an invalid event size");
    const eventPointer = withAllocation(size, "event buffer");
    const writtenPointer = withAllocation(4, "event written output");
    try {
      assert.equal(engine._cnc_web_poll_event(handle, eventPointer, size, writtenPointer), STATUS_OK, "event poll failed");
      const memory = new DataView(engine.HEAPU8.buffer);
      assert.equal(memory.getUint32(writtenPointer, true), size, "event size changed while polling");
      assert.equal(memory.getUint32(eventPointer, true), MAGIC, "event has invalid magic");
      assert.equal(memory.getUint16(eventPointer + 4, true), 1, "event protocol is unsupported");
      assert.equal(memory.getUint16(eventPointer + 6, true), 4, "event message kind is invalid");
      assert.equal(memory.getUint32(eventPointer + 8, true), size, "event total size is invalid");
      const text1Length = memory.getUint32(eventPointer + 56, true);
      const text2Length = memory.getUint32(eventPointer + 60, true);
      assert.equal(64 + text1Length + text2Length, size, "event text layout is invalid");
      const tick = memory.getUint32(eventPointer + 16, true);
      const type = memory.getUint16(eventPointer + 20, true);
      const flags = memory.getUint16(eventPointer + 22, true);
      const args = Array.from({ length: 6 }, (_, argument) => memory.getInt32(eventPointer + 32 + argument * 4, true));
      const textBytes = new Uint8Array(engine.HEAPU8.buffer, eventPointer + 64, text1Length + text2Length);
      const text1 = new TextDecoder().decode(textBytes.subarray(0, text1Length));
      const text2 = new TextDecoder().decode(textBytes.subarray(text1Length));
      events.push({ tick, type, flags, args, text1, text2 });
      if (type === EVENT_MOVIE) {
        const targetTick = Math.max(tick, currentTick) + 1;
        submitCommands(handle, targetTick, [{ type: COMMAND_GAME, args: [GAME_MOVIE_DONE, 0, 0, 0, 0, 0, 0] }]);
        movieAcknowledgements += 1;
      }
    } finally {
      engine._free(writtenPointer);
      engine._free(eventPointer);
    }
  }
  assert.fail("engine emitted more than 4096 events without draining");
}

function readSnapshot(handle) {
  const bytes = readOutput(handle, engine._cnc_web_snapshot_size, engine._cnc_web_write_snapshot, "snapshot");
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  assert.ok(bytes.byteLength >= 40, "snapshot header is truncated");
  assert.equal(view.getUint32(0, true), MAGIC, "snapshot magic is invalid");
  assert.equal(view.getUint16(4, true), 1, "snapshot protocol is unsupported");
  assert.equal(view.getUint16(6, true), MESSAGE_SNAPSHOT, "snapshot message kind is invalid");
  assert.equal(view.getUint32(8, true), bytes.byteLength, "snapshot total size is invalid");
  const sectionCount = view.getUint32(32, true);
  assert.equal(view.getUint32(12, true), sectionCount, "snapshot section count differs between headers");
  const sections = new Map();
  let offset = 40;
  for (let index = 0; index < sectionCount; index += 1) {
    assert.ok(offset + 16 <= bytes.byteLength, "snapshot section header is truncated");
    const kind = view.getUint16(offset, true);
    const flags = view.getUint16(offset + 2, true);
    const length = view.getUint32(offset + 4, true);
    const count = view.getUint32(offset + 8, true);
    assert.equal(view.getUint32(offset + 12, true), 0, "snapshot section reserved field is not zero");
    assert.ok(!sections.has(kind), "snapshot contains a duplicate section");
    offset += 16;
    assert.ok(offset + length <= bytes.byteLength, "snapshot section is truncated");
    sections.set(kind, { flags, length, count, offset });
    offset += length;
  }
  assert.equal(offset, bytes.byteLength, "snapshot has trailing bytes");

  const objectsSection = sections.get(SECTION_OBJECTS);
  assert.ok(objectsSection, "snapshot has no object section");
  assert.equal(objectsSection.flags, 0, "object section flags are unsupported");
  assert.equal(objectsSection.length, objectsSection.count * OBJECT_RECORD_BYTES, "object section layout is invalid");
  const objects = [];
  for (let index = 0; index < objectsSection.count; index += 1) {
    const objectOffset = objectsSection.offset + index * OBJECT_RECORD_BYTES;
    const decodeName = (nameOffset) => {
      const field = bytes.subarray(nameOffset, nameOffset + 16);
      const terminator = field.indexOf(0);
      return new TextDecoder().decode(terminator < 0 ? field : field.subarray(0, terminator));
    };
    objects.push({
      typeName: decodeName(objectOffset),
      assetName: decodeName(objectOffset + 16),
      type: view.getInt32(objectOffset + 112, true),
      id: view.getInt32(objectOffset + 116, true),
      maxStrength: view.getInt16(objectOffset + 160, true),
      strength: view.getInt16(objectOffset + 162, true),
      cellX: view.getUint16(objectOffset + 166, true),
      cellY: view.getUint16(objectOffset + 168, true),
      owner: view.getUint8(objectOffset + 182),
      subObject: view.getUint8(objectOffset + 184),
      objectFlags: view.getUint32(objectOffset + 204, true),
      canFireMask: view.getUint32(objectOffset + 212, true),
      // cnc_web_protocol.h v1: occupy_count u16 @216, pip_count u16 @218,
      // max_pips u16 @220, line_count u16 @222, then pips[18] i32 @296.
      // Note that the transport renderer exports all five slots, including
      // PIP_EMPTY, so pipCount is slot count rather than cargo occupancy.
      pipCount: view.getUint16(objectOffset + 218, true),
      maxPips: view.getUint16(objectOffset + 220, true),
      pips: Array.from({ length: 18 }, (_, pipIndex) => (
        view.getInt32(objectOffset + 296 + pipIndex * 4, true)
      )),
    });
  }

  const staticMapSection = sections.get(SECTION_STATIC_MAP);
  assert.ok(staticMapSection, "snapshot has no static-map section");
  assert.equal(staticMapSection.flags, 0, "static-map section flags are unsupported");
  assert.ok(staticMapSection.length >= 304, "static-map section is truncated");
  const staticMap = {
    cellX: view.getInt32(staticMapSection.offset, true),
    cellY: view.getInt32(staticMapSection.offset + 4, true),
    width: view.getInt32(staticMapSection.offset + 8, true),
    height: view.getInt32(staticMapSection.offset + 12, true),
  };
  assert.ok(staticMap.cellX >= 0 && staticMap.cellY >= 0 && staticMap.width > 0 && staticMap.height > 0
    && staticMap.cellX + staticMap.width <= 128 && staticMap.cellY + staticMap.height <= 128,
  "static-map bounds are invalid");

  const sidebarSection = sections.get(SECTION_SIDEBAR);
  assert.ok(sidebarSection, "snapshot has no sidebar section");
  assert.equal(sidebarSection.flags, 0, "sidebar section flags are unsupported");
  assert.ok(sidebarSection.length >= SIDEBAR_FIXED_BYTES, "sidebar section is truncated");
  const stats = {
    unitsKilled: view.getUint32(sidebarSection.offset + 36, true),
    buildingsKilled: view.getUint32(sidebarSection.offset + 40, true),
    unitsLost: view.getUint32(sidebarSection.offset + 44, true),
    buildingsLost: view.getUint32(sidebarSection.offset + 48, true),
  };
  const sidebarEntries = [];
  for (let index = 0; index < sidebarSection.count; index += 1) {
    const entryOffset = sidebarSection.offset + SIDEBAR_FIXED_BYTES + index * SIDEBAR_RECORD_BYTES;
    const assetBytes = bytes.subarray(entryOffset, entryOffset + 16);
    const terminator = assetBytes.indexOf(0);
    const flags = view.getUint32(entryOffset + 52, true);
    const placementCount = view.getUint32(entryOffset + 48, true);
    assert.ok(placementCount <= 36, "sidebar placement footprint is invalid");
    const placementOffsets = Array.from(
      { length: placementCount },
      (_, placementIndex) => view.getInt16(entryOffset + 56 + placementIndex * 2, true),
    );
    sidebarEntries.push({
      assetName: new TextDecoder().decode(terminator < 0 ? assetBytes : assetBytes.subarray(0, terminator)),
      buildableType: view.getInt32(entryOffset + 16, true),
      buildableId: view.getInt32(entryOffset + 20, true),
      objectType: view.getInt32(entryOffset + 24, true),
      superweaponType: view.getInt32(entryOffset + 28, true),
      cost: view.getInt32(entryOffset + 32, true),
      progress: view.getFloat32(entryOffset + 44, true),
      completed: Boolean(flags & 1),
      constructing: Boolean(flags & 2),
      onHold: Boolean(flags & 4),
      busy: Boolean(flags & 8),
      placementOffsets,
    });
  }
  const placementSection = sections.get(SECTION_PLACEMENT);
  const placement = placementSection
    ? {
      flags: bytes.slice(placementSection.offset, placementSection.offset + placementSection.length),
      ...staticMap,
    }
    : undefined;
  if (placementSection) {
    assert.equal(placementSection.flags, 0, "placement section flags are unsupported");
    assert.equal(placementSection.count, staticMap.width * staticMap.height, "placement grid count differs from the static map");
    assert.equal(placementSection.length, placementSection.count, "placement grid layout is invalid");
  }
  const shroudSection = sections.get(SECTION_SHROUD);
  assert.ok(shroudSection, "snapshot has no shroud section");
  assert.equal(shroudSection.flags, 0, "shroud section flags are unsupported");
  assert.equal(shroudSection.count, staticMap.width * staticMap.height, "shroud grid count differs from the static map");
  assert.equal(shroudSection.length, shroudSection.count * 2, "shroud grid layout is invalid");
  const shroudEntries = bytes.slice(shroudSection.offset, shroudSection.offset + shroudSection.length);
  for (let index = 0; index < shroudSection.count; index += 1) {
    assert.equal(shroudEntries[index * 2 + 1] & ~7, 0, "shroud cell flags are invalid");
  }
  const shroud = {
    ...staticMap,
    isVisible(cellX, cellY) {
      const x = cellX - staticMap.cellX;
      const y = cellY - staticMap.cellY;
      if (x < 0 || y < 0 || x >= staticMap.width || y >= staticMap.height) return false;
      return Boolean(shroudEntries[(y * staticMap.width + x) * 2 + 1] & 1);
    },
  };
  return {
    tick: view.getUint32(16, true),
    terminal: Boolean(view.getUint32(36, true) & SNAPSHOT_TERMINAL),
    objects,
    staticMap,
    stats,
    placement,
    shroud,
    sidebar: {
      credits: view.getInt32(sidebarSection.offset + 8, true),
      tiberium: view.getInt32(sidebarSection.offset + 16, true),
      entries: sidebarEntries,
    },
  };
}

function rootCombatants(snapshot, owner) {
  return snapshot.objects.filter((object) => (
    object.owner === owner
    && object.subObject === 0
    && object.strength > 0
    && ROOT_COMBAT_TYPES.has(object.type)
  ));
}

function availableAttackers(snapshot) {
  return rootCombatants(snapshot, HOUSE_GDI).filter((object) => (
    object.type !== 4 && Boolean(object.objectFlags & 1) && Boolean(object.canFireMask & (1 << HOUSE_GDI))
  ));
}

function objectKey(object) {
  return `${object.type}:${object.typeName}:${object.id}`;
}

function chooseTarget(hostiles) {
  return hostiles.toSorted((left, right) => (
    Number(left.type === 4) - Number(right.type === 4)
    || right.cellY - left.cellY
    || left.cellX - right.cellX
    || left.type - right.type
    || left.id - right.id
  ))[0];
}

const MISSION_THREE_STRUCTURE_PRIORITY = new Map([
  ["HAND", 0],
  ["FACT", 1],
  ["PROC", 2],
  ["GUN", 3],
  ["NUKE", 4],
  ["SAM", 5],
  ["SILO", 6],
]);

function chooseMissionThreeAssaultTarget(hostiles) {
  const mobile = hostiles.filter((object) => object.type !== 4);
  const westernGun = hostiles.find((object) => (
    object.typeName === "GUN" && object.cellX === 20 && object.cellY === 34
  ));
  if (westernGun) return westernGun;
  const production = hostiles.filter((object) => object.typeName === "FACT" || object.typeName === "HAND");
  if (production.length > 0) return production.toSorted((left, right) => (
    (MISSION_THREE_STRUCTURE_PRIORITY.get(left.typeName) ?? 100)
    - (MISSION_THREE_STRUCTURE_PRIORITY.get(right.typeName) ?? 100)
    || right.cellY - left.cellY
    || left.cellX - right.cellX
    || left.id - right.id
  ))[0];
  if (mobile.length > 12) return chooseTarget(mobile);
  return hostiles.toSorted((left, right) => (
    (MISSION_THREE_STRUCTURE_PRIORITY.get(left.typeName) ?? 100)
    - (MISSION_THREE_STRUCTURE_PRIORITY.get(right.typeName) ?? 100)
    || Number(left.type === 4) - Number(right.type === 4)
    || right.cellY - left.cellY
    || left.cellX - right.cellX
    || left.id - right.id
  ))[0];
}

function chooseMissionThreeDefenseTarget(hostiles) {
  return chooseTarget(hostiles.filter((object) => object.cellY >= 44 && object.cellX <= 28))
    ?? { cellX: 12, cellY: 52 };
}

function chooseMissionTwoDefenseTarget(hostiles) {
  return chooseTarget(hostiles.filter((object) => object.cellY >= 45));
}

const MISSION_FIVE_STRUCTURE_PRIORITY = new Map([
  ["FACT", 0],
  ["HAND", 1],
  ["AFLD", 2],
  ["PROC", 3],
  ["GUN", 4],
  ["SAM", 5],
  ["NUKE", 6],
  ["SILO", 7],
]);

const MISSION_FIVE_WEST_B_CORE_PRIORITY = new Map([
  ["FACT", 0],
  ["HAND", 1],
  ["AFLD", 2],
  ["NUKE", 3],
  ["PROC", 4],
]);

const MISSION_FIVE_WEST_B_CORE_SITES = new Set([
  "FACT:52:17",
  "HAND:41:22",
  "AFLD:42:18",
  "PROC:47:22",
  "NUKE:47:18",
  "NUKE:49:17",
]);

function chooseMissionFiveAssaultTarget(attackers, hostiles, preferStructures = false) {
  const localThreat = chooseFormationThreat(attackers, hostiles.filter((object) => (
    object.type !== 4 && (object.objectFlags & (1 << 12)) !== 0
  )), 7);
  if (localThreat) return localThreat;
  const nearbyGun = chooseFormationThreat(attackers, hostiles.filter((object) => object.typeName === "GUN"), 10);
  if (nearbyGun) return nearbyGun;
  const structureTarget = hostiles.filter((object) => object.type === 4).toSorted((left, right) => (
    (MISSION_FIVE_STRUCTURE_PRIORITY.get(left.typeName) ?? 50)
    - (MISSION_FIVE_STRUCTURE_PRIORITY.get(right.typeName) ?? 50)
    || left.strength - right.strength
    || left.cellY - right.cellY
    || left.cellX - right.cellX
    || left.type - right.type
    || left.id - right.id
  ))[0];
  if (preferStructures && structureTarget) return structureTarget;
  return structureTarget;
}

function chooseMissionFiveWestBAssaultTarget(
  attackers,
  hostiles,
  huntTriggered,
  requiredCoreSites,
  reserveHuntSwitch = false,
  cleanupWaypoint,
) {
  if (reserveHuntSwitch) {
    hostiles = hostiles.filter((object) => !(
      object.typeName === "NUKE" && object.cellX === 49 && object.cellY === 17
    ));
  }
  const engineerScreeningFactory = (
    missionFiveWestBEngineerPhase !== "captured"
    || (missionFiveShuttleFactCaptureTick !== undefined
      && missionFiveShuttleCaptures.length < 4)
  ) && requiredCoreSites?.some((site) => (
      site.typeName === "FACT" && site.cellX === 52 && site.cellY === 17
    ));
  if (engineerScreeningFactory) {
    const eastGun = hostiles.filter((object) => (
      object.typeName === "GUN"
      && object.cellY === 27
      && (object.cellX === 45 || object.cellX === 50)
    )).toSorted((left, right) => (
      left.strength - right.strength || right.cellX - left.cellX || left.id - right.id
    ))[0];
    if (eastGun) return eastGun;
    const eastMobileThreat = chooseFormationThreat(attackers, hostiles.filter((object) => (
      object.type !== 4
      && (object.objectFlags & (1 << 12)) !== 0
      && object.cellX >= 45
      && object.cellY <= 35
    )), 10);
    if (eastMobileThreat) return eastMobileThreat;
    // Keep the strike screen south-east of the factory. The engineer transport
    // owns the factory objective during this phase, so combat units must not
    // fall through to the normal FACT focus order.
    return { cellX: 55, cellY: 27 };
  }
  const nearbyGun = chooseFormationThreat(attackers, hostiles.filter((object) => (
    object.typeName === "GUN" && (huntTriggered || object.cellX >= 45)
  )), 10);
  if (nearbyGun) return nearbyGun;
  const coreStructure = hostiles.filter((object) => (
    object.type === 4
    && MISSION_FIVE_WEST_B_CORE_SITES.has(`${object.typeName}:${object.cellX}:${object.cellY}`)
  )).toSorted((left, right) => (
    MISSION_FIVE_WEST_B_CORE_PRIORITY.get(left.typeName)
    - MISSION_FIVE_WEST_B_CORE_PRIORITY.get(right.typeName)
    || left.strength - right.strength
    || left.cellY - right.cellY
    || left.cellX - right.cellX
    || left.id - right.id
  ))[0];
  if (!huntTriggered) {
    if (requiredCoreSites?.length > 0) {
      return hostiles.filter((hostile) => requiredCoreSites.some((site) => (
        hostile.typeName === site.typeName
        && hostile.cellX === site.cellX
        && hostile.cellY === site.cellY
      ))).toSorted((left, right) => (
        MISSION_FIVE_WEST_B_CORE_PRIORITY.get(left.typeName)
        - MISSION_FIVE_WEST_B_CORE_PRIORITY.get(right.typeName)
        || left.strength - right.strength
        || left.id - right.id
      ))[0];
    }
    if (coreStructure) return coreStructure;
    if (reserveHuntSwitch) {
      return chooseFormationThreat(attackers, hostiles.filter((object) => (
        object.type !== 4 && (object.objectFlags & (1 << 12)) !== 0
      )), 12)
        ?? chooseMissionFiveAssaultTarget(attackers, hostiles, true)
        ?? chooseTarget(hostiles)
        ?? cleanupWaypoint;
    }
    return undefined;
  }
  const localThreat = chooseFormationThreat(attackers, hostiles.filter((object) => (
    object.type !== 4 && (object.objectFlags & (1 << 12)) !== 0
  )), 7);
  if (localThreat) return localThreat;
  if (coreStructure) return coreStructure;
  return chooseMissionFiveAssaultTarget(attackers, hostiles, true);
}

function chooseMissionFiveDefenseTarget(hostiles, home) {
  return chooseTarget(hostiles.filter((object) => (
    Math.max(Math.abs(object.cellX - home.cellX), Math.abs(object.cellY - home.cellY)) <= 14
  )));
}

function chooseLocalThreat(attackers, hostiles, radius) {
  return hostiles
    .map((hostile) => ({
      hostile,
      distance: Math.min(...attackers.map((attacker) => Math.max(
        Math.abs(attacker.cellX - hostile.cellX),
        Math.abs(attacker.cellY - hostile.cellY),
      ))),
    }))
    .filter(({ distance }) => distance <= radius)
    .toSorted((left, right) => (
      left.distance - right.distance
      || Number(left.hostile.type === 4) - Number(right.hostile.type === 4)
      || left.hostile.strength - right.hostile.strength
      || left.hostile.id - right.hostile.id
    ))[0]?.hostile;
}

function chooseFormationThreat(attackers, hostiles, radius) {
  if (attackers.length === 0) return undefined;
  const requiredNearby = Math.min(4, Math.max(1, Math.ceil(attackers.length * 0.15)));
  return hostiles
    .map((hostile) => {
      const distances = attackers.map((attacker) => Math.max(
        Math.abs(attacker.cellX - hostile.cellX),
        Math.abs(attacker.cellY - hostile.cellY),
      )).toSorted((left, right) => left - right);
      return { hostile, distance: distances[requiredNearby - 1] };
    })
    .filter(({ distance }) => distance <= radius)
    .toSorted((left, right) => (
      left.distance - right.distance
      || Number(left.hostile.type === 4) - Number(right.hostile.type === 4)
      || left.hostile.strength - right.hostile.strength
      || left.hostile.id - right.hostile.id
    ))[0]?.hostile;
}

function decodePlacementOffset(offset) {
  const y = Math.floor((offset + 64) / 128);
  return { x: offset - y * 128, y };
}

function findLegalPlacement(snapshot, entry) {
  const grid = snapshot.placement;
  if (!grid || entry.placementOffsets.length === 0) return undefined;
  const gridIndex = (cellX, cellY) => {
    if (cellX < grid.cellX || cellY < grid.cellY
      || cellX >= grid.cellX + grid.width || cellY >= grid.cellY + grid.height) return undefined;
    return (cellY - grid.cellY) * grid.width + cellX - grid.cellX;
  };
  for (let cellY = grid.cellY; cellY < grid.cellY + grid.height; cellY += 1) {
    for (let cellX = grid.cellX; cellX < grid.cellX + grid.width; cellX += 1) {
      const anchorIndex = gridIndex(cellX, cellY);
      if (anchorIndex === undefined || !(grid.flags[anchorIndex] & 1)) continue;
      const legal = entry.placementOffsets.every((rawOffset) => {
        const offset = decodePlacementOffset(rawOffset);
        const footprintIndex = gridIndex(cellX + offset.x, cellY + offset.y);
        return footprintIndex !== undefined && Boolean(grid.flags[footprintIndex] & 2);
      });
      if (legal) return { x: cellX - grid.cellX, y: cellY - grid.cellY };
    }
  }
  return undefined;
}

function advance(handle, ticks) {
  const advanced = outputU32((output) => engine._cnc_web_advance(handle, ticks, output), "engine advance");
  assert.ok(advanced <= ticks, "engine advanced more ticks than requested");
  currentTick += advanced;
  drainEvents(handle);
  return advanced;
}

let handle = 0;
const startedAt = performance.now();
let commandBatches = 0;
let selectionCommands = 0;
let contextualOrders = 0;
let retargetCycles = 0;
let productionStarts = 0;
let infantryProductionStarts = 0;
let vehicleProductionStarts = 0;
let repairOrders = 0;
let deploymentOrders = 0;
let placementStarts = 0;
let placements = 0;
const repairedBuildingIds = new Set();
const missionTwoHomeGuardIds = new Set();
const missionThreeRepairTicks = new Map();
const startedMissionThreeStructures = new Set();
let missionThreeBaseAssaultStarted = false;
let missionThreeScoutId;
let missionThreeScoutStage = 0;
const missionThreeScoutArrivalTicks = [];
let missionThreeRouteStage = 0;
const missionThreeStrikeGroupIds = new Set();
let missionThreeAssaultStartedTick;
let missionFourRouteStage = 0;
const missionFourRouteArrivalTicks = [];
const missionFourExtractionKeys = new Set();
const missionFourCargoKeys = new Set();
let missionFourCargoLoadIssued = false;
let missionFourCargoSealed = false;
let missionFourCargoUnloadIssued = false;
let missionFourCargoUnloaded = false;
let missionFourVanguardStage = 0;
let missionFourScoutKey;
let missionFourScoutStage = 0;
const missionFourScoutArrivalTicks = [];
const missionFiveInitialForceKeys = new Set();
const missionFiveInitialFriendlyKeys = new Set();
const missionFiveCompletedInfantryKeys = new Set();
const missionFiveCompletedVehicleKeys = new Set();
const missionFiveRepairTicks = new Map();
const missionFiveSoldStructureIds = new Set();
let missionFiveReliefStage = 0;
const missionFiveReliefArrivalTicks = [];
let missionFiveRelievedTick;
let missionFiveBaseRepairedTick;
let missionFiveCrateRunnerKey;
let missionFiveCrateCollectedTick;
let missionFivePreviousFunds;
let missionFiveAssaultStartedTick;
const missionFiveStrikeGroupKeys = new Set();
const missionFiveHomeGuardKeys = new Set();
let missionFiveAssaultRouteStage = 0;
const missionFiveAssaultRouteArrivalTicks = [];
let missionFiveAssaultPhase = "staging";
let missionFiveAssaultWaveCount = 0;
let missionFiveWaveLaunchedTick;
let missionFiveAssaultProgressTick = 0;
let missionFiveLastForwardTargetKey;
let missionFiveLastForwardTargetStrength;
let missionFiveWestBRefineryScatterTick;
let missionFiveWestBEngineerPhase = "await-engineer";
let missionFiveWestBEngineerKey;
let missionFiveWestBEngineerProductionStarted = 0;
let missionFiveWestBApcKey;
let missionFiveWestBApcRouteStage = 0;
let missionFiveWestBLoadOrderTick = -Infinity;
let missionFiveWestBApcOrderTick = -Infinity;
let missionFiveWestBUnloadOrderTick = -Infinity;
let missionFiveWestBCaptureOrderTick = -Infinity;
let missionFiveWestBEngineerCaptureStage = 0;
let missionFiveWestBEngineerProducedTick;
let missionFiveWestBLoadTick;
let missionFiveWestBUnloadIssuedTick;
let missionFiveWestBEmptyPipsTick;
let missionFiveWestBEngineerRootTick;
let missionFiveWestBCaptureTick;
let missionFiveWestBInitialApcPipsLogged = false;
const missionFiveWestBEngineerTransitions = [];
let missionFiveShuttleFactCaptureTick;
let missionFiveShuttleFactSaleTick;
let missionFiveShuttleFactSaleFunds;
let missionFiveShuttleFactGoneTick;
let missionFiveShuttleFactGoneFunds;
let missionFiveShuttlePhase = "await-fact";
let missionFiveShuttleRouteStage = 0;
let missionFiveShuttleOrderTick = -Infinity;
let missionFiveShuttleUnloadTick = -Infinity;
let missionFiveShuttleEngineerStarts = 0;
let missionFiveShuttleEngineerStartTick = -Infinity;
let missionFiveShuttleEngineerResumeTick = -Infinity;
const missionFiveShuttleBaselineFriendlyKeys = new Set();
const missionFiveShuttleEngineers = new Map();
const missionFiveShuttleFactCrew = new Map();
const missionFiveShuttleAssignments = new Map();
const missionFiveShuttleRaidStages = new Map();
const missionFiveShuttleRaidOrderTicks = new Map();
const missionFiveShuttleCaptures = [];
const missionFiveShuttleCaptureKeys = new Set();
let missionFiveFootReservePhase = "waiting";
let missionFiveFootReserveRouteStage = 0;
let missionFiveFootReserveOrderTick = -Infinity;
let missionFiveFootEscortOrderTick = -Infinity;
let missionFiveFootReserveStagingTick;
const missionFiveFootReserveStagedEngineers = [];
let missionFiveWestBCleanupBatchTick;
let missionFiveWestBCleanupBatchSize = 0;
const missionFiveInitialHuntStructureKeys = new Set();
const missionFiveInitialSamStructureKeys = new Set();
let missionFiveSamSweepStage = 0;
const missionFiveSamDestroyedTicks = [];
const missionFiveAirstrikeReadyTicks = [];
const missionFiveAirstrikeOrders = [];
const missionFiveAirstrikeDischarges = [];
let missionFiveAirstrikeReadyLatched = false;
let missionFiveAirstrikePending;
let missionFiveHuntTriggeredTick;
const missionFiveKnownHostileStructures = new Map();
let missionFiveStaticSweepStartedTick;
let missionFiveStaticSweepForceOrderCycle = -1;
const missionFourProtectedVillageCells = new Set([
  "18:44",
  "19:46",
  "24:50",
  "25:53",
]);
let initialProtectedVillageCount = 0;
const initialProtectedVillageCells = new Set();
let westBPhase = "load";
let westBRouteStage = 0;
let westBCombatStage = 0;
let westBOwnUnloadIssued = false;
let westBWaitStarted;
let westBLoadIssued = false;
let westBLastScreenOrder;
let westBLastGrenadierOrder;
const westBGrenadierOrders = new Map();
const westBGrenadierOrderTicks = new Map();
const westBScatterTicks = new Map();
let westBJeepKey;
let westBReserveApcKey;
let westBReserveApcPositioned = false;
let westBReserveApcOrderTick = -Infinity;
let westBReserveApcCleanupTargetKey;
let westBTakeoverInitialized = false;
let westBTakeoverPhase = "support";
let westBTakeoverTargetKey;
let westBTakeoverOrderTick = -Infinity;
let westBTakeoverApcInitialStrength;
let westBTakeoverTargetInitialStrength;
let westBTakeoverTargetOrigin;
let westBTakeoverParkedTick;
let westBIntegratedOpeningNormalized = false;
let westBReinforcementsReady = false;
let westBLateAssaultStarted = false;
const westBInitialE2Keys = new Set();
let westBScreenRetreatStarted = false;
let westBScreenRetreatStage = 0;
let westBScreenContactTick;
let westBEndgameRouteStage = 0;
let westBEndgameRouteOrder;
let westBScreenOrder;
let westBFocusOrder;
let westBLureTankKey;
let westBLureDecoyKey;
let westBLurePhase = "approach";
let westBLureRetreatStage = 0;
let westBLureOrder;
let westBLureActivated = false;
let westBLureTargetOrigin;
let westBLureTargetInitialStrength;
let westBLureDecoyInitialStrength;
let westBLureProgressTick = -Infinity;
let westBLureProgressSignature;
const westBLureDecoyBlacklist = new Set();
let westBGuardPhase = "stage";
let westBGuardPhaseTick = 0;
let westBLineEstablished = false;
let westBGuardFocusing = false;
let westBAssaultTargetKey;
let westBAssaultAnchor;
let westBAssaultStartedTick;
let westBKiteActive = false;
let westBKiteTargetKey;
let westBKiteDecoyKey;
let westBKiteRetreatPoint;
let westBKiteOrderTick = -Infinity;
const westBCriticalReserveKeys = new Set();
const westBCriticalReservePoints = new Map();
const westBCriticalReserveOrderTicks = new Map();
let westBLastStand = false;
let westBIntegratedPhase = "opening";
const westBReserveLinePoints = new Map();
let westBReserveGuardOrderTick = -Infinity;
let westBReserveScreenOrderTick = -Infinity;
let westBReserveAssaultPhase = "support";
let westBReserveAssaultTargetKey;
let westBReserveAssaultPhaseTick = 0;
let westBReserveApcStopped = false;
let westBFinalCleanupRetreat = false;
let westBSupportClearOrderTick = -Infinity;
let westBSupportClearScreenTick = -Infinity;
const westBSupportClearE2Ticks = new Map();
let westBSupportClearPhase = "flank";
let westBSupportClearTargetKey;
let westBSupportClearRetreatStage = 0;
let westBLateTankPhase = "stage";
const westBLateTankOrderTicks = new Map();
let westBLateTankAttackTick = -Infinity;
let westBLateTankActive = false;
const westBStagingIndexes = new Map();
const westBGuardedGrenadiers = new Set();
const westBEndgameRoute = [
  { cellX: 46, cellY: 40 },
  { cellX: 46, cellY: 44 },
  { cellX: 45, cellY: 45 },
  { cellX: 44, cellY: 46 },
  { cellX: 43, cellY: 47 },
];
const westBForwardLine = [
  { cellX: 39, cellY: 36 },
  { cellX: 42, cellY: 36 },
  { cellX: 45, cellY: 36 },
  { cellX: 48, cellY: 36 },
  { cellX: 41, cellY: 33 },
  { cellX: 44, cellY: 33 },
  { cellX: 47, cellY: 33 },
];
const westBInitialVehicleKeys = new Set();
const westBTransitRoute = [
  { cellX: 29, cellY: 25 },
  { cellX: 34, cellY: 33 },
  { cellX: 38, cellY: 34 },
  { cellX: 40, cellY: 36 },
  { cellX: 46, cellY: 40 },
  { cellX: 46, cellY: 44 },
  { cellX: 45, cellY: 45 },
  { cellX: 44, cellY: 46 },
  { cellX: 43, cellY: 47 },
  { cellX: 43, cellY: 49 },
  { cellX: 43, cellY: 53 },
  { cellX: 34, cellY: 53 },
  { cellX: 30, cellY: 51 },
  { cellX: 31, cellY: 53 },
];
const westBCombatRoute = [
  { cellX: 31, cellY: 55 },
  { cellX: 42, cellY: 55 },
  { cellX: 52, cellY: 55 },
  { cellX: 55, cellY: 43 },
  { cellX: 55, cellY: 31 },
  { cellX: 50, cellY: 22 },
  { cellX: 43, cellY: 17 },
  { cellX: 34, cellY: 18 },
  { cellX: 27, cellY: 24 },
  { cellX: 23, cellY: 29 },
  { cellX: 31, cellY: 34 },
  { cellX: 42, cellY: 35 },
  { cellX: 53, cellY: 36 },
];
const missionThreeScoutRoute = [
  { cellX: 17, cellY: 43 },
  { cellX: 17, cellY: 41 },
  { cellX: 10, cellY: 54 },
];
const missionThreeAssaultRoute = [
  { cellX: 17, cellY: 43 },
  { cellX: 17, cellY: 41 },
  { cellX: 14, cellY: 35 },
  { cellX: 20, cellY: 34 },
];
function transitionMissionFiveWestBEngineer(nextPhase, tick, detail = {}) {
  if (missionFiveWestBEngineerPhase === nextPhase) return;
  const transition = { tick, from: missionFiveWestBEngineerPhase, to: nextPhase, ...detail };
  missionFiveWestBEngineerTransitions.push(transition);
  missionFiveWestBEngineerPhase = nextPhase;
  if (trace) console.error(JSON.stringify({ westBEngineerTransition: transition }));
}
let initialFriendly = 0;
let initialHostiles = 0;
let peakFriendly = 0;
let peakHostiles = 0;
let finalSnapshot;
try {
  const handlePointer = withAllocation(4, "handle output");
  try {
    assert.equal(engine._cnc_web_create(2, handlePointer), STATUS_OK, "cnc_web_create failed");
    handle = new DataView(engine.HEAPU8.buffer).getUint32(handlePointer, true);
    assert.notEqual(handle, 0, "cnc_web_create returned an invalid handle");
  } finally {
    engine._free(handlePointer);
  }

  const startBytes = startMessage();
  assert.equal(
    writeInput(startBytes, (pointer, length) => engine._cnc_web_start(handle, pointer, length)),
    STATUS_OK,
    `classic-freeware GDI Mission ${mission.number} start failed`,
  );
  drainEvents(handle);
  assert.ok(events.some((event) => event.type === EVENT_DIAGNOSTIC && event.args[0] === DIAGNOSTIC_START_READY), "mission start emitted no ready diagnostic");

  const restartFromBytes = (bytes, label) => {
    assert.equal(engine._cnc_web_destroy(handle), STATUS_OK, `${label} destroy failed`);
    const replacementHandlePointer = withAllocation(4, `${label} handle output`);
    try {
      assert.equal(engine._cnc_web_create(2, replacementHandlePointer), STATUS_OK,
        `${label} create failed`);
      handle = new DataView(engine.HEAPU8.buffer).getUint32(replacementHandlePointer, true);
      assert.notEqual(handle, 0, `${label} create returned an invalid handle`);
    } finally {
      engine._free(replacementHandlePointer);
    }
    assert.equal(
      writeInput(startBytes, (pointer, length) => engine._cnc_web_start(handle, pointer, length)),
      STATUS_OK,
      `${label} replacement start failed`,
    );
    assert.equal(
      writeInput(bytes, (pointer, length) => engine._cnc_web_load_save(handle, pointer, length)),
      STATUS_OK,
      `${label} reload failed`,
    );
    drainEvents(handle);
    const loaded = readSnapshot(handle);
    assert.equal(loaded.tick, currentTick, `${label} tick changed during reload`);
    return loaded;
  };

  let snapshot = readSnapshot(handle);
  initialFriendly = rootCombatants(snapshot, HOUSE_GDI).length;
  initialHostiles = rootCombatants(snapshot, HOUSE_NOD).length;
  if (mission.number === 5) {
    missionFivePreviousFunds = snapshot.sidebar.credits + snapshot.sidebar.tiberium;
    for (const object of rootCombatants(snapshot, HOUSE_GDI)) {
      missionFiveInitialFriendlyKeys.add(objectKey(object));
    }
    for (const attacker of availableAttackers(snapshot)) {
      missionFiveInitialForceKeys.add(objectKey(attacker));
    }
    const authoredHuntSites = new Set(mission.huntSites.map(({ typeName, cellX, cellY }) => (
      `${typeName}:${cellX}:${cellY}`
    )));
    const authoredSamSites = new Set(mission.samSites.map(({ site: { cellX, cellY } }) => (
      `SAM:${cellX}:${cellY}`
    )));
    for (const structure of rootCombatants(snapshot, HOUSE_NOD).filter((object) => object.type === 4)) {
      const site = `${structure.typeName}:${structure.cellX}:${structure.cellY}`;
      if (authoredHuntSites.has(site)) missionFiveInitialHuntStructureKeys.add(objectKey(structure));
      if (authoredSamSites.has(site)) missionFiveInitialSamStructureKeys.add(objectKey(structure));
    }
    if (trace) {
      console.error(JSON.stringify({
        initialMissionFiveStructures: rootCombatants(snapshot, HOUSE_NOD)
          .filter((object) => object.type === 4)
          .map(({ typeName, id, cellX, cellY }) => ({ typeName, id, cellX, cellY })),
      }));
    }
    assert.equal(missionFiveInitialHuntStructureKeys.size, authoredHuntSites.size,
      `GDI Mission 5 ${mission.variant} authored hunt-trigger structures changed`);
    assert.equal(missionFiveInitialSamStructureKeys.size, authoredSamSites.size,
      `GDI Mission 5 ${mission.variant} authored Air Strike SAM structures changed`);
  }
  if (mission.number === 4 && mission.variant === "west-b") {
    const protectedVillage = snapshot.objects.filter((object) => (
      object.owner === HOUSE_NEUTRAL
      && object.subObject === 0
      && object.type === 4
      && object.strength > 0
      && missionFourProtectedVillageCells.has(`${object.cellX}:${object.cellY}`)
    ));
    initialProtectedVillageCount = protectedVillage.length;
    for (const structure of protectedVillage) {
      initialProtectedVillageCells.add(`${structure.cellX}:${structure.cellY}`);
    }
  }
  assert.ok(initialFriendly > 0, `GDI Mission ${mission.number} started with no friendly combatants`);
  assert.ok(initialHostiles > 0, `GDI Mission ${mission.number} started with no Nod combatants`);

  while (!snapshot.terminal && snapshot.tick < MAX_TICKS) {
    if (mission.number === 4 && mission.variant === "west-b" && !westBIntegratedOpeningNormalized) {
      const boundaryFriendly = rootCombatants(snapshot, HOUSE_GDI);
      const boundaryHostiles = rootCombatants(snapshot, HOUSE_NOD);
      const openingBoundary = boundaryHostiles.length <= 7
        && boundaryHostiles.some((object) => object.typeName === "E3")
        && boundaryFriendly.some((object) => object.typeName === "APC"
          && object.strength === object.maxStrength)
        && boundaryFriendly.filter((object) => object.typeName === "E2").length >= 5;
      if (openingBoundary) {
        const normalized = readOutput(handle, engine._cnc_web_save_size,
          engine._cnc_web_write_save, "opening boundary save");
        snapshot = restartFromBytes(normalized, "opening-to-takeover");
        westBPhase = "combat";
        westBRouteStage = westBTransitRoute.length;
        westBCombatStage = 1;
        westBReinforcementsReady = true;
        westBLineEstablished = true;
        westBGuardPhase = "stage";
        westBTakeoverInitialized = false;
        westBTakeoverPhase = "support";
        westBTakeoverTargetKey = undefined;
        westBTakeoverOrderTick = -Infinity;
        westBTakeoverParkedTick = undefined;
        const loadedReserveApc = availableAttackers(snapshot).filter((attacker) => attacker.typeName === "APC")
          .toSorted((left, right) => right.strength - left.strength || left.id - right.id)[0];
        westBReserveApcKey = loadedReserveApc && objectKey(loadedReserveApc);
        westBReserveApcPositioned = Boolean(loadedReserveApc);
        westBReserveApcOrderTick = -Infinity;
        westBIntegratedOpeningNormalized = true;
        westBIntegratedPhase = "takeover";
        continue;
      }
    }
    if (mission.number === 4 && mission.variant === "west-b" && westBIntegratedPhase === "takeover") {
      const boundaryFriendly = rootCombatants(snapshot, HOUSE_GDI);
      const boundaryHostiles = rootCombatants(snapshot, HOUSE_NOD);
      const tankBoundary = boundaryHostiles.length === 5
        && boundaryFriendly.some((object) => object.typeName === "APC"
          && object.strength === object.maxStrength)
        && boundaryFriendly.filter((object) => object.typeName === "E2").length >= 5;
      if (tankBoundary) {
        const normalized = readOutput(handle, engine._cnc_web_save_size,
          engine._cnc_web_write_save, "tank boundary save");
        snapshot = restartFromBytes(normalized, "takeover-to-tank6");
        westBIntegratedPhase = "tank6";
        westBPhase = "integrated";
        westBGuardPhase = "stage";
        westBGuardPhaseTick = snapshot.tick;
        westBReserveLinePoints.clear();
        westBReserveGuardOrderTick = -Infinity;
        westBReserveScreenOrderTick = -Infinity;
        westBReserveAssaultPhase = "support";
        westBReserveAssaultTargetKey = undefined;
        westBReserveAssaultPhaseTick = snapshot.tick;
        westBReserveApcStopped = false;
        westBFinalCleanupRetreat = false;
        westBStagingIndexes.clear();
        westBGuardedGrenadiers.clear();
        westBGrenadierOrderTicks.clear();
        westBReserveApcOrderTick = -Infinity;
        const loadedReserveApc = availableAttackers(snapshot).filter((attacker) => attacker.typeName === "APC")
          .toSorted((left, right) => right.strength - left.strength || left.id - right.id)[0];
        westBReserveApcKey = loadedReserveApc && objectKey(loadedReserveApc);
        continue;
      }
    }
    if (mission.number === 4 && mission.variant === "west-b" && westBIntegratedPhase === "tank6") {
      const boundaryFriendly = rootCombatants(snapshot, HOUSE_GDI);
      const boundaryHostiles = rootCombatants(snapshot, HOUSE_NOD);
      const cleanupBoundary = boundaryHostiles.filter((object) => object.typeName === "LTNK").length === 1
        && boundaryHostiles.some((object) => object.typeName === "E1")
        && boundaryFriendly.some((object) => object.typeName === "APC")
        && boundaryFriendly.filter((object) => object.typeName === "E2").length >= 4;
      if (cleanupBoundary) {
        const normalized = readOutput(handle, engine._cnc_web_save_size,
          engine._cnc_web_write_save, "cleanup boundary save");
        snapshot = restartFromBytes(normalized, "tank6-to-cleanup");
        westBIntegratedPhase = "cleanup";
        westBPhase = "integrated";
        westBGuardPhase = "stage";
        westBGuardPhaseTick = snapshot.tick;
        westBReserveLinePoints.clear();
        westBReserveGuardOrderTick = -Infinity;
        westBReserveScreenOrderTick = -Infinity;
        westBReserveAssaultPhase = "support";
        westBReserveAssaultTargetKey = undefined;
        westBReserveAssaultPhaseTick = snapshot.tick;
        westBReserveApcStopped = false;
        westBFinalCleanupRetreat = false;
        westBStagingIndexes.clear();
        westBGuardedGrenadiers.clear();
        westBGrenadierOrderTicks.clear();
        westBReserveApcOrderTick = -Infinity;
        continue;
      }
    }
    if (mission.number === 4 && mission.variant === "west-b"
      && (westBIntegratedPhase === "tank6" || westBIntegratedPhase === "cleanup")) {
      const boundaryFriendly = rootCombatants(snapshot, HOUSE_GDI);
      const boundaryHostiles = rootCombatants(snapshot, HOUSE_NOD);
      const supportClearBoundary = boundaryHostiles.length === 2
        && boundaryHostiles.some((object) => object.typeName === "LTNK")
        && boundaryHostiles.some((object) => object.typeName === "BGGY")
        && !boundaryHostiles.some((object) => object.typeName === "E1")
        && boundaryFriendly.some((object) => object.typeName === "APC")
        && boundaryFriendly.filter((object) => object.typeName === "E2").length >= 3;
      if (supportClearBoundary) {
        const normalized = readOutput(handle, engine._cnc_web_save_size,
          engine._cnc_web_write_save, "support clear boundary save");
        snapshot = restartFromBytes(normalized, "tank6-to-support-clear");
        westBIntegratedPhase = "support-clear";
        westBPhase = "integrated";
        westBSupportClearOrderTick = -Infinity;
        westBSupportClearScreenTick = -Infinity;
        westBSupportClearE2Ticks.clear();
        westBSupportClearPhase = "flank";
        westBSupportClearTargetKey = undefined;
        westBSupportClearRetreatStage = 0;
        westBLateTankPhase = "stage";
        westBLateTankOrderTicks.clear();
        westBLateTankAttackTick = -Infinity;
        westBLateTankActive = false;
        continue;
      }
    }
    const friendly = rootCombatants(snapshot, HOUSE_GDI);
    const hostiles = rootCombatants(snapshot, HOUSE_NOD);
    const missionFiveAirstrike = mission.number === 5
      ? snapshot.sidebar.entries.find((entry) => entry.assetName === "SW_AirStrike")
      : undefined;
    if (mission.number === 5) {
      const availableFunds = snapshot.sidebar.credits + snapshot.sidebar.tiberium;
      if (mission.crate
        && missionFiveCrateRunnerKey !== undefined
        && missionFiveCrateCollectedTick === undefined
        && missionFivePreviousFunds !== undefined
        && availableFunds - missionFivePreviousFunds >= 1_000) {
        missionFiveCrateCollectedTick = snapshot.tick;
      }
      missionFivePreviousFunds = availableFunds;
      if (missionFiveCrateCollectedTick === undefined
        && missionFiveCrateRunnerKey !== undefined
        && !friendly.some((object) => objectKey(object) === missionFiveCrateRunnerKey)) {
        missionFiveCrateRunnerKey = undefined;
      }
      if (missionFiveAirstrike) {
        assert.equal(missionFiveAirstrike.buildableType, 24,
          `GDI Mission 5 ${mission.variant} Air Strike buildable type changed`);
        assert.equal(missionFiveAirstrike.buildableId, 3,
          `GDI Mission 5 ${mission.variant} Air Strike buildable id changed`);
        assert.equal(missionFiveAirstrike.objectType, 11,
          `GDI Mission 5 ${mission.variant} Air Strike object type changed`);
        assert.equal(missionFiveAirstrike.superweaponType, 3,
          `GDI Mission 5 ${mission.variant} Air Strike superweapon type changed`);
        if (missionFiveAirstrike.completed && !missionFiveAirstrikeReadyLatched) {
          missionFiveAirstrikeReadyTicks.push(snapshot.tick);
          missionFiveAirstrikeReadyLatched = true;
        } else if (!missionFiveAirstrike.completed) {
          missionFiveAirstrikeReadyLatched = false;
        }
      }
      if (missionFiveAirstrikePending) {
        const pendingTarget = hostiles.find((hostile) => (
          objectKey(hostile) === missionFiveAirstrikePending.targetKey
        ));
        const discharged = missionFiveAirstrike && !missionFiveAirstrike.completed;
        const a10Observed = friendly.some((object) => object.type === 3 && object.typeName === "A10");
        const targetDamaged = !pendingTarget || pendingTarget.strength < missionFiveAirstrikePending.targetStrength;
        if (discharged && (a10Observed || targetDamaged)) {
          missionFiveAirstrikeDischarges.push({
            orderTick: missionFiveAirstrikePending.orderTick,
            effectTick: snapshot.tick,
            target: missionFiveAirstrikePending.targetType,
            a10Observed,
            targetDamaged,
          });
          missionFiveAirstrikePending = undefined;
        }
      }
      for (const object of friendly) {
        const key = objectKey(object);
        if (missionFiveInitialFriendlyKeys.has(key)) continue;
        if (object.type === 1) missionFiveCompletedInfantryKeys.add(key);
        if (object.type === 2 && ["MTNK", "APC", "JEEP"].includes(object.typeName)) {
          missionFiveCompletedVehicleKeys.add(key);
        }
      }
    }
    if (mission.number === 5 && missionFiveBaseRepairedTick === undefined) {
      const baseStructures = friendly.filter((object) => object.type === 4);
      if (baseStructures.length === 7
        && baseStructures.every((structure) => structure.strength >= structure.maxStrength * 0.7)) {
        missionFiveBaseRepairedTick = snapshot.tick;
      }
    }
    if (mission.number === 5 && missionFiveHuntTriggeredTick === undefined
      && [...missionFiveInitialHuntStructureKeys].every((key) => (
        !hostiles.some((hostile) => objectKey(hostile) === key)
      ))) {
      missionFiveHuntTriggeredTick = snapshot.tick;
    }
    peakFriendly = Math.max(peakFriendly, friendly.length);
    peakHostiles = Math.max(peakHostiles, hostiles.length);
    const allAttackers = availableAttackers(snapshot);
    if (mission.number === 5 && mission.variant === "west-b") {
      const liveApcs = friendly.filter((object) => object.typeName === "APC" && object.type === 2);
      if (!missionFiveWestBInitialApcPipsLogged && liveApcs.length > 0) {
        missionFiveWestBInitialApcPipsLogged = true;
        const initialApcPips = liveApcs.map((apc) => ({
          key: objectKey(apc),
          strength: apc.strength,
          cellX: apc.cellX,
          cellY: apc.cellY,
          pipCount: apc.pipCount,
          maxPips: apc.maxPips,
          pips: apc.pips.slice(0, apc.pipCount),
        }));
        assert.ok(initialApcPips.every(({ pipCount, maxPips }) => pipCount === maxPips && maxPips === 5),
          "West-B APC pip slot export changed");
        if (trace) console.error(JSON.stringify({ westBInitialApcPips: initialApcPips }));
      }

      const visibleEngineer = friendly.find((object) => (
        object.typeName === "E6" && object.type === 1 && object.subObject === 0
      ));
      if (missionFiveReliefStage >= mission.reliefRoute.length
        && missionFiveWestBApcKey === undefined
        && missionFiveWestBEngineerKey === undefined) {
        const startingApc = liveApcs.filter((apc) => missionFiveInitialFriendlyKeys.has(objectKey(apc)))
          .toSorted((left, right) => right.strength - left.strength || left.id - right.id)[0];
        if (startingApc) {
          missionFiveWestBApcKey = objectKey(startingApc);
          transitionMissionFiveWestBEngineer("prepark", snapshot.tick, {
            apc: missionFiveWestBApcKey,
            startingApc: true,
            apcStrength: startingApc.strength,
          });
          missionFiveWestBApcOrderTick = -Infinity;
        }
      }
      if (visibleEngineer && missionFiveWestBEngineerKey === undefined) {
        missionFiveWestBEngineerKey = objectKey(visibleEngineer);
        missionFiveWestBEngineerProducedTick = snapshot.tick;
      }
      if (visibleEngineer && missionFiveWestBApcKey !== undefined
        && missionFiveWestBEngineerPhase === "parked-await-engineer") {
        transitionMissionFiveWestBEngineer("docking", snapshot.tick, {
          engineer: missionFiveWestBEngineerKey,
          apc: missionFiveWestBApcKey,
        });
        missionFiveWestBLoadOrderTick = -Infinity;
      }
      if (missionFiveWestBEngineerKey !== undefined && missionFiveWestBApcKey === undefined) {
        const apc = liveApcs.toSorted((left, right) => (
          Number(!missionFiveInitialFriendlyKeys.has(objectKey(left)))
          - Number(!missionFiveInitialFriendlyKeys.has(objectKey(right)))
          || right.strength - left.strength
          || Math.max(Math.abs(left.cellX - mission.home.cellX), Math.abs(left.cellY - mission.home.cellY))
            - Math.max(Math.abs(right.cellX - mission.home.cellX), Math.abs(right.cellY - mission.home.cellY))
          || left.id - right.id
        ))[0];
        if (apc) {
          missionFiveWestBApcKey = objectKey(apc);
          transitionMissionFiveWestBEngineer("parking", snapshot.tick, {
            engineer: missionFiveWestBEngineerKey,
            apc: missionFiveWestBApcKey,
            startingApc: missionFiveInitialFriendlyKeys.has(missionFiveWestBApcKey),
            apcStrength: apc.strength,
          });
          missionFiveWestBApcOrderTick = -Infinity;
          missionFiveWestBLoadOrderTick = -Infinity;
        }
      }
      const reservedApc = friendly.find((object) => objectKey(object) === missionFiveWestBApcKey);
      const occupiedPips = reservedApc?.pips.slice(0, reservedApc.pipCount).filter((pip) => pip !== 0) ?? [];
      if (missionFiveWestBEngineerPhase === "docking" && occupiedPips.includes(5)) {
        missionFiveWestBLoadTick = snapshot.tick;
        transitionMissionFiveWestBEngineer("escort", snapshot.tick, {
          pipCount: reservedApc.pipCount,
          occupiedPips,
        });
      }
      if (missionFiveWestBApcKey !== undefined
        && !reservedApc
        && !["capture", "captured"].includes(missionFiveWestBEngineerPhase)) {
        const lostApc = missionFiveWestBApcKey;
        missionFiveWestBApcKey = undefined;
        transitionMissionFiveWestBEngineer(visibleEngineer ? "await-apc" : "await-engineer", snapshot.tick, { lostApc });
      }
      const capturedFactory = friendly.find((object) => (
        object.typeName === "FACT" && object.type === 4 && object.cellX === 52 && object.cellY === 17
      ));
      if (capturedFactory && missionFiveWestBEngineerPhase !== "captured") {
        missionFiveWestBCaptureTick = snapshot.tick;
        transitionMissionFiveWestBEngineer("captured", snapshot.tick, {
          factory: objectKey(capturedFactory),
          strength: capturedFactory.strength,
        });
      }
      if (missionFiveShuttleFactCaptureTick === undefined) {
        for (const engineer of friendly.filter((object) => (
          object.type === 1 && object.typeName === "E6"
          && objectKey(object) !== missionFiveWestBEngineerKey
        ))) {
          const key = objectKey(engineer);
          if (missionFiveShuttleEngineers.has(key)) continue;
          missionFiveShuttleEngineers.set(key, {
            tick: snapshot.tick, id: engineer.id, strength: engineer.strength,
            cellX: engineer.cellX, cellY: engineer.cellY,
          });
          if (trace) console.error(JSON.stringify({ westBFootReserveEngineer: {
            tick: snapshot.tick, key, id: engineer.id, strength: engineer.strength,
            cellX: engineer.cellX, cellY: engineer.cellY,
            funds: snapshot.sidebar.credits + snapshot.sidebar.tiberium,
          } }));
        }
      }
      if (capturedFactory && missionFiveShuttleFactCaptureTick === undefined) {
        missionFiveShuttleFactCaptureTick = snapshot.tick;
        missionFiveShuttlePhase = "raid";
        missionFiveShuttleRouteStage = missionFiveFootReserveRouteStage;
        missionFiveShuttleOrderTick = -Infinity;
        for (const object of friendly) missionFiveShuttleBaselineFriendlyKeys.add(objectKey(object));
        if (trace) console.error(JSON.stringify({ westBShuttleFactCapture: {
          tick: snapshot.tick,
          key: objectKey(capturedFactory),
          strength: capturedFactory.strength,
          funds: snapshot.sidebar.credits + snapshot.sidebar.tiberium,
          sidebar: snapshot.sidebar.entries.map((entry) => ({
            assetName: entry.assetName,
            cost: entry.cost,
            buildableType: entry.buildableType,
            buildableId: entry.buildableId,
            objectType: entry.objectType,
            completed: entry.completed,
            constructing: entry.constructing,
            onHold: entry.onHold,
            busy: entry.busy,
          })),
        } }));
      }
      if (missionFiveShuttleFactSaleTick !== undefined
        && missionFiveShuttleFactGoneTick === undefined && !capturedFactory) {
        missionFiveShuttleFactGoneTick = snapshot.tick;
        missionFiveShuttleFactGoneFunds = snapshot.sidebar.credits + snapshot.sidebar.tiberium;
        if (trace) console.error(JSON.stringify({ westBShuttleFactSaleComplete: {
          tick: snapshot.tick,
          funds: missionFiveShuttleFactGoneFunds,
          refund: missionFiveShuttleFactGoneFunds - missionFiveShuttleFactSaleFunds,
          crew: [...missionFiveShuttleFactCrew.values()],
          huntTriggeredTick: missionFiveHuntTriggeredTick,
        } }));
      }
      if (missionFiveShuttleFactCaptureTick !== undefined) {
        for (const object of friendly.filter((candidate) => (
          !missionFiveShuttleBaselineFriendlyKeys.has(objectKey(candidate))
          && Math.max(Math.abs(candidate.cellX - 52), Math.abs(candidate.cellY - 17)) <= 4
        ))) {
          const key = objectKey(object);
          if (!missionFiveShuttleFactCrew.has(key)) {
            missionFiveShuttleFactCrew.set(key, {
              tick: snapshot.tick,
              typeName: object.typeName,
              id: object.id,
              strength: object.strength,
              cellX: object.cellX,
              cellY: object.cellY,
            });
          }
        }
        for (const engineer of friendly.filter((object) => (
          object.type === 1 && object.typeName === "E6"
          && !missionFiveShuttleBaselineFriendlyKeys.has(objectKey(object))
        ))) {
          const key = objectKey(engineer);
          if (!missionFiveShuttleEngineers.has(key)) {
            missionFiveShuttleEngineers.set(key, {
              tick: snapshot.tick,
              id: engineer.id,
              strength: engineer.strength,
              cellX: engineer.cellX,
              cellY: engineer.cellY,
            });
            if (trace) console.error(JSON.stringify({ westBShuttleEngineerBuilt: {
              tick: snapshot.tick,
              key,
              id: engineer.id,
              strength: engineer.strength,
              cellX: engineer.cellX,
              cellY: engineer.cellY,
              funds: snapshot.sidebar.credits + snapshot.sidebar.tiberium,
            } }));
          }
        }
        const raidSites = [
          { typeName: "PROC", cellX: 47, cellY: 22 },
          { typeName: "AFLD", cellX: 42, cellY: 18 },
          { typeName: "NUKE", cellX: 47, cellY: 18 },
          { typeName: "NUKE", cellX: 49, cellY: 17 },
        ];
        for (const site of raidSites) {
          const key = `${site.typeName}:${site.cellX}:${site.cellY}`;
          if (missionFiveShuttleCaptureKeys.has(key)) continue;
          const structure = friendly.find((object) => (
            object.type === 4 && object.typeName === site.typeName
            && object.cellX === site.cellX && object.cellY === site.cellY
          ));
          if (!structure) continue;
          missionFiveShuttleCaptureKeys.add(key);
          const capture = {
            tick: snapshot.tick,
            typeName: structure.typeName,
            id: structure.id,
            strength: structure.strength,
            cellX: structure.cellX,
            cellY: structure.cellY,
          };
          missionFiveShuttleCaptures.push(capture);
          if (trace) console.error(JSON.stringify({ westBShuttleCapture: capture }));
        }
      }
    }
    const missionFiveAvailableAttackers = mission.number === 5
      && mission.variant === "west-b"
      && missionFiveWestBApcKey !== undefined
      && missionFiveWestBEngineerPhase !== "captured"
      ? allAttackers.filter((attacker) => (
        objectKey(attacker) !== missionFiveWestBApcKey
        && objectKey(attacker) !== missionFiveWestBEngineerKey
        && !missionFiveShuttleEngineers.has(objectKey(attacker))
      ))
      : allAttackers.filter((attacker) => (
        objectKey(attacker) !== missionFiveWestBEngineerKey
        && !missionFiveShuttleEngineers.has(objectKey(attacker))
      ));
    const attackers = missionFiveShuttleFactCaptureTick !== undefined
      && missionFiveShuttleCaptures.length < 4
      ? missionFiveAvailableAttackers.filter((attacker) => (
        objectKey(attacker) !== missionFiveWestBApcKey
        && !missionFiveShuttleEngineers.has(objectKey(attacker))
        && !(attacker.type === 1 && attacker.typeName === "E6"
          && !missionFiveShuttleBaselineFriendlyKeys.has(objectKey(attacker)))
      ))
      : missionFiveAvailableAttackers;
    const missionFiveInitialForce = mission.number === 5
      ? attackers.filter((attacker) => missionFiveInitialForceKeys.has(objectKey(attacker)))
      : [];
    if (mission.number === 5 && missionFiveReliefStage < mission.reliefRoute.length) {
      const waypoint = mission.reliefRoute[missionFiveReliefStage];
      if (missionFiveInitialForce.some((attacker) => (
        Math.abs(attacker.cellX - waypoint.cellX) <= 1
        && Math.abs(attacker.cellY - waypoint.cellY) <= 1
      ))) {
        missionFiveReliefArrivalTicks.push(snapshot.tick);
        missionFiveReliefStage += 1;
        if (missionFiveReliefStage === mission.reliefRoute.length) missionFiveRelievedTick = snapshot.tick;
      }
    }
    if (mission.number === 4 && mission.scoutRoute && missionFourScoutKey === undefined) {
      const scout = attackers.filter((attacker) => attacker.typeName === mission.scoutType)
        .toSorted((left, right) => right.strength - left.strength || left.id - right.id)[0];
      if (scout) missionFourScoutKey = objectKey(scout);
    }
    const missionFourScout = mission.number === 4 && mission.scoutRoute
      ? attackers.find((attacker) => objectKey(attacker) === missionFourScoutKey)
      : undefined;
    const activeMissionFourScout = missionFourScoutStage < (mission.scoutRoute?.length ?? 0)
      ? missionFourScout
      : undefined;
    const missionFourCombatAttackers = missionFourScout
      ? attackers.filter((attacker) => attacker !== missionFourScout)
      : attackers;
    if (mission.number === 4 && mission.objective === "extract" && missionFourExtractionKeys.size === 0) {
      const candidates = mission.runner === "jeep" || mission.runner === "apc"
        ? attackers.filter((attacker) => attacker.typeName === mission.runner.toUpperCase())
        : mission.runner === "apc-one"
          ? attackers.filter((attacker) => attacker.typeName === "APC")
            .toSorted((left, right) => right.strength - left.strength || left.id - right.id)
            .slice(0, 1)
          : mission.runner === "vehicles"
            ? attackers.filter((attacker) => attacker.type === 2)
          : attackers;
      for (const candidate of candidates) missionFourExtractionKeys.add(objectKey(candidate));
    }
    if (mission.number === 4 && mission.variant === "east-a"
      && missionFourCargoKeys.size === 0 && !missionFourCargoLoadIssued) {
      const rocketSoldiers = attackers.filter((attacker) => attacker.typeName === "E2")
        .toSorted((left, right) => left.id - right.id);
      const cargo = rocketSoldiers.length >= 2
        ? [rocketSoldiers[0], rocketSoldiers.at(-1)]
        : rocketSoldiers;
      for (const passenger of cargo) missionFourCargoKeys.add(objectKey(passenger));
    }
    if (mission.number === 2) {
      for (const id of missionTwoHomeGuardIds) {
        if (!attackers.some((attacker) => attacker.id === id)) missionTwoHomeGuardIds.delete(id);
      }
      const guardCandidates = attackers
        .filter((attacker) => !missionTwoHomeGuardIds.has(attacker.id))
        .toSorted((left, right) => (
          right.cellY - left.cellY
          || Math.abs(left.cellX - 55) - Math.abs(right.cellX - 55)
          || left.id - right.id
        ));
      for (const candidate of guardCandidates) {
        if (missionTwoHomeGuardIds.size >= 8) break;
        missionTwoHomeGuardIds.add(candidate.id);
      }
    }
    const visibleHostiles = (mission.number === 4 && mission.objective === "eliminate") || mission.number === 5
      ? hostiles.filter((hostile) => snapshot.shroud.isVisible(hostile.cellX, hostile.cellY))
      : hostiles;
    if (mission.number === 5) {
      for (const structure of visibleHostiles.filter((hostile) => hostile.type === 4)) {
        missionFiveKnownHostileStructures.set(objectKey(structure), structure);
      }
      for (const key of missionFiveKnownHostileStructures.keys()) {
        if (!hostiles.some((hostile) => objectKey(hostile) === key)) {
          missionFiveKnownHostileStructures.delete(key);
        }
      }
    }
    const target = mission.number === 4 && mission.objective === "eliminate"
      ? chooseLocalThreat(missionFourCombatAttackers, visibleHostiles, mission.threatRadius)
      : chooseTarget(visibleHostiles);
    if (trace && snapshot.tick % 600 === 0) {
      console.error(JSON.stringify({
        tick: snapshot.tick,
        friendly: friendly.length,
        attackers: attackers.length,
        hostile: hostiles.length,
        visibleHostile: visibleHostiles.length,
        target: target && { typeName: target.typeName, id: target.id, strength: target.strength, cellX: target.cellX, cellY: target.cellY },
        buildings: friendly.filter((object) => object.type === 4).map(({ typeName, id, strength, cellX, cellY }) => ({ typeName, id, strength, cellX, cellY })),
        credits: snapshot.sidebar.credits,
        productionStarts,
        missionThreeScoutStage,
        missionThreeBaseAssaultStarted,
        missionThreeRouteStage,
        missionThreeStrikeGroup: missionThreeStrikeGroupIds.size,
        missionFourRouteStage,
        missionFourScoutStage,
        missionFiveReliefStage,
        missionFiveRelievedTick,
        missionFiveBaseRepairedTick,
        missionFiveCrateCollectedTick,
        missionFiveCrateRunner: friendly.find((object) => objectKey(object) === missionFiveCrateRunnerKey),
        missionFiveAssaultStartedTick,
        missionFiveAssaultPhase,
        missionFiveAssaultWaveCount,
        missionFiveAssaultRouteStage,
        missionFiveHuntTriggeredTick,
        missionFiveSamSweepStage,
        missionFiveAirstrikeOrders: missionFiveAirstrikeOrders.length,
        missionFiveAirstrikeDischarges: missionFiveAirstrikeDischarges.length,
        missionFiveStrikeGroup: missionFiveStrikeGroupKeys.size,
        missionFiveHomeGuard: missionFiveHomeGuardKeys.size,
        missionFiveStrikeComposition: friendly.filter((object) => (
          missionFiveStrikeGroupKeys.has(objectKey(object))
        )).map(({ typeName, id, strength, cellX, cellY }) => ({ typeName, id, strength, cellX, cellY })),
        missionFiveGuardComposition: friendly.filter((object) => (
          missionFiveHomeGuardKeys.has(objectKey(object))
        )).map(({ typeName, id, strength, cellX, cellY }) => ({ typeName, id, strength, cellX, cellY })),
        missionFourExtraction: friendly
          .filter((object) => missionFourExtractionKeys.has(objectKey(object)))
          .map(({ typeName, id, strength, cellX, cellY }) => ({ typeName, id, strength, cellX, cellY })),
        sidebar: snapshot.sidebar.entries.map(({ assetName, completed, constructing, onHold, busy }) => ({ assetName, completed, constructing, onHold, busy })),
      }));
    }
    const commands = [];
    if (mission.number === 5 && mission.variant === "west-b"
      && missionFiveShuttleFactCaptureTick === undefined) {
      const footEngineers = friendly.filter((object) => (
        object.type === 1 && object.typeName === "E6"
        && missionFiveShuttleEngineers.has(objectKey(object))
      )).toSorted((left, right) => left.id - right.id);
      const factApc = friendly.find((object) => objectKey(object) === missionFiveWestBApcKey);
      const footContextOrder = (group, destination, flags = 0) => {
        if (group.length === 0 || !destination) return;
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        for (const object of group) {
          commands.push({
            type: COMMAND_SELECT_OBJECT,
            args: [object.type, object.id, 0, 0, 0, 0, 0],
          });
        }
        if (flags) commands.push({
          type: COMMAND_INPUT, flags,
          args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
        });
        commands.push({
          type: COMMAND_INPUT, flags,
          args: [INPUT_COMMAND_AT_POSITION,
            destination.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            destination.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0],
        });
        if (flags) commands.push({
          type: COMMAND_INPUT,
          args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
        });
        selectionCommands += group.length;
        contextualOrders += 1;
      };
      if (missionFiveWestBEngineerPhase === "capture" && factApc
        && snapshot.tick - missionFiveFootEscortOrderTick >= 120) {
        footContextOrder([factApc], { cellX: 50, cellY: 24 }, MODIFIER_ALT);
        missionFiveFootEscortOrderTick = snapshot.tick;
      }
      if (missionFiveFootReservePhase === "waiting"
        && missionFiveAssaultStartedTick === undefined && footEngineers.length > 0) {
        const holdPoint = { cellX: 35, cellY: 61 };
        const holding = footEngineers.every((engineer) => Math.max(
          Math.abs(engineer.cellX - holdPoint.cellX),
          Math.abs(engineer.cellY - holdPoint.cellY),
        ) <= 1);
        if (!holding && snapshot.tick - missionFiveFootReserveOrderTick >= 60) {
          footContextOrder(footEngineers, holdPoint, MODIFIER_ALT);
          missionFiveFootReserveOrderTick = snapshot.tick;
        }
      }
      if (missionFiveFootReservePhase === "waiting"
        && missionFiveAssaultStartedTick !== undefined) {
        missionFiveFootReservePhase = "outbound";
        missionFiveFootReserveOrderTick = -Infinity;
      }
      if (missionFiveFootReservePhase === "outbound" && footEngineers.length > 0) {
        const route = [
          { cellX: 42, cellY: 54 },
          { cellX: 53, cellY: 53 },
          { cellX: 53, cellY: 42 },
          { cellX: 59, cellY: 31 },
        ];
        const waypoint = route[missionFiveFootReserveRouteStage];
        if (waypoint
          && footEngineers.length === 4
          && footEngineers.every((engineer) => Math.max(
            Math.abs(engineer.cellX - waypoint.cellX),
            Math.abs(engineer.cellY - waypoint.cellY),
          ) <= 2)) {
          missionFiveFootReserveRouteStage += 1;
          missionFiveFootReserveOrderTick = -Infinity;
        }
        const nextWaypoint = route[missionFiveFootReserveRouteStage];
        if (!nextWaypoint) {
          missionFiveFootReservePhase = "staged";
          missionFiveFootReserveStagingTick ??= snapshot.tick;
          if (missionFiveFootReserveStagedEngineers.length === 0) {
            for (const engineer of footEngineers) {
              missionFiveFootReserveStagedEngineers.push({
                key: objectKey(engineer),
                strength: engineer.strength,
                maxStrength: engineer.maxStrength,
                cellX: engineer.cellX,
                cellY: engineer.cellY,
              });
            }
          }
          commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
          for (const engineer of footEngineers) commands.push({
            type: COMMAND_SELECT_OBJECT,
            args: [engineer.type, engineer.id, 0, 0, 0, 0, 0],
          });
          commands.push({ type: COMMAND_UNIT, args: [UNIT_REQUEST_STOP, 0, 0, 0, 0, 0, 0] });
          if (trace) console.error(JSON.stringify({ westBFootReservePhase: {
            tick: snapshot.tick, phase: missionFiveFootReservePhase,
            engineers: footEngineers.map(({ id, strength, cellX, cellY }) => (
              { id, strength, cellX, cellY }
            )),
          } }));
        } else if (missionFiveFootReserveRouteStage < missionFiveAssaultRouteStage
          && snapshot.tick - missionFiveFootReserveOrderTick >= 60) {
          footContextOrder(footEngineers, nextWaypoint, MODIFIER_ALT);
          missionFiveFootReserveOrderTick = snapshot.tick;
        }
      }
      if (trace && snapshot.tick % 300 === 0) console.error(JSON.stringify({
        westBFootReserve: true,
        tick: snapshot.tick,
        phase: missionFiveFootReservePhase,
        routeStage: missionFiveFootReserveRouteStage,
        engineers: footEngineers.map(({ id, strength, cellX, cellY }) => (
          { id, strength, cellX, cellY }
        )),
      }));
    }
    if (mission.number === 5 && mission.variant === "west-b"
      && missionFiveWestBApcKey !== undefined
      && missionFiveWestBEngineerPhase !== "captured") {
      const reservedApc = friendly.find((object) => objectKey(object) === missionFiveWestBApcKey);
      const visibleEngineer = friendly.find((object) => objectKey(object) === missionFiveWestBEngineerKey);
      const occupiedPips = reservedApc?.pips.slice(0, reservedApc.pipCount).filter((pip) => pip !== 0) ?? [];
      const queueContextOrder = (group, destination, flags = 0) => {
        if (group.length === 0 || !destination) return;
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        for (const object of group) {
          commands.push({ type: COMMAND_SELECT_OBJECT, args: [object.type, object.id, 0, 0, 0, 0, 0] });
        }
        if (flags !== 0) {
          commands.push({
            type: COMMAND_INPUT,
            flags,
            args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
          });
        }
        commands.push({
          type: COMMAND_INPUT,
          flags,
          args: [
            INPUT_COMMAND_AT_POSITION,
            destination.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            destination.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0,
          ],
        });
        if (flags !== 0) {
          commands.push({ type: COMMAND_INPUT, args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0] });
        }
        selectionCommands += group.length;
        contextualOrders += 1;
        retargetCycles += 1;
      };
      const queueStop = (group) => {
        if (group.length === 0) return;
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        for (const object of group) {
          commands.push({ type: COMMAND_SELECT_OBJECT, args: [object.type, object.id, 0, 0, 0, 0, 0] });
        }
        commands.push({ type: COMMAND_UNIT, args: [UNIT_REQUEST_STOP, 0, 0, 0, 0, 0, 0] });
        selectionCommands += group.length;
      };

      if ((missionFiveWestBEngineerPhase === "prepark" || missionFiveWestBEngineerPhase === "parking")
        && reservedApc) {
        const parkPoint = { cellX: 31, cellY: 58 };
        if (Math.abs(reservedApc.cellX - parkPoint.cellX) <= 1
          && Math.abs(reservedApc.cellY - parkPoint.cellY) <= 1) {
          const nextPhase = visibleEngineer ? "docking" : "parked-await-engineer";
          transitionMissionFiveWestBEngineer(nextPhase, snapshot.tick, {
            apcStrength: reservedApc.strength,
            parkPoint,
          });
          missionFiveWestBApcOrderTick = -Infinity;
          missionFiveWestBLoadOrderTick = -Infinity;
          queueStop([reservedApc]);
        } else if (snapshot.tick - missionFiveWestBApcOrderTick >= 60) {
          queueContextOrder([reservedApc], parkPoint, MODIFIER_ALT);
          missionFiveWestBApcOrderTick = snapshot.tick;
        }
      } else if (missionFiveWestBEngineerPhase === "docking" && reservedApc && visibleEngineer) {
        if (missionFiveWestBApcOrderTick === -Infinity) {
          queueStop([reservedApc]);
          missionFiveWestBApcOrderTick = snapshot.tick;
        }
        if (snapshot.tick - missionFiveWestBLoadOrderTick >= 60) {
          // E6 alone must receive the contextual ENTER order. Selecting the APC
          // too would resolve a different action and invalidate this proof.
          queueContextOrder([visibleEngineer], reservedApc);
          missionFiveWestBLoadOrderTick = snapshot.tick;
        }
      } else if (missionFiveWestBEngineerPhase === "escort" && reservedApc) {
        const route = mission.assaultRoute.slice(0, 4);
        const waypoint = route[missionFiveWestBApcRouteStage];
        if (waypoint
          && missionFiveWestBApcRouteStage < missionFiveAssaultRouteStage
          && Math.abs(reservedApc.cellX - waypoint.cellX) <= 2
          && Math.abs(reservedApc.cellY - waypoint.cellY) <= 2) {
          missionFiveWestBApcRouteStage += 1;
          missionFiveWestBApcOrderTick = -Infinity;
        }
        const nextWaypoint = route[missionFiveWestBApcRouteStage];
        if (missionFiveAssaultPhase === "assault"
          && missionFiveWestBApcRouteStage < missionFiveAssaultRouteStage
          && nextWaypoint
          && snapshot.tick - missionFiveWestBApcOrderTick >= 60) {
          queueContextOrder([reservedApc], nextWaypoint);
          missionFiveWestBApcOrderTick = snapshot.tick;
        } else if (missionFiveAssaultPhase !== "assault" && missionFiveWestBApcRouteStage === 0
          && (Math.abs(reservedApc.cellX - 31) > 1 || Math.abs(reservedApc.cellY - 58) > 1)
          && snapshot.tick - missionFiveWestBApcOrderTick >= 120) {
          queueContextOrder([reservedApc], { cellX: 31, cellY: 58 }, MODIFIER_ALT);
          missionFiveWestBApcOrderTick = snapshot.tick;
        } else if (!nextWaypoint && missionFiveAssaultRouteStage >= 4) {
          transitionMissionFiveWestBEngineer("screening", snapshot.tick, {
            apcRouteStage: missionFiveWestBApcRouteStage,
          });
          missionFiveWestBApcOrderTick = -Infinity;
        }
      } else if (missionFiveWestBEngineerPhase === "screening" && reservedApc) {
        const eastGuns = hostiles.filter((hostile) => (
          hostile.typeName === "GUN" && hostile.cellY === 27
          && (hostile.cellX === 45 || hostile.cellX === 50)
        ));
        const eastMobileThreats = hostiles.filter((hostile) => (
          hostile.type !== 4 && hostile.cellX >= 48 && hostile.cellY >= 23 && hostile.cellY <= 35
        ));
        if (eastGuns.length === 0 && eastMobileThreats.length === 0) {
          transitionMissionFiveWestBEngineer("ingress", snapshot.tick, {
            apcStrength: reservedApc.strength,
          });
          missionFiveWestBApcOrderTick = -Infinity;
        } else if (snapshot.tick - missionFiveWestBApcOrderTick >= 120) {
          // Never contextual-click the transport's current cell while it is
          // screening: that is an unload. A STOP order holds the route-three
          // position without exposing the engineer.
          queueStop([reservedApc]);
          missionFiveWestBApcOrderTick = snapshot.tick;
        }
      } else if (missionFiveWestBEngineerPhase === "ingress" && reservedApc) {
        const unloadPoint = { cellX: 56, cellY: 16 };
        if (Math.abs(reservedApc.cellX - unloadPoint.cellX) <= 1
          && Math.abs(reservedApc.cellY - unloadPoint.cellY) <= 1) {
          transitionMissionFiveWestBEngineer("unloading", snapshot.tick, {
            apcStrength: reservedApc.strength,
          });
          missionFiveWestBUnloadOrderTick = -Infinity;
        } else if (snapshot.tick - missionFiveWestBApcOrderTick >= 60) {
          queueContextOrder([reservedApc], unloadPoint);
          missionFiveWestBApcOrderTick = snapshot.tick;
        }
      }
      if (missionFiveWestBEngineerPhase === "unloading" && reservedApc) {
        if (occupiedPips.length === 0) missionFiveWestBEmptyPipsTick ??= snapshot.tick;
        if (visibleEngineer) missionFiveWestBEngineerRootTick ??= snapshot.tick;
        if (missionFiveWestBEmptyPipsTick !== undefined && visibleEngineer) {
          transitionMissionFiveWestBEngineer("capture", snapshot.tick, {
            pipCount: reservedApc.pipCount,
            occupiedPips,
            engineer: objectKey(visibleEngineer),
          });
          missionFiveWestBCaptureOrderTick = -Infinity;
        } else if (snapshot.tick - missionFiveWestBUnloadOrderTick >= 90) {
          // A transport contextual self-click is the native unload command.
          queueContextOrder([reservedApc], reservedApc);
          missionFiveWestBUnloadOrderTick = snapshot.tick;
          missionFiveWestBUnloadIssuedTick ??= snapshot.tick;
        }
      }
      if (missionFiveWestBEngineerPhase === "capture" && visibleEngineer) {
        const factory = hostiles.find((hostile) => (
          hostile.typeName === "FACT" && hostile.cellX === 52 && hostile.cellY === 17
        ));
        const captureApproach = [
          { cellX: 55, cellY: 16 },
        ];
        const approach = captureApproach[missionFiveWestBEngineerCaptureStage];
        if (approach && Math.abs(visibleEngineer.cellX - approach.cellX) <= 1
          && Math.abs(visibleEngineer.cellY - approach.cellY) <= 1) {
          missionFiveWestBEngineerCaptureStage += 1;
          missionFiveWestBCaptureOrderTick = -Infinity;
        }
        const nextApproach = captureApproach[missionFiveWestBEngineerCaptureStage];
        if (nextApproach && snapshot.tick - missionFiveWestBCaptureOrderTick >= 60) {
          queueContextOrder([visibleEngineer], nextApproach, MODIFIER_ALT);
          missionFiveWestBCaptureOrderTick = snapshot.tick;
        } else if (!nextApproach && factory && snapshot.tick - missionFiveWestBCaptureOrderTick >= 60) {
          queueContextOrder([visibleEngineer], factory);
          missionFiveWestBCaptureOrderTick = snapshot.tick;
        }
      }
      if (trace && snapshot.tick % 300 === 0) {
        console.error(JSON.stringify({
          westBEngineer: true,
          tick: snapshot.tick,
          phase: missionFiveWestBEngineerPhase,
          engineer: visibleEngineer && {
            strength: visibleEngineer.strength,
            cellX: visibleEngineer.cellX,
            cellY: visibleEngineer.cellY,
          },
          apc: reservedApc && {
            strength: reservedApc.strength,
            cellX: reservedApc.cellX,
            cellY: reservedApc.cellY,
            pipCount: reservedApc.pipCount,
            occupiedPips,
          },
          apcRouteStage: missionFiveWestBApcRouteStage,
        }));
      }
    }
    if (mission.number === 5
      && mission.crate
      && missionFiveReliefStage >= mission.reliefRoute.length
      && missionFiveBaseRepairedTick !== undefined
      && missionFiveCrateCollectedTick === undefined) {
      if (missionFiveCrateRunnerKey === undefined) {
        const runnerPriority = mission.variant === "west-b"
          ? new Map([["JEEP", 0], ["APC", 1], ["MTNK", 2], ["E1", 3], ["E2", 4]])
          : new Map([["APC", 0], ["MTNK", 1], ["E1", 2], ["E2", 3]]);
        const runner = availableAttackers(snapshot).toSorted((left, right) => (
          (runnerPriority.get(left.typeName) ?? 20) - (runnerPriority.get(right.typeName) ?? 20)
          || right.strength - left.strength
          || left.id - right.id
        ))[0];
        if (runner) missionFiveCrateRunnerKey = objectKey(runner);
      }
      const runner = availableAttackers(snapshot).find((object) => (
        objectKey(object) === missionFiveCrateRunnerKey
      ));
      if (runner) {
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        commands.push({ type: COMMAND_SELECT_OBJECT, args: [runner.type, runner.id, 0, 0, 0, 0, 0] });
        commands.push({
          type: COMMAND_INPUT,
          flags: MODIFIER_ALT,
          args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
        });
        commands.push({
          type: COMMAND_INPUT,
          flags: MODIFIER_ALT,
          args: [
            INPUT_COMMAND_AT_POSITION,
            mission.crate.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            mission.crate.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0,
          ],
        });
        commands.push({
          type: COMMAND_INPUT,
          args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
        });
        selectionCommands += 1;
        contextualOrders += 1;
      }
    }
    if (mission.number === 5
      && missionFiveAirstrike?.completed
      && missionFiveAirstrikePending === undefined
      && [...missionFiveInitialSamStructureKeys].every((key) => (
        !hostiles.some((hostile) => objectKey(hostile) === key)
      ))) {
      const airstrikePriority = new Map([["FACT", 0], ["HAND", 1], ["PROC", 2], ["AFLD", 3]]);
      const airstrikeTarget = hostiles.filter((hostile) => hostile.type === 4).toSorted((left, right) => (
        (airstrikePriority.get(left.typeName) ?? 20) - (airstrikePriority.get(right.typeName) ?? 20)
        || left.strength - right.strength
        || left.cellY - right.cellY
        || left.cellX - right.cellX
        || left.id - right.id
      ))[0];
      if (airstrikeTarget) {
        commands.push({
          type: COMMAND_SUPERWEAPON,
          flags: 0,
          args: [
            SUPERWEAPON_PLACE,
            missionFiveAirstrike.buildableType,
            missionFiveAirstrike.buildableId,
            airstrikeTarget.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            airstrikeTarget.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0,
            0,
          ],
        });
        missionFiveAirstrikeOrders.push({
          tick: snapshot.tick,
          target: airstrikeTarget.typeName,
          cellX: airstrikeTarget.cellX,
          cellY: airstrikeTarget.cellY,
        });
        missionFiveAirstrikePending = {
          orderTick: snapshot.tick,
          targetKey: objectKey(airstrikeTarget),
          targetType: airstrikeTarget.typeName,
          targetStrength: airstrikeTarget.strength,
        };
      }
    }
    if (mission.number === 4 && mission.variant === "west-b") {
      if (westBInitialVehicleKeys.size === 0) {
        for (const grenadier of friendly.filter((object) => object.typeName === "E2")) westBInitialE2Keys.add(objectKey(grenadier));
        for (const vehicle of attackers.filter((attacker) => attacker.type === 2)) {
          westBInitialVehicleKeys.add(objectKey(vehicle));
          if (vehicle.typeName === "JEEP") westBJeepKey = objectKey(vehicle);
        }
      }
      if (westBPhase === "load") {
        const apcs = friendly.filter((object) => object.typeName === "APC")
          .toSorted((left, right) => left.cellX - right.cellX || left.id - right.id);
        const cargoGroups = ["E1", "E2"].map((typeName) => friendly.filter((object) => object.typeName === typeName));
        if (friendly.filter((object) => object.type === 1).length === 0) {
          westBPhase = "transit";
        } else if (!westBLoadIssued) {
          for (let groupIndex = 0; groupIndex < Math.min(apcs.length, cargoGroups.length); groupIndex += 1) {
            const cargo = cargoGroups[groupIndex];
            if (cargo.length === 0) continue;
            const apc = apcs[groupIndex];
            commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
            for (const passenger of cargo) commands.push({ type: COMMAND_SELECT_OBJECT, args: [passenger.type, passenger.id, 0, 0, 0, 0, 0] });
            commands.push({ type: COMMAND_INPUT, args: [INPUT_COMMAND_AT_POSITION, apc.cellX * CELL_PIXELS + 12, apc.cellY * CELL_PIXELS + 12, 0, 0, 0, 0] });
          }
          westBLoadIssued = true;
        }
      }
      if (westBPhase === "transit") {
        const travelers = attackers.filter((attacker) => westBInitialVehicleKeys.has(objectKey(attacker))
          && (westBRouteStage < 5 || objectKey(attacker) === westBJeepKey));
        const waypoint = westBTransitRoute[westBRouteStage];
        const arrivals = travelers.filter((unit) => Math.abs(unit.cellX - waypoint.cellX) <= 2 && Math.abs(unit.cellY - waypoint.cellY) <= 2).length;
        if (travelers.length > 0 && arrivals === travelers.length) westBRouteStage += 1;
        const targetWaypoint = westBTransitRoute[westBRouteStage];
        if (!targetWaypoint) {
          westBPhase = "wait";
          westBWaitStarted = snapshot.tick;
        } else if (travelers.length > 0) {
          commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
          for (const traveler of travelers) commands.push({ type: COMMAND_SELECT_OBJECT, args: [traveler.type, traveler.id, 0, 0, 0, 0, 0] });
          commands.push({ type: COMMAND_INPUT, args: [INPUT_COMMAND_AT_POSITION, targetWaypoint.cellX * CELL_PIXELS + 12, targetWaypoint.cellY * CELL_PIXELS + 12, 0, 0, 0, 0] });
        }
      }
      if (westBPhase === "wait") {
        if (!westBOwnUnloadIssued) {
          const loadedApcs = attackers.filter((attacker) => attacker.typeName === "APC" && westBInitialVehicleKeys.has(objectKey(attacker)));
          for (const apc of loadedApcs) {
            commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
            commands.push({ type: COMMAND_SELECT_OBJECT, args: [apc.type, apc.id, 0, 0, 0, 0, 0] });
            commands.push({ type: COMMAND_INPUT, args: [INPUT_COMMAND_AT_POSITION, apc.cellX * CELL_PIXELS + 12, apc.cellY * CELL_PIXELS + 12, 0, 0, 0, 0] });
          }
          westBOwnUnloadIssued = true;
        }
        if (snapshot.tick - westBWaitStarted >= 30) westBPhase = "combat";
      }
      if (westBPhase === "combat" && !westBReinforcementsReady) {
        const deliveredGrenadiers = friendly.filter((object) => object.typeName === "E2" && !westBInitialE2Keys.has(objectKey(object)));
        if (deliveredGrenadiers.length >= 4) westBReinforcementsReady = true;
      }
      if (westBPhase === "combat" && westBReinforcementsReady) {
        const waypoint = westBCombatRoute[westBCombatStage];
        if (waypoint) {
          const arrivals = attackers.filter((unit) => Math.abs(unit.cellX - waypoint.cellX) <= 4 && Math.abs(unit.cellY - waypoint.cellY) <= 4).length;
          if (arrivals >= Math.min(6, Math.max(2, Math.floor(attackers.length / 3)))) westBCombatStage += 1;
        } else if (hostiles.length > 0) {
          westBCombatStage = 0;
        }
        const nextWaypoint = westBCombatRoute[westBCombatStage] ?? westBCombatRoute.at(-1);
        const explorationTarget = visibleHostiles.length === 0 && hostiles.length > 0
          ? chooseTarget(hostiles)
          : nextWaypoint;
        if (!westBReserveApcPositioned && snapshot.tick >= 2600) {
          const reserveCandidate = attackers.filter((attacker) => attacker.typeName === "APC"
            && attacker.cellX <= 30 && attacker.strength === attacker.maxStrength)
            .toSorted((left, right) => right.strength - left.strength || left.id - right.id)[0];
          if (reserveCandidate) {
            westBReserveApcKey = objectKey(reserveCandidate);
            westBReserveApcPositioned = true;
          }
        }
        const reserveApc = attackers.find((attacker) => objectKey(attacker) === westBReserveApcKey);
        const screen = attackers.filter((attacker) => attacker.typeName !== "E2"
          && objectKey(attacker) !== westBReserveApcKey
          && !westBLureDecoyBlacklist.has(objectKey(attacker)));
        const grenadiers = attackers.filter((attacker) => attacker.typeName === "E2");
        const remainingSupport = hostiles.filter((hostile) => hostile.typeName !== "LTNK");
        const reserveGrenadiers = hostiles.length <= 12;
        const chooseFor = (group, priority) => group.length === 0 ? undefined : visibleHostiles.map((hostile) => ({
          hostile,
          distance: Math.min(...group.map((attacker) => Math.max(Math.abs(hostile.cellX - attacker.cellX), Math.abs(hostile.cellY - attacker.cellY)))),
        })).filter(({ distance }) => distance <= 12).toSorted((a, b) => (priority[a.hostile.typeName] ?? 9) - (priority[b.hostile.typeName] ?? 9) || a.distance - b.distance || a.hostile.strength - b.hostile.strength)[0]?.hostile;
        const issueGroupOrder = (group, order) => {
          if (group.length === 0 || !order) return;
          commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
          for (const attacker of group) commands.push({ type: COMMAND_SELECT_OBJECT, args: [attacker.type, attacker.id, 0, 0, 0, 0, 0] });
          commands.push({ type: COMMAND_INPUT, args: [INPUT_COMMAND_AT_POSITION, order.cellX * CELL_PIXELS + 12, order.cellY * CELL_PIXELS + 12, 0, 0, 0, 0] });
        };
        const apcTakeover = Boolean(reserveApc && hostiles.length <= 7
          && hostiles.some((hostile) => hostile.typeName === "E3"));
        if (reserveApc && !apcTakeover) {
          const reserveThreats = hostiles.filter((hostile) => hostile.typeName === "LTNK");
          if (reserveThreats.length > 0) {
            const reservePoint = { cellX: 43, cellY: 30 };
            const parked = Math.abs(reserveApc.cellX - reservePoint.cellX) <= 1
              && Math.abs(reserveApc.cellY - reservePoint.cellY) <= 1;
            if (!parked && snapshot.tick - westBReserveApcOrderTick >= 120) {
              issueGroupOrder([reserveApc], reservePoint);
              westBReserveApcOrderTick = snapshot.tick;
            }
            westBReserveApcCleanupTargetKey = undefined;
          } else {
            const cleanupTarget = hostiles.filter((hostile) => hostile.typeName === "BGGY")
              .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0]
              ?? hostiles.filter((hostile) => hostile.typeName === "E1")
                .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0]
              ?? chooseTarget(hostiles);
            const cleanupKey = cleanupTarget && objectKey(cleanupTarget);
            if (cleanupTarget && (westBReserveApcCleanupTargetKey !== cleanupKey
              || snapshot.tick - westBReserveApcOrderTick >= 60)) {
              issueGroupOrder([reserveApc], cleanupTarget);
              westBReserveApcCleanupTargetKey = cleanupKey;
              westBReserveApcOrderTick = snapshot.tick;
            }
          }
        }
        if (apcTakeover) {
          if (!westBTakeoverInitialized) {
            commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
            for (const grenadier of grenadiers) {
              commands.push({ type: COMMAND_SELECT_OBJECT, args: [grenadier.type, grenadier.id, 0, 0, 0, 0, 0] });
            }
            commands.push({ type: COMMAND_UNIT, args: [5, 0, 0, 0, 0, 0, 0] });
            commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
            for (const unit of screen) {
              commands.push({ type: COMMAND_SELECT_OBJECT, args: [unit.type, unit.id, 0, 0, 0, 0, 0] });
            }
            if (screen.length > 0) commands.push({ type: COMMAND_UNIT, args: [5, 0, 0, 0, 0, 0, 0] });
            westBTakeoverInitialized = true;
          }
          const supportTarget = hostiles.filter((hostile) => hostile.typeName === "E3")
            .toSorted((left, right) => left.strength - right.strength || left.cellY - right.cellY || left.id - right.id)[0];
          if (supportTarget) {
            westBTakeoverPhase = "support";
            const supportKey = objectKey(supportTarget);
            if (westBTakeoverTargetKey !== supportKey || snapshot.tick - westBTakeoverOrderTick >= 60) {
              if (supportTarget.cellY >= 48 && screen.length > 0) {
                issueGroupOrder(screen, supportTarget);
                issueGroupOrder([reserveApc], { cellX: 43, cellY: 30 });
              } else {
                issueGroupOrder([reserveApc], supportTarget);
              }
              westBTakeoverTargetKey = supportKey;
              westBTakeoverOrderTick = snapshot.tick;
            }
          } else {
            const tankTarget = hostiles.filter((hostile) => hostile.typeName === "LTNK")
              .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0];
            if (tankTarget) {
              const tankKey = objectKey(tankTarget);
              if (westBTakeoverPhase === "support" || westBTakeoverTargetKey !== tankKey) {
                westBTakeoverPhase = "approach";
                westBTakeoverTargetKey = tankKey;
                westBTakeoverApcInitialStrength = reserveApc.strength;
                westBTakeoverTargetInitialStrength = tankTarget.strength;
                westBTakeoverTargetOrigin = { cellX: tankTarget.cellX, cellY: tankTarget.cellY };
                westBTakeoverOrderTick = -Infinity;
                westBTakeoverParkedTick = undefined;
              }
              if (westBTakeoverPhase === "approach") {
                if (snapshot.tick - westBTakeoverOrderTick >= 60) {
                  issueGroupOrder([reserveApc], tankTarget);
                  westBTakeoverOrderTick = snapshot.tick;
                }
                if (reserveApc.strength < westBTakeoverApcInitialStrength
                  || tankTarget.strength < westBTakeoverTargetInitialStrength
                  || tankTarget.cellX !== westBTakeoverTargetOrigin.cellX
                  || tankTarget.cellY !== westBTakeoverTargetOrigin.cellY) {
                  westBTakeoverPhase = "retreat";
                  westBTakeoverOrderTick = -Infinity;
                }
              }
              if (westBTakeoverPhase === "retreat") {
                const reservePoint = { cellX: 43, cellY: 30 };
                const parked = Math.abs(reserveApc.cellX - reservePoint.cellX) <= 1
                  && Math.abs(reserveApc.cellY - reservePoint.cellY) <= 1;
                if (!parked && snapshot.tick - westBTakeoverOrderTick >= 60) {
                  issueGroupOrder([reserveApc], reservePoint);
                  westBTakeoverOrderTick = snapshot.tick;
                }
                if (parked) {
                  westBTakeoverParkedTick ??= snapshot.tick;
                  if (snapshot.tick - westBTakeoverParkedTick >= 90) {
                    westBTakeoverPhase = "approach";
                    westBTakeoverApcInitialStrength = reserveApc.strength;
                    westBTakeoverTargetInitialStrength = tankTarget.strength;
                    westBTakeoverTargetOrigin = { cellX: tankTarget.cellX, cellY: tankTarget.cellY };
                    westBTakeoverOrderTick = -Infinity;
                    westBTakeoverParkedTick = undefined;
                  }
                }
              }
            } else {
              const cleanupTarget = hostiles.filter((hostile) => hostile.typeName === "BGGY")
                .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0]
                ?? hostiles.filter((hostile) => hostile.typeName === "E1")
                  .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0]
                ?? chooseTarget(hostiles);
              const cleanupKey = cleanupTarget && objectKey(cleanupTarget);
              if (cleanupTarget && (westBTakeoverTargetKey !== cleanupKey
                || snapshot.tick - westBTakeoverOrderTick >= 60)) {
                issueGroupOrder([reserveApc], cleanupTarget);
                westBTakeoverTargetKey = cleanupKey;
                westBTakeoverOrderTick = snapshot.tick;
              }
            }
          }
        } else if (reserveGrenadiers) {
          const holdPoint = { cellX: 43, cellY: 42 };
          if (westBScreenOrder !== "hold") {
            issueGroupOrder(screen, holdPoint);
            westBScreenOrder = "hold";
          }
          const assaultTarget = screen.length === 0
            ? hostiles.filter((hostile) => hostile.typeName === "LTNK")
              .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0]
              ?? hostiles.filter((hostile) => hostile.typeName === "BGGY")
                .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0]
              ?? chooseTarget(hostiles)
            : undefined;
          const assaultTargetKey = assaultTarget && objectKey(assaultTarget);
          const tanksRemain = hostiles.some((hostile) => hostile.typeName === "LTNK");
          if (tanksRemain && !westBLastStand) {
            for (const candidate of grenadiers
              .filter((grenadier) => grenadier.strength <= 9 && !westBCriticalReserveKeys.has(objectKey(grenadier)))
              .toSorted((left, right) => left.strength - right.strength || left.id - right.id)) {
              const combatCount = grenadiers.filter((grenadier) => !westBCriticalReserveKeys.has(objectKey(grenadier))).length;
              if (combatCount <= 2) break;
              westBCriticalReserveKeys.add(objectKey(candidate));
              westBCriticalReservePoints.set(objectKey(candidate), {
                cellX: Math.max(39, Math.min(56, candidate.cellX)),
                cellY: 30,
              });
            }
          }
          let criticalReserves = tanksRemain
            ? grenadiers.filter((grenadier) => westBCriticalReserveKeys.has(objectKey(grenadier)))
            : [];
          for (const reserve of criticalReserves) {
            const reservePoint = westBCriticalReservePoints.get(objectKey(reserve));
            if (reservePoint && (Math.abs(reserve.cellX - reservePoint.cellX) > 2
              || Math.abs(reserve.cellY - reservePoint.cellY) > 2)
              && snapshot.tick - (westBCriticalReserveOrderTicks.get(objectKey(reserve)) ?? -Infinity) >= 60) {
              issueGroupOrder([reserve], reservePoint);
              westBCriticalReserveOrderTicks.set(objectKey(reserve), snapshot.tick);
            }
          }
          let combatGrenadiers = tanksRemain
            ? grenadiers.filter((grenadier) => !westBCriticalReserveKeys.has(objectKey(grenadier)))
            : grenadiers;
          if (tanksRemain && combatGrenadiers.length === 0 && grenadiers.length > 0) {
            westBLastStand = true;
            westBCriticalReserveKeys.clear();
            criticalReserves = [];
            combatGrenadiers = grenadiers;
          }
          if (westBKiteTargetKey !== assaultTargetKey) {
            westBKiteActive = false;
            westBKiteTargetKey = assaultTargetKey;
            westBKiteDecoyKey = undefined;
            westBKiteRetreatPoint = undefined;
            westBKiteOrderTick = -Infinity;
          }
          if (!westBKiteActive && assaultTarget?.typeName === "LTNK"
            && assaultTarget.cellX >= 59 && combatGrenadiers.length === 2) {
            const decoy = combatGrenadiers.toSorted((left, right) => right.strength - left.strength || left.id - right.id)[0];
            westBKiteActive = true;
            westBKiteDecoyKey = objectKey(decoy);
            westBKiteRetreatPoint = {
              cellX: Math.max(snapshot.staticMap.cellX, assaultTarget.cellX - 10),
              cellY: Math.max(snapshot.staticMap.cellY, assaultTarget.cellY - 10),
            };
          }
          const kiteDecoy = westBKiteActive && combatGrenadiers.length > 1
            ? combatGrenadiers.find((grenadier) => objectKey(grenadier) === westBKiteDecoyKey)
            : undefined;
          if (kiteDecoy && snapshot.tick - westBKiteOrderTick >= 60) {
            issueGroupOrder([kiteDecoy], westBKiteRetreatPoint);
            westBKiteOrderTick = snapshot.tick;
          }
          const firingGrenadiers = kiteDecoy
            ? combatGrenadiers.filter((grenadier) => grenadier !== kiteDecoy)
            : combatGrenadiers;
          if (!assaultTarget) {
            westBAssaultTargetKey = undefined;
            westBAssaultAnchor = undefined;
            westBAssaultStartedTick = undefined;
          } else if (westBAssaultTargetKey !== objectKey(assaultTarget)) {
            westBAssaultTargetKey = objectKey(assaultTarget);
            westBAssaultAnchor = { cellX: assaultTarget.cellX, cellY: assaultTarget.cellY };
            westBAssaultStartedTick = snapshot.tick;
            westBGuardPhase = "stage";
            westBGrenadierOrders.clear();
            westBGrenadierOrderTicks.clear();
          }
          const assaultOffsets = [
            assaultTarget?.cellX >= 59 ? { cellX: -3, cellY: -3 } : { cellX: -4, cellY: -4 },
            { cellX: -1, cellY: -4 },
            { cellX: 2, cellY: -4 },
            { cellX: 4, cellY: -1 },
            { cellX: 5, cellY: -4 },
            { cellX: -4, cellY: -1 },
            { cellX: 3, cellY: -1 },
          ];
          const supportOffsets = [
            { cellX: -3, cellY: -3 },
            { cellX: 0, cellY: -3 },
            { cellX: 3, cellY: -3 },
            { cellX: -3, cellY: 0 },
            { cellX: 3, cellY: 0 },
            { cellX: -3, cellY: 3 },
            { cellX: 3, cellY: 3 },
          ];
          const activeOffsets = tanksRemain && !westBLastStand ? assaultOffsets : supportOffsets;
          const singleShooterPoint = westBAssaultAnchor && firingGrenadiers.length === 1
            ? {
              cellX: Math.max(snapshot.staticMap.cellX, westBAssaultAnchor.cellX - 3),
              cellY: westBAssaultAnchor.cellY,
            }
            : undefined;
          const activeForwardLine = singleShooterPoint
            ? westBForwardLine.map(() => singleShooterPoint)
            : westBAssaultAnchor
            ? activeOffsets.map((offset) => ({
              cellX: Math.max(snapshot.staticMap.cellX, Math.min(
                snapshot.staticMap.cellX + snapshot.staticMap.width - 1,
                westBAssaultAnchor.cellX + offset.cellX,
              )),
              cellY: Math.max(snapshot.staticMap.cellY, Math.min(
                snapshot.staticMap.cellY + snapshot.staticMap.height - 1,
                westBAssaultAnchor.cellY + offset.cellY,
              )),
            }))
              : westBForwardLine;
          const stagingTolerance = westBAssaultAnchor ? 2 : 3;
          for (const grenadier of firingGrenadiers.toSorted((left, right) => left.id - right.id)) {
            if (!westBStagingIndexes.has(objectKey(grenadier))) {
              westBStagingIndexes.set(objectKey(grenadier), westBStagingIndexes.size % westBForwardLine.length);
            }
            const stagingIndex = westBStagingIndexes.get(objectKey(grenadier));
            const stagingPoint = activeForwardLine[stagingIndex];
            const reserveKey = `stage:${stagingIndex}:${stagingPoint.cellX}:${stagingPoint.cellY}`;
            const atStagingPoint = Math.abs(grenadier.cellX - stagingPoint.cellX) <= stagingTolerance
              && Math.abs(grenadier.cellY - stagingPoint.cellY) <= stagingTolerance;
            if (westBGuardPhase === "stage" && !atStagingPoint
              && (westBGrenadierOrders.get(objectKey(grenadier)) !== reserveKey
                || snapshot.tick - (westBGrenadierOrderTicks.get(objectKey(grenadier)) ?? -Infinity) >= 120)) {
              issueGroupOrder([grenadier], stagingPoint);
              westBGrenadierOrders.set(objectKey(grenadier), reserveKey);
              westBGrenadierOrderTicks.set(objectKey(grenadier), snapshot.tick);
            }
          }
          const stagedGrenadiers = firingGrenadiers.filter((grenadier) => {
            const stagingPoint = activeForwardLine[westBStagingIndexes.get(objectKey(grenadier))];
            return Math.abs(grenadier.cellX - stagingPoint.cellX) <= stagingTolerance
              && Math.abs(grenadier.cellY - stagingPoint.cellY) <= stagingTolerance;
          });
          const lineReady = firingGrenadiers.length > 0 && (
            stagedGrenadiers.length === firingGrenadiers.length
            || (assaultTarget && (
              stagedGrenadiers.length >= Math.min(4, firingGrenadiers.length)
              || snapshot.tick - westBAssaultStartedTick >= 240
            ))
          );
          if (westBGuardPhase === "stage" && lineReady) {
            if (assaultTarget?.typeName === "BGGY") {
              issueGroupOrder(firingGrenadiers, assaultTarget);
              westBGuardFocusing = true;
            } else {
              commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
              for (const grenadier of firingGrenadiers) {
                commands.push({ type: COMMAND_SELECT_OBJECT, args: [grenadier.type, grenadier.id, 0, 0, 0, 0, 0] });
              }
              commands.push({ type: COMMAND_UNIT, args: [4, 0, 0, 0, 0, 0, 0] });
              westBGuardFocusing = false;
            }
            westBGuardPhase = "guard";
            westBGuardPhaseTick = snapshot.tick;
            westBLineEstablished = true;
          } else if (westBGuardPhase === "guard"
            && snapshot.tick - westBGuardPhaseTick >= (westBGuardFocusing ? 75 : 45)) {
            commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
            for (const grenadier of firingGrenadiers) {
              commands.push({ type: COMMAND_SELECT_OBJECT, args: [grenadier.type, grenadier.id, 0, 0, 0, 0, 0] });
            }
            commands.push({ type: COMMAND_UNIT, args: [5, 0, 0, 0, 0, 0, 0] });
            westBGuardPhase = "stage";
            if (westBAssaultAnchor) {
              westBAssaultAnchor = { cellX: assaultTarget.cellX, cellY: assaultTarget.cellY };
              westBAssaultStartedTick = snapshot.tick;
            }
            westBGrenadierOrders.clear();
            westBGrenadierOrderTicks.clear();
          }
          if (westBLineEstablished) {
            let lureTarget = hostiles.find((hostile) => objectKey(hostile) === westBLureTankKey);
            if (westBLureTankKey !== undefined && !lureTarget) {
              westBLureTankKey = undefined;
              westBLureDecoyKey = undefined;
              westBLurePhase = "approach";
              westBLureRetreatStage = 0;
              westBLureOrder = undefined;
              westBLureActivated = false;
              westBLureTargetOrigin = undefined;
              westBLureTargetInitialStrength = undefined;
              westBLureDecoyInitialStrength = undefined;
              westBFocusOrder = undefined;
            }
            if (!lureTarget) {
              lureTarget = hostiles.filter((hostile) => hostile.typeName === "E3" && hostile.cellY <= 51)
                .toSorted((left, right) => left.strength - right.strength || left.cellY - right.cellY || left.id - right.id)[0]
                ?? hostiles.filter((hostile) => hostile.typeName === "LTNK")
                .toSorted((left, right) => left.id - right.id)[0]
                ?? hostiles.filter((hostile) => hostile.typeName === "BGGY")
                  .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0]
                ?? chooseTarget(hostiles);
              if (lureTarget) {
                westBLureTankKey = objectKey(lureTarget);
                westBLureTargetOrigin = { cellX: lureTarget.cellX, cellY: lureTarget.cellY };
                westBLureTargetInitialStrength = lureTarget.strength;
              }
            }
            let decoy = screen.find((unit) => objectKey(unit) === westBLureDecoyKey);
            if (!decoy && westBLureActivated) {
              westBLureActivated = false;
              westBLureDecoyKey = undefined;
              westBLurePhase = "approach";
              westBLureRetreatStage = 0;
              westBLureOrder = undefined;
              westBLureProgressSignature = undefined;
              westBLureProgressTick = snapshot.tick;
              westBLureTargetOrigin = lureTarget
                ? { cellX: lureTarget.cellX, cellY: lureTarget.cellY }
                : undefined;
              westBLureTargetInitialStrength = lureTarget?.strength;
            }
            if (decoy && westBLurePhase === "approach") {
              const progressSignature = `${decoy.cellX}:${decoy.cellY}:${lureTarget?.strength}:${lureTarget?.cellX}:${lureTarget?.cellY}`;
              if (progressSignature !== westBLureProgressSignature) {
                westBLureProgressSignature = progressSignature;
                westBLureProgressTick = snapshot.tick;
              } else if (snapshot.tick - westBLureProgressTick >= 180) {
                westBLureDecoyBlacklist.add(objectKey(decoy));
                westBLureDecoyKey = undefined;
                westBLureOrder = undefined;
                westBLureProgressSignature = undefined;
                westBLureProgressTick = snapshot.tick;
                decoy = undefined;
              }
            }
            if (!decoy && !westBLureActivated) {
              decoy = screen.filter((unit) => !westBLureDecoyBlacklist.has(objectKey(unit))).toSorted((left, right) => (
                ({ JEEP: 0, E1: 1, APC: 2 }[left.typeName] ?? 9)
                - ({ JEEP: 0, E1: 1, APC: 2 }[right.typeName] ?? 9)
                || right.strength - left.strength
                || left.id - right.id
              ))[0];
              westBLureDecoyKey = decoy && objectKey(decoy);
              westBLureOrder = undefined;
              westBLurePhase = "approach";
              westBLureRetreatStage = 0;
              westBLureDecoyInitialStrength = decoy?.strength;
              westBLureTargetOrigin = lureTarget
                ? { cellX: lureTarget.cellX, cellY: lureTarget.cellY }
                : undefined;
              westBLureTargetInitialStrength = lureTarget?.strength;
              westBLureProgressSignature = decoy
                ? `${decoy.cellX}:${decoy.cellY}:${lureTarget?.strength}:${lureTarget?.cellX}:${lureTarget?.cellY}`
                : undefined;
              westBLureProgressTick = snapshot.tick;
            }
            if (lureTarget && decoy) {
              if (westBLurePhase === "approach") {
                const attackKey = `attack:${objectKey(lureTarget)}:${objectKey(decoy)}`;
                if (westBLureOrder !== attackKey) {
                  issueGroupOrder([decoy], lureTarget);
                  westBLureOrder = attackKey;
                }
                if (lureTarget.strength < (westBLureTargetInitialStrength ?? lureTarget.strength)
                  || (westBLureTargetOrigin
                    && (lureTarget.cellX !== westBLureTargetOrigin.cellX || lureTarget.cellY !== westBLureTargetOrigin.cellY))) {
                  westBLureActivated = true;
                  westBLurePhase = "retreat";
                  westBLureRetreatStage = 0;
                  westBLureOrder = undefined;
                }
              }
              if (westBLurePhase === "retreat") {
                const retreatProgressSignature = `retreat:${westBLureRetreatStage}:${decoy.cellX}:${decoy.cellY}`;
                if (retreatProgressSignature !== westBLureProgressSignature) {
                  westBLureProgressSignature = retreatProgressSignature;
                  westBLureProgressTick = snapshot.tick;
                }
                if (snapshot.tick - westBLureProgressTick >= 300) {
                  westBLureDecoyBlacklist.add(objectKey(decoy));
                  westBLureTankKey = undefined;
                  westBLureActivated = false;
                  westBLureDecoyKey = undefined;
                  westBLurePhase = "approach";
                  westBLureRetreatStage = 0;
                  westBLureOrder = undefined;
                  westBLureProgressSignature = undefined;
                  westBLureProgressTick = snapshot.tick;
                  westBLureTargetOrigin = undefined;
                  westBLureTargetInitialStrength = undefined;
                } else {
                  const retreatRoute = [
                    { cellX: 50, cellY: 50 },
                    { cellX: 45, cellY: 49 },
                    { cellX: 43, cellY: 48 },
                    { cellX: 43, cellY: 47 },
                    { cellX: 44, cellY: 46 },
                    { cellX: 45, cellY: 45 },
                    { cellX: 46, cellY: 40 },
                    { cellX: 46, cellY: 34 },
                  ];
                  const waypoint = retreatRoute[westBLureRetreatStage] ?? retreatRoute.at(-1);
                  if (Math.abs(decoy.cellX - waypoint.cellX) <= 2 && Math.abs(decoy.cellY - waypoint.cellY) <= 2
                  ) {
                    if (westBLureRetreatStage < retreatRoute.length - 1) {
                      westBLureRetreatStage += 1;
                      westBLureOrder = undefined;
                    } else {
                      westBLureDecoyBlacklist.add(objectKey(decoy));
                      westBLureActivated = false;
                      westBLureDecoyKey = undefined;
                      westBLurePhase = "approach";
                      westBLureRetreatStage = 0;
                      westBLureOrder = undefined;
                      westBLureProgressSignature = undefined;
                      westBLureProgressTick = snapshot.tick;
                      westBLureTargetOrigin = { cellX: lureTarget.cellX, cellY: lureTarget.cellY };
                      westBLureTargetInitialStrength = lureTarget.strength;
                    }
                  }
                  const nextWaypoint = retreatRoute[westBLureRetreatStage] ?? retreatRoute.at(-1);
                  const retreatKey = `retreat:${westBLureRetreatStage}`;
                  if (westBLureOrder !== retreatKey) {
                    issueGroupOrder([decoy], nextWaypoint);
                    westBLureOrder = retreatKey;
                  }
                }
              }
            }
          }
        } else {
          const screenTarget = chooseFor(screen, { BGGY: 0, E3: 1, E1: 2, LTNK: 3 });
          const grenadeTarget = chooseFor(grenadiers, { BGGY: 0, E3: 1, E1: 2, LTNK: 3 });
          issueGroupOrder(screen, screenTarget ?? explorationTarget);
          issueGroupOrder(grenadiers, grenadeTarget ?? screenTarget ?? explorationTarget);
          westBGrenadierOrders.clear();
          westBGrenadierOrderTicks.clear();
        }
      }
      if (westBIntegratedPhase === "tank6" || westBIntegratedPhase === "cleanup") {
        const reserveApc = attackers.find((attacker) => objectKey(attacker) === westBReserveApcKey)
          ?? attackers.filter((attacker) => attacker.typeName === "APC")
            .toSorted((left, right) => right.strength - left.strength || left.id - right.id)[0];
        if (reserveApc && westBReserveApcKey === undefined) westBReserveApcKey = objectKey(reserveApc);
        const grenadiers = attackers.filter((attacker) => attacker.typeName === "E2")
          .toSorted((left, right) => left.id - right.id);
        const screen = attackers.filter((attacker) => attacker.typeName !== "E2" && attacker !== reserveApc);
        const issueReserveOrder = (group, order) => {
          if (group.length === 0 || !order) return;
          commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
          for (const attacker of group) {
            commands.push({ type: COMMAND_SELECT_OBJECT, args: [attacker.type, attacker.id, 0, 0, 0, 0, 0] });
          }
          commands.push({ type: COMMAND_INPUT, args: [INPUT_COMMAND_AT_POSITION,
            order.cellX * CELL_PIXELS + 12, order.cellY * CELL_PIXELS + 12, 0, 0, 0, 0] });
        };
        if (screen.length > 0 && snapshot.tick - westBReserveScreenOrderTick >= 90) {
          issueReserveOrder(screen, { cellX: 43, cellY: 30 });
          westBReserveScreenOrderTick = snapshot.tick;
        }
        const tanks = hostiles.filter((hostile) => hostile.typeName === "LTNK")
          .toSorted((left, right) => left.strength - right.strength || left.id - right.id);
        const tank = tanks[0];
        const finalCleanupActive = tanks.length === 1
          && hostiles.some((hostile) => hostile.typeName === "E1");
        const stageForTank = Boolean(tank && tanks.length >= 2);
        const tankOffsets = [
          { cellX: -4, cellY: -4 },
          { cellX: -1, cellY: -4 },
          { cellX: 2, cellY: -4 },
          { cellX: 4, cellY: -1 },
          { cellX: -4, cellY: -1 },
        ];
        const cleanupLine = [
          { cellX: 43, cellY: 43 },
          { cellX: 46, cellY: 43 },
          { cellX: 49, cellY: 43 },
          { cellX: 52, cellY: 43 },
        ];
        for (const [fallbackIndex, grenadier] of grenadiers.entries()) {
          const grenadierKey = objectKey(grenadier);
          if (!westBStagingIndexes.has(grenadierKey)) westBStagingIndexes.set(grenadierKey, fallbackIndex);
          const index = westBStagingIndexes.get(grenadierKey);
          if (finalCleanupActive) {
            westBReserveLinePoints.set(grenadierKey, cleanupLine[fallbackIndex % cleanupLine.length]);
          } else if (stageForTank) {
            const offset = tankOffsets[index % tankOffsets.length];
            westBReserveLinePoints.set(grenadierKey, {
              cellX: Math.max(snapshot.staticMap.cellX, Math.min(
                snapshot.staticMap.cellX + snapshot.staticMap.width - 1,
                tank.cellX + offset.cellX,
              )),
              cellY: Math.max(snapshot.staticMap.cellY, Math.min(
                snapshot.staticMap.cellY + snapshot.staticMap.height - 1,
                tank.cellY + offset.cellY,
              )),
            });
          }
        }
        const readyGrenadiers = [];
        for (const grenadier of grenadiers) {
          const grenadierKey = objectKey(grenadier);
          const anchor = westBReserveLinePoints.get(grenadierKey);
          if (!anchor) continue;
          if (Math.abs(grenadier.cellX - anchor.cellX) <= 2
            && Math.abs(grenadier.cellY - anchor.cellY) <= 2) {
            readyGrenadiers.push(grenadier);
          } else if ((stageForTank || finalCleanupActive) && westBGuardPhase === "stage"
            && snapshot.tick - (westBGrenadierOrderTicks.get(grenadierKey) ?? -Infinity) >= 90) {
            issueReserveOrder([grenadier], anchor);
            westBGrenadierOrderTicks.set(grenadierKey, snapshot.tick);
          }
        }
        if (finalCleanupActive && westBReserveGuardOrderTick === -Infinity
          && readyGrenadiers.length === grenadiers.length && grenadiers.length > 0) {
          commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
          for (const grenadier of grenadiers) {
            commands.push({ type: COMMAND_SELECT_OBJECT, args: [grenadier.type, grenadier.id, 0, 0, 0, 0, 0] });
          }
          commands.push({ type: COMMAND_UNIT, args: [UNIT_REQUEST_STOP, 0, 0, 0, 0, 0, 0] });
          westBReserveGuardOrderTick = snapshot.tick;
        }
        if (reserveApc && finalCleanupActive) {
          const finalInfantry = hostiles.filter((hostile) => hostile.typeName === "E1")
            .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0];
          if (reserveApc.strength <= 150 || finalInfantry.strength < finalInfantry.maxStrength) {
            westBFinalCleanupRetreat = true;
          }
          if (snapshot.tick - westBReserveApcOrderTick >= 45) {
            issueReserveOrder([reserveApc], westBFinalCleanupRetreat
              ? { cellX: 46, cellY: 43 }
              : finalInfantry);
            westBReserveApcOrderTick = snapshot.tick;
          }
          westBReserveAssaultPhase = westBFinalCleanupRetreat ? "final-retreat" : "final-infantry";
        } else if (stageForTank) {
          const tankKey = objectKey(tank);
          if (westBReserveAssaultTargetKey !== tankKey) {
            westBReserveAssaultTargetKey = tankKey;
            westBReserveAssaultPhase = "tank-stage";
            westBReserveAssaultPhaseTick = snapshot.tick;
            westBReserveApcOrderTick = -Infinity;
            westBReserveApcStopped = false;
            westBGuardPhase = "stage";
            westBGrenadierOrderTicks.clear();
          }
          if (reserveApc) {
            const decoyPoint = { cellX: tank.cellX, cellY: tank.cellY - 4 };
            const atDecoyPoint = reserveApc.cellX === decoyPoint.cellX
              && reserveApc.cellY === decoyPoint.cellY;
            if (atDecoyPoint && !westBReserveApcStopped) {
              commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
              commands.push({ type: COMMAND_SELECT_OBJECT, args: [reserveApc.type, reserveApc.id, 0, 0, 0, 0, 0] });
              commands.push({ type: COMMAND_UNIT, args: [UNIT_REQUEST_STOP, 0, 0, 0, 0, 0, 0] });
              westBReserveApcStopped = true;
            } else if (!atDecoyPoint && snapshot.tick - westBReserveApcOrderTick >= 60) {
              issueReserveOrder([reserveApc], decoyPoint);
              westBReserveApcOrderTick = snapshot.tick;
              westBReserveApcStopped = false;
            }
          }
          const lineReady = grenadiers.length > 0 && (
            readyGrenadiers.length === grenadiers.length
            || readyGrenadiers.length >= Math.min(3, grenadiers.length)
          );
          if (westBGuardPhase === "stage" && lineReady) {
            commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
            westBGuardedGrenadiers.clear();
            for (const grenadier of readyGrenadiers) {
              commands.push({ type: COMMAND_SELECT_OBJECT, args: [grenadier.type, grenadier.id, 0, 0, 0, 0, 0] });
              westBGuardedGrenadiers.add(objectKey(grenadier));
            }
            commands.push({ type: COMMAND_UNIT, args: [4, 0, 0, 0, 0, 0, 0] });
            westBGuardPhase = "guard";
            westBGuardPhaseTick = snapshot.tick;
            westBReserveAssaultPhase = "tank-guard";
          } else if (westBGuardPhase === "guard" && snapshot.tick - westBGuardPhaseTick >= 45) {
            const guardedGrenadiers = grenadiers.filter((grenadier) =>
              westBGuardedGrenadiers.has(objectKey(grenadier)));
            commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
            for (const grenadier of guardedGrenadiers) {
              commands.push({ type: COMMAND_SELECT_OBJECT, args: [grenadier.type, grenadier.id, 0, 0, 0, 0, 0] });
            }
            commands.push({ type: COMMAND_UNIT, args: [UNIT_REQUEST_STOP, 0, 0, 0, 0, 0, 0] });
            westBGuardPhase = "stage";
            westBReserveAssaultPhase = "tank-stage";
            westBReserveAssaultPhaseTick = snapshot.tick;
            for (const grenadier of guardedGrenadiers) {
              westBGrenadierOrderTicks.delete(objectKey(grenadier));
            }
            westBGuardedGrenadiers.clear();
          }
        }
        if (trace && snapshot.tick % 30 === 0) console.error(JSON.stringify({
          integratedTank6: true,
          tick: snapshot.tick,
          phase: westBReserveAssaultPhase,
          apc: reserveApc && { strength: reserveApc.strength, cellX: reserveApc.cellX, cellY: reserveApc.cellY },
          e2: grenadiers.map(({ id, strength, cellX, cellY }) => ({ id, strength, cellX, cellY })),
          hostile: hostiles.map(({ typeName, id, strength, cellX, cellY }) => ({ typeName, id, strength, cellX, cellY })),
        }));
      }
      if (westBIntegratedPhase === "support-clear") {
        const reserveApc = attackers.filter((attacker) => attacker.typeName === "APC")
          .toSorted((left, right) => right.strength - left.strength || left.id - right.id)[0];
        const grenadiers = attackers.filter((attacker) => attacker.typeName === "E2")
          .toSorted((left, right) => left.id - right.id);
        const screen = attackers.filter((attacker) => attacker.typeName === "E1");
        const issueSupportClearOrder = (group, order) => {
          if (group.length === 0 || !order) return;
          commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
          for (const attacker of group) {
            commands.push({ type: COMMAND_SELECT_OBJECT, args: [attacker.type, attacker.id, 0, 0, 0, 0, 0] });
          }
          commands.push({ type: COMMAND_INPUT, args: [INPUT_COMMAND_AT_POSITION,
            order.cellX * CELL_PIXELS + 12, order.cellY * CELL_PIXELS + 12, 0, 0, 0, 0] });
        };
        const lateTankOnly = hostiles.length > 0
          && hostiles.every((hostile) => hostile.typeName === "LTNK")
          && reserveApc === undefined;
        if (lateTankOnly && !westBLateTankActive) {
          westBLateTankActive = true;
          westBLateTankPhase = "stage";
          westBLateTankAttackTick = -Infinity;
        }
        if (westBLateTankActive) {
          const tank = hostiles.find((hostile) => hostile.typeName === "LTNK");
          const stagePoints = [
            { cellX: 44, cellY: 48 },
            { cellX: 43, cellY: 53 },
            { cellX: 44, cellY: 57 },
            { cellX: 54, cellY: 57 },
          ];
          const reducedStagePoints = new Map();
          if (grenadiers.length === 3) {
            const byStrength = grenadiers.toSorted((left, right) =>
              right.strength - left.strength || left.id - right.id);
            reducedStagePoints.set(objectKey(byStrength[0]), stagePoints[3]);
            reducedStagePoints.set(objectKey(byStrength[1]), stagePoints[0]);
            reducedStagePoints.set(objectKey(byStrength[2]), stagePoints[1]);
          }
          const readyGrenadiers = [];
          if (tank && westBLateTankPhase === "stage") {
            for (const [index, grenadier] of grenadiers.entries()) {
              const point = reducedStagePoints.get(objectKey(grenadier))
                ?? stagePoints[index % stagePoints.length];
              const ready = Math.abs(grenadier.cellX - point.cellX) <= 1
                && Math.abs(grenadier.cellY - point.cellY) <= 1;
              if (ready) {
                readyGrenadiers.push(grenadier);
              } else if (snapshot.tick - (westBLateTankOrderTicks.get(objectKey(grenadier)) ?? -Infinity) >= 60) {
                issueSupportClearOrder([grenadier], point);
                westBLateTankOrderTicks.set(objectKey(grenadier), snapshot.tick);
              }
            }
            if (readyGrenadiers.length === grenadiers.length && grenadiers.length > 0) {
              westBLateTankPhase = "assault";
              westBLateTankAttackTick = -Infinity;
            }
          }
          if (tank && westBLateTankPhase === "assault"
            && snapshot.tick - westBLateTankAttackTick >= 60) {
            issueSupportClearOrder(screen, tank);
            if (grenadiers.length === 1) {
              const loneGrenadier = grenadiers[0];
              const grenadierKey = objectKey(loneGrenadier);
              const lastScatterTick = westBScatterTicks.get(grenadierKey);
              const distance = Math.max(
                Math.abs(tank.cellX - loneGrenadier.cellX),
                Math.abs(tank.cellY - loneGrenadier.cellY),
              );
              if (distance <= 2 && snapshot.tick - (lastScatterTick ?? -Infinity) >= 90) {
                commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
                commands.push({ type: COMMAND_SELECT_OBJECT, args: [loneGrenadier.type, loneGrenadier.id, 0, 0, 0, 0, 0] });
                commands.push({ type: COMMAND_UNIT, args: [UNIT_SCATTER, 0, 0, 0, 0, 0, 0] });
                westBScatterTicks.set(grenadierKey, snapshot.tick);
              } else if (lastScatterTick === undefined || snapshot.tick - lastScatterTick >= 60) {
                issueSupportClearOrder(grenadiers, tank);
              }
            } else if (grenadiers.length > 1) {
              issueSupportClearOrder(grenadiers, tank);
            }
            westBLateTankAttackTick = snapshot.tick;
          }
          if (trace && snapshot.tick % 30 === 0) console.error(JSON.stringify({
            integratedLateTank: true,
            tick: snapshot.tick,
            phase: westBLateTankPhase,
            e2: grenadiers.map(({ id, strength, cellX, cellY }) => ({ id, strength, cellX, cellY })),
            tank: tank && { id: tank.id, strength: tank.strength, cellX: tank.cellX, cellY: tank.cellY },
          }));
        } else {
          const buggy = hostiles.filter((hostile) => hostile.typeName === "BGGY")
            .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0];
          const supportTank = hostiles.find((hostile) => hostile.typeName === "LTNK");
          const supportInfantry = hostiles.filter((hostile) => hostile.typeName === "E1")
            .toSorted((left, right) => left.strength - right.strength || left.id - right.id)[0];
          if (screen.length > 0 && snapshot.tick - westBSupportClearScreenTick >= 90) {
            issueSupportClearOrder(screen, buggy
              ? { cellX: 43, cellY: 53 }
              : supportTank ?? { cellX: 43, cellY: 53 });
            westBSupportClearScreenTick = snapshot.tick;
          }
          const retreatPoints = [
            { cellX: 42, cellY: 44 },
            { cellX: 45, cellY: 44 },
            { cellX: 48, cellY: 44 },
            { cellX: 50, cellY: 43 },
          ];
          const earlyReducedStagePoints = new Map();
          if (grenadiers.length === 3) {
            const byStrength = grenadiers.toSorted((left, right) =>
              right.strength - left.strength || left.id - right.id);
            earlyReducedStagePoints.set(objectKey(byStrength[0]), { cellX: 46, cellY: 51 });
            earlyReducedStagePoints.set(objectKey(byStrength[1]), { cellX: 44, cellY: 48 });
            earlyReducedStagePoints.set(objectKey(byStrength[2]), { cellX: 43, cellY: 53 });
          }
          for (const [index, grenadier] of grenadiers.entries()) {
            const retreatPoint = earlyReducedStagePoints.get(objectKey(grenadier))
              ?? retreatPoints[index % retreatPoints.length];
            const ready = Math.abs(grenadier.cellX - retreatPoint.cellX) <= 1
              && Math.abs(grenadier.cellY - retreatPoint.cellY) <= 1;
            if (snapshot.tick - (westBSupportClearE2Ticks.get(objectKey(grenadier)) ?? -Infinity)
              >= (ready ? 90 : 60)) {
              if (buggy) {
                issueSupportClearOrder([grenadier], retreatPoint);
              } else {
                commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
                commands.push({ type: COMMAND_SELECT_OBJECT, args: [grenadier.type, grenadier.id, 0, 0, 0, 0, 0] });
                commands.push({ type: COMMAND_UNIT, args: [UNIT_REQUEST_STOP, 0, 0, 0, 0, 0, 0] });
              }
              westBSupportClearE2Ticks.set(objectKey(grenadier), snapshot.tick);
            }
          }
          if (reserveApc && westBSupportClearPhase === "flank") {
            const flankRoute = [
              { cellX: 43, cellY: 48 },
              { cellX: 43, cellY: 52 },
              { cellX: 43, cellY: 57 },
              { cellX: 58, cellY: 57 },
            ];
            const waypoint = flankRoute[westBSupportClearRetreatStage] ?? flankRoute.at(-1);
            if (Math.abs(reserveApc.cellX - waypoint.cellX) <= 1
              && Math.abs(reserveApc.cellY - waypoint.cellY) <= 1) {
              if (westBSupportClearRetreatStage < flankRoute.length - 1) {
                westBSupportClearRetreatStage += 1;
                westBSupportClearOrderTick = -Infinity;
              } else {
                westBSupportClearPhase = "attack";
                westBSupportClearTargetKey = undefined;
                westBSupportClearRetreatStage = 0;
                westBSupportClearOrderTick = -Infinity;
              }
            }
            const nextWaypoint = flankRoute[westBSupportClearRetreatStage] ?? flankRoute.at(-1);
            if (westBSupportClearPhase === "flank"
              && snapshot.tick - westBSupportClearOrderTick >= 45) {
              issueSupportClearOrder([reserveApc], nextWaypoint);
              westBSupportClearOrderTick = snapshot.tick;
            }
          }
          if (reserveApc && westBSupportClearPhase === "attack" && buggy) {
            const buggyKey = objectKey(buggy);
            if (westBSupportClearTargetKey !== buggyKey
              || snapshot.tick - westBSupportClearOrderTick >= 45) {
              issueSupportClearOrder([reserveApc], buggy);
              westBSupportClearTargetKey = buggyKey;
              westBSupportClearOrderTick = snapshot.tick;
            }
          }
          if (reserveApc && !buggy && supportTank
            && snapshot.tick - westBSupportClearOrderTick >= 45) {
            issueSupportClearOrder([reserveApc], supportInfantry ?? { cellX: 43, cellY: 55 });
            westBSupportClearOrderTick = snapshot.tick;
          }
          if (trace && snapshot.tick % 30 === 0) console.error(JSON.stringify({
            integratedSupportClear: true,
            tick: snapshot.tick,
            phase: westBSupportClearPhase,
            apc: reserveApc && { strength: reserveApc.strength, cellX: reserveApc.cellX, cellY: reserveApc.cellY },
            buggy: buggy && { strength: buggy.strength, cellX: buggy.cellX, cellY: buggy.cellY },
          }));
        }
      }
      if (trace && snapshot.tick % 300 === 0) console.error(JSON.stringify({ wb: true, tick: snapshot.tick, phase: westBPhase, transit: westBRouteStage, combat: westBCombatStage, guardPhase: westBGuardPhase, lineEstablished: westBLineEstablished, lurePhase: westBLurePhase, lureTarget: westBLureTankKey, lureDecoy: westBLureDecoyKey, friendly: friendly.map(({typeName,id,strength,cellX,cellY})=>({typeName,id,strength,cellX,cellY})), hostile: hostiles.length, visible: visibleHostiles.map(({typeName,id,strength,cellX,cellY})=>({typeName,id,strength,cellX,cellY})), peakFriendly }));
      if (trace && westBPhase === "transit" && westBRouteStage >= 6) console.error(JSON.stringify({ crossing: true, tick: snapshot.tick, stage: westBRouteStage, hostile: hostiles.length, travelers: attackers.filter((attacker) => westBInitialVehicleKeys.has(objectKey(attacker))).map(({typeName,id,cellX,cellY})=>({typeName,id,cellX,cellY})) }));
    }
    if (activeMissionFourScout) {
      const scoutTarget = mission.scoutRoute[missionFourScoutStage];
      if (Math.abs(activeMissionFourScout.cellX - scoutTarget.cellX) <= 1
        && Math.abs(activeMissionFourScout.cellY - scoutTarget.cellY) <= 1) {
        missionFourScoutArrivalTicks.push(snapshot.tick);
        missionFourScoutStage += 1;
      }
      const nextScoutTarget = mission.scoutRoute[missionFourScoutStage];
      if (nextScoutTarget) {
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        commands.push({
          type: COMMAND_SELECT_OBJECT,
          args: [activeMissionFourScout.type, activeMissionFourScout.id, 0, 0, 0, 0, 0],
        });
        commands.push({
          type: COMMAND_INPUT,
          args: [
            INPUT_COMMAND_AT_POSITION,
            nextScoutTarget.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            nextScoutTarget.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0,
          ],
        });
        selectionCommands += 1;
        contextualOrders += 1;
      }
    }
    let missionThreeDeploying = false;
    if (mission.number === 3 && !friendly.some((object) => object.type === 4)) {
      const mcv = friendly.find((object) => object.typeName === "MCV");
      if (mcv) {
        missionThreeDeploying = true;
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        commands.push({ type: COMMAND_SELECT_OBJECT, args: [mcv.type, mcv.id, 0, 0, 0, 0, 0] });
        commands.push({
          type: COMMAND_INPUT,
          args: [INPUT_COMMAND_AT_POSITION, mcv.cellX * CELL_PIXELS + CELL_PIXELS / 2, mcv.cellY * CELL_PIXELS + CELL_PIXELS / 2, 0, 0, 0, 0],
        });
        deploymentOrders += 1;
        selectionCommands += 1;
        contextualOrders += 1;
      }
    }
    if (mission.number === 5 && mission.variant === "west-b") {
      const fireSaleStructures = friendly.filter((object) => (
        object.type === 4
        && object.typeName === "WEAP"
        && missionFiveCompletedVehicleKeys.size >= 1
        && (object.objectFlags & (1 << 5))
        && !missionFiveSoldStructureIds.has(object.id)
      ));
      if (fireSaleStructures.length > 0) {
        for (const building of fireSaleStructures) {
          commands.push({ type: COMMAND_STRUCTURE, args: [STRUCTURE_SELL, building.id, 0, 0, 0, 0, 0] });
          missionFiveSoldStructureIds.add(building.id);
        }
      }

      if (missionFiveShuttleFactCaptureTick !== undefined) {
        const shuttleContextOrder = (group, destination, flags = 0) => {
          if (group.length === 0 || !destination) return;
          commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
          for (const object of group) {
            commands.push({
              type: COMMAND_SELECT_OBJECT,
              args: [object.type, object.id, 0, 0, 0, 0, 0],
            });
          }
          if (flags) {
            commands.push({
              type: COMMAND_INPUT,
              flags,
              args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
            });
          }
          commands.push({
            type: COMMAND_INPUT,
            flags,
            args: [
              INPUT_COMMAND_AT_POSITION,
              destination.cellX * CELL_PIXELS + CELL_PIXELS / 2,
              destination.cellY * CELL_PIXELS + CELL_PIXELS / 2,
              0, 0, 0, 0,
            ],
          });
          if (flags) {
            commands.push({
              type: COMMAND_INPUT,
              args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
            });
          }
          selectionCommands += group.length;
          contextualOrders += 1;
        };
        const shuttleStop = (group) => {
          if (group.length === 0) return;
          commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
          for (const object of group) {
            commands.push({
              type: COMMAND_SELECT_OBJECT,
              args: [object.type, object.id, 0, 0, 0, 0, 0],
            });
          }
          commands.push({ type: COMMAND_UNIT, args: [UNIT_REQUEST_STOP, 0, 0, 0, 0, 0, 0] });
          selectionCommands += group.length;
        };

        const shuttleApc = friendly.find((object) => (
          objectKey(object) === missionFiveWestBApcKey
        ));
        const shuttlePips = shuttleApc?.pips.slice(0, shuttleApc.pipCount)
          .filter((pip) => pip !== 0) ?? [];
        const visibleShuttleEngineers = friendly.filter((object) => (
          object.type === 1 && object.typeName === "E6"
          && missionFiveShuttleEngineers.has(objectKey(object))
        )).toSorted((left, right) => (
          (missionFiveShuttleEngineers.get(objectKey(left))?.tick ?? Infinity)
            - (missionFiveShuttleEngineers.get(objectKey(right))?.tick ?? Infinity)
          || left.id - right.id
        ));

        const capturedFactoryForSale = friendly.find((object) => (
          object.type === 4 && object.typeName === "FACT"
          && object.cellX === 52 && object.cellY === 17
        ));
        if (missionFiveShuttleFactSaleTick === undefined
          && capturedFactoryForSale && (capturedFactoryForSale.objectFlags & (1 << 5))) {
          missionFiveShuttleFactSaleTick = snapshot.tick;
          missionFiveShuttleFactSaleFunds = snapshot.sidebar.credits + snapshot.sidebar.tiberium;
          commands.push({
            type: COMMAND_STRUCTURE,
            args: [STRUCTURE_SELL, capturedFactoryForSale.id, 0, 0, 0, 0, 0],
          });
          missionFiveSoldStructureIds.add(capturedFactoryForSale.id);
          if (trace) console.error(JSON.stringify({ westBShuttleFactSaleOrder: {
            tick: snapshot.tick,
            funds: missionFiveShuttleFactSaleFunds,
            strength: capturedFactoryForSale.strength,
          } }));
        }

        const engineerEntry = snapshot.sidebar.entries.find((entry) => entry.assetName === "E6");
        if (missionFiveShuttleFactGoneTick !== undefined
          && missionFiveShuttleEngineers.size < 4 && engineerEntry) {
          if (engineerEntry.onHold
            && snapshot.tick - missionFiveShuttleEngineerResumeTick >= 120) {
            commands.push({
              type: COMMAND_SIDEBAR,
              args: [SIDEBAR_START_CONSTRUCTION,
                engineerEntry.buildableType, engineerEntry.buildableId, 0, 0, 0, 0],
            });
            missionFiveShuttleEngineerResumeTick = snapshot.tick;
            if (trace) console.error(JSON.stringify({ westBShuttleEngineerResume: {
              tick: snapshot.tick,
              starts: missionFiveShuttleEngineerStarts,
              progress: engineerEntry.progress,
              funds: snapshot.sidebar.credits + snapshot.sidebar.tiberium,
            } }));
          } else if (!engineerEntry.constructing && !engineerEntry.completed
            && !engineerEntry.onHold && !engineerEntry.busy
            && missionFiveShuttleEngineerStarts < 4
            && snapshot.sidebar.credits + snapshot.sidebar.tiberium >= engineerEntry.cost
            && snapshot.tick - missionFiveShuttleEngineerStartTick >= 120) {
            commands.push({
              type: COMMAND_SIDEBAR,
              args: [SIDEBAR_START_CONSTRUCTION,
                engineerEntry.buildableType, engineerEntry.buildableId, 0, 0, 0, 0],
            });
            missionFiveShuttleEngineerStarts += 1;
            missionFiveShuttleEngineerStartTick = snapshot.tick;
            productionStarts += 1;
            infantryProductionStarts += 1;
            if (trace) console.error(JSON.stringify({ westBShuttleEngineerStart: {
              tick: snapshot.tick,
              index: missionFiveShuttleEngineerStarts,
              cost: engineerEntry.cost,
              funds: snapshot.sidebar.credits + snapshot.sidebar.tiberium,
            } }));
          }
        }

        const distanceTo = (object, point) => Math.max(
          Math.abs(object.cellX - point.cellX),
          Math.abs(object.cellY - point.cellY),
        );
        if (missionFiveShuttlePhase === "return" && shuttleApc) {
          const returnRoute = [
            { cellX: 56, cellY: 29 },
            { cellX: 53, cellY: 42 },
            { cellX: 53, cellY: 53 },
            { cellX: 42, cellY: 54 },
            { cellX: 31, cellY: 58 },
          ];
          const waypoint = returnRoute[missionFiveShuttleRouteStage];
          if (waypoint && distanceTo(shuttleApc, waypoint) <= 2) {
            missionFiveShuttleRouteStage += 1;
            missionFiveShuttleOrderTick = -Infinity;
          }
          const nextWaypoint = returnRoute[missionFiveShuttleRouteStage];
          if (!nextWaypoint) {
            missionFiveShuttlePhase = "await-load";
            missionFiveShuttleOrderTick = -Infinity;
            shuttleStop([shuttleApc]);
            if (trace) console.error(JSON.stringify({ westBShuttlePhase: {
              tick: snapshot.tick, phase: missionFiveShuttlePhase,
              apc: { strength: shuttleApc.strength, cellX: shuttleApc.cellX, cellY: shuttleApc.cellY },
            } }));
          } else if (snapshot.tick - missionFiveShuttleOrderTick >= 60) {
            shuttleContextOrder([shuttleApc], nextWaypoint, MODIFIER_ALT);
            missionFiveShuttleOrderTick = snapshot.tick;
          }
        } else if (missionFiveShuttlePhase === "await-load" && shuttleApc) {
          if (shuttlePips.filter((pip) => pip === 5).length >= 5) {
            missionFiveShuttlePhase = "outbound";
            missionFiveShuttleRouteStage = 0;
            missionFiveShuttleOrderTick = -Infinity;
            if (trace) console.error(JSON.stringify({ westBShuttlePhase: {
              tick: snapshot.tick, phase: missionFiveShuttlePhase,
              pips: shuttlePips,
              apc: { strength: shuttleApc.strength, cellX: shuttleApc.cellX, cellY: shuttleApc.cellY },
            } }));
          } else if (missionFiveShuttleEngineers.size >= 5
            && visibleShuttleEngineers.length > 0
            && snapshot.tick - missionFiveShuttleOrderTick >= 60) {
            shuttleContextOrder(visibleShuttleEngineers, shuttleApc);
            missionFiveShuttleOrderTick = snapshot.tick;
          } else if (snapshot.tick - missionFiveShuttleOrderTick >= 300) {
            shuttleStop([shuttleApc]);
            missionFiveShuttleOrderTick = snapshot.tick;
          }
        } else if (missionFiveShuttlePhase === "outbound" && shuttleApc) {
          const outboundRoute = [
            { cellX: 42, cellY: 54 },
            { cellX: 53, cellY: 53 },
            { cellX: 53, cellY: 42 },
            { cellX: 56, cellY: 29 },
          ];
          const waypoint = outboundRoute[missionFiveShuttleRouteStage];
          if (waypoint && distanceTo(shuttleApc, waypoint) <= 2) {
            missionFiveShuttleRouteStage += 1;
            missionFiveShuttleOrderTick = -Infinity;
          }
          const nextWaypoint = outboundRoute[missionFiveShuttleRouteStage];
          if (!nextWaypoint) {
            missionFiveShuttlePhase = "unloading";
            missionFiveShuttleUnloadTick = -Infinity;
            shuttleStop([shuttleApc]);
            if (trace) console.error(JSON.stringify({ westBShuttlePhase: {
              tick: snapshot.tick, phase: missionFiveShuttlePhase,
              pips: shuttlePips,
              apc: { strength: shuttleApc.strength, cellX: shuttleApc.cellX, cellY: shuttleApc.cellY },
            } }));
          } else if (snapshot.tick - missionFiveShuttleOrderTick >= 60) {
            shuttleContextOrder([shuttleApc], nextWaypoint, MODIFIER_ALT);
            missionFiveShuttleOrderTick = snapshot.tick;
          }
        } else if (missionFiveShuttlePhase === "unloading" && shuttleApc) {
          if (shuttlePips.length === 0 && visibleShuttleEngineers.length >= 5) {
            missionFiveShuttlePhase = "raid";
            if (trace) console.error(JSON.stringify({ westBShuttlePhase: {
              tick: snapshot.tick, phase: missionFiveShuttlePhase,
              engineers: visibleShuttleEngineers.map((engineer) => ({
                key: objectKey(engineer), strength: engineer.strength,
                cellX: engineer.cellX, cellY: engineer.cellY,
              })),
            } }));
          } else if (snapshot.tick - missionFiveShuttleUnloadTick >= 90) {
            shuttleContextOrder([shuttleApc], shuttleApc);
            missionFiveShuttleUnloadTick = snapshot.tick;
          }
        }

        if (missionFiveShuttlePhase === "raid") {
          const raidSites = [
            { key: "PROC:47:22", typeName: "PROC", cellX: 47, cellY: 22,
              approach: { cellX: 53, cellY: 27 } },
            { key: "AFLD:42:18", typeName: "AFLD", cellX: 42, cellY: 18,
              approach: { cellX: 46, cellY: 24 } },
            { key: "NUKE:47:18", typeName: "NUKE", cellX: 47, cellY: 18,
              approach: { cellX: 50, cellY: 24 } },
            { key: "NUKE:49:17", typeName: "NUKE", cellX: 49, cellY: 17,
              approach: { cellX: 52, cellY: 23 } },
          ];
          for (let index = 0; index < visibleShuttleEngineers.length; index += 1) {
            const engineer = visibleShuttleEngineers[index];
            const engineerKey = objectKey(engineer);
            if (!missionFiveShuttleAssignments.has(engineerKey)) {
              const site = raidSites[index];
              if (!site) continue;
              missionFiveShuttleAssignments.set(engineerKey, site.key);
              missionFiveShuttleRaidStages.set(engineerKey, 0);
              if (trace) console.error(JSON.stringify({ westBShuttleAssignment: {
                tick: snapshot.tick,
                engineer: engineerKey,
                target: site.key,
                strength: engineer.strength,
                cellX: engineer.cellX,
                cellY: engineer.cellY,
              } }));
            }
          }
          for (const engineer of visibleShuttleEngineers) {
            const engineerKey = objectKey(engineer);
            const site = raidSites.find((candidate) => (
              candidate.key === missionFiveShuttleAssignments.get(engineerKey)
            ));
            const target = site && hostiles.find((hostile) => (
              hostile.type === 4 && hostile.typeName === site.typeName
              && hostile.cellX === site.cellX && hostile.cellY === site.cellY
            ));
            if (!site || !target) continue;
            if (site.key === "AFLD:42:18"
              && missionFiveShuttleCaptureKeys.size < 3) continue;
            let stage = missionFiveShuttleRaidStages.get(engineerKey) ?? 0;
            if (stage === 0 && distanceTo(engineer, site.approach) <= 2) {
              stage = 1;
              missionFiveShuttleRaidStages.set(engineerKey, stage);
            }
            const lastOrderTick = missionFiveShuttleRaidOrderTicks.get(engineerKey) ?? -Infinity;
            if (snapshot.tick - lastOrderTick >= 60) {
              shuttleContextOrder([engineer], stage === 0 ? site.approach : target,
                stage === 0 ? MODIFIER_ALT : 0);
              missionFiveShuttleRaidOrderTicks.set(engineerKey, snapshot.tick);
            }
            if (trace && snapshot.tick % 300 === 0) {
              console.error(JSON.stringify({ westBShuttleRaid: {
                tick: snapshot.tick,
                engineer: engineerKey,
                target: site.key,
                stage,
                strength: engineer.strength,
                cellX: engineer.cellX,
                cellY: engineer.cellY,
              } }));
            }
          }
        }

        if (trace && snapshot.tick % 300 === 0) {
          console.error(JSON.stringify({ westBShuttle: {
            tick: snapshot.tick,
            phase: missionFiveShuttlePhase,
            routeStage: missionFiveShuttleRouteStage,
            starts: missionFiveShuttleEngineerStarts,
            built: missionFiveShuttleEngineers.size,
            captures: missionFiveShuttleCaptures,
            apc: shuttleApc && {
              strength: shuttleApc.strength,
              cellX: shuttleApc.cellX,
              cellY: shuttleApc.cellY,
              pips: shuttlePips,
            },
            visibleEngineers: visibleShuttleEngineers.map((engineer) => ({
              key: objectKey(engineer), strength: engineer.strength,
              cellX: engineer.cellX, cellY: engineer.cellY,
            })),
          } }));
        }
      }
    }
    if (mission.number >= 2) {
      for (const building of friendly.filter((object) => (
        object.type === 4
        && !missionFiveSoldStructureIds.has(object.id)
        && !(missionFiveWestBStrategy && missionFiveShuttleFactCaptureTick !== undefined)
        && object.strength < object.maxStrength
        && !(object.objectFlags & (1 << 1))
        && (mission.number !== 3 || snapshot.sidebar.credits + snapshot.sidebar.tiberium >= 1_000)
        && (mission.number !== 5 || snapshot.sidebar.credits + snapshot.sidebar.tiberium >= 300)
        && (mission.number !== 3 || snapshot.tick - (missionThreeRepairTicks.get(object.id) ?? -900) >= 900)
        && (mission.number !== 5 || snapshot.tick - (missionFiveRepairTicks.get(object.id) ?? -600) >= 600)
      ))) {
        if (mission.number !== 3 && mission.number !== 5 && repairedBuildingIds.has(building.id)) continue;
        commands.push({ type: COMMAND_STRUCTURE, args: [STRUCTURE_REPAIR_START, 0, 0, 0, 0, 0, 0] });
        commands.push({ type: COMMAND_STRUCTURE, args: [STRUCTURE_REPAIR, building.id, 0, 0, 0, 0, 0] });
        if (mission.number === 3) missionThreeRepairTicks.set(building.id, snapshot.tick);
        else if (mission.number === 5) missionFiveRepairTicks.set(building.id, snapshot.tick);
        else repairedBuildingIds.add(building.id);
        repairOrders += 1;
      }
    }
    if (mission.number === 2) {
      const minigunner = snapshot.sidebar.entries.find((entry) => entry.assetName === "E1");
      if (minigunner && !minigunner.constructing && !minigunner.completed && !minigunner.onHold && !minigunner.busy
        && snapshot.sidebar.credits + snapshot.sidebar.tiberium >= minigunner.cost) {
        commands.push({
          type: COMMAND_SIDEBAR,
          args: [SIDEBAR_START_CONSTRUCTION, minigunner.buildableType, minigunner.buildableId, 0, 0, 0, 0],
        });
        productionStarts += 1;
      }
    }
    if (mission.number === 5
      && missionFiveReliefStage >= mission.reliefRoute.length
      && missionFiveBaseRepairedTick !== undefined
      && (mission.crate === undefined || missionFiveCrateCollectedTick !== undefined)) {
      let availableFunds = snapshot.sidebar.credits + snapshot.sidebar.tiberium;
      const preferredInfantry = mission.variant === "west-b"
        ? missionFiveShuttleFactGoneTick !== undefined
          ? "E2"
          : infantryProductionStarts % 4 === 3 ? "E2" : "E1"
        : infantryProductionStarts % 3 === 2 ? "E1" : "E2";
      const infantry = snapshot.sidebar.entries.find((entry) => entry.assetName === preferredInfantry)
        ?? snapshot.sidebar.entries.find((entry) => entry.assetName === (preferredInfantry === "E1" ? "E2" : "E1"));
      const engineer = mission.variant === "west-b"
        ? snapshot.sidebar.entries.find((entry) => entry.assetName === "E6")
        : undefined;
      let queuedEngineer = false;
      if (engineer && missionFiveWestBEngineerProductionStarted < 5
        && !engineer.constructing && !engineer.completed && !engineer.onHold && !engineer.busy
        && availableFunds >= engineer.cost + 200) {
        commands.push({
          type: COMMAND_SIDEBAR,
          args: [SIDEBAR_START_CONSTRUCTION, engineer.buildableType, engineer.buildableId, 0, 0, 0, 0],
        });
        missionFiveWestBEngineerProductionStarted += 1;
        queuedEngineer = true;
        productionStarts += 1;
        infantryProductionStarts += 1;
        availableFunds -= engineer.cost;
        if (trace) console.error(JSON.stringify({ westBEngineerProduction: true, tick: snapshot.tick, cost: engineer.cost }));
      }
      const liveApcAvailable = friendly.some((object) => object.typeName === "APC" && object.type === 2);
      const preferredVehicles = mission.variant === "west-b" && !liveApcAvailable ? ["APC", "JEEP"] : ["JEEP", "APC"];
      const vehicle = preferredVehicles
        .map((assetName) => snapshot.sidebar.entries.find((entry) => entry.assetName === assetName))
        .find(Boolean);
      const queueVehicle = (reserve) => {
        if (!vehicle || vehicle.constructing || vehicle.completed || vehicle.onHold || vehicle.busy
          || availableFunds < vehicle.cost + reserve) return;
        commands.push({
          type: COMMAND_SIDEBAR,
          args: [SIDEBAR_START_CONSTRUCTION, vehicle.buildableType, vehicle.buildableId, 0, 0, 0, 0],
        });
        productionStarts += 1;
        vehicleProductionStarts += 1;
        availableFunds -= vehicle.cost;
      };
      if (mission.variant !== "west-a" && (vehicleProductionStarts < 1
        || (mission.variant === "west-b"
          && missionFiveWestBEngineerKey !== undefined
          && !liveApcAvailable
          && vehicleProductionStarts < 3))) queueVehicle(200);
      const infantryReserve = missionFiveAssaultStartedTick === undefined ? 200 : 0;
      if (!queuedEngineer && infantry && !infantry.constructing && !infantry.completed && !infantry.onHold && !infantry.busy
        && availableFunds >= infantry.cost + infantryReserve) {
        commands.push({
          type: COMMAND_SIDEBAR,
          args: [SIDEBAR_START_CONSTRUCTION, infantry.buildableType, infantry.buildableId, 0, 0, 0, 0],
        });
        productionStarts += 1;
        infantryProductionStarts += 1;
        availableFunds -= infantry.cost;
        if (mission.variant === "west-b" && missionFiveShuttleFactGoneTick !== undefined) {
          if (trace) console.error(JSON.stringify({ westBPostRefundInfantry: {
            tick: snapshot.tick,
            assetName: infantry.assetName,
            cost: infantry.cost,
            fundsAfterQueue: availableFunds,
          } }));
        }
      }
      if (mission.variant === "west-a") queueVehicle(600);
    }
    if (mission.number === 3 && !missionThreeDeploying) {
      const builtAssets = new Set(friendly.filter((object) => object.type === 4).map((object) => object.typeName));
      const completedStructure = snapshot.sidebar.entries.find((entry) => entry.objectType === 15 && entry.completed);
      const missingStructure = ["NUKE", "PYLE", "PROC"].find((assetName) => (
        !builtAssets.has(assetName) && !startedMissionThreeStructures.has(assetName)
      ));
      const structure = completedStructure ?? (missingStructure
        ? snapshot.sidebar.entries.find((entry) => entry.assetName === missingStructure && entry.objectType === 15)
        : undefined);
      if (structure?.completed) {
        if (snapshot.placement) {
          const cell = findLegalPlacement(snapshot, structure);
          assert.ok(cell, `no legal ${structure.assetName} placement was exported`);
          commands.push({
            type: COMMAND_SIDEBAR,
            args: [SIDEBAR_PLACE, structure.buildableType, structure.buildableId, cell.x, cell.y, 0, 0],
          });
          placements += 1;
        } else {
          commands.push({
            type: COMMAND_SIDEBAR,
            args: [SIDEBAR_START_PLACEMENT, structure.buildableType, structure.buildableId, 0, 0, 0, 0],
          });
          placementStarts += 1;
        }
      } else if (structure && !structure.constructing && !structure.onHold && !structure.busy
        && snapshot.sidebar.credits + snapshot.sidebar.tiberium >= structure.cost) {
        commands.push({
          type: COMMAND_SIDEBAR,
          args: [SIDEBAR_START_CONSTRUCTION, structure.buildableType, structure.buildableId, 0, 0, 0, 0],
        });
        startedMissionThreeStructures.add(structure.assetName);
        productionStarts += 1;
      }

      if (builtAssets.has("PYLE")) {
        const preferredInfantry = infantryProductionStarts % 3 === 0 ? "E1" : "E2";
        const infantry = snapshot.sidebar.entries.find((entry) => entry.assetName === preferredInfantry)
          ?? snapshot.sidebar.entries.find((entry) => entry.assetName === (preferredInfantry === "E1" ? "E2" : "E1"));
        if (infantry && !infantry.constructing && !infantry.completed && !infantry.onHold && !infantry.busy
          && snapshot.sidebar.credits + snapshot.sidebar.tiberium >= infantry.cost + 800) {
          commands.push({
            type: COMMAND_SIDEBAR,
            args: [SIDEBAR_START_CONSTRUCTION, infantry.buildableType, infantry.buildableId, 0, 0, 0, 0],
          });
          productionStarts += 1;
          infantryProductionStarts += 1;
        }
      }
    }
    const missionThreeBaseReady = ["NUKE", "PYLE", "PROC"].every((assetName) => (
      friendly.some((object) => object.type === 4 && object.typeName === assetName)
    ));
    if (mission.number === 3 && missionThreeScoutId === undefined && missionThreeBaseReady) {
      const candidate = attackers.find((attacker) => attacker.typeName === "JEEP");
      if (candidate) missionThreeScoutId = candidate.id;
    }
    const activeScout = mission.number === 3
      ? attackers.find((attacker) => attacker.id === missionThreeScoutId)
      : undefined;
    if (mission.number === 3 && missionThreeScoutId !== undefined && !activeScout && snapshot.tick >= 6_000) {
      missionThreeScoutStage = missionThreeScoutRoute.length;
    }
    if (activeScout && missionThreeScoutStage < missionThreeScoutRoute.length) {
      const scoutTarget = missionThreeScoutRoute[missionThreeScoutStage];
      if (Math.abs(activeScout.cellX - scoutTarget.cellX) <= 1
        && Math.abs(activeScout.cellY - scoutTarget.cellY) <= 1) {
        missionThreeScoutArrivalTicks.push(snapshot.tick);
        missionThreeScoutStage += 1;
      }
      const nextScoutTarget = missionThreeScoutRoute[missionThreeScoutStage];
      if (nextScoutTarget) {
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        commands.push({ type: COMMAND_SELECT_OBJECT, args: [activeScout.type, activeScout.id, 0, 0, 0, 0, 0] });
        commands.push({
          type: COMMAND_INPUT,
          args: [
            INPUT_COMMAND_AT_POSITION,
            nextScoutTarget.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            nextScoutTarget.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0,
          ],
        });
        selectionCommands += 1;
        contextualOrders += 1;
      }
    }
    const assaultReady = mission.number === 1
      || (mission.number === 2 && (snapshot.tick >= missionTwoAssaultTick || !friendly.some((object) => object.type === 4)))
      || (mission.number === 3 && missionThreeBaseReady && missionThreeScoutStage >= missionThreeScoutRoute.length
        && (attackers.length >= 70 || snapshot.tick >= missionThreeAssaultTick))
      || (mission.number === 5 && missionFiveReliefStage >= mission.reliefRoute.length
        && (mission.crate === undefined || missionFiveCrateCollectedTick !== undefined)
        && snapshot.tick >= (mission.earliestAssaultTick ?? 0)
        && (attackers.length >= missionFiveAssaultForce
          || (mission.variant === "west-b"
            && missionFiveWestBApcKey !== undefined
            && allAttackers.length >= missionFiveAssaultForce)
          || snapshot.tick >= missionFiveAssaultTick
          || (mission.variant === "west-b" && missionFiveAssaultWaveCount > 0
            && attackers.length >= (mission.relaunchForce ?? 26))));
    if (mission.number === 3 && assaultReady && !missionThreeBaseAssaultStarted) {
      missionThreeBaseAssaultStarted = true;
      missionThreeAssaultStartedTick = snapshot.tick;
    }
    if (mission.number === 5) {
      for (const key of missionFiveStrikeGroupKeys) {
        if (!attackers.some((attacker) => objectKey(attacker) === key)) missionFiveStrikeGroupKeys.delete(key);
      }
      for (const key of missionFiveHomeGuardKeys) {
        if (!attackers.some((attacker) => objectKey(attacker) === key)) missionFiveHomeGuardKeys.delete(key);
      }
      const westBFactoryStillLive = mission.variant !== "west-b" || hostiles.some((hostile) => (
        hostile.typeName === "FACT" && hostile.cellX === 52 && hostile.cellY === 17
      ));
      if (mission.variant === "west-b" && !westBFactoryStillLive) {
        for (const key of missionFiveHomeGuardKeys) missionFiveStrikeGroupKeys.add(key);
        missionFiveHomeGuardKeys.clear();
        if (missionFiveAssaultRouteStage === (mission.factoryRouteStage ?? mission.assaultTargetStage)
          && !(missionFiveShuttleFactCaptureTick !== undefined
            && missionFiveShuttleCaptures.length < 4)) {
          missionFiveAssaultRouteStage += 1;
          missionFiveAssaultRouteArrivalTicks.push(snapshot.tick);
          missionFiveAssaultProgressTick = snapshot.tick;
        }
      }
      const initialHuntStructuresRemain = [...missionFiveInitialHuntStructureKeys].some((key) => (
        hostiles.some((hostile) => objectKey(hostile) === key)
      ));
      if (!initialHuntStructuresRemain
        && hostiles.length > 0
        && hostiles.every((hostile) => hostile.type === 4)) {
        missionFiveAssaultPhase = "cleanup";
        missionFiveStrikeGroupKeys.clear();
      }
      const missionFiveStrikeExhaustionThreshold = mission.variant === "west-b"
        && missionFiveHuntTriggeredTick === undefined
        && missionFiveAssaultRouteStage === 6
        ? 1
        : 3;
      if (missionFiveAssaultPhase === "assault"
        && !(missionFiveShuttleFactCaptureTick !== undefined
          && missionFiveShuttleCaptures.length < 4)
        && ((snapshot.tick - missionFiveWaveLaunchedTick >= 300
          && missionFiveStrikeGroupKeys.size < missionFiveStrikeExhaustionThreshold)
          || snapshot.tick - missionFiveAssaultProgressTick >= 6_000)) {
        missionFiveAssaultPhase = "rebuild";
        missionFiveStrikeGroupKeys.clear();
        missionFiveAssaultRouteStage = mission.variant === "west-b"
          ? mission.coreRouteHolds?.find(({ sites }) => (
            sites.some((site) => hostiles.some((hostile) => (
              hostile.typeName === site.typeName
              && hostile.cellX === site.cellX
              && hostile.cellY === site.cellY
            )))
          ))?.routeStage ?? 0
          : 0;
        missionFiveAssaultProgressTick = snapshot.tick;
      }
      const launchThreshold = missionFiveAssaultWaveCount === 0
        ? snapshot.tick >= missionFiveAssaultTick
          ? mission.variant === "west-b" ? 26 : mission.variant === "east-a" ? 34 : 35
          : mission.variant === "west-b" && missionFiveWestBApcKey !== undefined
            ? missionFiveAssaultForce - 1
            : missionFiveAssaultForce
        : mission.variant === "west-b" ? mission.relaunchForce ?? 26
          : mission.variant === "east-a" ? 28 : mission.relaunchForce ?? 40;
      if (assaultReady && (missionFiveAssaultPhase === "staging" || missionFiveAssaultPhase === "rebuild")
        && attackers.length >= launchThreshold) {
        missionFiveAssaultPhase = "assault";
        missionFiveAssaultWaveCount += 1;
        missionFiveWaveLaunchedTick = snapshot.tick;
        missionFiveAssaultProgressTick = snapshot.tick;
        if (missionFiveAssaultStartedTick === undefined) missionFiveAssaultStartedTick = snapshot.tick;
      }
      if (missionFiveAssaultPhase === "assault") {
        const evictedGuardTankKeys = [];
        if (mission.homeGuardSize && westBFactoryStillLive
          && attackers.filter((attacker) => attacker.typeName === "MTNK").length < 2) {
          for (const attacker of attackers.filter((candidate) => candidate.typeName === "MTNK")) {
            const key = objectKey(attacker);
            if (missionFiveHomeGuardKeys.delete(key)) evictedGuardTankKeys.push(key);
          }
        }
        if (mission.homeGuardSize && westBFactoryStillLive
          && missionFiveHomeGuardKeys.size < mission.homeGuardSize) {
          const guardPriority = mission.variant === "west-b"
            ? new Map([["E1", 0], ["E2", 0], ["JEEP", 1], ["APC", 2], ["MTNK", 3]])
            : new Map([["MTNK", 0], ["JEEP", 1], ["APC", 2], ["E2", 3], ["E1", 4]]);
          const guardCandidates = attackers.filter((attacker) => (
            !missionFiveStrikeGroupKeys.has(objectKey(attacker))
            && !missionFiveHomeGuardKeys.has(objectKey(attacker))
          )).toSorted((left, right) => (
            (guardPriority.get(left.typeName) ?? 20) - (guardPriority.get(right.typeName) ?? 20)
            || right.strength - left.strength
            || left.id - right.id
          ));
          let guardTankCount = attackers.filter((attacker) => (
            attacker.typeName === "MTNK" && missionFiveHomeGuardKeys.has(objectKey(attacker))
          )).length;
          const desiredGuardTankCount = attackers.filter((attacker) => attacker.typeName === "MTNK").length >= 2
            ? 1
            : 0;
          const balancedGuardLimit = Math.ceil(mission.homeGuardSize / 2);
          const guardTypeCounts = new Map(["E1", "E2"].map((typeName) => [
            typeName,
            attackers.filter((attacker) => (
              attacker.typeName === typeName && missionFiveHomeGuardKeys.has(objectKey(attacker))
            )).length,
          ]));
          for (const defender of guardCandidates) {
            if (missionFiveHomeGuardKeys.size >= mission.homeGuardSize) break;
            if (defender.typeName === "MTNK" && guardTankCount >= desiredGuardTankCount) continue;
            if (mission.variant === "west-b"
              && ["E1", "E2"].includes(defender.typeName)
              && (guardTypeCounts.get(defender.typeName) ?? 0) >= balancedGuardLimit) continue;
            missionFiveHomeGuardKeys.add(objectKey(defender));
            if (defender.typeName === "MTNK") guardTankCount += 1;
            if (["E1", "E2"].includes(defender.typeName)) {
              guardTypeCounts.set(defender.typeName, (guardTypeCounts.get(defender.typeName) ?? 0) + 1);
            }
          }
          if (mission.variant === "west-b" && missionFiveHomeGuardKeys.size < mission.homeGuardSize) {
            for (const defender of guardCandidates) {
              if (missionFiveHomeGuardKeys.size >= mission.homeGuardSize) break;
              if (missionFiveHomeGuardKeys.has(objectKey(defender))) continue;
              if (defender.typeName === "MTNK" && guardTankCount >= desiredGuardTankCount) continue;
              missionFiveHomeGuardKeys.add(objectKey(defender));
              if (defender.typeName === "MTNK") guardTankCount += 1;
            }
          }
        }
        for (const key of evictedGuardTankKeys) missionFiveStrikeGroupKeys.add(key);
        const reserve = mission.variant === "west-b" && !westBFactoryStillLive
          ? missionFiveHuntTriggeredTick !== undefined && hostiles.some((hostile) => (
            hostile.type !== 4 && (hostile.objectFlags & (1 << 12)) !== 0
          ))
            ? hostiles.some((hostile) => (
              hostile.type !== 4 && (hostile.objectFlags & (1 << 12)) !== 0
              && Math.max(
                Math.abs(hostile.cellX - mission.home.cellX),
                Math.abs(hostile.cellY - mission.home.cellY),
              ) <= 14
            )) ? 50 : hostiles.some((hostile) => (
              hostile.typeName === "HAND" && hostile.cellX === 41 && hostile.cellY === 22
            ))
              ? attackers.length >= 50 ? 30 : attackers.length
              : 5
            : 0
          : mission.homeGuardSize ?? (mission.variant === "east-a" ? 8 : 4);
        const desiredStrikeGroup = Math.min(50, Math.max(0, attackers.length - reserve));
        if (missionFiveWestBStrategy
          && missionFiveHuntTriggeredTick !== undefined
          && reserve === 30
          && desiredStrikeGroup >= 20
          && missionFiveWestBCleanupBatchTick === undefined) {
          missionFiveWestBCleanupBatchTick = snapshot.tick;
          missionFiveWestBCleanupBatchSize = desiredStrikeGroup;
        }
        const priority = new Map([["MTNK", 0], ["APC", 1], ["JEEP", 2], ["E2", 3], ["E1", 4]]);
        const reinforcements = attackers
          .filter((attacker) => (
            !missionFiveStrikeGroupKeys.has(objectKey(attacker))
            && !missionFiveHomeGuardKeys.has(objectKey(attacker))
          ))
          .toSorted((left, right) => (
            (priority.get(left.typeName) ?? 20) - (priority.get(right.typeName) ?? 20)
            || right.strength - left.strength
            || left.type - right.type
            || left.id - right.id
          ));
        if (mission.variant !== "east-a" || snapshot.tick === missionFiveWaveLaunchedTick) {
          for (const attacker of reinforcements) {
            if (missionFiveStrikeGroupKeys.size >= desiredStrikeGroup) break;
            missionFiveStrikeGroupKeys.add(objectKey(attacker));
          }
        }
      }
    }
    const missionFiveStrikeGroup = mission.number === 5
      ? attackers.filter((attacker) => missionFiveStrikeGroupKeys.has(objectKey(attacker)))
      : [];
    const missionFiveAssaultActive = mission.number === 5 && missionFiveAssaultPhase === "assault";
    const missionFiveCleanupActive = mission.number === 5 && missionFiveAssaultPhase === "cleanup";
    const missionFiveForwardGroup = missionFiveCleanupActive ? attackers : missionFiveStrikeGroup;
    if (mission.number === 5 && mission.samSweepSites) {
      while (missionFiveSamSweepStage < mission.samSweepSites.length) {
        const { site } = mission.samSweepSites[missionFiveSamSweepStage];
        const siteTypeName = site.typeName ?? "SAM";
        const liveTarget = hostiles.some((hostile) => (
          hostile.typeName === siteTypeName && hostile.cellX === site.cellX && hostile.cellY === site.cellY
        ));
        if (liveTarget) break;
        if (siteTypeName === "SAM") missionFiveSamDestroyedTicks.push(snapshot.tick);
        missionFiveSamSweepStage += 1;
        missionFiveAssaultProgressTick = snapshot.tick;
      }
      if (missionFiveSamSweepStage >= mission.samSweepSites.length
        && mission.postSweepRouteStage !== undefined
        && missionFiveAssaultRouteStage < mission.postSweepRouteStage) {
        missionFiveAssaultRouteStage = mission.postSweepRouteStage;
      }
    }
    const missionFiveSamSweepSite = mission.number === 5
      ? mission.samSweepSites?.[missionFiveSamSweepStage]
      : undefined;
    const missionFiveSamSweepTarget = missionFiveSamSweepSite
      ? hostiles.find((hostile) => (
        hostile.typeName === (missionFiveSamSweepSite.site.typeName ?? "SAM")
        && hostile.cellX === missionFiveSamSweepSite.site.cellX
        && hostile.cellY === missionFiveSamSweepSite.site.cellY
        && snapshot.shroud.isVisible(hostile.cellX, hostile.cellY)
      )) ?? missionFiveSamSweepSite.approach
      : undefined;
    const missionFiveSamSweepActive = missionFiveAssaultActive
      && missionFiveSamSweepTarget !== undefined
      && missionFiveAssaultRouteStage >= (mission.preSweepRouteStage ?? 0);
    const missionFiveSamSweepThreat = missionFiveSamSweepActive
      ? chooseFormationThreat(
        missionFiveForwardGroup,
        visibleHostiles.filter((object) => object.type !== 4 && (object.objectFlags & (1 << 12)) !== 0),
        7,
      ) ?? chooseFormationThreat(
        missionFiveForwardGroup,
        visibleHostiles.filter((object) => object.typeName === "GUN"),
        10,
      )
      : undefined;
    if ((missionFiveAssaultActive || missionFiveCleanupActive) && !missionFiveSamSweepActive) {
      if (missionFiveAssaultRouteStage >= mission.assaultRoute.length
        && visibleHostiles.length === 0 && hostiles.length > 0) {
        missionFiveAssaultRouteStage = 0;
      }
      const assaultWaypoint = mission.assaultRoute[missionFiveAssaultRouteStage];
      if (assaultWaypoint) {
        const arrivalRadius = missionFiveAssaultRouteStage >= (mission.precisionRouteStage ?? 5) ? 2 : 4;
        const arrivals = missionFiveForwardGroup.filter((attacker) => (
          Math.abs(attacker.cellX - assaultWaypoint.cellX) <= arrivalRadius
          && Math.abs(attacker.cellY - assaultWaypoint.cellY) <= arrivalRadius
        )).length;
        const requiredFormation = missionFiveAssaultRouteStage >= (mission.formationReleaseStage ?? 5)
          ? 1
          : Math.min(missionFiveForwardGroup.length, Math.max(4, Math.ceil(missionFiveForwardGroup.length * 0.6)));
        const coreRouteHold = mission.coreRouteHolds?.find(({ routeStage }) => (
          routeStage === missionFiveAssaultRouteStage
        ));
        const holdForWestBCore = (
          missionFiveShuttleFactCaptureTick !== undefined
          && missionFiveShuttleCaptures.length < 4
        ) || coreRouteHold?.sites.some((site) => (
          hostiles.some((hostile) => (
            hostile.typeName === site.typeName
            && hostile.cellX === site.cellX
            && hostile.cellY === site.cellY
          ))
        ));
        if (!holdForWestBCore && missionFiveForwardGroup.length > 0 && arrivals >= requiredFormation) {
          missionFiveAssaultRouteArrivalTicks.push(snapshot.tick);
          missionFiveAssaultRouteStage += 1;
          missionFiveAssaultProgressTick = snapshot.tick;
        }
      }
    }
    const missionFiveRouteThreat = mission.number === 5
      ? mission.variant === "west-b" && missionFiveAssaultRouteStage >= 2
        ? chooseFormationThreat(
          missionFiveForwardGroup,
          visibleHostiles.filter((object) => object.type !== 4 && (object.objectFlags & (1 << 12)) !== 0),
          7,
        ) ?? chooseFormationThreat(
          missionFiveForwardGroup,
          visibleHostiles.filter((object) => object.typeName === "GUN"),
          10,
        )
        : mission.variant === "east-a" && missionFiveAssaultRouteStage >= 2
          ? chooseFormationThreat(
            missionFiveForwardGroup,
            visibleHostiles.filter((object) => object.type !== 4 && (object.objectFlags & (1 << 12)) !== 0),
            7,
          ) ?? chooseFormationThreat(
            missionFiveForwardGroup,
            visibleHostiles.filter((object) => object.typeName === "GUN"),
            10,
          )
        : chooseFormationThreat(missionFiveForwardGroup, visibleHostiles.filter((object) => (
          object.type !== 4 && (object.objectFlags & (1 << 12)) !== 0
        )), 6)
          ?? chooseFormationThreat(missionFiveForwardGroup, visibleHostiles.filter((object) => object.typeName === "GUN"), 8)
      : undefined;
    const missionFiveForwardTarget = missionFiveSamSweepActive
      ? missionFiveSamSweepThreat ?? missionFiveSamSweepTarget
      : mission.number === 5
      && (missionFiveCleanupActive
        || missionFiveAssaultRouteStage >= (mission.assaultTargetStage ?? 5))
      ? mission.variant === "west-b"
        ? chooseMissionFiveWestBAssaultTarget(
          missionFiveForwardGroup,
          visibleHostiles,
          missionFiveHuntTriggeredTick !== undefined,
          mission.coreRouteHolds?.find(({ routeStage }) => (
            routeStage === missionFiveAssaultRouteStage
          ))?.sites.filter((site) => (
            (missionFiveShuttleFactCaptureTick !== undefined
              && missionFiveShuttleCaptures.length < 4
              && site.typeName === "FACT" && site.cellX === 52 && site.cellY === 17)
            || hostiles.some((hostile) => (
              hostile.typeName === site.typeName
              && hostile.cellX === site.cellX
              && hostile.cellY === site.cellY
            ))
          )),
          missionFiveHuntTriggeredTick === undefined
            && hostiles.some((hostile) => (
              hostile.typeName === "NUKE" && hostile.cellX === 49 && hostile.cellY === 17
            ))
            && hostiles.length > 2,
          mission.assaultRoute[Math.floor(snapshot.tick / 1_200) % mission.assaultRoute.length],
        )
        : chooseMissionFiveAssaultTarget(
          missionFiveForwardGroup,
          visibleHostiles,
          mission.variant === "east-a" || missionFiveHuntTriggeredTick !== undefined,
        )
      : missionFiveRouteThreat;
    if (missionFiveAssaultActive) {
      if (missionFiveLastForwardTargetKey !== undefined
        && !hostiles.some((hostile) => objectKey(hostile) === missionFiveLastForwardTargetKey)) {
        missionFiveAssaultProgressTick = snapshot.tick;
      }
      if (missionFiveForwardTarget?.type !== undefined) {
        const forwardTargetKey = objectKey(missionFiveForwardTarget);
        if (forwardTargetKey === missionFiveLastForwardTargetKey
          && missionFiveLastForwardTargetStrength !== undefined
          && missionFiveForwardTarget.strength < missionFiveLastForwardTargetStrength) {
          missionFiveAssaultProgressTick = snapshot.tick;
        }
        missionFiveLastForwardTargetKey = forwardTargetKey;
        missionFiveLastForwardTargetStrength = missionFiveForwardTarget.strength;
      }
    } else if (!missionFiveAssaultActive) {
      missionFiveLastForwardTargetKey = undefined;
      missionFiveLastForwardTargetStrength = undefined;
    }
    let orderTarget = mission.number === 3
      ? assaultReady ? chooseMissionThreeAssaultTarget(hostiles) : chooseMissionThreeDefenseTarget(hostiles)
      : mission.number === 2
        ? assaultReady ? target : chooseMissionTwoDefenseTarget(hostiles)
        : mission.number === 5
          ? missionFiveReliefStage < mission.reliefRoute.length
            ? mission.reliefRoute[missionFiveReliefStage]
            : missionFiveAssaultActive || missionFiveCleanupActive
              ? missionFiveForwardTarget
                ?? mission.assaultRoute[missionFiveAssaultRouteStage]
                ?? chooseTarget(visibleHostiles)
              : mission.variant === "west-b" && missionFiveAssaultPhase === "rebuild"
                ? chooseMissionFiveDefenseTarget(visibleHostiles, mission.home) ?? mission.home
                : chooseMissionFiveDefenseTarget(visibleHostiles, mission.home) ?? mission.home
        : mission.number === 4 && mission.objective === "extract"
          ? (mission.threatRadius > 0 ? chooseLocalThreat(attackers, hostiles, mission.threatRadius) : undefined)
            ?? mission.route[missionFourRouteStage]
          : target;
    if (mission.number === 5 && mission.variant === "west-b"
      && missionFiveHuntTriggeredTick !== undefined
      && hostiles.some((hostile) => (
        hostile.type !== 4 && (hostile.objectFlags & (1 << 12)) !== 0
      ))) {
      const armedMobileHostiles = visibleHostiles.filter((hostile) => (
        hostile.type !== 4 && (hostile.objectFlags & (1 << 12)) !== 0
      ));
      const localHomeThreat = chooseMissionFiveDefenseTarget(armedMobileHostiles, mission.home);
      const armedMobileRemains = hostiles.some((hostile) => (
        hostile.type !== 4 && (hostile.objectFlags & (1 << 12)) !== 0
      ));
      const hostileHand = hostiles.find((hostile) => (
        hostile.typeName === "HAND" && hostile.cellX === 41 && hostile.cellY === 22
      ));
      if (!localHomeThreat && armedMobileRemains && hostileHand && attackers.length < 50) {
        missionFiveStrikeGroupKeys.clear();
      }
      orderTarget = localHomeThreat
        ?? hostileHand
        ?? chooseTarget(armedMobileHostiles)
        ?? mission.home;
    }
    let missionFiveStaticSweepTarget;
    let missionFiveStaticSweepForceAttack = false;
    let missionFiveStaticSweepForceCycle;
    const missionFiveInitialHuntCleared = mission.number === 5
      && [...missionFiveInitialHuntStructureKeys].every((key) => (
        !hostiles.some((hostile) => objectKey(hostile) === key)
      ));
    const missionFiveKnownSamSites = mission.samSites?.filter((site) => (
      [...missionFiveKnownHostileStructures.values()].some((structure) => (
        structure.typeName === "SAM"
        && structure.cellX === site.site.cellX
        && structure.cellY === site.site.cellY
      ))
    ));
    const missionFiveSoleSamSite = hostiles.length === 1 && hostiles[0].typeName === "SAM"
      ? mission.samSites?.find((site) => (
        site.site.cellX === hostiles[0].cellX && site.site.cellY === hostiles[0].cellY
      ))
      : undefined;
    const missionFiveStaticSweepSites = missionFiveSoleSamSite
      ? [missionFiveSoleSamSite]
      : hostiles.length > 0
      && hostiles.every((hostile) => hostile.typeName === "SAM")
      && missionFiveKnownSamSites?.length > 0
        ? missionFiveKnownSamSites
        : mission.cleanupSites ?? mission.samSites;
    if (mission.number === 5
      && missionFiveInitialHuntCleared
      && missionFiveStaticSweepSites?.length > 0
      && hostiles.length > 0
      && visibleHostiles.length === 0
      && hostiles.every((hostile) => hostile.type === 4)) {
      if (missionFiveStaticSweepStartedTick === undefined) missionFiveStaticSweepStartedTick = snapshot.tick;
      const sweepElapsed = snapshot.tick - missionFiveStaticSweepStartedTick;
      const sweepIndex = Math.floor(sweepElapsed / 1_800)
        % missionFiveStaticSweepSites.length;
      const sweepPhase = sweepElapsed % 1_800;
      const sweepSite = missionFiveStaticSweepSites[sweepIndex];
      missionFiveStaticSweepForceAttack = sweepPhase >= 600;
      missionFiveStaticSweepForceCycle = Math.floor(sweepElapsed / 1_800);
      missionFiveStaticSweepTarget = missionFiveStaticSweepForceAttack
        ? missionFiveStaticSweepForceOrderCycle === missionFiveStaticSweepForceCycle
          ? undefined
          : sweepSite.site
        : sweepSite.approach;
      orderTarget = missionFiveStaticSweepTarget;
    }
    if (trace && mission.number === 5 && snapshot.tick % 600 === 0) {
      console.error(JSON.stringify({
        missionFiveOrder: true,
        tick: snapshot.tick,
        phase: missionFiveAssaultPhase,
        routeStage: missionFiveAssaultRouteStage,
        samSweepStage: missionFiveSamSweepStage,
        forward: missionFiveForwardGroup.length,
        orderTarget: orderTarget && {
          typeName: orderTarget.typeName,
          id: orderTarget.id,
          cellX: orderTarget.cellX,
          cellY: orderTarget.cellY,
        },
        knownStructures: [...missionFiveKnownHostileStructures.values()].map((structure) => ({
          typeName: structure.typeName,
          id: structure.id,
          cellX: structure.cellX,
          cellY: structure.cellY,
        })),
        forwardSample: missionFiveForwardGroup.slice(0, 8).map(({ typeName, id, cellX, cellY }) => (
          { typeName, id, cellX, cellY }
        )),
      }));
    }
    if (mission.number === 4 && mission.scoutRoute
      && missionFourScoutStage < mission.scoutRoute.length && !orderTarget) {
      orderTarget = mission.scoutHold;
    }
    if (mission.number === 4 && mission.objective === "extract"
      && missionFourRouteStage < mission.route.length) {
      const routeTarget = mission.route[missionFourRouteStage];
      const routeRunners = friendly.filter((object) => (
        object.type !== 4
        && missionFourExtractionKeys.has(objectKey(object))
      ));
      const arrivals = routeRunners.filter((object) => (
        Math.abs(object.cellX - routeTarget.cellX) <= 1
        && Math.abs(object.cellY - routeTarget.cellY) <= 1
      )).length;
      const arrived = arrivals >= Math.min(mission.arrivalCount ?? 1, routeRunners.length)
        && routeRunners.length > 0;
      if (arrived) {
        missionFourRouteArrivalTicks.push(snapshot.tick);
        missionFourRouteStage += 1;
      }
      orderTarget = (mission.threatRadius > 0 ? chooseLocalThreat(attackers, hostiles, mission.threatRadius) : undefined)
        ?? mission.route[missionFourRouteStage]
        ?? orderTarget;
    }
    if (mission.number === 4 && mission.variant === "east-a") {
      const queueContextOrder = (group, destination, flags = 0) => {
        if (group.length === 0 || !destination) return;
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        for (const object of group) {
          commands.push({ type: COMMAND_SELECT_OBJECT, args: [object.type, object.id, 0, 0, 0, 0, 0] });
        }
        commands.push({
          type: COMMAND_INPUT,
          flags,
          args: [
            INPUT_COMMAND_AT_POSITION,
            destination.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            destination.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0,
          ],
        });
        selectionCommands += group.length;
        contextualOrders += 1;
        retargetCycles += 1;
      };
      const distance = (object, destination) => Math.max(
        Math.abs(object.cellX - destination.cellX),
        Math.abs(object.cellY - destination.cellY),
      );
      const runner = attackers.find((attacker) => missionFourExtractionKeys.has(objectKey(attacker)));
      const visibleCargo = attackers.filter((attacker) => missionFourCargoKeys.has(objectKey(attacker)));

      if (!missionFourCargoLoadIssued && runner && visibleCargo.length === 2) {
        queueContextOrder(visibleCargo, runner);
        missionFourCargoLoadIssued = true;
      }
      if (missionFourCargoLoadIssued && visibleCargo.length === 0) missionFourCargoSealed = true;
      if (missionFourCargoUnloadIssued
        && visibleCargo.length === missionFourCargoKeys.size) missionFourCargoUnloaded = true;

      const vanguard = attackers.filter((attacker) => (
        !missionFourExtractionKeys.has(objectKey(attacker))
        && (missionFourCargoUnloaded || !missionFourCargoKeys.has(objectKey(attacker)))
      ));
      const currentVanguardWaypoint = mission.route[missionFourVanguardStage];
      if (currentVanguardWaypoint
        && vanguard.some((attacker) => distance(attacker, currentVanguardWaypoint) <= 2)) {
        missionFourVanguardStage += 1;
      }
      const vanguardWaypoint = mission.route[missionFourVanguardStage];
      const visibleEastHostiles = hostiles.filter((hostile) => (
        snapshot.shroud.isVisible(hostile.cellX, hostile.cellY)
      ));
      const tank = hostiles.find((hostile) => (
        hostile.typeName === "LTNK" && hostile.cellX >= 40 && hostile.cellY >= 50
      ));
      const front = vanguard.toSorted((left, right) => (
        (vanguardWaypoint ? distance(left, vanguardWaypoint) - distance(right, vanguardWaypoint) : 0)
        || right.strength - left.strength
        || left.type - right.type
        || left.id - right.id
      )).slice(0, 6);
      const localThreat = chooseLocalThreat(front, visibleEastHostiles, 3);
      const staged = missionFourVanguardStage >= 6 || missionFourRouteStage >= 6;
      const vanguardTarget = staged
        ? tank ?? localThreat ?? vanguardWaypoint
        : localThreat ?? vanguardWaypoint;
      if (trace && snapshot.tick % 600 === 0) {
        console.error(JSON.stringify({
          eastA: {
            vanguardStage: missionFourVanguardStage,
            cargoSealed: missionFourCargoSealed,
            cargoUnloaded: missionFourCargoUnloaded,
            vanguard: vanguard.map(({ typeName, id, strength, cellX, cellY }) => (
              { typeName, id, strength, cellX, cellY }
            )),
            target: vanguardTarget && {
              typeName: vanguardTarget.typeName,
              id: vanguardTarget.id,
              strength: vanguardTarget.strength,
              cellX: vanguardTarget.cellX,
              cellY: vanguardTarget.cellY,
            },
          },
        }));
      }
      queueContextOrder(front, vanguardTarget);

      const rushReady = missionFourCargoUnloaded && (!tank || tank.strength <= 100);
      if (runner && rushReady) {
        queueContextOrder([runner], mission.route.at(-1), MODIFIER_ALT);
      } else if (runner && missionFourRouteStage < 6
        && missionFourRouteStage < missionFourVanguardStage) {
        queueContextOrder([runner], mission.route[missionFourRouteStage]);
      } else if (runner && missionFourRouteStage >= 6
        && missionFourCargoSealed && !missionFourCargoUnloadIssued) {
        queueContextOrder([runner], runner);
        missionFourCargoUnloadIssued = true;
      }
    }
    if (mission.number === 4 && mission.objective === "eliminate" && !target
      && (!mission.scoutRoute || missionFourScoutStage >= mission.scoutRoute.length)) {
      if (missionFourRouteStage >= mission.route.length && hostiles.length > 0) missionFourRouteStage = 0;
      const routeTarget = mission.route[missionFourRouteStage];
      if (routeTarget) {
        const arrivals = friendly.filter((object) => (
          object.type !== 4
          && Math.abs(object.cellX - routeTarget.cellX) <= 2
          && Math.abs(object.cellY - routeTarget.cellY) <= 2
        )).length;
        if (arrivals >= Math.min(4, Math.max(1, attackers.length))) {
          missionFourRouteArrivalTicks.push(snapshot.tick);
          missionFourRouteStage += 1;
        }
        orderTarget = mission.route[missionFourRouteStage] ?? routeTarget;
      }
    }
    if (mission.number === 3) {
      for (const id of missionThreeStrikeGroupIds) {
        if (!attackers.some((attacker) => attacker.id === id)) missionThreeStrikeGroupIds.delete(id);
      }
      const reserve = 25;
      const desiredStrikeGroup = Math.min(40, Math.max(0, attackers.length - reserve));
      if (missionThreeBaseAssaultStarted
        && (missionThreeStrikeGroupIds.size === 0
          || (missionThreeStrikeGroupIds.size < 8 && attackers.length >= reserve + 23))) {
        const reinforcements = attackers
          .filter((attacker) => attacker.id !== missionThreeScoutId && !missionThreeStrikeGroupIds.has(attacker.id))
          .toSorted((left, right) => (
            right.maxStrength - left.maxStrength
            || left.cellY - right.cellY
            || left.cellX - right.cellX
            || left.id - right.id
          ));
        for (const attacker of reinforcements) {
          if (missionThreeStrikeGroupIds.size >= desiredStrikeGroup) break;
          missionThreeStrikeGroupIds.add(attacker.id);
        }
      }
    }
    const missionThreeStrikeGroup = attackers.filter((attacker) => missionThreeStrikeGroupIds.has(attacker.id));
    if (mission.number === 3 && assaultReady && missionThreeBaseAssaultStarted
      && missionThreeRouteStage < missionThreeAssaultRoute.length) {
      const routeTarget = missionThreeAssaultRoute[missionThreeRouteStage];
      const arrivals = missionThreeStrikeGroup.filter((attacker) => (
        Math.abs(attacker.cellX - routeTarget.cellX) <= 4 && Math.abs(attacker.cellY - routeTarget.cellY) <= 4
      )).length;
      if (arrivals >= Math.min(6, Math.max(2, Math.floor(missionThreeStrikeGroup.length / 8)))) missionThreeRouteStage += 1;
      orderTarget = missionThreeAssaultRoute[missionThreeRouteStage] ?? orderTarget;
    }
    const commandingAttackers = mission.number === 3
      ? missionThreeBaseAssaultStarted
        ? missionThreeStrikeGroup
        : attackers
          .filter((attacker) => attacker.id !== missionThreeScoutId)
          .toSorted((left, right) => left.cellY - right.cellY || left.cellX - right.cellX || left.id - right.id)
          .slice(0, 48)
      : mission.number === 2
        ? assaultReady
          ? attackers.filter((attacker) => !missionTwoHomeGuardIds.has(attacker.id))
          : attackers.filter((attacker) => missionTwoHomeGuardIds.has(attacker.id))
        : mission.number === 5
          ? missionFiveReliefStage < mission.reliefRoute.length
            ? missionFiveInitialForce
            : missionFiveAssaultActive || missionFiveCleanupActive
              ? missionFiveForwardGroup
              : attackers.filter((attacker) => (
                missionFiveCrateCollectedTick !== undefined
                || objectKey(attacker) !== missionFiveCrateRunnerKey
              ))
        : mission.number === 4 && mission.objective === "extract"
          ? mission.variant === "east-a"
            ? []
            : attackers.filter((attacker) => missionFourExtractionKeys.has(objectKey(attacker)))
          : missionFourScout
            ? attackers.filter((attacker) => attacker !== missionFourScout)
            : attackers;
    if (!(mission.number === 4 && mission.variant === "west-b")
      && !missionThreeDeploying && commandingAttackers.length > 0 && orderTarget) {
      const westBFinalRefineryAssault = mission.number === 5
        && mission.variant === "west-b"
        && missionFiveHuntTriggeredTick === undefined
        && missionFiveAssaultRouteStage === 6
        && orderTarget.typeName === "PROC"
        && orderTarget.cellX === 47
        && orderTarget.cellY === 22;
      const scatterWestBFinalRefineryAssault = westBFinalRefineryAssault
        && missionFiveWestBRefineryScatterTick === undefined;
      const commandGroups = westBFinalRefineryAssault
        ? [
            commandingAttackers.filter((attacker) => attacker.typeName !== "E2"),
            commandingAttackers.filter((attacker) => attacker.typeName === "E2"),
          ].filter((group) => group.length > 0)
        : mission.number === 5
        ? Array.from({ length: Math.ceil(commandingAttackers.length / 10) }, (_, index) => (
          commandingAttackers.slice(index * 10, index * 10 + 10)
        ))
        : [commandingAttackers];
      if (scatterWestBFinalRefineryAssault) missionFiveWestBRefineryScatterTick = snapshot.tick;
      for (const commandGroup of commandGroups) {
        const commandTarget = westBFinalRefineryAssault
          && commandGroup.every((attacker) => attacker.typeName === "E2")
          ? chooseFormationThreat(commandGroup, visibleHostiles.filter((hostile) => (
              hostile.typeName === "E1" || hostile.typeName === "E3"
            )), 7) ?? orderTarget
          : orderTarget;
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        for (const attacker of commandGroup) {
          commands.push({ type: COMMAND_SELECT_OBJECT, args: [attacker.type, attacker.id, 0, 0, 0, 0, 0] });
        }
        if (scatterWestBFinalRefineryAssault
          && !commandGroup.every((attacker) => attacker.typeName === "E2")) {
          commands.push({ type: COMMAND_UNIT, args: [UNIT_SCATTER, 0, 0, 0, 0, 0, 0] });
          continue;
        }
        const commandModifiers = mission.number === 5 && missionFiveReliefStage < mission.reliefRoute.length
          ? MODIFIER_ALT
          : mission.number === 5 && (missionFiveStaticSweepForceAttack || commandTarget.typeName === "SAM")
            ? MODIFIER_CTRL
            : 0;
        if (commandModifiers !== 0) {
          commands.push({
            type: COMMAND_INPUT,
            flags: commandModifiers,
            args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
          });
        }
        commands.push({
          type: COMMAND_INPUT,
          flags: commandModifiers,
          args: [
            INPUT_COMMAND_AT_POSITION,
            commandTarget.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            commandTarget.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0,
            0,
            0,
            0,
          ],
        });
        if (commandModifiers !== 0) {
          commands.push({
            type: COMMAND_INPUT,
            args: [INPUT_SPECIAL_KEYS, 0, 0, 0, 0, 0, 0],
          });
        }
        selectionCommands += commandGroup.length;
        contextualOrders += 1;
        retargetCycles += 1;
      }
      if (mission.number === 5
        && missionFiveStaticSweepForceAttack
        && missionFiveStaticSweepForceCycle !== undefined) {
        missionFiveStaticSweepForceOrderCycle = missionFiveStaticSweepForceCycle;
      }
    }
    if (mission.number === 3 && !missionThreeDeploying && missionThreeBaseAssaultStarted) {
      const homeAttackers = attackers
        .filter((attacker) => attacker.id !== missionThreeScoutId && !missionThreeStrikeGroupIds.has(attacker.id))
        .toSorted((left, right) => left.cellY - right.cellY || left.cellX - right.cellX || left.id - right.id)
        .slice(0, 48);
      const homeTarget = chooseMissionThreeDefenseTarget(hostiles);
      if (homeAttackers.length > 0 && homeTarget) {
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        for (const attacker of homeAttackers) {
          commands.push({ type: COMMAND_SELECT_OBJECT, args: [attacker.type, attacker.id, 0, 0, 0, 0, 0] });
        }
        commands.push({
          type: COMMAND_INPUT,
          args: [
            INPUT_COMMAND_AT_POSITION,
            homeTarget.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            homeTarget.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0,
          ],
        });
        selectionCommands += homeAttackers.length;
        contextualOrders += 1;
        retargetCycles += 1;
      }
    }
    if (missionFiveAssaultActive) {
      const homeAttackers = attackers.filter((attacker) => (
        !missionFiveStrikeGroupKeys.has(objectKey(attacker))
      ));
      const defenseTarget = chooseMissionFiveDefenseTarget(visibleHostiles, mission.home);
      const homeAssignments = defenseTarget
        ? [{ group: homeAttackers, target: defenseTarget }]
        : mission.guardPosts
          ? mission.guardPosts.map((target, postIndex) => ({
            target,
            group: homeAttackers.filter((attacker) => (
              attacker.id % mission.guardPosts.length === postIndex
              && (Math.abs(attacker.cellX - target.cellX) > 2
                || Math.abs(attacker.cellY - target.cellY) > 2)
            )),
          }))
          : [{ group: homeAttackers, target: mission.home }];
      for (const { group, target: homeTarget } of homeAssignments) {
        if (group.length === 0) continue;
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        for (const attacker of group) {
          commands.push({ type: COMMAND_SELECT_OBJECT, args: [attacker.type, attacker.id, 0, 0, 0, 0, 0] });
        }
        commands.push({
          type: COMMAND_INPUT,
          args: [
            INPUT_COMMAND_AT_POSITION,
            homeTarget.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            homeTarget.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0,
          ],
        });
        selectionCommands += group.length;
        contextualOrders += 1;
        retargetCycles += 1;
      }
    }
    if (mission.number === 2 && assaultReady) {
      const homeGuard = attackers.filter((attacker) => missionTwoHomeGuardIds.has(attacker.id));
      const homeTarget = chooseMissionTwoDefenseTarget(hostiles)
        ?? (!friendly.some((object) => object.type === 4) ? target : undefined);
      if (homeGuard.length > 0 && homeTarget) {
        commands.push({ type: COMMAND_CLEAR_SELECTION, args: [0, 0, 0, 0, 0, 0, 0] });
        for (const attacker of homeGuard) {
          commands.push({ type: COMMAND_SELECT_OBJECT, args: [attacker.type, attacker.id, 0, 0, 0, 0, 0] });
        }
        commands.push({
          type: COMMAND_INPUT,
          args: [
            INPUT_COMMAND_AT_POSITION,
            homeTarget.cellX * CELL_PIXELS + CELL_PIXELS / 2,
            homeTarget.cellY * CELL_PIXELS + CELL_PIXELS / 2,
            0, 0, 0, 0,
          ],
        });
        selectionCommands += homeGuard.length;
        contextualOrders += 1;
        retargetCycles += 1;
      }
    }
    if (commands.length > 0) {
      submitCommands(handle, snapshot.tick + 1, commands);
      commandBatches += 1;
    }
    const requested = Math.min(
      mission.number === 4 && mission.variant === "west-b" && hostiles.length <= 12
        ? 10
        : mission.number === 5 && mission.variant === "west-b"
          ? 60
          : mission.number === 5
            ? 60
            : TICKS_PER_ORDER,
      MAX_TICKS - snapshot.tick,
    );
    const advanced = advance(handle, requested);
    snapshot = readSnapshot(handle);
    assert.equal(snapshot.tick, currentTick, "snapshot tick differs from the ABI advance count");
    if (advanced === 0 && !snapshot.terminal) assert.fail("engine stopped before reaching a terminal state");
  }
  finalSnapshot = snapshot;

  if (missionFiveWestBStrategy) {
    if (trace) console.error(JSON.stringify({ westBShuttleSummary: {
      factCaptureTick: missionFiveShuttleFactCaptureTick,
      factSaleTick: missionFiveShuttleFactSaleTick,
      factSaleFunds: missionFiveShuttleFactSaleFunds,
      factGoneTick: missionFiveShuttleFactGoneTick,
      factGoneFunds: missionFiveShuttleFactGoneFunds,
      refund: missionFiveShuttleFactGoneFunds === undefined
        ? undefined : missionFiveShuttleFactGoneFunds - missionFiveShuttleFactSaleFunds,
      factCrew: [...missionFiveShuttleFactCrew.values()],
      phase: missionFiveShuttlePhase,
      engineerStarts: missionFiveShuttleEngineerStarts,
      engineers: [...missionFiveShuttleEngineers.entries()],
      assignments: [...missionFiveShuttleAssignments.entries()],
      captures: missionFiveShuttleCaptures,
      huntTriggeredTick: missionFiveHuntTriggeredTick,
      footReservePhase: missionFiveFootReservePhase,
      footReserveRouteStage: missionFiveFootReserveRouteStage,
      footReserveStagingTick: missionFiveFootReserveStagingTick,
      footReserveStagedEngineers: missionFiveFootReserveStagedEngineers,
      cleanupBatchTick: missionFiveWestBCleanupBatchTick,
      cleanupBatchSize: missionFiveWestBCleanupBatchSize,
      terminalTick: finalSnapshot.tick,
      finalFunds: finalSnapshot.sidebar.credits + finalSnapshot.sidebar.tiberium,
    } }));
    assert.equal(missionFiveWestBEngineerProductionStarted, 5,
      "West-B did not prebuild exactly five engineers");
    assert.ok(missionFiveWestBEngineerKey !== undefined,
      "West-B did not designate the primary factory-capture engineer");
    assert.equal(missionFiveWestBEngineerPhase, "captured",
      "West-B primary engineer did not complete the factory capture");
    assert.equal(missionFiveShuttleEngineers.size, 4,
      "West-B did not retain exactly four prebuilt reserve engineers");
    assert.ok(!missionFiveShuttleEngineers.has(missionFiveWestBEngineerKey),
      "West-B primary engineer was counted as a reserve engineer");
    assert.equal(missionFiveShuttleEngineerStarts, 0,
      "West-B queued an engineer after capturing the factory");
    assert.ok(missionFiveShuttleFactCaptureTick !== undefined,
      "West-B did not capture the Nod factory");
    assert.ok(missionFiveShuttleFactSaleTick !== undefined
      && missionFiveShuttleFactCaptureTick <= missionFiveShuttleFactSaleTick,
    "West-B sold the Nod factory before its capture was observed");
    assert.ok(missionFiveShuttleFactGoneTick !== undefined
      && missionFiveShuttleFactSaleTick < missionFiveShuttleFactGoneTick,
    "West-B factory sale did not complete after the sale order");
    assert.equal(missionFiveShuttleFactGoneFunds - missionFiveShuttleFactSaleFunds, 2_500,
      "West-B factory sale refund changed");
    assert.ok([...missionFiveShuttleEngineers.values()].every(({ tick }) => (
      tick < missionFiveShuttleFactCaptureTick
    )), "West-B observed a reserve engineer produced after the factory capture");
    assert.equal(missionFiveFootReservePhase, "staged",
      "West-B reserve engineers did not reach their staging area");
    assert.ok(missionFiveFootReserveStagingTick !== undefined,
      "West-B did not record reserve-engineer staging");
    assert.ok(missionFiveFootReserveStagingTick < missionFiveShuttleFactCaptureTick,
      "West-B reserve engineers staged after the factory capture");
    assert.equal(missionFiveFootReserveStagedEngineers.length, 4,
      "West-B did not stage all four reserve engineers");
    assert.ok(missionFiveFootReserveStagedEngineers.every(({ strength, maxStrength }) => (
      strength === maxStrength
    )), "West-B reserve engineers did not reach staging at full strength");
    assert.equal(missionFiveShuttleAssignments.size, 4,
      "West-B did not assign all four reserve engineers to linked structures");
    assert.equal(new Set(missionFiveShuttleAssignments.values()).size, 4,
      "West-B reserve engineers did not receive unique linked-structure assignments");
    assert.ok([...missionFiveShuttleAssignments.keys()].every((key) => (
      missionFiveShuttleEngineers.has(key)
    )), "West-B assigned a non-reserve engineer to a linked structure");
    assert.deepEqual(
      missionFiveShuttleCaptures.map(({ typeName, cellX, cellY }) => (
        `${typeName}:${cellX}:${cellY}`
      )),
      ["PROC:47:22", "NUKE:47:18", "NUKE:49:17", "AFLD:42:18"],
      "prebuilt foot reserve did not capture every remaining Hunt-linked structure",
    );
    assert.ok(missionFiveHuntTriggeredTick !== undefined
      && missionFiveHuntTriggeredTick >= Math.max(...missionFiveShuttleCaptures.map(({ tick }) => tick)),
    "West-B triggered Hunt before completing the four linked captures");
    assert.ok(missionFiveWestBCleanupBatchTick !== undefined,
      "West-B did not assemble its post-Hunt cleanup batch");
    assert.ok(missionFiveWestBCleanupBatchSize >= 20,
      "West-B post-Hunt cleanup batch was smaller than 20 units");
  }
  const gameOverEvents = events.filter((event) => event.type === EVENT_GAME_OVER);
  const outcomeEvents = events.filter((event) => event.type === EVENT_CAMPAIGN_OUTCOME);
  const terminalSummary = JSON.stringify({
    tick: finalSnapshot.tick,
    productionStarts,
    repairOrders,
    credits: finalSnapshot.sidebar.credits,
    stats: finalSnapshot.stats,
    friendly: rootCombatants(finalSnapshot, HOUSE_GDI).map(({ typeName, id, strength, cellX, cellY }) => ({ typeName, id, strength, cellX, cellY })),
    hostile: rootCombatants(finalSnapshot, HOUSE_NOD).map(({ typeName, id, strength, cellX, cellY }) => ({ typeName, id, strength, cellX, cellY })),
  });
  assert.equal(gameOverEvents.length, 1, `mission did not emit exactly one authoritative game-over event: ${terminalSummary}`);
  assert.equal(outcomeEvents.length, 1, `mission did not emit exactly one campaign outcome: ${terminalSummary}`);
  const gameOver = gameOverEvents[0];
  const outcome = outcomeEvents[0];
  assert.equal(gameOver.flags & 1, 0, `GDI Mission ${mission.number} unexpectedly ended as multiplayer`);
  assert.ok(gameOver.flags & 2, "game-over event does not identify the human player");
  assert.ok(gameOver.flags & 4, `GDI Mission ${mission.number} ended without a win: ${terminalSummary}`);
  assert.ok(outcome.flags & 4, "campaign outcome is not a win");
  assert.equal(outcome.tick, gameOver.tick, "campaign outcome and game-over ticks differ");
  assert.equal(outcome.args[4], mission.scenario, `campaign outcome scenario is not GDI Mission ${mission.number}`);
  assert.equal(outcome.args[5], HOUSE_GDI, "campaign outcome house is not GDI");
  assert.equal(outcome.text1, mission.scenarioRoot, `campaign outcome scenario root is not ${mission.scenarioRoot}`);
  assert.ok(finalSnapshot.terminal, "final snapshot is not terminal");
  assert.equal(finalSnapshot.tick, gameOver.tick, "terminal snapshot and game-over ticks differ");
  assert.equal(advance(handle, 1), 0, "terminal engine accepted another simulation tick");

  const finalFriendly = rootCombatants(finalSnapshot, HOUSE_GDI).length;
  const remainingHostiles = rootCombatants(finalSnapshot, HOUSE_NOD);
  const finalHostiles = remainingHostiles.length;
  const finalProtectedVillageCount = finalSnapshot.objects.filter((object) => (
    object.owner === HOUSE_NEUTRAL
    && object.subObject === 0
    && object.type === 4
    && object.strength > 0
    && missionFourProtectedVillageCells.has(`${object.cellX}:${object.cellY}`)
  )).length;
  const stats = finalSnapshot.stats;
  if (mission.number === 3) {
    assert.equal(initialFriendly, 12, "GDI Mission 3 initial counted force changed");
    assert.equal(initialHostiles, 63, "GDI Mission 3 initial counted Nod force changed");
    assert.ok(deploymentOrders >= 1, "GDI Mission 3 acceptance did not deploy the MCV");
    assert.equal(startedMissionThreeStructures.size, 3, "GDI Mission 3 acceptance did not start the full core base");
    assert.ok(placementStarts >= 3, "GDI Mission 3 acceptance did not enter placement for each core structure");
    assert.ok(placements >= 3, "GDI Mission 3 acceptance did not place each core structure");
    assert.ok(infantryProductionStarts > 0, "GDI Mission 3 acceptance did not produce infantry");
    assert.equal(missionThreeScoutStage, missionThreeScoutRoute.length, "GDI Mission 3 scout did not complete its route");
    assert.ok(missionThreeScoutArrivalTicks[0] < 4_500, "GDI Mission 3 scout reached the first timer-cancel cell too late");
    assert.ok(missionThreeScoutArrivalTicks[1] < 7_200, "GDI Mission 3 scout reached the second timer-cancel cell too late");
    assert.equal(missionThreeRouteStage, missionThreeAssaultRoute.length, "GDI Mission 3 strike force did not complete its assault route");
    assert.ok(missionThreeAssaultStartedTick !== undefined, "GDI Mission 3 never began its base assault");
    assert.equal(finalHostiles, 0, "GDI Mission 3 won with counted Nod combatants still present");
  }
  if (mission.number === 4) {
    const expectedInitialHostiles = {
      "west-a": 32,
      "west-b": 35,
      "east-a": 40,
    }[mission.variant];
    assert.equal(initialFriendly, 11, `GDI Mission 4 ${mission.variant} initial counted force changed`);
    assert.equal(initialHostiles, expectedInitialHostiles,
      `GDI Mission 4 ${mission.variant} initial counted Nod force changed`);
    if (mission.objective === "extract") {
      assert.equal(missionFourRouteStage, mission.route.length,
        `GDI Mission 4 ${mission.variant} extraction force did not complete its route`);
      assert.equal(missionFourRouteArrivalTicks.length, mission.route.length,
        `GDI Mission 4 ${mission.variant} did not record every route arrival`);
      assert.ok(finalFriendly > 0, `GDI Mission 4 ${mission.variant} won without a surviving GDI force`);
      assert.ok(finalHostiles > 0,
        `GDI Mission 4 ${mission.variant} unexpectedly required eliminating the full Nod force`);
    }
    if (mission.variant === "east-a") {
      assert.ok(missionFourCargoLoadIssued, "GDI Mission 4 east-a never issued its cargo load order");
      assert.ok(missionFourCargoSealed, "GDI Mission 4 east-a never confirmed its cargo was loaded");
      assert.ok(missionFourCargoUnloadIssued, "GDI Mission 4 east-a never issued its cargo unload order");
      assert.ok(missionFourCargoUnloaded, "GDI Mission 4 east-a never confirmed its cargo was unloaded");
      assert.ok(missionFourVanguardStage >= 6,
        "GDI Mission 4 east-a vanguard did not reach the eastern staging area");
    }
    if (mission.variant === "west-b") {
      assert.equal(finalHostiles, 0, "GDI Mission 4 west-b won with counted Nod combatants still present");
      assert.equal(initialProtectedVillageCount, missionFourProtectedVillageCells.size,
        "GDI Mission 4 west-b did not start with exactly four protected village structures");
      assert.equal(initialProtectedVillageCells.size, missionFourProtectedVillageCells.size,
        "GDI Mission 4 west-b protected village coordinates changed");
      assert.ok(finalProtectedVillageCount > 0,
        "GDI Mission 4 west-b won without a surviving protected village structure");
      assert.ok(peakFriendly > initialFriendly,
        "GDI Mission 4 west-b acceptance did not receive the authored GDI reinforcements");
    }
  }
  if (mission.number === 5) {
    const expectedInitialFriendly = mission.variant === "west-b" ? 20 : 18;
    const expectedInitialHostiles = mission.variant === "west-b" ? 44 : 50;
    const expectedReliefForce = mission.variant === "west-b" ? 13 : 11;
    assert.equal(initialFriendly, expectedInitialFriendly,
      `GDI Mission 5 ${mission.variant} initial counted force changed`);
    assert.equal(initialHostiles, expectedInitialHostiles,
      `GDI Mission 5 ${mission.variant} initial counted Nod force changed`);
    assert.equal(missionFiveInitialForceKeys.size, expectedReliefForce,
      `GDI Mission 5 ${mission.variant} protected relief force changed`);
    assert.equal(missionFiveReliefStage, mission.reliefRoute.length,
      `GDI Mission 5 ${mission.variant} relief force did not complete its link-up route`);
    assert.equal(missionFiveReliefArrivalTicks.length, mission.reliefRoute.length,
      `GDI Mission 5 ${mission.variant} did not record every relief-route arrival`);
    assert.ok(missionFiveRelievedTick !== undefined,
      `GDI Mission 5 ${mission.variant} never completed the base link-up`);
    assert.ok(repairOrders >= 7,
      `GDI Mission 5 ${mission.variant} did not repair the authored damaged base`);
    assert.ok(missionFiveBaseRepairedTick !== undefined,
      `GDI Mission 5 ${mission.variant} never restored the damaged base`);
    assert.ok(infantryProductionStarts > 0,
      `GDI Mission 5 ${mission.variant} did not produce infantry`);
    assert.ok(vehicleProductionStarts > 0,
      `GDI Mission 5 ${mission.variant} did not produce vehicles`);
    assert.ok(missionFiveCompletedInfantryKeys.size > 0,
      `GDI Mission 5 ${mission.variant} did not observe completed infantry production`);
    assert.ok(missionFiveCompletedVehicleKeys.size > 0,
      `GDI Mission 5 ${mission.variant} did not observe completed vehicle production`);
    assert.ok(missionFiveAssaultStartedTick !== undefined,
      `GDI Mission 5 ${mission.variant} never began its Nod-base assault`);
    assert.ok(missionFiveRelievedTick < missionFiveAssaultStartedTick,
      `GDI Mission 5 ${mission.variant} attacked before completing the base link-up`);
    assert.ok(missionFiveBaseRepairedTick < missionFiveAssaultStartedTick,
      `GDI Mission 5 ${mission.variant} attacked before restoring the damaged base`);
    if (mission.crate) {
      assert.ok(missionFiveCrateCollectedTick !== undefined,
        `GDI Mission 5 ${mission.variant} never collected its authored campaign crate`);
      assert.ok(missionFiveCrateCollectedTick < missionFiveAssaultStartedTick,
        `GDI Mission 5 ${mission.variant} attacked before collecting its authored campaign crate`);
    }
    assert.ok(missionFiveAssaultWaveCount > 0,
      `GDI Mission 5 ${mission.variant} never launched an assault wave`);
    assert.ok(missionFiveHuntTriggeredTick !== undefined,
      `GDI Mission 5 ${mission.variant} never triggered the authored Nod counterattack`);
    assert.ok(missionFiveHuntTriggeredTick < finalSnapshot.tick,
      `GDI Mission 5 ${mission.variant} did not continue after triggering the authored Nod counterattack`);
    assert.equal(finalHostiles, 0,
      `GDI Mission 5 ${mission.variant} won with counted Nod combatants still present`);
    assert.ok(finalFriendly > 0,
      `GDI Mission 5 ${mission.variant} won without a surviving GDI force`);
    assert.ok(peakFriendly > initialFriendly,
      `GDI Mission 5 ${mission.variant} acceptance did not assemble a larger strike force`);
  }
  const commandTypes = mission.number === 3
    ? [
      "sidebar-start-construction",
      "sidebar-start-placement",
      "sidebar-place",
      ...(repairOrders > 0 ? ["structure-repair"] : []),
      "clear-selection",
      "select-object",
      "context-command-at-position",
    ]
    : mission.number === 2
      ? ["sidebar-start-construction", "structure-repair", "clear-selection", "select-object", "context-command-at-position"]
      : mission.number === 5
        ? [
          "sidebar-start-construction",
          "structure-repair",
          ...(missionFiveSoldStructureIds.size > 0 ? ["structure-sell"] : []),
          ...(missionFiveAirstrikeOrders.length > 0 ? ["superweapon-place"] : []),
          "clear-selection",
          "select-object",
          "context-command-at-position",
        ]
      : ["clear-selection", "select-object", "context-command-at-position"];
  console.log(JSON.stringify({
    format: `cncweb-classic-freeware-mission-${mission.number === 4
      ? `four-${mission.variant}`
      : mission.number === 5
        ? `five-${mission.variant}`
        : ["zero", "one", "two", "three"][mission.number]}-acceptance`,
    version: 1,
    packageId: manifest.package_id,
    packageRevision,
    missionId: mission.id,
    scenarioRoot: outcome.text1,
    won: true,
    terminal: true,
    tick: finalSnapshot.tick,
    simulatedSeconds: Number((finalSnapshot.tick / TICK_HZ).toFixed(3)),
    wallClockMs: Number((performance.now() - startedAt).toFixed(3)),
    movieAcknowledgements,
    commandBatches,
    selectionCommands,
    contextualOrders,
    retargetCycles,
    productionStarts,
    infantryProductionStarts,
    vehicleProductionStarts,
    repairOrders,
    deploymentOrders,
    placementStarts,
    placements,
    ...(mission.number === 2 ? { assaultTick: missionTwoAssaultTick } : {}),
    ...(mission.number === 3 ? {
      assaultTick: missionThreeAssaultStartedTick,
      assaultFallbackTick: missionThreeAssaultTick,
      scoutArrivalTicks: missionThreeScoutArrivalTicks,
    } : {}),
    ...(mission.number === 4 ? {
      variant: mission.variant,
      objective: mission.objective,
      routeArrivalTicks: missionFourRouteArrivalTicks,
      ...(mission.variant === "east-a" ? {
        cargo: {
          loadIssued: missionFourCargoLoadIssued,
          sealed: missionFourCargoSealed,
          unloadIssued: missionFourCargoUnloadIssued,
          unloaded: missionFourCargoUnloaded,
        },
        vanguardStage: missionFourVanguardStage,
      } : {}),
      ...(mission.variant === "west-b" ? {
        protectedVillage: {
          initial: initialProtectedVillageCount,
          final: finalProtectedVillageCount,
        },
      } : {}),
    } : {}),
    ...(mission.number === 5 ? {
      variant: mission.variant,
      assaultTick: missionFiveAssaultStartedTick,
      assaultFallbackTick: missionFiveAssaultTick,
      assaultForce: missionFiveAssaultForce,
      reliefArrivalTicks: missionFiveReliefArrivalTicks,
      relievedTick: missionFiveRelievedTick,
      baseRepairedTick: missionFiveBaseRepairedTick,
      assaultRouteArrivalTicks: missionFiveAssaultRouteArrivalTicks,
      assaultWaves: missionFiveAssaultWaveCount,
      huntTriggeredTick: missionFiveHuntTriggeredTick,
      productionCompletions: {
        infantry: missionFiveCompletedInfantryKeys.size,
        vehicles: missionFiveCompletedVehicleKeys.size,
      },
      ...(mission.crate ? { crateCollectedTick: missionFiveCrateCollectedTick } : {}),
      samSweepDestroyedTicks: missionFiveSamDestroyedTicks,
      airstrike: {
        readyTicks: missionFiveAirstrikeReadyTicks,
        orders: missionFiveAirstrikeOrders,
        discharges: missionFiveAirstrikeDischarges,
        pending: missionFiveAirstrikePending !== undefined,
      },
    } : {}),
    commandTypes,
    forces: {
      initialFriendly,
      initialHostiles,
      peakFriendly,
      peakHostiles,
      finalFriendly,
      finalHostiles,
      remainingHostiles: remainingHostiles.map(({ typeName, assetName, type, id, strength, cellX, cellY }) => ({
        typeName,
        assetName,
        type,
        id,
        strength,
        cellX,
        cellY,
      })),
    },
    battle: {
      unitsKilled: stats.unitsKilled,
      buildingsKilled: stats.buildingsKilled,
      totalKilled: stats.unitsKilled + stats.buildingsKilled,
      unitsLost: stats.unitsLost,
      buildingsLost: stats.buildingsLost,
      totalLost: stats.unitsLost + stats.buildingsLost,
    },
    gameOver: {
      score: gameOver.args[0],
      leadership: gameOver.args[1],
      efficiency: gameOver.args[2],
      remainingCredits: gameOver.args[3],
    },
  }));
} finally {
  if (handle !== 0) {
    assert.equal(engine._cnc_web_destroy(handle), STATUS_OK, "runtime destroy failed");
  }
}
