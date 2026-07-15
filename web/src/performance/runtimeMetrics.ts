export interface RuntimeMetricsReport {
  format: "cncweb-runtime-performance";
  version: 3;
  core: "uninitialized" | "demo" | "wasm";
  packageRevision: string | null;
  missionId: string | null;
  buildId: string | null;
  acceptanceSession: string | null;
  running: boolean;
  visibilityState: "visible" | "hidden" | "unavailable";
  requestedWindowMs: number;
  observedWindowMs: number;
  frames: number;
  rafSpanMs: number;
  meanRafIntervalMs: number;
  p95RafIntervalMs: number;
  maximumRafIntervalMs: number;
  approximateRafFps: number;
  snapshotSamples: number;
  snapshotTickDelta: number;
  snapshotSpanMs: number;
  observedTickHz: number;
  maximumSnapshotGapMs: number;
  latestTick: number;
  latestSnapshotDeclaredBytes: number;
  maximumSnapshotDeclaredBytes: number;
  meanSnapshotDeclaredBytes: number;
  maximumSnapshotBufferBytes: number;
  snapshotDeclaredBytes: number;
  snapshotDeclaredBytesPerSecond: number;
  classicBaselineUploads: number;
  classicDeltaUploads: number;
  classicUnchangedUpdates: number;
  classicUploadSamples: number;
  classicPixelsUploaded: number;
  classicPixelsUploadedPerSecond: number;
  longTaskSupported: boolean;
  longTasks: number;
  maximumLongTaskMs: number;
  totalLongTaskMs: number;
}

export interface RuntimeMetricsApi {
  /** Returns rolling metrics for the last 1-60 seconds (10 seconds by default). */
  snapshot(windowMs?: number): RuntimeMetricsReport;
}

declare global {
  interface Window {
    __cncwebRuntimeMetrics?: RuntimeMetricsApi;
  }
}

interface TimedSample {
  at: number;
}

interface SnapshotSample extends TimedSample {
  tick: number;
  declaredBytes: number;
  bufferBytes: number;
}

interface UploadSample extends TimedSample {
  kind: "baseline" | "delta" | "unchanged";
  pixels: number;
}

interface LongTaskSample extends TimedSample {
  duration: number;
}

const MINIMUM_WINDOW_MS = 1_000;
const MAXIMUM_WINDOW_MS = 60_000;
const FRAME_SERIES_CAPACITY = 32_768;
const SERIES_CAPACITY = 8_192;

interface RuntimeSessionIdentity {
  core: RuntimeMetricsReport["core"];
  packageRevision: string | null;
  missionId: string | null;
}

function acceptanceSessionFromLocation(): string | null {
  if (typeof window === "undefined") return null;
  const value = new URLSearchParams(window.location.search).get("acceptance");
  return value && /^[a-f0-9]{32,64}$/.test(value) ? value : null;
}

class BoundedSeries<T> {
  private readonly values: Array<T | undefined>;
  private next = 0;
  private count = 0;

  constructor(capacity: number) {
    this.values = new Array(capacity);
  }

  push(value: T): void {
    this.values[this.next] = value;
    this.next = (this.next + 1) % this.values.length;
    this.count = Math.min(this.count + 1, this.values.length);
  }

  clear(): void {
    this.values.fill(undefined);
    this.next = 0;
    this.count = 0;
  }

  toArray(): T[] {
    const output: T[] = [];
    const first = (this.next - this.count + this.values.length) % this.values.length;
    for (let index = 0; index < this.count; index += 1) {
      const value = this.values[(first + index) % this.values.length];
      if (value !== undefined) output.push(value);
    }
    return output;
  }
}

function rounded(value: number): number {
  return Math.round(value * 100) / 100;
}

function roundedUp(value: number): number {
  return Math.ceil(value * 100) / 100;
}

function roundedRate(value: number): number {
  return Math.round(value * 10_000) / 10_000;
}

function currentVisibilityState(): RuntimeMetricsReport["visibilityState"] {
  if (typeof document === "undefined") return "unavailable";
  return document.visibilityState === "visible" ? "visible" : "hidden";
}

function mean(values: readonly number[]): number {
  return values.length === 0 ? 0 : values.reduce((sum, value) => sum + value, 0) / values.length;
}

function percentile95(values: readonly number[]): number {
  if (values.length === 0) return 0;
  const ordered = [...values].sort((left, right) => left - right);
  return ordered[Math.min(ordered.length - 1, Math.ceil(ordered.length * 0.95) - 1)];
}

function maximum(values: readonly number[]): number {
  return values.reduce((result, value) => Math.max(result, value), 0);
}

function finiteInteger(value: number, label: string): void {
  if (!Number.isSafeInteger(value) || value < 0) throw new RangeError(`${label} must be a non-negative safe integer`);
}

export class RuntimePerformanceMetrics {
  // 32,768 retains a full minute even on 500 Hz displays, with headroom for
  // timer jitter. Lower-frequency worker/upload/task streams use 8,192.
  private readonly frames = new BoundedSeries<number>(FRAME_SERIES_CAPACITY);
  private readonly snapshots = new BoundedSeries<SnapshotSample>(SERIES_CAPACITY);
  private readonly uploads = new BoundedSeries<UploadSample>(SERIES_CAPACITY);
  private readonly longTasks = new BoundedSeries<LongTaskSample>(SERIES_CAPACITY);
  private sessionStartedAt = performance.now();
  private latestTick = 0;
  private identity: RuntimeSessionIdentity = { core: "uninitialized", packageRevision: null, missionId: null };
  private buildId: string | null = null;
  private readonly acceptanceSession = acceptanceSessionFromLocation();
  private runtimeRunning = false;
  private visibilityState = currentVisibilityState();
  private longTaskObserver?: PerformanceObserver;
  readonly longTaskSupported: boolean;

  constructor(observeLongTasks = true) {
    this.longTaskSupported = observeLongTasks
      && typeof PerformanceObserver !== "undefined"
      && PerformanceObserver.supportedEntryTypes.includes("longtask");
    if (this.longTaskSupported) {
      this.longTaskObserver = new PerformanceObserver((list) => {
        for (const entry of list.getEntries()) this.recordLongTask(entry.startTime, entry.duration);
      });
      this.longTaskObserver.observe({ type: "longtask", buffered: true });
    }
  }

  reset(now = performance.now()): void {
    if (!Number.isFinite(now) || now < 0) throw new RangeError("Metrics reset time is invalid");
    this.frames.clear();
    this.snapshots.clear();
    this.uploads.clear();
    this.longTasks.clear();
    this.sessionStartedAt = now;
    this.latestTick = 0;
  }

  setSessionIdentity(core: "demo" | "wasm", packageRevision: string | null, missionId: string): void {
    if (core === "wasm" && (!packageRevision || !/^[a-f0-9]{64}$/.test(packageRevision))) {
      throw new Error("Wasm runtime metrics require a full lowercase package revision");
    }
    if (core === "demo" && packageRevision !== null) throw new Error("Demo runtime metrics cannot name a content revision");
    if (!/^[a-z0-9][a-z0-9_.-]{0,127}$/.test(missionId)) throw new Error("Runtime metrics mission ID is invalid");
    this.identity = { core, packageRevision, missionId };
    this.runtimeRunning = false;
    this.reset();
  }

  setBuildId(buildId: string): void {
    if (!/^[a-f0-9]{16}$/.test(buildId)) throw new Error("Runtime metrics build ID is invalid");
    if (this.buildId !== buildId) this.reset();
    this.buildId = buildId;
  }

  markRunningState(running: boolean, now = performance.now()): void {
    if (this.runtimeRunning === running) return;
    this.runtimeRunning = running;
    this.reset(now);
  }

  markVisibilityState(visibilityState: RuntimeMetricsReport["visibilityState"], now = performance.now()): void {
    if (visibilityState !== "visible" && visibilityState !== "hidden" && visibilityState !== "unavailable") {
      throw new Error("Runtime metrics visibility state is invalid");
    }
    if (this.visibilityState === visibilityState) return;
    this.visibilityState = visibilityState;
    this.reset(now);
  }

  recordFrame(now = performance.now()): void {
    if (Number.isFinite(now) && now >= 0) this.frames.push(now);
  }

  recordSnapshot(tick: number, declaredBytes: number, bufferBytes: number, now = performance.now()): void {
    finiteInteger(tick, "Snapshot tick");
    finiteInteger(declaredBytes, "Snapshot declared bytes");
    finiteInteger(bufferBytes, "Snapshot buffer bytes");
    if (declaredBytes > bufferBytes) throw new RangeError("Snapshot declared bytes cannot exceed its buffer envelope");
    if (!Number.isFinite(now) || now < 0) throw new RangeError("Snapshot sample time is invalid");
    // An unannounced rewind is still a timeline discontinuity. Clear every
    // series so pre-rewind frames/uploads cannot make the new segment pass.
    if (tick < this.latestTick) this.markTimelineDiscontinuity(now);
    this.latestTick = tick;
    this.snapshots.push({ at: now, tick, declaredBytes, bufferBytes });
  }

  markTimelineDiscontinuity(now = performance.now()): void {
    // The worker acknowledges a load before publishing its new full baseline.
    // Begin a wholly new measurement segment so its reported duration cannot
    // include pre-load frames, tasks, cadence, or uploads.
    this.reset(now);
  }

  recordClassicUpload(kind: UploadSample["kind"], pixels: number, now = performance.now()): void {
    finiteInteger(pixels, "Classic upload pixels");
    if (!Number.isFinite(now) || now < 0) throw new RangeError("Classic upload sample time is invalid");
    this.uploads.push({ at: now, kind, pixels });
  }

  recordLongTask(at: number, duration: number): void {
    if (!Number.isFinite(at) || at < 0 || !Number.isFinite(duration) || duration < 0) return;
    this.longTasks.push({ at, duration });
  }

  snapshot(windowMs = 10_000, now = performance.now()): RuntimeMetricsReport {
    if (!Number.isFinite(windowMs) || windowMs < MINIMUM_WINDOW_MS || windowMs > MAXIMUM_WINDOW_MS) {
      throw new RangeError("Runtime metrics window must be between 1000 and 60000 milliseconds");
    }
    if (!Number.isFinite(now) || now < 0) throw new RangeError("Runtime metrics sample time is invalid");
    const actualVisibilityState = currentVisibilityState();
    if (actualVisibilityState !== "unavailable" && actualVisibilityState !== this.visibilityState) {
      this.markVisibilityState(actualVisibilityState, now);
    }
    // A DevTools capture can run before the observer callback task. Consume
    // queued entries synchronously so a just-finished long task cannot evade
    // the acceptance report.
    for (const entry of this.longTaskObserver?.takeRecords() ?? []) {
      this.recordLongTask(entry.startTime, entry.duration);
    }
    const windowStart = Math.max(this.sessionStartedAt, now - windowMs);
    const observedWindowMs = Math.max(0, now - windowStart);
    const observedSeconds = observedWindowMs / 1000;
    const frames = this.frames.toArray().filter((at) => at >= windowStart && at <= now);
    const frameIntervals = frames.slice(1).map((at, index) => at - frames[index]);
    const frameGaps = frames.length > 0
      ? [frames[0] - windowStart, ...frames.slice(1).map((at, index) => at - frames[index]), now - frames.at(-1)!]
      : [observedWindowMs];
    const snapshots = this.snapshots.toArray().filter(({ at }) => at >= windowStart && at <= now);
    const snapshotGaps = snapshots.length > 0
      ? [snapshots[0].at - windowStart, ...snapshots.slice(1).map(({ at }, index) => at - snapshots[index].at), now - snapshots.at(-1)!.at]
      : [observedWindowMs];
    const firstSnapshot = snapshots[0];
    const lastSnapshot = snapshots.at(-1);
    const rafSpanMs = frames.length > 1 ? frames.at(-1)! - frames[0] : 0;
    const snapshotSpanMs = firstSnapshot && lastSnapshot ? lastSnapshot.at - firstSnapshot.at : 0;
    const snapshotTickDelta = firstSnapshot && lastSnapshot ? lastSnapshot.tick - firstSnapshot.tick : 0;
    const tickElapsedSeconds = snapshotSpanMs / 1000;
    const declaredSizes = snapshots.map(({ declaredBytes }) => declaredBytes);
    const declaredBytes = declaredSizes.reduce((sum, value) => sum + value, 0);
    const bufferSizes = snapshots.map(({ bufferBytes }) => bufferBytes);
    const uploads = this.uploads.toArray().filter(({ at }) => at >= windowStart && at <= now);
    const uploadedPixels = uploads.reduce((sum, { pixels }) => sum + pixels, 0);
    const tasks = this.longTasks.toArray().filter(({ at, duration }) => at + duration >= windowStart && at <= now);
    const taskDurations = tasks.map(({ duration }) => duration);
    const meanFrameInterval = mean(frameIntervals);

    return Object.freeze({
      format: "cncweb-runtime-performance",
      version: 3,
      ...this.identity,
      buildId: this.buildId,
      acceptanceSession: this.acceptanceSession,
      running: this.runtimeRunning,
      visibilityState: this.visibilityState,
      requestedWindowMs: windowMs,
      observedWindowMs: Math.floor(observedWindowMs),
      frames: frames.length,
      rafSpanMs: rounded(rafSpanMs),
      meanRafIntervalMs: roundedUp(meanFrameInterval),
      p95RafIntervalMs: roundedUp(percentile95(frameIntervals)),
      maximumRafIntervalMs: roundedUp(maximum(frameGaps)),
      approximateRafFps: meanFrameInterval > 0 ? rounded(1000 / meanFrameInterval) : 0,
      snapshotSamples: snapshots.length,
      snapshotTickDelta,
      snapshotSpanMs: rounded(snapshotSpanMs),
      observedTickHz: tickElapsedSeconds > 0 && firstSnapshot && lastSnapshot
        ? roundedRate(snapshotTickDelta / tickElapsedSeconds)
        : 0,
      maximumSnapshotGapMs: roundedUp(maximum(snapshotGaps)),
      latestTick: lastSnapshot?.tick ?? this.latestTick,
      latestSnapshotDeclaredBytes: lastSnapshot?.declaredBytes ?? 0,
      maximumSnapshotDeclaredBytes: maximum(declaredSizes),
      meanSnapshotDeclaredBytes: rounded(mean(declaredSizes)),
      maximumSnapshotBufferBytes: maximum(bufferSizes),
      snapshotDeclaredBytes: declaredBytes,
      snapshotDeclaredBytesPerSecond: observedSeconds > 0
        ? rounded(declaredBytes / observedSeconds)
        : 0,
      classicBaselineUploads: uploads.filter(({ kind }) => kind === "baseline").length,
      classicDeltaUploads: uploads.filter(({ kind }) => kind === "delta").length,
      classicUnchangedUpdates: uploads.filter(({ kind }) => kind === "unchanged").length,
      classicUploadSamples: uploads.length,
      classicPixelsUploaded: uploadedPixels,
      classicPixelsUploadedPerSecond: observedSeconds > 0 ? rounded(uploadedPixels / observedSeconds) : 0,
      longTaskSupported: this.longTaskSupported,
      longTasks: tasks.length,
      maximumLongTaskMs: rounded(maximum(taskDurations)),
      totalLongTaskMs: rounded(taskDurations.reduce((sum, value) => sum + value, 0)),
    });
  }

  destroy(): void {
    this.longTaskObserver?.disconnect();
    this.longTaskObserver = undefined;
  }
}

export const runtimePerformanceMetrics = new RuntimePerformanceMetrics();

export function setRuntimeMetricsBuildId(buildId: string): void {
  runtimePerformanceMetrics.setBuildId(buildId);
}

export function setRuntimeMetricsSessionIdentity(core: "demo" | "wasm", packageRevision: string | null, missionId: string): void {
  runtimePerformanceMetrics.setSessionIdentity(core, packageRevision, missionId);
}

if (typeof window !== "undefined") {
  const api: RuntimeMetricsApi = {
    snapshot: (windowMs) => runtimePerformanceMetrics.snapshot(windowMs),
  };
  Object.defineProperty(window, "__cncwebRuntimeMetrics", {
    configurable: true,
    enumerable: false,
    value: Object.freeze(api),
  });
  if (typeof document !== "undefined") {
    document.addEventListener("visibilitychange", () => {
      runtimePerformanceMetrics.markVisibilityState(currentVisibilityState());
    });
  }
}
