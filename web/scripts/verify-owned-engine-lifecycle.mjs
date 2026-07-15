#!/usr/bin/env node

import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { pathToFileURL } from "node:url";

import { TextWriter, Uint8ArrayReader, Uint8ArrayWriter, ZipReader } from "@zip.js/zip.js";

const [moduleArgument, assetBaseArgument, packageArgument, expectedRevision] = process.argv.slice(2);
if (!moduleArgument || !assetBaseArgument || !packageArgument || !/^[a-f0-9]{64}$/.test(expectedRevision ?? "")) {
  console.error("usage: verify-owned-engine-lifecycle.mjs ENGINE.js ENGINE_ASSET_BASE PACKAGE.cncweb PACKAGE_REVISION");
  process.exit(2);
}

const modulePath = resolve(moduleArgument);
const packagePath = resolve(packageArgument);
const assetBase = new URL(assetBaseArgument);
const packageBytes = new Uint8Array(readFileSync(packagePath));
const archive = new ZipReader(new Uint8ArrayReader(packageBytes));
let manifest;
let engineFiles;
try {
  const entries = await archive.getEntries();
  const manifestEntry = entries.find((entry) => entry.filename === "manifest.json" && !entry.directory && entry.getData);
  assert.ok(manifestEntry?.getData, "owned package has no canonical manifest");
  manifest = JSON.parse(await manifestEntry.getData(new TextWriter()));
  const calculatedRevision = createHash("sha256").update(JSON.stringify(manifest), "utf8").digest("hex");
  assert.equal(calculatedRevision, expectedRevision, "owned package revision differs from browser ContentStore identity");
  const descriptors = new Map(manifest.files.map((file) => [file.path, file]));
  engineFiles = [];
  for (const entry of entries) {
    if (entry.directory || !entry.filename.startsWith("engine/td/") || !entry.getData) continue;
    const descriptor = descriptors.get(entry.filename);
    assert.ok(descriptor, "engine file is absent from the manifest");
    const data = await entry.getData(new Uint8ArrayWriter());
    assert.equal(data.byteLength, descriptor.size, "engine file size differs from its manifest");
    assert.equal(createHash("sha256").update(data).digest("hex"), descriptor.sha256, "engine file hash differs from its manifest");
    engineFiles.push({ path: entry.filename, data });
  }
  assert.ok(engineFiles.length >= 15, "owned package has too few engine files for GDI mission 1");
} finally {
  await archive.close();
}

const { default: createModule } = await import(pathToFileURL(modulePath).href);
const engine = await createModule({ locateFile: (path) => new URL(path, assetBase).href });
assert.equal(engine._cnc_web_abi_version(), 2, "unexpected browser ABI version");

const mountRoot = `/cnc-content/${expectedRevision.slice(0, 16)}`;
for (const file of engineFiles) {
  const destination = `${mountRoot}/${file.path}`;
  engine.FS.mkdirTree(dirname(destination));
  engine.FS.writeFile(destination, file.data);
}

const MAGIC = 0x57434e43;
const STATUS_OK = 0;
const MESSAGE_START = 1;
const MESSAGE_COMMAND = 2;
const EVENT_MOVIE = 5;
const EVENT_DIAGNOSTIC = 14;
const DIAGNOSTIC_START_READY = 6;
const COMMAND_GAME = 7;
const GAME_MOVIE_DONE = 0;
const TICKS_PER_BRANCH = 30;
const handleTicks = new Map();

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

function createHandle() {
  const pointer = withAllocation(4, "handle output");
  try {
    assert.equal(engine._cnc_web_create(2, pointer), STATUS_OK, "cnc_web_create failed");
    const handle = new DataView(engine.HEAPU8.buffer).getUint32(pointer, true);
    assert.notEqual(handle, 0, "cnc_web_create returned an invalid handle");
    handleTicks.set(handle, 0);
    return handle;
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
  view.setInt32(20, 1, true);
  view.setInt32(24, 0, true);
  view.setInt32(28, 0, true);
  view.setInt32(32, 1, true);
  view.setInt32(36, -1, true);
  view.setUint32(40, 1, true);
  view.setUint32(44, 1, true);
  view.setBigUint64(48, 0n, true);
  view.setUint32(56, content.byteLength, true);
  view.setUint32(60, 0, true);
  view.setBigUint64(64, BigInt(`0x${expectedRevision.slice(0, 16)}`) || 1n, true);
  bytes.set(content, 72);
  return bytes;
}

function movieDoneMessage(targetTick) {
  const bytes = new Uint8Array(64);
  const view = new DataView(bytes.buffer);
  view.setUint32(0, MAGIC, true);
  view.setUint16(4, 1, true);
  view.setUint16(6, MESSAGE_COMMAND, true);
  view.setUint32(8, bytes.byteLength, true);
  view.setUint32(12, 1, true);
  view.setUint32(16, targetTick, true);
  view.setUint16(20, 32, true);
  view.setUint16(22, 0, true);
  view.setBigUint64(24, 0n, true);
  view.setUint16(32, COMMAND_GAME, true);
  view.setUint16(34, 0, true);
  view.setInt32(36, GAME_MOVIE_DONE, true);
  return bytes;
}

function submitMovieDone(handle, targetTick) {
  const bytes = movieDoneMessage(targetTick);
  assert.equal(writeInput(bytes, (pointer, length) => engine._cnc_web_submit_commands(handle, pointer, length)), STATUS_OK, "movie acknowledgement failed");
}

function drainEvents(handle) {
  const types = [];
  const diagnostics = [];
  const digests = [];
  for (let index = 0; index < 1024; index += 1) {
    const size = outputU32((output) => engine._cnc_web_event_size(handle, output), "event-size query");
    if (size === 0) return { types, diagnostics, digests };
    assert.ok(size >= 64 && size <= 1024 * 1024, "engine emitted an invalid event size");
    const eventPointer = withAllocation(size, "event buffer");
    const writtenPointer = withAllocation(4, "event written output");
    try {
      assert.equal(engine._cnc_web_poll_event(handle, eventPointer, size, writtenPointer), STATUS_OK, "event poll failed");
      const memory = new DataView(engine.HEAPU8.buffer);
      assert.equal(memory.getUint32(writtenPointer, true), size, "event size changed while polling");
      assert.equal(memory.getUint32(eventPointer, true), MAGIC, "event has invalid magic");
      digests.push(createHash("sha256")
        .update(new Uint8Array(engine.HEAPU8.buffer, eventPointer, size))
        .digest("hex"));
      const eventTick = memory.getUint32(eventPointer + 16, true);
      const type = memory.getUint16(eventPointer + 20, true);
      types.push(type);
      if (type === EVENT_MOVIE) {
        const currentTick = Math.max(eventTick, handleTicks.get(handle) ?? 0);
        assert.ok(currentTick < 0xffffffff, "cannot schedule a movie acknowledgement after the final tick");
        submitMovieDone(handle, currentTick + 1);
      }
      if (type === EVENT_DIAGNOSTIC) diagnostics.push(memory.getInt32(eventPointer + 32, true));
    } finally {
      engine._free(writtenPointer);
      engine._free(eventPointer);
    }
  }
  const remainingSize = outputU32((output) => engine._cnc_web_event_size(handle, output), "event-size bound query");
  assert.equal(remainingSize, 0, "engine emitted more than 1024 events without draining");
  return { types, diagnostics, digests };
}

function start(handle) {
  const bytes = startMessage();
  assert.equal(writeInput(bytes, (pointer, length) => engine._cnc_web_start(handle, pointer, length)), STATUS_OK, "owned GDI mission 1 start failed");
  handleTicks.set(handle, 0);
  return drainEvents(handle);
}

function advance(handle, count, captureTrace = true) {
  const trace = [];
  for (let index = 0; index < count; index += 1) {
    const advanced = outputU32((output) => engine._cnc_web_advance(handle, 1, output), "engine advance");
    assert.equal(advanced, 1, "owned mission reached an unexpected early terminal state");
    handleTicks.set(handle, (handleTicks.get(handle) ?? 0) + 1);
    const events = drainEvents(handle);
    if (captureTrace) {
      trace.push({
        stateHash: stateHash(handle).toString(16).padStart(16, "0"),
        eventDigests: events.digests,
      });
    }
  }
  return trace;
}

function stateHash(handle) {
  const pointer = withAllocation(8, "state-hash output");
  try {
    assert.equal(engine._cnc_web_state_hash(handle, pointer), STATUS_OK, "state-hash query failed");
    return new DataView(engine.HEAPU8.buffer).getBigUint64(pointer, true);
  } finally {
    engine._free(pointer);
  }
}

function save(handle) {
  const size = outputU32((output) => engine._cnc_web_save_size(handle, output), "save-size query");
  assert.ok(size > 60, "owned mission save is unexpectedly small");
  const dataPointer = withAllocation(size, "save buffer");
  const writtenPointer = withAllocation(4, "save written output");
  try {
    assert.equal(engine._cnc_web_write_save(handle, dataPointer, size, writtenPointer), STATUS_OK, "save write failed");
    const written = new DataView(engine.HEAPU8.buffer).getUint32(writtenPointer, true);
    assert.equal(written, size, "save size changed while writing");
    return new Uint8Array(engine.HEAPU8.buffer, dataPointer, size).slice();
  } finally {
    engine._free(writtenPointer);
    engine._free(dataPointer);
  }
}

function load(handle, bytes) {
  assert.equal(writeInput(bytes, (pointer, length) => engine._cnc_web_load_save(handle, pointer, length)), STATUS_OK, "save load failed");
  handleTicks.set(handle, new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(16, true));
  return drainEvents(handle);
}

let handle = createHandle();
const startup = start(handle);
assert.ok(startup.diagnostics.includes(DIAGNOSTIC_START_READY), "owned mission start emitted no ready diagnostic");
// Leave the classic-surface baseline untouched until savedHash is captured.
// State hashing materializes the next classic delta, so tracing this warm-up
// would make the natural save point incomparable with the post-load baseline.
advance(handle, 5, false);
const savedHash = stateHash(handle);
const saved = save(handle);
const firstBranchTrace = advance(handle, TICKS_PER_BRANCH);
const firstBranchHash = stateHash(handle);
const sameInstanceLoadEvents = load(handle, saved);
assert.equal(stateHash(handle), savedHash, "same-instance load did not restore the saved deterministic state");
const sameInstanceTrace = advance(handle, TICKS_PER_BRANCH);
assert.deepEqual(sameInstanceTrace, firstBranchTrace, "same-instance deterministic tick/event trace diverged after load");
assert.equal(stateHash(handle), firstBranchHash, "same-instance deterministic replay diverged after load");
assert.equal(engine._cnc_web_destroy(handle), STATUS_OK, "first owned runtime destroy failed");

handle = createHandle();
start(handle);
const freshInstanceLoadEvents = load(handle, saved);
assert.deepEqual(freshInstanceLoadEvents.digests, sameInstanceLoadEvents.digests, "fresh-runtime load events diverged from same-instance load");
assert.equal(stateHash(handle), savedHash, "fresh-runtime load did not restore the saved deterministic state");
const freshInstanceTrace = advance(handle, TICKS_PER_BRANCH);
assert.deepEqual(freshInstanceTrace, firstBranchTrace, "fresh-runtime deterministic tick/event trace diverged after load-before-first-tick");
assert.equal(stateHash(handle), firstBranchHash, "fresh-runtime deterministic replay diverged after load-before-first-tick");
assert.equal(engine._cnc_web_destroy(handle), STATUS_OK, "second owned runtime destroy failed");

console.log(JSON.stringify({
  format: "cncweb-owned-engine-lifecycle",
  version: 1,
  packageRevision: expectedRevision,
  missionId: "gdi-01-east-a",
  startupEvents: startup.types.length,
  saveBytes: saved.byteLength,
  replayTicks: TICKS_PER_BRANCH,
  sameInstanceReplay: true,
  freshInstanceReplay: true,
}));
