/// <reference lib="webworker" />

import { TransferBufferPool } from "./bufferPool";
import type { SimulationCore } from "./core";
import { DemoCore } from "./DemoCore";
import { acquireContentMountLeaseFromOpfs, type ContentMountLease } from "./contentMount";
import { decodeCommandBatch, resolveImmediateCommandTick, SIMULATION_PROTOCOL_VERSION, type MainToWorkerMessage, type WorkerToMainMessage } from "./protocol";
import { assertSimulationSaveAllowed } from "./savePolicy";
import { SimulationRunGate } from "./SimulationRunGate";
import { SnapshotTransferPump } from "./snapshotTransferPump";
import { nextSimulationStepDelay } from "./tickTiming";
import { isFatalWasmCoreError, WasmCore } from "./WasmCore";

const scope = self as DedicatedWorkerGlobalScope;
const TICK_RATE = 15;
const STEP_MS = 1000 / TICK_RATE;
const pool = new TransferBufferPool(3, 128 * 1024);
const snapshots = new SnapshotTransferPump(pool, (buffer) => send({ type: "snapshot", buffer }, [buffer]));
let core: SimulationCore | undefined;
let contentMountLease: ContentMountLease | undefined;
let teardownPromise: Promise<unknown | undefined> | undefined;
let initialized = false;
const runGate = new SimulationRunGate();
let terminalState = false;
let timer: number | undefined;
let lastTime = performance.now();
let accumulator = 0;
let currentTick = 0;
let acceptanceHooksEnabled = false;

function isLoopbackWorker(): boolean {
  return scope.location.protocol === "http:" || scope.location.protocol === "https:"
    ? ["localhost", "127.0.0.1", "[::1]", "::1"].includes(scope.location.hostname)
    : false;
}

function send(message: WorkerToMainMessage, transfer: Transferable[] = []): void {
  scope.postMessage(message, transfer);
}

function reportError(error: unknown, requestId?: number, fatal = false): void {
  send({ type: "error", requestId, fatal, message: error instanceof Error ? error.message : String(error) });
}

function stopAfterFatalError(): void {
  runGate.stop();
  stopTimer();
  snapshots.clearPending();
  send({ type: "running", running: false });
}

function teardownRuntime(): Promise<unknown | undefined> {
  if (teardownPromise) return teardownPromise;
  teardownPromise = (async () => {
    runGate.stop();
    initialized = false;
    stopTimer();
    snapshots.clearPending();
    const activeCore = core;
    const activeLease = contentMountLease;
    core = undefined;
    contentMountLease = undefined;
    let cleanupError: unknown;
    try {
      activeCore?.destroy();
    } catch (error) {
      cleanupError = error;
    }
    try {
      await activeLease?.release();
    } catch (error) {
      cleanupError ??= error;
    }
    return cleanupError;
  })();
  return teardownPromise;
}

async function failRuntime(error: unknown, requestId?: number): Promise<void> {
  stopAfterFatalError();
  const cleanupError = await teardownRuntime();
  if (cleanupError) console.error("Simulation runtime cleanup failed", cleanupError);
  reportError(error, requestId, true);
  scope.close();
}

function emitSnapshot(): void {
  if (!core) return;
  currentTick = snapshots.request(core) ?? currentTick;
}

function emitEvents(): boolean {
  if (!core) return false;
  let terminal = false;
  for (const event of core.drainEvents()) {
    send({ type: "event", event });
    if (event.kind === "game-over") terminal = true;
  }
  return terminal;
}

function stopTimer(): void {
  if (timer !== undefined) scope.clearTimeout(timer);
  timer = undefined;
}

function schedule(): void {
  stopTimer();
  timer = scope.setTimeout(step, nextSimulationStepDelay(accumulator, STEP_MS));
}

function step(): void {
  timer = undefined;
  if (!runGate.running || !core) return;
  const now = performance.now();
  accumulator += Math.min(250, now - lastTime);
  lastTime = now;
  let advanced = 0;
  try {
    while (accumulator >= STEP_MS && advanced < 4) {
      const didAdvance = core.advance();
      if (didAdvance) {
        currentTick = core.currentTick();
        accumulator -= STEP_MS;
        advanced += 1;
      }
      if (emitEvents() || !didAdvance) {
        terminalState = true;
        runGate.requestRunning(false, true);
        stopTimer();
        send({ type: "running", running: false });
        break;
      }
    }
    // A core can report its durable terminal state without advancing a tick.
    // Publish that state as well so the last retained frame has terminal flags.
    if (advanced > 0 || terminalState) emitSnapshot();
  } catch (error) {
    void failRuntime(error);
    return;
  }
  if (runGate.running) schedule();
}

async function initialize(message: Extract<MainToWorkerMessage, { type: "initialize" }>): Promise<void> {
  if (initialized) throw new Error("Simulation worker is already initialized");
  if (message.protocolVersion !== SIMULATION_PROTOCOL_VERSION) throw new Error("Browser and worker protocols do not match");
  if (message.core === "wasm") {
    if (message.acceptanceSession !== undefined && !/^[a-f0-9]{32,64}$/.test(message.acceptanceSession)) {
      throw new Error("Release-acceptance session is invalid");
    }
    acceptanceHooksEnabled = Boolean(message.acceptanceSession && isLoopbackWorker());
    const lease = await acquireContentMountLeaseFromOpfs(message.contentMount, (progress) => send({ type: "mount-progress", progress }));
    contentMountLease = lease;
    send({
      type: "mount-progress",
      progress: {
        phase: "mounting",
        packageId: message.contentMount.packageId,
        completedFiles: message.contentMount.files.length,
        totalFiles: message.contentMount.files.length,
        completedBytes: lease.prepared.totalBytes,
        totalBytes: lease.prepared.totalBytes,
      },
    });
    core = await WasmCore.create(message.emscriptenModuleUrl, lease.prepared);
    send({
      type: "mount-progress",
      progress: {
        phase: "complete",
        packageId: message.contentMount.packageId,
        completedFiles: message.contentMount.files.length,
        totalFiles: message.contentMount.files.length,
        completedBytes: lease.prepared.totalBytes,
        totalBytes: lease.prepared.totalBytes,
      },
    });
  } else {
    acceptanceHooksEnabled = false;
    core = new DemoCore();
  }
  initialized = true;
  send({
    type: "ready",
    capabilities: {
      saves: core.supportsSaves,
      stateHashes: false,
      tickRate: TICK_RATE,
      protocolVersion: SIMULATION_PROTOCOL_VERSION,
    },
  });
}

scope.onmessage = (event: MessageEvent<MainToWorkerMessage>) => {
  const message = event.data;
  void (async () => {
    try {
      if (message.type === "initialize") {
        await initialize(message);
        return;
      }
      if (message.type === "shutdown") {
        const cleanupError = await teardownRuntime();
        if (cleanupError) console.error("Simulation runtime cleanup failed", cleanupError);
        send({ type: "shutdown-complete", requestId: message.requestId });
        scope.close();
        return;
      }
      if (!core || !initialized) throw new Error("Simulation worker has not been initialized");
      switch (message.type) {
        case "start":
          await core.start(message.configuration);
          currentTick = core.currentTick();
          terminalState = emitEvents();
          accumulator = 0;
          lastTime = performance.now();
          runGate.begin(terminalState, message.deferRunningUntilLoad);
          send({ type: "started", requestId: message.requestId });
          send({ type: "running", running: runGate.running });
          emitSnapshot();
          if (runGate.running) schedule();
          break;
        case "commands":
          core.submitCommands(resolveImmediateCommandTick(decodeCommandBatch(message.buffer), currentTick));
          break;
        case "recycle":
          currentTick = snapshots.recycle(message.buffer, core) ?? currentTick;
          break;
        case "set-running":
          runGate.requestRunning(message.running, terminalState);
          accumulator = 0;
          lastTime = performance.now();
          send({ type: "running", running: runGate.running });
          if (runGate.running) schedule();
          else stopTimer();
          break;
        case "cancel-deferred-load":
          if (runGate.completeLoad(terminalState)) {
            accumulator = 0;
            lastTime = performance.now();
            send({ type: "running", running: runGate.running });
            if (runGate.running) schedule();
            else stopTimer();
          }
          break;
        case "save": {
          assertSimulationSaveAllowed(terminalState);
          const copy = core.save();
          const buffer = copy.buffer.slice(copy.byteOffset, copy.byteOffset + copy.byteLength) as ArrayBuffer;
          send({ type: "saved", requestId: message.requestId, buffer }, [buffer]);
          break;
        }
        case "load":
          core.load(new Uint8Array(message.buffer));
          currentTick = core.currentTick();
          terminalState = false;
          send({ type: "loaded", requestId: message.requestId });
          emitSnapshot();
          if (runGate.completeLoad(terminalState)) {
            accumulator = 0;
            lastTime = performance.now();
            send({ type: "running", running: runGate.running });
            if (runGate.running) schedule();
            else stopTimer();
          }
          break;
        case "acceptance-force-victory":
          if (!acceptanceHooksEnabled || !core.acceptanceForceVictory) throw new Error("Release-acceptance victory hook is unavailable");
          if (runGate.running) throw new Error("Pause the simulation before forcing an acceptance victory");
          if (terminalState) throw new Error("The simulation is already terminal");
          core.acceptanceForceVictory();
          send({ type: "acceptance-victory-forced", requestId: message.requestId });
          break;
      }
    } catch (error) {
      if (message.type === "start") {
        try { emitEvents(); } catch { /* Retain the original start error. */ }
      }
      const requestId = "requestId" in message && typeof message.requestId === "number" ? message.requestId : undefined;
      const fatal = message.type === "initialize" || message.type === "recycle" || isFatalWasmCoreError(error);
      const deferredLoadFailed = message.type === "load" && runGate.failLoad(terminalState, fatal);
      if (fatal) {
        await failRuntime(error, requestId);
        return;
      }
      if (deferredLoadFailed) {
        accumulator = 0;
        lastTime = performance.now();
        send({ type: "running", running: runGate.running });
        if (runGate.running) schedule();
        else stopTimer();
      }
      reportError(error, requestId, fatal);
    }
  })();
};
