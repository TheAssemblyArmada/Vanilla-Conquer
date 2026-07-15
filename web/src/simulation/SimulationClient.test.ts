import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { SimulationClient } from "./SimulationClient";
import { runtimePerformanceMetrics } from "../performance/runtimeMetrics";
import { Faction, GameMode, type StartConfiguration } from "./protocol";
import { SnapshotSectionKind, snapshotByteLength, writeSnapshot } from "./snapshot";

class FakeWorker {
  static latest?: FakeWorker;
  onmessage: ((event: MessageEvent) => void) | null = null;
  onerror: ((event: ErrorEvent) => void) | null = null;
  readonly posted: unknown[] = [];
  readonly terminate = vi.fn();

  constructor() {
    FakeWorker.latest = this;
  }

  postMessage(message: unknown): void {
    this.posted.push(message);
  }

  emit(message: unknown): void {
    this.onmessage?.({ data: message } as MessageEvent);
  }
}

function validSnapshot(tick = 1, baseTick = tick): ArrayBuffer {
  const buffer = new ArrayBuffer(snapshotByteLength(1, 1, 0));
  writeSnapshot(buffer, {
    tick,
    worldWidth: 1,
    worldHeight: 1,
    classicWidth: 1,
    classicHeight: 1,
    classicPixels: new Uint8Array(1),
    palette: new Uint8Array(256 * 4),
    sprites: [],
    cameraX: 0,
    cameraY: 0,
    zoom: 1,
  });
  new DataView(buffer).setUint32(20, baseTick, true);
  return buffer;
}

function singleSectionSnapshot(
  kind: SnapshotSectionKind,
  count: number,
  payload: Uint8Array,
  tick: number,
  baseTick: number,
): ArrayBuffer {
  const buffer = new ArrayBuffer(40 + 16 + payload.byteLength);
  const view = new DataView(buffer);
  view.setUint32(0, 0x57434e43, true);
  view.setUint16(4, 1, true);
  view.setUint16(6, 3, true);
  view.setUint32(8, buffer.byteLength, true);
  view.setUint32(12, 1, true);
  view.setUint32(16, tick, true);
  view.setUint32(20, baseTick, true);
  view.setUint32(32, 1, true);
  view.setUint16(40, kind, true);
  view.setUint32(44, payload.byteLength, true);
  view.setUint32(48, count, true);
  new Uint8Array(buffer, 56).set(payload);
  return buffer;
}

function staticMapSnapshot(tick: number, baseTick: number, retained: boolean): ArrayBuffer {
  const count = 4;
  const payload = new Uint8Array(304 + (retained ? 0 : count * 36));
  const view = new DataView(payload.buffer);
  view.setInt32(8, 2, true);
  view.setInt32(12, 2, true);
  view.setInt32(24, 2, true);
  view.setInt32(28, 2, true);
  view.setInt32(32, 1, true);
  view.setUint32(300, count, true);
  return singleSectionSnapshot(SnapshotSectionKind.StaticMap, count, payload, tick, baseTick);
}

function dirtyClassicSnapshot(tick: number, baseTick: number): ArrayBuffer {
  const payload = new Uint8Array(33);
  const view = new DataView(payload.buffer);
  view.setUint32(0, 1, true);
  view.setUint32(4, 1, true);
  view.setUint32(8, 1, true);
  view.setUint32(12, 2, true);
  view.setUint32(24, 1, true);
  view.setUint32(28, 1, true);
  return singleSectionSnapshot(SnapshotSectionKind.ClassicSurface, 1, payload, tick, baseTick);
}

const startConfiguration: StartConfiguration = {
  game: "demo",
  seed: 1,
  scenario: 1,
  variation: 0,
  direction: 0,
  buildLevel: 1,
  sabotagedStructure: -1,
  faction: Faction.Gdi,
  gameMode: GameMode.Campaign,
  playerId: 42n,
  contentDirectory: "/content",
  overrideMapName: "",
  contentIdHash: 1n,
};

describe("SimulationClient snapshot acknowledgement", () => {
  beforeEach(() => {
    FakeWorker.latest = undefined;
    vi.stubGlobal("Worker", FakeWorker);
  });

  afterEach(() => {
    window.history.replaceState(null, "", "/");
    vi.unstubAllGlobals();
  });

  it("terminates without recycling when a snapshot listener rejects a delta", async () => {
    const client = new SimulationClient();
    void client.ready().catch(() => undefined);
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    const errorListener = vi.fn();
    const runningListener = vi.fn();
    client.onError(errorListener);
    client.onRunningChange(runningListener);
    client.onSnapshot(() => { throw new Error("ingestion failed"); });

    worker.emit({ type: "snapshot", buffer: validSnapshot() });

    expect(worker.terminate).toHaveBeenCalledTimes(1);
    expect(worker.posted).not.toContainEqual(expect.objectContaining({ type: "recycle" }));
    expect(runningListener).toHaveBeenCalledWith(false);
    expect(errorListener).toHaveBeenCalledWith(expect.objectContaining({ message: "ingestion failed" }));
    await expect(client.save()).rejects.toThrow(/worker has failed/i);
    expect((client as unknown as { pending: Map<number, unknown> }).pending.size).toBe(0);
    client.dispose();
    expect(worker.posted).not.toContainEqual(expect.objectContaining({ type: "shutdown" }));
  });

  it("terminates without recycling a malformed snapshot", () => {
    const client = new SimulationClient();
    void client.ready().catch(() => undefined);
    const worker = FakeWorker.latest!;
    const errorListener = vi.fn();
    client.onError(errorListener);

    worker.emit({ type: "snapshot", buffer: new ArrayBuffer(1) });

    expect(worker.terminate).toHaveBeenCalledTimes(1);
    expect(worker.posted).not.toContainEqual(expect.objectContaining({ type: "recycle" }));
    expect(errorListener).toHaveBeenCalledWith(expect.objectContaining({ message: expect.stringMatching(/truncated/i) }));
  });

  it("accepts linked retained maps after full snapshots, including a later full replacement", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    const retained: boolean[] = [];
    client.onSnapshot((snapshot) => retained.push(snapshot.staticMap?.retained ?? false));

    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(5, 5, false) });
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(6, 5, false) });
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(100, 6, true) });

    expect(retained).toEqual([false, false, true]);
    expect(worker.terminate).not.toHaveBeenCalled();
    client.dispose();
    const shutdown = worker.posted.at(-1) as { requestId: number };
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
  });

  it("fails closed when a retained snapshot skips the preceding materialized tick", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    const errorListener = vi.fn();
    client.onError(errorListener);
    client.onSnapshot(() => undefined);

    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(5, 5, false) });
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(9, 4, true) });

    expect(worker.terminate).toHaveBeenCalledOnce();
    expect(errorListener).toHaveBeenCalledWith(expect.objectContaining({ message: expect.stringMatching(/base tick 4.*preceding tick 5/i) }));
    expect(worker.posted).not.toContainEqual(expect.objectContaining({ type: "recycle" }));
  });

  it("requires a full classic baseline before accepting dirty rectangles", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    const errorListener = vi.fn();
    client.onError(errorListener);
    client.onSnapshot(() => undefined);

    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(5, 5, false) });
    worker.emit({ type: "snapshot", buffer: dirtyClassicSnapshot(6, 5) });

    expect(worker.terminate).toHaveBeenCalledOnce();
    expect(errorListener).toHaveBeenCalledWith(expect.objectContaining({ message: expect.stringMatching(/full surface baseline/i) }));
  });

  it("accepts a linked dirty rectangle after a full classic surface", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    const formats: number[] = [];
    client.onSnapshot((snapshot) => formats.push(snapshot.classicFormat));

    worker.emit({ type: "snapshot", buffer: validSnapshot(5, 5) });
    worker.emit({ type: "snapshot", buffer: dirtyClassicSnapshot(20, 5) });

    expect(formats).toEqual([1, 2]);
    expect(worker.terminate).not.toHaveBeenCalled();
    client.dispose();
    const shutdown = worker.posted.at(-1) as { requestId: number };
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
  });

  it("passes the deferred-resume contract to the worker", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });

    const started = client.start(startConfiguration, { deferRunningUntilLoad: true });
    await Promise.resolve();
    const message = worker.posted.find((value) => (value as { type?: string }).type === "start") as { requestId: number; deferRunningUntilLoad: boolean };
    expect(message.deferRunningUntilLoad).toBe(true);
    worker.emit({ type: "started", requestId: message.requestId });
    await started;
    client.dispose();
    const shutdown = worker.posted.at(-1) as { requestId: number };
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
  });

  it("requires a self-based full bootstrap after start succeeds", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    client.onSnapshot(() => undefined);
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(5, 5, false) });

    const started = client.start(startConfiguration);
    await Promise.resolve();
    const startMessage = worker.posted.find((value) => (value as { type?: string }).type === "start") as { requestId: number };
    worker.emit({ type: "started", requestId: startMessage.requestId });
    await started;
    const errorListener = vi.fn();
    client.onError(errorListener);
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(6, 5, false) });

    expect(worker.terminate).toHaveBeenCalledOnce();
    expect(errorListener).toHaveBeenCalledWith(expect.objectContaining({ message: expect.stringMatching(/bootstrap snapshot base tick/i) }));
  });

  it("rejects load when the replacement bootstrap cannot be installed", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();

    const loading = client.load(new ArrayBuffer(64));
    const rejected = expect(loading).rejects.toThrow(/truncated/i);
    await Promise.resolve();
    const loadMessage = worker.posted.find((value) => (value as { type?: string }).type === "load") as { requestId: number };
    worker.emit({ type: "loaded", requestId: loadMessage.requestId });
    worker.emit({ type: "snapshot", buffer: new ArrayBuffer(1) });

    await rejected;
    expect(worker.terminate).toHaveBeenCalledOnce();
    expect((client as unknown as { pending: Map<number, unknown> }).pending.size).toBe(0);
  });

  it("rejects a retained replacement after a successful load resets the chain", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    client.onSnapshot(() => undefined);
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(5, 5, false) });

    const loading = client.load(new ArrayBuffer(64));
    const rejected = expect(loading).rejects.toThrow(/accepted full baseline/i);
    await Promise.resolve();
    const loadMessage = worker.posted.find((value) => (value as { type?: string }).type === "load") as { requestId: number };
    worker.emit({ type: "loaded", requestId: loadMessage.requestId });
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(6, 5, true) });

    await rejected;
    expect(worker.terminate).toHaveBeenCalledOnce();
  });

  it("preserves the accepted chain after a transactional load failure", async () => {
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    const retained: boolean[] = [];
    client.onSnapshot((snapshot) => retained.push(snapshot.staticMap?.retained ?? false));
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(5, 5, false) });

    const loading = client.load(new ArrayBuffer(64));
    await Promise.resolve();
    const loadMessage = worker.posted.find((value) => (value as { type?: string }).type === "load") as { requestId: number };
    worker.emit({ type: "error", requestId: loadMessage.requestId, fatal: false, message: "load rejected" });
    await expect(loading).rejects.toThrow("load rejected");
    worker.emit({ type: "snapshot", buffer: staticMapSnapshot(6, 5, true) });

    expect(retained).toEqual([false, true]);
    expect(worker.terminate).not.toHaveBeenCalled();
    client.dispose();
    const shutdown = worker.posted.at(-1) as { requestId: number };
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
  });

  it("records exact wasm content and mission identity for runtime evidence", () => {
    const revision = "ab".repeat(32);
    const client = new SimulationClient({
      core: "wasm",
      emscriptenModuleUrl: "https://example.test/tiberiandawn.js",
      missionId: "gdi-01-east-a",
      contentMount: { packageId: "owned", revision, storageKey: "owned", files: [] },
    });
    void client.ready().catch(() => undefined);

    expect(runtimePerformanceMetrics.snapshot(1_000)).toMatchObject({
      core: "wasm",
      packageRevision: revision,
      missionId: "gdi-01-east-a",
    });
    const worker = FakeWorker.latest!;
    client.dispose();
    const shutdown = worker.posted.at(-1) as { requestId: number };
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
  });

  it("forwards the narrow victory request only for an intentional loopback acceptance session", async () => {
    const acceptanceSession = "ab".repeat(16);
    window.history.replaceState(null, "", `/?acceptance=${acceptanceSession}`);
    const revision = "cd".repeat(32);
    const client = new SimulationClient({
      core: "wasm",
      emscriptenModuleUrl: "http://127.0.0.1:4173/engine/tiberiandawn.js",
      missionId: "gdi-01-east-a",
      contentMount: { packageId: "freeware", revision, storageKey: "freeware", files: [] },
    });
    const worker = FakeWorker.latest!;
    expect(worker.posted[0]).toMatchObject({ type: "initialize", acceptanceSession });
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });

    const forcing = client.forceVictoryForAcceptance();
    await Promise.resolve();
    const request = worker.posted.find((message) => (message as { type?: string }).type === "acceptance-force-victory") as { requestId: number };
    expect(request).toBeDefined();
    worker.emit({ type: "acceptance-victory-forced", requestId: request.requestId });
    await forcing;

    client.dispose();
    const shutdown = worker.posted.at(-1) as { requestId: number };
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
  });

  it("rejects victory requests in ordinary sessions without messaging the worker", async () => {
    const client = new SimulationClient();
    void client.ready().catch(() => undefined);
    const worker = FakeWorker.latest!;
    await expect(client.forceVictoryForAcceptance()).rejects.toThrow(/outside an intentional loopback session/i);
    expect(worker.posted).not.toContainEqual(expect.objectContaining({ type: "acceptance-force-victory" }));
    client.dispose();
    const shutdown = worker.posted.at(-1) as { requestId: number };
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
  });

  it("invalidates runtime evidence across pause, load, terminal, and disposal boundaries", async () => {
    const runningState = vi.spyOn(runtimePerformanceMetrics, "markRunningState");
    const discontinuity = vi.spyOn(runtimePerformanceMetrics, "markTimelineDiscontinuity");
    const client = new SimulationClient();
    const worker = FakeWorker.latest!;
    worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
    await client.ready();
    runningState.mockClear();
    discontinuity.mockClear();

    worker.emit({ type: "running", running: true });
    expect(runtimePerformanceMetrics.snapshot(1_000).running).toBe(true);
    client.setRunning(false);
    expect(runtimePerformanceMetrics.snapshot(1_000).running).toBe(false);
    worker.emit({ type: "running", running: false });
    client.setRunning(true);
    worker.emit({ type: "running", running: true });

    const loading = client.load(new ArrayBuffer(64));
    await Promise.resolve();
    const loadMessage = worker.posted.find((value) => (value as { type?: string }).type === "load") as { requestId: number };
    expect(loadMessage).toBeDefined();
    expect(discontinuity).toHaveBeenCalledTimes(1);
    let loadResolved = false;
    void loading.then(() => { loadResolved = true; });
    worker.emit({ type: "loaded", requestId: loadMessage.requestId });
    await Promise.resolve();
    expect(loadResolved).toBe(false);
    worker.emit({ type: "snapshot", buffer: validSnapshot() });
    await loading;
    expect(loadResolved).toBe(true);
    expect(discontinuity).toHaveBeenCalledTimes(2);

    worker.emit({ type: "event", event: { kind: "game-over" } });
    expect(runtimePerformanceMetrics.snapshot(1_000).running).toBe(false);
    expect(runningState).toHaveBeenLastCalledWith(false);

    client.dispose();
    expect(runningState).toHaveBeenLastCalledWith(false);
    const shutdown = worker.posted.at(-1) as { requestId: number };
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
  });

  it("waits for shutdown acknowledgement before terminating", async () => {
    const client = new SimulationClient();
    void client.ready().catch(() => undefined);
    const worker = FakeWorker.latest!;

    client.dispose();

    const shutdown = worker.posted.at(-1) as { type: string; requestId: number };
    expect(shutdown.type).toBe("shutdown");
    expect(worker.terminate).not.toHaveBeenCalled();
    worker.emit({ type: "shutdown-complete", requestId: shutdown.requestId });
    expect(worker.terminate).toHaveBeenCalledTimes(1);
  });

  it("terminates after the bounded shutdown fallback and rejects pending work", async () => {
    vi.useFakeTimers();
    try {
      const client = new SimulationClient();
      const worker = FakeWorker.latest!;
      worker.emit({ type: "ready", capabilities: { saves: true, stateHashes: false, tickRate: 15, protocolVersion: 1 } });
      const started = client.start(startConfiguration);
      await Promise.resolve();

      client.dispose();

      await expect(started).rejects.toThrow(/disposed/i);
      expect((client as unknown as { pending: Map<number, unknown> }).pending.size).toBe(0);
      expect(worker.terminate).not.toHaveBeenCalled();
      await vi.advanceTimersByTimeAsync(500);
      expect(worker.terminate).toHaveBeenCalledTimes(1);
    } finally {
      vi.useRealTimers();
    }
  });
});
