import type { Page } from "@playwright/test";

export interface EngineProbeResult {
  inDedicatedWorker: boolean;
  abi: number;
  createStatus: number;
  handle: number;
  startStatus: number;
  eventQueryStatus: number;
  eventSize: number;
  pollStatus: number;
  eventWritten: number;
  eventMagic: number;
  eventProtocol: number;
  eventKind: number;
  eventType: number;
  diagnosticFlags: number;
  diagnosticCode: number;
  diagnosticStatus: number;
  diagnosticId: string;
  diagnosticDetail: string;
  destroyStatus: number;
  memoryBytes: number;
}

const engineProbeWorker = String.raw`
self.onmessage = async (message) => {
  let engine;
  let handle = 0;
  let handlePointer = 0;
  try {
    const moduleUrl = message.data.moduleUrl;
    const namespace = await import(moduleUrl);
    if (typeof namespace.default !== "function") throw new Error("Emscripten module has no default factory");
    engine = await namespace.default({
      locateFile(path) {
        return new URL(path, moduleUrl).href;
      },
    });

    const abi = engine._cnc_web_abi_version();
    handlePointer = engine._malloc(4);
    if (!handlePointer) throw new Error("Could not allocate handle output");
    const createStatus = engine._cnc_web_create(abi, handlePointer);
    handle = engine.HEAPU32[handlePointer >>> 2];

    const contentRoot = new TextEncoder().encode("/cnc-web-e2e-missing-content");
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
    if (!startPointer || !eventSizePointer) throw new Error("Could not allocate probe buffers");
    let startStatus;
    let eventQueryStatus;
    let eventSize;
    let pollStatus;
    let eventWritten;
    let eventMagic;
    let eventProtocol;
    let eventKind;
    let eventType;
    let diagnosticFlags;
    let diagnosticCode;
    let diagnosticStatus;
    let diagnosticId;
    let diagnosticDetail;
    try {
      engine.HEAPU8.set(new Uint8Array(start), startPointer);
      startStatus = engine._cnc_web_start(handle, startPointer, start.byteLength);
      eventQueryStatus = engine._cnc_web_event_size(handle, eventSizePointer);
      eventSize = engine.HEAPU32[eventSizePointer >>> 2];
      const eventPointer = engine._malloc(eventSize);
      if (!eventPointer) throw new Error("Could not allocate diagnostic output");
      try {
        pollStatus = engine._cnc_web_poll_event(handle, eventPointer, eventSize, eventSizePointer);
        eventWritten = engine.HEAPU32[eventSizePointer >>> 2];
        const event = new DataView(engine.HEAPU8.buffer, eventPointer, eventSize);
        eventMagic = event.getUint32(0, true);
        eventProtocol = event.getUint16(4, true);
        eventKind = event.getUint16(6, true);
        eventType = event.getUint16(20, true);
        diagnosticFlags = event.getUint16(22, true);
        diagnosticCode = event.getInt32(32, true);
        diagnosticStatus = event.getInt32(36, true);
        const idLength = event.getUint32(56, true);
        const detailLength = event.getUint32(60, true);
        const decoder = new TextDecoder();
        diagnosticId = decoder.decode(new Uint8Array(engine.HEAPU8.buffer, eventPointer + 64, idLength));
        diagnosticDetail = decoder.decode(new Uint8Array(engine.HEAPU8.buffer, eventPointer + 64 + idLength, detailLength));
      } finally {
        engine._free(eventPointer);
      }
    } finally {
      engine._free(eventSizePointer);
      engine._free(startPointer);
    }

    const destroyStatus = engine._cnc_web_destroy(handle);
    handle = 0;
    self.postMessage({
      ok: true,
      result: {
        inDedicatedWorker: typeof WorkerGlobalScope !== "undefined" && self instanceof WorkerGlobalScope,
        abi,
        createStatus,
        handle: engine.HEAPU32[handlePointer >>> 2],
        startStatus,
        eventQueryStatus,
        eventSize,
        pollStatus,
        eventWritten,
        eventMagic,
        eventProtocol,
        eventKind,
        eventType,
        diagnosticFlags,
        diagnosticCode,
        diagnosticStatus,
        diagnosticId,
        diagnosticDetail,
        destroyStatus,
        memoryBytes: engine.HEAPU8.byteLength,
      },
    });
  } catch (error) {
    if (engine && handle) {
      try { engine._cnc_web_destroy(handle); } catch { /* Worker termination reclaims the module. */ }
    }
    self.postMessage({
      ok: false,
      error: error instanceof Error ? error.message : String(error),
      stack: error instanceof Error ? error.stack : undefined,
    });
  } finally {
    if (engine && handlePointer) engine._free(handlePointer);
  }
};
`;

export async function probeEngineInDedicatedWorker(page: Page): Promise<EngineProbeResult> {
  return page.evaluate(async (source) => {
    const moduleUrl = new URL("engine/tiberiandawn.js", document.baseURI).href;
    const workerUrl = URL.createObjectURL(new Blob([source], { type: "text/javascript" }));
    const worker = new Worker(workerUrl, { type: "module", name: "engine-abi-e2e" });
    try {
      return await new Promise<EngineProbeResult>((resolve, reject) => {
        const timeout = window.setTimeout(() => reject(new Error("Engine worker probe timed out")), 20_000);
        worker.onmessage = (event: MessageEvent<{ ok: true; result: EngineProbeResult } | { ok: false; error: string; stack?: string }>) => {
          window.clearTimeout(timeout);
          if (event.data.ok) resolve(event.data.result);
          else reject(new Error(`${event.data.error}${event.data.stack ? `\n${event.data.stack}` : ""}`));
        };
        worker.onerror = (event) => {
          window.clearTimeout(timeout);
          reject(new Error(event.message || "Engine worker probe failed"));
        };
        worker.postMessage({ moduleUrl });
      });
    } finally {
      worker.terminate();
      URL.revokeObjectURL(workerUrl);
    }
  }, engineProbeWorker);
}
