import { readFileSync } from "node:fs";

import { expect, test, type Page } from "@playwright/test";
import type { RuntimeMetricsReport } from "../src/performance/runtimeMetrics";

interface DemoBudgets {
  startupToPlayableMs: number;
  measurementWindowMs: number;
  tickHzMinimum: number;
  tickHzMaximum: number;
  maximumTickGapMs: number;
  minimumRafSamples: number;
  maximumMeanRafIntervalMs: number;
  maximumP95RafIntervalMs: number;
  maximumStartupLongTasks: number;
  maximumStartupLongTaskMs: number;
  maximumSteadyLongTasks: number;
  maximumSteadyLongTaskMs: number;
}

interface BrowserProbe {
  playableAt: number | null;
  playableTick: number | null;
  tickSamples: Array<{ at: number; tick: number }>;
  rafSamples: Array<{ at: number; interval: number }>;
  longTasks: Array<{ at: number; duration: number }>;
  longTaskSupported: boolean;
}

interface PerformanceMetrics {
  startupToPlayableMs: number;
  measurementWindowMs: number;
  tickStart: number;
  tickEnd: number;
  tickDelta: number;
  observedTickHz: number;
  maximumTickGapMs: number;
  rafSamples: number;
  meanRafIntervalMs: number;
  p95RafIntervalMs: number;
  approximateRafFps: number;
  longTaskSupported: boolean;
  startupLongTasks: number;
  maximumStartupLongTaskMs: number;
  steadyLongTasks: number;
  maximumSteadyLongTaskMs: number;
}

const budgetDocument = JSON.parse(
  readFileSync(new URL("../performance-budgets.json", import.meta.url), "utf8"),
) as { format: string; version: number; assetFreeDemo: DemoBudgets };

if (budgetDocument.format !== "cncweb-performance-budgets" || budgetDocument.version !== 1) {
  throw new Error("Unsupported browser performance budget document");
}
const budgets = budgetDocument.assetFreeDemo;
if (
  !budgets
  || Object.values(budgets).some((value) => !Number.isFinite(value) || value <= 0)
  || budgets.tickHzMinimum >= budgets.tickHzMaximum
) {
  throw new Error("Asset-free browser performance budgets are malformed");
}

async function installBrowserProbe(page: Page): Promise<void> {
  await page.addInitScript(() => {
    const probe: BrowserProbe = {
      playableAt: null,
      playableTick: null,
      tickSamples: [],
      rafSamples: [],
      longTasks: [],
      longTaskSupported: false,
    };
    (window as Window & { __cncwebPerformanceProbe?: BrowserProbe }).__cncwebPerformanceProbe = probe;

    if (typeof PerformanceObserver !== "undefined"
      && PerformanceObserver.supportedEntryTypes.includes("longtask")) {
      probe.longTaskSupported = true;
      const observer = new PerformanceObserver((list) => {
        for (const entry of list.getEntries()) {
          probe.longTasks.push({ at: entry.startTime, duration: entry.duration });
        }
      });
      observer.observe({ type: "longtask", buffered: true });
    }

    let previousFrame: number | null = null;
    let previousTick: number | null = null;
    const sample = (now: number) => {
      const status = document.querySelector(".runtime-status")?.textContent ?? "";
      const match = /tick\s+([\d,]+)/i.exec(status);
      const tick = match ? Number(match[1].replaceAll(",", "")) : null;
      if (tick !== null && Number.isFinite(tick) && tick !== previousTick) {
        previousTick = tick;
        probe.tickSamples.push({ at: now, tick });
      }

      if (probe.playableAt === null) {
        const notice = document.querySelector(".notice-strip")?.textContent ?? "";
        const pause = [...document.querySelectorAll("button")].find(
          (button) => button.textContent?.trim() === "Pause" && !(button as HTMLButtonElement).disabled,
        );
        if (notice.includes("Demo simulation · no compatible campaign content loaded") && pause && tick !== null && tick >= 2) {
          probe.playableAt = now;
          probe.playableTick = tick;
          previousFrame = now;
        }
      } else if (previousFrame !== null) {
        probe.rafSamples.push({ at: now, interval: now - previousFrame });
        previousFrame = now;
      }
      requestAnimationFrame(sample);
    };
    requestAnimationFrame(sample);
  });
}

async function collectMetrics(page: Page): Promise<PerformanceMetrics> {
  return page.evaluate(({ measurementWindowMs }) => {
    const probe = (window as Window & { __cncwebPerformanceProbe?: BrowserProbe }).__cncwebPerformanceProbe;
    if (!probe || probe.playableAt === null || probe.playableTick === null) {
      throw new Error("Browser performance probe never observed a playable demo");
    }
    const windowStart = probe.playableAt;
    const windowEnd = windowStart + measurementWindowMs;
    const ticks = probe.tickSamples.filter((sample) => sample.at >= windowStart && sample.at <= windowEnd);
    const frames = probe.rafSamples.filter((sample) => sample.at <= windowEnd);
    if (ticks.length < 2 || frames.length < 2) throw new Error("Performance probe collected too few samples");

    const tickFirst = ticks[0];
    const tickLast = ticks[ticks.length - 1];
    const tickElapsedSeconds = (tickLast.at - tickFirst.at) / 1000;
    const tickBoundaries = [windowStart, ...ticks.map((sample) => sample.at), windowEnd];
    const tickGaps = tickBoundaries.slice(1).map((at, index) => at - tickBoundaries[index]);
    const frameIntervals = frames.map((sample) => sample.interval);
    const meanFrame = frameIntervals.reduce((sum, value) => sum + value, 0) / frameIntervals.length;
    const sortedFrames = [...frameIntervals].sort((left, right) => left - right);
    const p95Frame = sortedFrames[Math.min(sortedFrames.length - 1, Math.ceil(sortedFrames.length * 0.95) - 1)];
    const startupTasks = probe.longTasks.filter((task) => task.at < windowStart);
    const steadyTasks = probe.longTasks.filter((task) => task.at >= windowStart && task.at < windowEnd);
    const longest = (tasks: Array<{ duration: number }>) => tasks.reduce(
      (maximum, task) => Math.max(maximum, task.duration),
      0,
    );

    return {
      startupToPlayableMs: Math.round(windowStart * 100) / 100,
      measurementWindowMs,
      tickStart: tickFirst.tick,
      tickEnd: tickLast.tick,
      tickDelta: tickLast.tick - tickFirst.tick,
      observedTickHz: Math.round(((tickLast.tick - tickFirst.tick) / tickElapsedSeconds) * 100) / 100,
      maximumTickGapMs: Math.round(Math.max(...tickGaps) * 100) / 100,
      rafSamples: frames.length,
      meanRafIntervalMs: Math.round(meanFrame * 100) / 100,
      p95RafIntervalMs: Math.round(p95Frame * 100) / 100,
      approximateRafFps: Math.round((1000 / meanFrame) * 100) / 100,
      longTaskSupported: probe.longTaskSupported,
      startupLongTasks: startupTasks.length,
      maximumStartupLongTaskMs: Math.round(longest(startupTasks) * 100) / 100,
      steadyLongTasks: steadyTasks.length,
      maximumSteadyLongTaskMs: Math.round(longest(steadyTasks) * 100) / 100,
    };
  }, { measurementWindowMs: budgets.measurementWindowMs });
}

test("@performance keeps the asset-free demo responsive at its 15 Hz contract", async ({ page }, testInfo) => {
  test.setTimeout(20_000);
  const pageErrors: string[] = [];
  page.on("pageerror", (error) => pageErrors.push(error.message));
  await installBrowserProbe(page);

  await page.goto("/", { waitUntil: "domcontentloaded" });
  await expect.poll(() => page.evaluate(() => (
    (window as Window & { __cncwebPerformanceProbe?: BrowserProbe })
      .__cncwebPerformanceProbe?.playableAt ?? null
  ))).not.toBeNull();
  await page.waitForTimeout(budgets.measurementWindowMs + 250);

  const metrics = await collectMetrics(page);
  const runtimeMetrics = await page.evaluate((windowMs) => {
    if (!window.__cncwebRuntimeMetrics) throw new Error("Runtime performance telemetry is unavailable");
    return window.__cncwebRuntimeMetrics.snapshot(windowMs);
  }, budgets.measurementWindowMs) as RuntimeMetricsReport;
  console.log(`CNCWEB_PERF_METRICS ${JSON.stringify(metrics)}`);
  console.log(`CNCWEB_RUNTIME_METRICS ${JSON.stringify(runtimeMetrics)}`);
  await testInfo.attach("asset-free-demo-performance.json", {
    body: JSON.stringify({ budgets, metrics, runtimeMetrics }, null, 2),
    contentType: "application/json",
  });

  expect(pageErrors).toEqual([]);
  expect(metrics.startupToPlayableMs).toBeLessThanOrEqual(budgets.startupToPlayableMs);
  expect(metrics.tickDelta).toBeGreaterThan(0);
  expect(metrics.observedTickHz).toBeGreaterThanOrEqual(budgets.tickHzMinimum);
  expect(metrics.observedTickHz).toBeLessThanOrEqual(budgets.tickHzMaximum);
  expect(metrics.maximumTickGapMs).toBeLessThanOrEqual(budgets.maximumTickGapMs);
  expect(metrics.rafSamples).toBeGreaterThanOrEqual(budgets.minimumRafSamples);
  expect(metrics.meanRafIntervalMs).toBeLessThanOrEqual(budgets.maximumMeanRafIntervalMs);
  expect(metrics.p95RafIntervalMs).toBeLessThanOrEqual(budgets.maximumP95RafIntervalMs);
  expect(metrics.longTaskSupported).toBe(true);
  expect(metrics.startupLongTasks).toBeLessThanOrEqual(budgets.maximumStartupLongTasks);
  expect(metrics.maximumStartupLongTaskMs).toBeLessThanOrEqual(budgets.maximumStartupLongTaskMs);
  expect(metrics.steadyLongTasks).toBeLessThanOrEqual(budgets.maximumSteadyLongTasks);
  expect(metrics.maximumSteadyLongTaskMs).toBeLessThanOrEqual(budgets.maximumSteadyLongTaskMs);

  // Exercise the same bounded API used by the manual owned-content C09 run;
  // the independent page probe above remains the source of demo CI budgets.
  expect(runtimeMetrics).toMatchObject({
    format: "cncweb-runtime-performance",
    version: 3,
    core: "demo",
    packageRevision: null,
    missionId: "demo",
    running: true,
    visibilityState: "visible",
  });
  expect(runtimeMetrics.observedTickHz).toBeGreaterThanOrEqual(budgets.tickHzMinimum);
  expect(runtimeMetrics.observedTickHz).toBeLessThanOrEqual(budgets.tickHzMaximum);
  expect(runtimeMetrics.maximumSnapshotGapMs).toBeLessThanOrEqual(budgets.maximumTickGapMs);
  expect(runtimeMetrics.p95RafIntervalMs).toBeLessThanOrEqual(budgets.maximumP95RafIntervalMs);
  expect(runtimeMetrics.longTaskSupported).toBe(true);
  expect(runtimeMetrics.longTasks).toBeLessThanOrEqual(budgets.maximumSteadyLongTasks);
  expect(runtimeMetrics.maximumLongTaskMs).toBeLessThanOrEqual(budgets.maximumSteadyLongTaskMs);

});
