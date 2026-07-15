#!/usr/bin/env node

import assert from "node:assert/strict";
import { resolve } from "node:path";
import { pathToFileURL } from "node:url";

const modulePath = resolve(process.argv[2] ?? "build/web-td/tiberiandawn.js");
const assetBase = new URL(process.argv[3] ?? "http://127.0.0.1:8765/");
const { default: createModule } = await import(pathToFileURL(modulePath).href);
const engine = await createModule({
  locateFile(path) {
    return new URL(path, assetBase).href;
  },
});

assert.equal(engine._cnc_web_abi_version(), 2, "unexpected browser ABI version");
assert.equal(typeof engine._cnc_web_set_campaign_transition, "function", "campaign transition export is missing");
assert.equal(typeof engine._CNC_Web_Acceptance_Force_Victory, "function", "narrow release-acceptance export is missing");

const handlePointer = engine._malloc(4);
assert.notEqual(handlePointer, 0, "failed to allocate the handle output");

try {
  const createStatus = engine._cnc_web_create(2, handlePointer);
  const handle = engine.HEAPU32[handlePointer >>> 2];
  assert.equal(createStatus, 0, "cnc_web_create failed");
  assert.notEqual(handle, 0, "cnc_web_create returned an invalid handle");

  // Creation must not touch protected content. A missing StartV1 mount is a
  // recoverable content error with a pollable runtime diagnostic.
  const contentRoot = new TextEncoder().encode("/cnc-web-smoke-missing-content");
  const start = new ArrayBuffer(72 + contentRoot.byteLength);
  const startView = new DataView(start);
  startView.setUint32(0, 0x57434e43, true);
  startView.setUint16(4, 1, true);
  startView.setUint16(6, 1, true);
  startView.setUint32(8, start.byteLength, true);
  startView.setUint32(12, 1, true);
  startView.setUint32(16, 1, true);
  startView.setInt32(20, 1, true);
  startView.setInt32(24, 0, true);
  startView.setInt32(28, 0, true);
  startView.setInt32(32, 1, true);
  startView.setInt32(36, -1, true);
  startView.setUint32(40, 1, true);
  startView.setUint32(44, 1, true);
  startView.setBigUint64(48, 42n, true);
  startView.setUint32(56, contentRoot.byteLength, true);
  startView.setUint32(60, 0, true);
  startView.setBigUint64(64, 1n, true);
  new Uint8Array(start, 72).set(contentRoot);

  const startPointer = engine._malloc(start.byteLength);
  const eventSizePointer = engine._malloc(4);
  assert.notEqual(startPointer, 0, "failed to allocate StartV1");
  assert.notEqual(eventSizePointer, 0, "failed to allocate event size output");
  try {
    engine.HEAPU8.set(new Uint8Array(start), startPointer);
    assert.equal(engine._cnc_web_start(handle, startPointer, start.byteLength), 4, "missing mount was not rejected");
    assert.equal(engine._cnc_web_event_size(handle, eventSizePointer), 0, "failed to query startup diagnostic");
    const eventSize = engine.HEAPU32[eventSizePointer >>> 2];
    assert.ok(eventSize >= 64, "missing mount did not emit a startup diagnostic");
    const eventPointer = engine._malloc(eventSize);
    assert.notEqual(eventPointer, 0, "failed to allocate startup diagnostic");
    try {
      assert.equal(engine._cnc_web_poll_event(handle, eventPointer, eventSize, eventSizePointer), 0);
      const event = new DataView(engine.HEAPU8.buffer, eventPointer, eventSize);
      assert.equal(event.getUint16(20, true), 14, "startup error was not a runtime diagnostic");
      assert.equal(event.getInt32(32, true), 3, "unexpected runtime diagnostic code");
    } finally {
      engine._free(eventPointer);
    }
  } finally {
    engine._free(eventSizePointer);
    engine._free(startPointer);
  }

  assert.equal(engine._cnc_web_destroy(handle), 0, "cnc_web_destroy failed");

  console.log(
    JSON.stringify({
      abi: engine._cnc_web_abi_version(),
      createStatus,
      handle,
      memoryBytes: engine.HEAPU8.byteLength,
    }),
  );
} finally {
  engine._free(handlePointer);
}
