import { describe, expect, it, vi } from "vitest";
import { RuntimePerformanceMetrics } from "./runtimeMetrics";

describe("RuntimePerformanceMetrics", () => {
  it("reports a bounded 60-second owned-runtime measurement", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    metrics.reset(1_000);
    metrics.markRunningState(true, 1_000);
    for (let frame = 0; frame <= 3_600; frame += 1) metrics.recordFrame(1_000 + frame * (1000 / 60));
    for (let tick = 0; tick <= 900; tick += 1) {
      const at = 1_000 + tick * (1000 / 15);
      metrics.recordSnapshot(tick, 200_000 + tick % 2_000, 262_144, at);
      metrics.recordClassicUpload(tick === 0 ? "baseline" : tick % 3 === 0 ? "unchanged" : "delta", tick === 0 ? 1536 * 1536 : tick % 3 === 0 ? 0 : 64, at);
    }
    metrics.recordLongTask(40_000, 72);

    const report = metrics.snapshot(60_000, 61_000);
    expect(report).toMatchObject({
      format: "cncweb-runtime-performance",
      version: 3,
      core: "uninitialized",
      packageRevision: null,
      running: true,
      visibilityState: "visible",
      observedWindowMs: 60_000,
      frames: 3_600,
      rafSpanMs: 59_983.33,
      snapshotSamples: 900,
      snapshotTickDelta: 899,
      snapshotSpanMs: 59_933.33,
      observedTickHz: 15,
      maximumSnapshotBufferBytes: 262_144,
      snapshotDeclaredBytes: 180_404_550,
      classicBaselineUploads: 1,
      classicDeltaUploads: 600,
      classicUnchangedUpdates: 299,
      classicUploadSamples: 900,
      longTasks: 1,
      maximumLongTaskMs: 72,
    });
    expect(report.approximateRafFps).toBeCloseTo(60);
    expect(report.p95RafIntervalMs).toBeCloseTo(1000 / 60, 1);
    expect(report.snapshotDeclaredBytesPerSecond).toBeGreaterThan(3_000_000);
    expect(Object.isFrozen(report)).toBe(true);
    metrics.destroy();
  });

  it("starts a fresh cadence segment when a save load rewinds the tick", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    metrics.reset(0);
    metrics.recordSnapshot(100, 100, 128, 1_000);
    metrics.recordSnapshot(101, 100, 128, 1_100);
    metrics.recordSnapshot(40, 100, 128, 1_200);
    metrics.recordSnapshot(41, 100, 128, 1_300);
    expect(metrics.snapshot(1_000, 1_300)).toMatchObject({ snapshotSamples: 2, latestTick: 41, observedTickHz: 10 });
  });

  it("binds reports to a validated runtime and build identity", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    const revision = "ab".repeat(32);
    metrics.setSessionIdentity("wasm", revision, "gdi-01-east-a");
    metrics.setBuildId("0123456789abcdef");
    expect(metrics.snapshot(1_000, performance.now())).toMatchObject({
      version: 3,
      core: "wasm",
      packageRevision: revision,
      missionId: "gdi-01-east-a",
      buildId: "0123456789abcdef",
    });
    expect(() => metrics.setSessionIdentity("wasm", null, "gdi-01-east-a")).toThrow(/package revision/i);
    expect(() => metrics.setBuildId("development")).toThrow(/build ID/i);
  });

  it("starts a fresh cadence and upload segment on an acknowledged forward-jump load", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    metrics.reset(0);
    metrics.recordSnapshot(0, 100, 128, 100);
    metrics.recordClassicUpload("baseline", 100, 100);
    metrics.markTimelineDiscontinuity(150);
    metrics.recordSnapshot(500, 100, 128, 200);
    metrics.recordSnapshot(501, 100, 128, 300);
    metrics.recordClassicUpload("baseline", 100, 200);
    expect(metrics.snapshot(1_000, 300)).toMatchObject({
      snapshotSamples: 2,
      latestTick: 501,
      observedTickHz: 10,
      classicBaselineUploads: 1,
    });
  });

  it("cannot carry samples across a pause or resume transition", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    metrics.reset(0);
    metrics.markRunningState(true, 0);
    metrics.recordFrame(500);
    metrics.recordSnapshot(8, 100, 128, 500);

    metrics.markRunningState(false, 900);
    expect(metrics.snapshot(1_000, 1_000)).toMatchObject({
      running: false,
      observedWindowMs: 100,
      frames: 0,
      snapshotSamples: 0,
    });

    metrics.markRunningState(true, 1_000);
    metrics.recordFrame(1_500);
    expect(metrics.snapshot(1_000, 2_000)).toMatchObject({
      running: true,
      observedWindowMs: 1_000,
      frames: 1,
      snapshotSamples: 0,
    });
  });

  it("invalidates the measurement when document visibility changes", () => {
    const visibility = vi.spyOn(document, "visibilityState", "get").mockReturnValue("visible");
    try {
      const metrics = new RuntimePerformanceMetrics(false);
      metrics.reset(0);
      metrics.markRunningState(true, 0);
      metrics.recordFrame(500);

      visibility.mockReturnValue("hidden");
      expect(metrics.snapshot(1_000, 1_000)).toMatchObject({
        visibilityState: "hidden",
        observedWindowMs: 0,
        frames: 0,
      });

      visibility.mockReturnValue("visible");
      expect(metrics.snapshot(1_000, 1_500)).toMatchObject({
        visibilityState: "visible",
        observedWindowMs: 0,
        frames: 0,
      });
      metrics.destroy();
    } finally {
      visibility.mockRestore();
    }
  });

  it("rejects impossible snapshot envelope accounting", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    expect(() => metrics.recordSnapshot(1, 129, 128, 1)).toThrow(/buffer envelope/i);
  });

  it("synchronously captures queued long tasks, including a window-edge overlap", () => {
    const queued = [{ startTime: 750, duration: 100 } as PerformanceEntry];
    class FakePerformanceObserver {
      static readonly supportedEntryTypes = ["longtask"];
      constructor(_callback: PerformanceObserverCallback) {}
      observe(): void {}
      disconnect(): void {}
      takeRecords(): PerformanceEntryList { return queued.splice(0); }
    }
    vi.stubGlobal("PerformanceObserver", FakePerformanceObserver as unknown as typeof PerformanceObserver);
    try {
      const metrics = new RuntimePerformanceMetrics();
      metrics.reset(0);
      expect(metrics.snapshot(1_000, 1_800)).toMatchObject({
        longTaskSupported: true,
        longTasks: 1,
        maximumLongTaskMs: 100,
        totalLongTaskMs: 100,
      });
      metrics.destroy();
    } finally {
      vi.unstubAllGlobals();
    }
  });

  it("includes both rolling-window edges in cadence gap measurements", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    metrics.reset(0);
    metrics.recordFrame(5_000);
    metrics.recordFrame(6_000);
    metrics.recordSnapshot(1, 100, 128, 5_000);
    metrics.recordSnapshot(2, 100, 128, 6_000);
    const report = metrics.snapshot(10_000, 10_000);
    expect(report.maximumRafIntervalMs).toBe(5_000);
    expect(report.maximumSnapshotGapMs).toBe(5_000);
  });

  it("reports the complete observed window as the gap when no samples arrive", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    metrics.reset(1_000);
    const report = metrics.snapshot(10_000, 11_000);
    expect(report.maximumRafIntervalMs).toBe(10_000);
    expect(report.maximumSnapshotGapMs).toBe(10_000);
  });

  it("keeps a fixed-size ring during multi-hour sample counts", () => {
    const metrics = new RuntimePerformanceMetrics(false);
    metrics.reset(0);
    for (let index = 0; index < 100_000; index += 1) metrics.recordFrame(index);
    const report = metrics.snapshot(60_000, 100_000);
    expect(report.frames).toBe(32_768);
    expect(report.approximateRafFps).toBe(1_000);
  });

  it("exposes a stable read-only browser API", () => {
    expect(window.__cncwebRuntimeMetrics).toBeDefined();
    expect(Object.isFrozen(window.__cncwebRuntimeMetrics)).toBe(true);
    expect(() => window.__cncwebRuntimeMetrics!.snapshot(999)).toThrow(/between 1000 and 60000/i);
  });
});
