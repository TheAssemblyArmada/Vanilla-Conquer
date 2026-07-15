import { existsSync, readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

import { expect, test, type Page } from "@playwright/test";
import type { RuntimeMetricsReport } from "../src/performance/runtimeMetrics";
import { dismissBattlefieldGuide } from "./compositedPixels";

interface ClassicFreewareDescriptor {
  format: string;
  version: number;
  package: {
    id: string;
    contentSha256: string;
    source: { product: string; provider: string };
    archive: { url: string; bytes: number; sha256: string };
  };
}

interface ClassicFreewareBudgets {
  maximumColdStartupMs: number;
  measurementWindowMs: number;
  minimumObservedWindowMs: number;
  tickHzMinimum: number;
  tickHzMaximum: number;
  maximumSnapshotGapMs: number;
  minimumRafSamples: number;
  minimumSnapshotSamples: number;
  maximumMeanRafIntervalMs: number;
  maximumP95RafIntervalMs: number;
  maximumRafIntervalMs: number;
  maximumLongTasks: number;
  maximumLongTaskMs: number;
  maximumClassicBaselineUploads: number;
}

const enabled = process.env.CNCWEB_CLASSIC_FREEWARE_PERFORMANCE === "1";
// Recording/encoding a 1440×900 WebGL canvas materially perturbs RAF and long
// tasks on software-rendered CI. Keep the measured run free of test artifacts;
// the JSON metrics attachment is the acceptance evidence.
test.use({ screenshot: "off", trace: "off", video: "off" });
const distRoot = fileURLToPath(new URL("../dist/", import.meta.url));
const requiredArtifacts = [
  "index.html",
  "classic-freeware-v1.json",
  "classic-freeware-gdi-v1.cncweb",
  "engine/tiberiandawn.js",
  "engine/tiberiandawn.wasm",
];
const missingArtifacts = requiredArtifacts.filter((path) => !existsSync(`${distRoot}/${path}`));

const budgetDocument = JSON.parse(readFileSync(`${distRoot}/../performance-budgets.json`, "utf8")) as {
  format: string;
  version: number;
  classicFreeware?: ClassicFreewareBudgets;
};
if (budgetDocument.format !== "cncweb-performance-budgets" || budgetDocument.version !== 1 || !budgetDocument.classicFreeware) {
  throw new Error("Classic-freeware performance budgets are unavailable");
}
const budgets = Object.freeze(budgetDocument.classicFreeware);
if (Object.values(budgets).some((value) => !Number.isFinite(value) || value <= 0) || budgets.tickHzMinimum >= budgets.tickHzMaximum) {
  throw new Error("Classic-freeware performance budgets are malformed");
}

async function currentTick(page: Page): Promise<number> {
  const label = await page.locator(".runtime-status").getAttribute("aria-label");
  const match = /tick\s+([\d,]+)/i.exec(label ?? "");
  if (!match) return 0;
  return Number(match[1].replaceAll(",", ""));
}

async function runtimeMetrics(page: Page, windowMs: number): Promise<RuntimeMetricsReport | null> {
  return page.evaluate((requestedWindowMs) => (
    window.__cncwebRuntimeMetrics?.snapshot(requestedWindowMs) ?? null
  ), windowMs);
}

test.describe("real classic-freeware performance", () => {
  const unavailableReason = !enabled
    ? "Set CNCWEB_CLASSIC_FREEWARE_PERFORMANCE=1 after building the real freeware sidecar into web/dist"
    : `Missing generated browser artifacts: ${missingArtifacts.join(", ")}`;
  test.skip(!enabled || missingArtifacts.length > 0, unavailableReason);

  test("@performance keeps a cold-bootstrapped Wasm mission responsive", async ({ page }, testInfo) => {
    test.setTimeout(3 * 60_000);
    const descriptor = JSON.parse(readFileSync(`${distRoot}/classic-freeware-v1.json`, "utf8")) as ClassicFreewareDescriptor;
    expect(descriptor).toMatchObject({
      format: "cncweb-classic-freeware",
      version: 1,
      package: {
        id: "classic-freeware-gdi-v1",
        contentSha256: expect.stringMatching(/^[a-f0-9]{64}$/),
        source: { product: "tiberian-dawn-freeware", provider: "ea-freeware" },
        archive: {
          url: "./classic-freeware-gdi-v1.cncweb",
          bytes: expect.any(Number),
          sha256: expect.stringMatching(/^[a-f0-9]{64}$/),
        },
      },
    });
    expect(descriptor.package.archive.bytes).toBeGreaterThan(0);

    const pageErrors: string[] = [];
    const archiveRequests: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));
    page.on("request", (request) => {
      if (new URL(request.url()).pathname.endsWith(".cncweb")) archiveRequests.push(request.url());
    });

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await expect(page.locator(".mission-picker select").first()).toHaveValue(descriptor.package.id, {
      timeout: budgets.maximumColdStartupMs,
    });
    await expect(page.locator(".mission-picker select").nth(1)).toHaveValue("gdi-01-east-a");
    await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled({
      timeout: budgets.maximumColdStartupMs,
    });
    await expect.poll(() => currentTick(page), { timeout: budgets.maximumColdStartupMs }).toBeGreaterThan(1);

    const startupToPlayableMs = await page.evaluate(() => performance.now());
    expect(startupToPlayableMs).toBeLessThanOrEqual(budgets.maximumColdStartupMs);
    expect(archiveRequests).toHaveLength(1);

    // Keep the contextual-action export and selected-unit DOM path active for
    // the entire measurement window; an idle battlefield misses that hot path.
    await dismissBattlefieldGuide(page);
    const battlefield = page.getByLabel("Real-time strategy battlefield");
    const bounds = await battlefield.boundingBox();
    expect(bounds).not.toBeNull();
    await battlefield.click({ position: { x: bounds!.width * 0.725, y: bounds!.height * 0.63 } });
    await expect(page.locator(".selection-status")).toContainText("Mobile Construction Vehicle selected");

    // Let the metrics ring accumulate one uninterrupted, visible, running
    // segment after package verification and Wasm mission startup are done.
    await page.waitForTimeout(budgets.measurementWindowMs + 250);
    const metrics = await runtimeMetrics(page, budgets.measurementWindowMs);
    if (!metrics) throw new Error("Runtime performance telemetry is unavailable");

    console.log(`CNCWEB_CLASSIC_FREEWARE_PERF ${JSON.stringify({ startupToPlayableMs, metrics })}`);
    await testInfo.attach("classic-freeware-performance.json", {
      body: JSON.stringify({ budgets, descriptor, startupToPlayableMs, metrics }, null, 2),
      contentType: "application/json",
    });

    expect(pageErrors).toEqual([]);
    await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    expect(metrics).toMatchObject({
      format: "cncweb-runtime-performance",
      version: 3,
      core: "wasm",
      packageRevision: expect.stringMatching(/^[a-f0-9]{64}$/),
      missionId: "gdi-01-east-a",
      buildId: expect.stringMatching(/^[a-f0-9]{16}$/),
      running: true,
      visibilityState: "visible",
      requestedWindowMs: budgets.measurementWindowMs,
      longTaskSupported: true,
    });
    expect(metrics.observedWindowMs).toBeGreaterThanOrEqual(budgets.minimumObservedWindowMs);
    expect(metrics.snapshotTickDelta).toBeGreaterThan(0);
    expect(metrics.observedTickHz).toBeGreaterThanOrEqual(budgets.tickHzMinimum);
    expect(metrics.observedTickHz).toBeLessThanOrEqual(budgets.tickHzMaximum);
    expect(metrics.maximumSnapshotGapMs).toBeLessThanOrEqual(budgets.maximumSnapshotGapMs);
    expect(metrics.frames).toBeGreaterThanOrEqual(budgets.minimumRafSamples);
    expect(metrics.snapshotSamples).toBeGreaterThanOrEqual(budgets.minimumSnapshotSamples);
    expect(metrics.meanRafIntervalMs).toBeLessThanOrEqual(budgets.maximumMeanRafIntervalMs);
    expect(metrics.p95RafIntervalMs).toBeLessThanOrEqual(budgets.maximumP95RafIntervalMs);
    expect(metrics.maximumRafIntervalMs).toBeLessThanOrEqual(budgets.maximumRafIntervalMs);
    expect(metrics.longTasks).toBeLessThanOrEqual(budgets.maximumLongTasks);
    expect(metrics.maximumLongTaskMs).toBeLessThanOrEqual(budgets.maximumLongTaskMs);
    expect(metrics.classicBaselineUploads).toBeLessThanOrEqual(budgets.maximumClassicBaselineUploads);
    expect(metrics.classicUploadSamples).toBeGreaterThanOrEqual(metrics.snapshotSamples - 2);
    expect(metrics.classicUploadSamples).toBeLessThanOrEqual(metrics.snapshotSamples + 2);
    expect(metrics.classicUploadSamples).toBe(
      metrics.classicBaselineUploads + metrics.classicDeltaUploads + metrics.classicUnchangedUpdates,
    );
    expect(metrics.latestSnapshotDeclaredBytes).toBeGreaterThan(0);
    expect(metrics.maximumSnapshotDeclaredBytes).toBeGreaterThanOrEqual(metrics.latestSnapshotDeclaredBytes);
    expect(metrics.maximumSnapshotBufferBytes).toBeGreaterThanOrEqual(metrics.maximumSnapshotDeclaredBytes);
  });
});
