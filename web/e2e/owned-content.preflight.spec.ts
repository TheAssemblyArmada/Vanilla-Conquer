import { expect, test, type Page } from "@playwright/test";
import { rename, writeFile } from "node:fs/promises";
import { loadOwnedBrowserPreflightEnvironment } from "../scripts/owned-browser-preflight-env.mjs";

const owned = loadOwnedBrowserPreflightEnvironment();

interface CameraState { x: number; y: number; zoom: number }
interface RuntimeIdentity {
  core: string;
  packageRevision: string | null;
  missionId: string | null;
  buildId: string | null;
  acceptanceSession: string | null;
}

async function currentTick(page: Page): Promise<number> {
  const label = await page.locator(".runtime-status").getAttribute("aria-label");
  const match = /tick\s+([\d,]+)/i.exec(label ?? "");
  if (!match) throw new Error("Runtime status did not expose a numeric tick");
  return Number(match[1].replaceAll(",", ""));
}

async function pauseAtStableTick(page: Page): Promise<number> {
  await page.getByRole("button", { name: "Pause", exact: true }).click();
  await expect(page.getByRole("button", { name: "Resume", exact: true })).toBeEnabled();
  await page.waitForTimeout(250);
  const tick = await currentTick(page);
  await page.waitForTimeout(350);
  expect(await currentTick(page)).toBe(tick);
  return tick;
}

async function cameraState(page: Page): Promise<CameraState> {
  return page.getByLabel("Real-time strategy battlefield").evaluate((element) => ({
    x: Number((element as HTMLCanvasElement).dataset.cameraX),
    y: Number((element as HTMLCanvasElement).dataset.cameraY),
    zoom: Number((element as HTMLCanvasElement).dataset.cameraZoom),
  }));
}

function sameCamera(left: CameraState, right: CameraState): boolean {
  return Math.abs(left.x - right.x) < 0.01 && Math.abs(left.y - right.y) < 0.01 && Math.abs(left.zoom - right.zoom) < 0.001;
}

async function middleDragCamera(page: Page): Promise<void> {
  const bounds = await page.getByLabel("Real-time strategy battlefield").boundingBox();
  if (!bounds) throw new Error("Battlefield pointer bounds are unavailable");
  await page.mouse.move(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
  await page.mouse.down({ button: "middle" });
  await page.mouse.move(bounds.x + bounds.width / 2 - 80, bounds.y + bounds.height / 2 - 40, { steps: 4 });
  await page.mouse.up({ button: "middle" });
}

async function runtimeIdentity(page: Page): Promise<RuntimeIdentity | null> {
  return page.evaluate(() => {
    const api = (window as typeof window & { __cncwebRuntimeMetrics?: { snapshot(windowMs?: number): RuntimeIdentity } }).__cncwebRuntimeMetrics;
    if (!api) return null;
    const report = api.snapshot(1_000);
    return {
      core: report.core,
      packageRevision: report.packageRevision,
      missionId: report.missionId,
      buildId: report.buildId,
      acceptanceSession: report.acceptanceSession,
    };
  });
}

async function waitForExactRuntime(page: Page): Promise<RuntimeIdentity> {
  const expected: RuntimeIdentity = {
    core: "wasm",
    packageRevision: owned.packageRevision,
    missionId: owned.missionId,
    buildId: owned.buildId,
    acceptanceSession: owned.acceptanceSession,
  };
  await expect.poll(() => runtimeIdentity(page), { timeout: 3 * 60_000 }).toEqual(expected);
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled({ timeout: 3 * 60_000 });
  await expect.poll(() => page.evaluate(() => {
    const selects = document.querySelectorAll<HTMLSelectElement>(".mission-picker select");
    return { packageId: selects[0]?.value ?? null, missionId: selects[1]?.value ?? null };
  })).toEqual({ packageId: owned.packageId, missionId: owned.missionId });
  await expect.poll(() => page.evaluate(() => Boolean(document.querySelector(".error-banner, .diagnostic-error")))).toBe(false);
  return expected;
}

async function importThroughDisclosure(page: Page): Promise<void> {
  const importButton = page.getByRole("button", { name: /^Import pack \(\d+\)$/ });
  await expect(importButton).toBeEnabled({ timeout: 2 * 60_000 });
  const chooserPromise = page.waitForEvent("filechooser");
  await importButton.click();
  await (await chooserPromise).setFiles(owned.packagePath);

  const dialog = page.locator(".import-dialog");
  await expect(dialog).toHaveCount(1);
  await expect(dialog.getByText("Package archive", { exact: true })).toBeVisible();
  await expect(dialog.getByText("Private storage available", { exact: true })).toBeVisible();
  await expect(dialog.getByText("The package will be validated and stored only in this browser profile. It is never uploaded or added to the offline shell cache.", { exact: true })).toBeVisible();
  await expect(dialog.getByText("Clearing this site’s browser data removes imported content and local saves. Installing ends the current simulation, so save first if you want to return to it. The importer checks expanded size and quota again before committing anything.", { exact: true })).toBeVisible();

  const confirm = dialog.getByRole("button", { name: "Validate & install", exact: true });
  await expect(confirm).toBeEnabled({ timeout: 2 * 60_000 });
  const nextWorker = page.waitForEvent("worker", { timeout: 5 * 60_000 });
  await confirm.click();
  await expect(dialog).toHaveCount(0);
  await nextWorker;
  await waitForExactRuntime(page);
}

async function serviceWorkerControlled(page: Page): Promise<boolean> {
  return page.evaluate(async () => {
    const controlled = new URL(navigator.serviceWorker.controller?.scriptURL ?? "http://invalid/").pathname.endsWith("/sw.js");
    const cached = (await caches.keys()).some((name) => name.startsWith("theater-shell-"));
    return controlled && cached;
  });
}

async function writePrivateReport(path: string, value: unknown): Promise<void> {
  const temporary = `${path}.${process.pid}.tmp`;
  await writeFile(temporary, `${JSON.stringify(value)}\n`, { encoding: "utf8", flag: "wx", mode: 0o600 });
  await rename(temporary, path);
}

test.describe("private owned-content browser preflight", () => {
  test.skip(!owned.enabled, owned.reason);

  test("imports, exercises, saves, and resumes the exact Wasm mission online and offline", async ({ context, page }) => {
    let pageErrorCount = 0;
    page.on("pageerror", () => { pageErrorCount += 1; });

    await page.goto(owned.acceptanceURL, { waitUntil: "domcontentloaded" });
    expect(page.url()).toBe(owned.acceptanceURL);
    await expect(page.getByRole("button", { name: /^Import pack \(\d+\)$/ })).toBeEnabled({ timeout: 2 * 60_000 });
    await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled({ timeout: 2 * 60_000 });

    // Import once from a fresh origin, then import the identical revision while
    // it is active. The latter covers shared mount-lease teardown before the
    // store acquires its exclusive replacement lock.
    await importThroughDisclosure(page);
    await importThroughDisclosure(page);

    const battlefield = page.getByLabel("Real-time strategy battlefield");
    const bounds = await battlefield.boundingBox();
    if (!bounds) throw new Error("Battlefield pointer bounds are unavailable");

    await page.getByRole("button", { name: "Select units on next tap", exact: true }).click();
    await page.mouse.click(bounds.x + bounds.width * 0.45, bounds.y + bounds.height * 0.5);
    await expect.poll(() => page.evaluate(() => Boolean(document.querySelector(".command-marker.select")))).toBe(true);

    await page.getByRole("button", { name: "Issue a contextual move or attack order on next tap", exact: true }).click();
    await page.mouse.click(bounds.x + bounds.width * 0.6, bounds.y + bounds.height * 0.55);
    await expect.poll(() => page.evaluate(() => Boolean(document.querySelector(".command-marker.order")))).toBe(true);
    await expect(page.getByRole("button", { name: "Stop selected units", exact: true })).toBeEnabled();
    await page.getByRole("button", { name: "Stop selected units", exact: true }).click();

    const cameraBefore = await cameraState(page);
    await page.getByRole("button", { name: "Zoom in", exact: true }).click();
    await middleDragCamera(page);
    const savedCamera = await cameraState(page);
    expect(savedCamera.zoom > cameraBefore.zoom && (savedCamera.x !== cameraBefore.x || savedCamera.y !== cameraBefore.y)).toBe(true);

    const savedTick = await pauseAtStableTick(page);
    const save = page.getByRole("button", { name: "Save", exact: true });
    await expect(save).toBeEnabled();
    await save.click();
    await expect.poll(() => page.evaluate(() => document.querySelector(".notice-strip")?.textContent?.startsWith("Manual save saved locally") ?? false)).toBe(true);

    await page.getByRole("button", { name: /Reset camera view/ }).click();
    expect(sameCamera(await cameraState(page), savedCamera)).toBe(false);
    await page.getByRole("button", { name: "Resume", exact: true }).click();
    await expect.poll(() => currentTick(page)).toBeGreaterThan(savedTick + 10);
    const advancedTick = await pauseAtStableTick(page);

    const load = page.getByRole("button", { name: /^Load \(\d+\)$/ });
    await expect(load).toBeEnabled();
    await load.click();
    await expect.poll(() => page.evaluate(() => document.querySelector(".notice-strip")?.textContent?.startsWith("Loaded Manual save from tick ") ?? false)).toBe(true);
    await expect.poll(async () => sameCamera(await cameraState(page), savedCamera)).toBe(true);
    await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
    const loadedTick = await pauseAtStableTick(page);
    expect(loadedTick >= savedTick && loadedTick < advancedTick && loadedTick <= savedTick + 6).toBe(true);

    const onlineReload = await page.reload({ waitUntil: "domcontentloaded" });
    expect(onlineReload).not.toBeNull();
    expect(page.url()).toBe(owned.acceptanceURL);
    await waitForExactRuntime(page);
    await expect.poll(async () => sameCamera(await cameraState(page), savedCamera)).toBe(true);
    const onlineResumedTick = await pauseAtStableTick(page);
    expect(onlineResumedTick >= savedTick && onlineResumedTick < advancedTick).toBe(true);

    await expect.poll(() => serviceWorkerControlled(page), { timeout: 2 * 60_000 }).toBe(true);

    let offlineFromServiceWorker = false;
    let offlineResumedTick = 0;
    await context.setOffline(true);
    try {
      const offlineReload = await page.reload({ waitUntil: "domcontentloaded" });
      offlineFromServiceWorker = offlineReload?.fromServiceWorker() ?? false;
      expect(offlineFromServiceWorker).toBe(true);
      expect(page.url()).toBe(owned.acceptanceURL);
      await waitForExactRuntime(page);
      await expect.poll(async () => sameCamera(await cameraState(page), savedCamera)).toBe(true);
      offlineResumedTick = await pauseAtStableTick(page);
      expect(offlineResumedTick >= savedTick && offlineResumedTick < advancedTick).toBe(true);
    } finally {
      await context.setOffline(false);
    }

    expect(pageErrorCount).toBe(0);
    await expect.poll(() => page.evaluate(() => Boolean(document.querySelector(".error-banner, .diagnostic-error")))).toBe(false);

    await writePrivateReport(owned.reportPath, {
      format: "cncweb-owned-browser-preflight",
      version: 1,
      identity: {
        packageId: owned.packageId,
        packageRevision: owned.packageRevision,
        missionId: owned.missionId,
        buildId: owned.buildId,
        acceptanceSession: owned.acceptanceSession,
        core: "wasm",
      },
      checks: {
        disclosureImport: true,
        sameRevisionReplacement: true,
        exactRuntimeMetricsIdentity: true,
        commandControlsAndIngressExercised: true,
        cameraControls: true,
        manualSaveAdvanceLoad: true,
        onlineRefreshResume: true,
        serviceWorkerControl: true,
        offlineExactUrlReloadResume: offlineFromServiceWorker,
      },
      ticks: { saved: savedTick, advanced: advancedTick, loaded: loadedTick, onlineResumed: onlineResumedTick, offlineResumed: offlineResumedTick },
      notAssessed: ["audio", "command-semantics", "performance", "victory"],
    });
  });
});
