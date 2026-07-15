import {
  encodeCommandBatch,
  SIMULATION_PROTOCOL_VERSION,
  type MainToWorkerMessage,
  type ContentMountProgress,
  type ContentMountRequest,
  type SimulationCapabilities,
  type SimulationCommand,
  type SimulationEvent,
  type StartConfiguration,
  type WorkerToMainMessage,
} from "./protocol";
import { runtimePerformanceMetrics, setRuntimeMetricsSessionIdentity } from "../performance/runtimeMetrics";
import { localAcceptanceSession } from "./acceptanceHooks";
import { SnapshotView } from "./snapshot";

type SnapshotListener = (snapshot: SnapshotView) => void;
type ErrorListener = (error: Error) => void;
type RunningListener = (running: boolean) => void;
type MountProgressListener = (progress: ContentMountProgress) => void;

interface PendingRequest {
  resolve: (value: ArrayBuffer | void) => void;
  reject: (error: Error) => void;
}

export type SimulationClientOptions =
  | { core?: "demo" }
  | { core: "wasm"; emscriptenModuleUrl: string; contentMount: ContentMountRequest; missionId: string };

export interface SimulationStartOptions {
  deferRunningUntilLoad?: boolean;
}

const SHUTDOWN_TIMEOUT_MS = 500;

export class SimulationClient {
  private readonly worker: Worker;
  private readonly pending = new Map<number, PendingRequest>();
  private readonly snapshots = new Set<SnapshotListener>();
  private readonly errors = new Set<ErrorListener>();
  private readonly runningListeners = new Set<RunningListener>();
  private readonly mountProgressListeners = new Set<MountProgressListener>();
  private readonly eventListeners = new Set<(event: SimulationEvent) => void>();
  private nextRequestId = 1;
  private disposed = false;
  private failed = false;
  private terminated = false;
  private loadAwaitingSnapshotRequestId?: number;
  private shutdownRequestId?: number;
  private shutdownTimer?: ReturnType<typeof setTimeout>;
  private acceptedSnapshotTick?: number;
  private hasStaticMapBaseline = false;
  private hasClassicSurfaceBaseline = false;
  private readonly readyPromise: Promise<SimulationCapabilities>;
  private readonly acceptanceEnabled: boolean;
  private resolveReady!: (capabilities: SimulationCapabilities) => void;
  private rejectReady!: (error: Error) => void;

  constructor(options: SimulationClientOptions = {}) {
    runtimePerformanceMetrics.reset();
    if (options.core === "wasm") setRuntimeMetricsSessionIdentity("wasm", options.contentMount.revision, options.missionId);
    else setRuntimeMetricsSessionIdentity("demo", null, "demo");
    this.worker = new Worker(new URL("./simulation.worker.ts", import.meta.url), { type: "module", name: "theater-simulation" });
    this.worker.onmessage = (event: MessageEvent<WorkerToMainMessage>) => this.handleMessage(event.data);
    this.worker.onerror = (event) => this.fail(new Error(event.message || "Simulation worker failed"), true);
    this.readyPromise = new Promise<SimulationCapabilities>((resolve, reject) => {
      this.resolveReady = resolve;
      this.rejectReady = reject;
    });
    const acceptanceSession = options.core === "wasm" ? localAcceptanceSession() : undefined;
    this.acceptanceEnabled = Boolean(acceptanceSession);
    const initialize: MainToWorkerMessage = options.core === "wasm"
      ? { type: "initialize", protocolVersion: SIMULATION_PROTOCOL_VERSION, core: "wasm", emscriptenModuleUrl: options.emscriptenModuleUrl, contentMount: options.contentMount, ...(acceptanceSession ? { acceptanceSession } : {}) }
      : { type: "initialize", protocolVersion: SIMULATION_PROTOCOL_VERSION, core: "demo" };
    this.post(initialize);
  }

  ready(): Promise<SimulationCapabilities> {
    return this.readyPromise;
  }

  async start(configuration: StartConfiguration, options: SimulationStartOptions = {}): Promise<void> {
    await this.ready();
    const requestId = this.nextRequestId++;
    const result = this.request(requestId);
    this.post({ type: "start", requestId, configuration, deferRunningUntilLoad: Boolean(options.deferRunningUntilLoad) });
    await result;
  }

  sendCommands(commands: readonly SimulationCommand[], targetTick = 0): void {
    const buffer = encodeCommandBatch(targetTick, commands);
    this.post({ type: "commands", buffer }, [buffer]);
  }

  setRunning(running: boolean): void {
    this.post({ type: "set-running", running });
    // Invalidate the current evidence segment immediately. The worker echo is
    // authoritative, but it cannot arrive until after this call stack.
    runtimePerformanceMetrics.markRunningState(running);
  }

  cancelDeferredLoad(): void {
    this.post({ type: "cancel-deferred-load" });
  }

  async save(): Promise<ArrayBuffer> {
    await this.ready();
    const requestId = this.nextRequestId++;
    const result = this.request(requestId);
    this.post({ type: "save", requestId });
    return (await result) as ArrayBuffer;
  }

  async load(buffer: ArrayBuffer): Promise<void> {
    await this.ready();
    const requestId = this.nextRequestId++;
    const result = this.request(requestId);
    this.post({ type: "load", requestId, buffer }, [buffer]);
    // A load attempt is a measurement discontinuity even when the backend
    // rejects it and transactionally restores the old state.
    runtimePerformanceMetrics.markTimelineDiscontinuity();
    await result;
  }

  async forceVictoryForAcceptance(): Promise<void> {
    if (!this.acceptanceEnabled) throw new Error("Release-acceptance hooks are unavailable outside an intentional loopback session");
    await this.ready();
    const requestId = this.nextRequestId++;
    const result = this.request(requestId);
    this.post({ type: "acceptance-force-victory", requestId });
    await result;
  }

  recycle(snapshot: SnapshotView): void {
    if (this.disposed || this.failed || snapshot.buffer.byteLength === 0) return;
    this.post({ type: "recycle", buffer: snapshot.buffer }, [snapshot.buffer]);
  }

  onSnapshot(listener: SnapshotListener): () => void {
    this.snapshots.add(listener);
    return () => this.snapshots.delete(listener);
  }

  onError(listener: ErrorListener): () => void {
    this.errors.add(listener);
    return () => this.errors.delete(listener);
  }

  onRunningChange(listener: RunningListener): () => void {
    this.runningListeners.add(listener);
    return () => this.runningListeners.delete(listener);
  }

  onEvent(listener: (event: SimulationEvent) => void): () => void {
    this.eventListeners.add(listener);
    return () => this.eventListeners.delete(listener);
  }

  onMountProgress(listener: MountProgressListener): () => void {
    this.mountProgressListeners.add(listener);
    return () => this.mountProgressListeners.delete(listener);
  }

  dispose(): void {
    if (this.disposed) return;
    this.disposed = true;
    runtimePerformanceMetrics.markRunningState(false);
    const error = new Error("Simulation client was disposed");
    this.rejectReady(error);
    for (const pending of this.pending.values()) pending.reject(error);
    this.pending.clear();
    this.loadAwaitingSnapshotRequestId = undefined;
    if (this.failed) return;
    const requestId = this.nextRequestId++;
    this.shutdownRequestId = requestId;
    this.shutdownTimer = setTimeout(() => this.terminateWorker(), SHUTDOWN_TIMEOUT_MS);
    this.worker.postMessage({ type: "shutdown", requestId } satisfies MainToWorkerMessage);
  }

  private request(requestId: number): Promise<ArrayBuffer | void> {
    this.assertActive();
    return new Promise((resolve, reject) => this.pending.set(requestId, { resolve, reject }));
  }

  private post(message: MainToWorkerMessage, transfer: Transferable[] = []): void {
    this.assertActive();
    this.worker.postMessage(message, transfer);
  }

  private assertActive(): void {
    if (this.disposed) throw new Error("Simulation client is disposed");
    if (this.failed) throw new Error("Simulation worker has failed");
  }

  private completeRequest(requestId: number, value?: ArrayBuffer): void {
    const request = this.pending.get(requestId);
    if (!request) return;
    this.pending.delete(requestId);
    request.resolve(value);
  }

  private handleMessage(message: WorkerToMainMessage): void {
    if (this.disposed && message.type !== "shutdown-complete") return;
    switch (message.type) {
      case "ready":
        this.resolveReady(message.capabilities);
        break;
      case "started":
        this.resetSnapshotChain();
        this.completeRequest(message.requestId);
        break;
      case "loaded":
        runtimePerformanceMetrics.markTimelineDiscontinuity();
        this.resetSnapshotChain();
        /* The worker emits the replacement bootstrap immediately after this
         * acknowledgement. Resolve load only once that snapshot has parsed
         * and every listener has installed it, so callers cannot act on the
         * new timeline through stale presentation state. */
        if (this.pending.has(message.requestId)) this.loadAwaitingSnapshotRequestId = message.requestId;
        break;
      case "acceptance-victory-forced":
        this.completeRequest(message.requestId);
        break;
      case "shutdown-complete":
        if (message.requestId === this.shutdownRequestId) this.terminateWorker();
        break;
      case "saved":
        this.completeRequest(message.requestId, message.buffer);
        break;
      case "snapshot":
        try {
          const snapshot = new SnapshotView(message.buffer);
          this.validateSnapshotChain(snapshot);
          runtimePerformanceMetrics.recordSnapshot(snapshot.tick, snapshot.byteLength, message.buffer.byteLength);
          for (const listener of this.snapshots) listener(snapshot);
          this.acceptSnapshot(snapshot);
          if (this.snapshots.size === 0) this.recycle(snapshot);
          if (this.loadAwaitingSnapshotRequestId !== undefined) {
            const requestId = this.loadAwaitingSnapshotRequestId;
            this.loadAwaitingSnapshotRequestId = undefined;
            this.completeRequest(requestId);
          }
        } catch (error) {
          // Parsing/listener failure means this delta was not durably accepted.
          // Recycling it would let the worker publish a later delta relative to
          // a baseline the app may never have ingested. Stop the stream instead.
          this.fail(error instanceof Error ? error : new Error(String(error)), true);
        }
        break;
      case "running":
        runtimePerformanceMetrics.markRunningState(message.running);
        for (const listener of this.runningListeners) listener(message.running);
        break;
      case "event":
        if (message.event.kind === "game-over") runtimePerformanceMetrics.markRunningState(false);
        for (const listener of this.eventListeners) listener(message.event);
        break;
      case "mount-progress":
        for (const listener of this.mountProgressListeners) listener(message.progress);
        break;
      case "error": {
        const error = new Error(message.message);
        if (message.requestId !== undefined) {
          const request = this.pending.get(message.requestId);
          request?.reject(error);
          this.pending.delete(message.requestId);
          if (this.loadAwaitingSnapshotRequestId === message.requestId) this.loadAwaitingSnapshotRequestId = undefined;
        }
        if (message.fatal) {
          this.fail(error, true);
        } else {
          this.handleError(error);
        }
        break;
      }
    }
  }

  private validateSnapshotChain(snapshot: SnapshotView): void {
    if (this.acceptedSnapshotTick === undefined) {
      if (snapshot.requiresBaseline) throw new Error("Retained snapshot requires an accepted full baseline");
      if (snapshot.baseTick !== snapshot.tick) throw new Error("Bootstrap snapshot base tick does not match its tick");
      return;
    }
    if (snapshot.requiresBaseline && snapshot.baseTick !== this.acceptedSnapshotTick) {
      throw new Error(`Snapshot base tick ${snapshot.baseTick} does not match preceding tick ${this.acceptedSnapshotTick}`);
    }
    if (!snapshot.requiresBaseline && snapshot.baseTick !== snapshot.tick && snapshot.baseTick !== this.acceptedSnapshotTick) {
      throw new Error(`Full snapshot base tick ${snapshot.baseTick} matches neither its tick nor preceding tick ${this.acceptedSnapshotTick}`);
    }
    if (snapshot.staticMap?.retained && !this.hasStaticMapBaseline) {
      throw new Error("Retained static map requires an accepted full static-map baseline");
    }
    if (snapshot.classicFormat === 2 && !this.hasClassicSurfaceBaseline) {
      throw new Error("Classic surface delta requires an accepted full surface baseline");
    }
  }

  private acceptSnapshot(snapshot: SnapshotView): void {
    if (snapshot.staticMap && !snapshot.staticMap.retained) this.hasStaticMapBaseline = true;
    if (snapshot.classicFormat === 1) this.hasClassicSurfaceBaseline = true;
    this.acceptedSnapshotTick = snapshot.tick;
  }

  private resetSnapshotChain(): void {
    this.acceptedSnapshotTick = undefined;
    this.hasStaticMapBaseline = false;
    this.hasClassicSurfaceBaseline = false;
  }

  private handleError(error: Error): void {
    for (const listener of this.errors) listener(error);
  }

  private fail(error: Error, rejectPending: boolean): void {
    if (this.failed) return;
    if (rejectPending) {
      this.failed = true;
      this.terminateWorker();
      runtimePerformanceMetrics.markRunningState(false);
      for (const listener of this.runningListeners) listener(false);
    }
    this.rejectReady(error);
    if (rejectPending) {
      for (const pending of this.pending.values()) pending.reject(error);
      this.pending.clear();
      this.loadAwaitingSnapshotRequestId = undefined;
    }
    this.handleError(error);
  }

  private terminateWorker(): void {
    if (this.terminated) return;
    this.terminated = true;
    if (this.shutdownTimer !== undefined) clearTimeout(this.shutdownTimer);
    this.shutdownTimer = undefined;
    this.worker.terminate();
  }
}
