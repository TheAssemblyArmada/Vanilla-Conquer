import { expect, test, type Page } from "@playwright/test";
import { expectCompositedBattlefield } from "./compositedPixels";

const enabled = process.env.CNCWEB_CLASSIC_FREEWARE_CROSS_BROWSER === "1";
const packageId = "classic-freeware-gdi-v1";

async function currentTick(page: Page): Promise<number> {
  const label = await page.locator(".runtime-status").getAttribute("aria-label");
  const match = /tick\s+([\d,]+)/i.exec(label ?? "");
  return match ? Number(match[1].replaceAll(",", "")) : 0;
}

async function waitForRealMission(page: Page): Promise<void> {
  await expect(page.locator(".mission-picker select").first()).toHaveValue(packageId, { timeout: 3 * 60_000 });
  await expect(page.locator(".mission-picker select").nth(1)).toHaveValue("gdi-01-east-a");
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled({ timeout: 3 * 60_000 });
  await expect.poll(() => currentTick(page), { timeout: 3 * 60_000 }).toBeGreaterThan(1);
  await expect.poll(() => page.evaluate(() => window.__cncwebRuntimeMetrics?.snapshot(1_000))).toMatchObject({
    core: "wasm",
    missionId: "gdi-01-east-a",
    running: true,
  });
  await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
}

test.describe("classic-freeware desktop browser matrix", () => {
  test.skip(!enabled, "Set CNCWEB_CLASSIC_FREEWARE_CROSS_BROWSER=1 after building the integrated sidecar");
  test.setTimeout(6 * 60_000);

  test("boots the real mission when OPFS is exposed and otherwise fails over explicitly", async ({ page }, testInfo) => {
    const errors: string[] = [];
    const archiveRequests: string[] = [];
    page.on("pageerror", (error) => errors.push(error.message));
    page.on("request", (request) => {
      if (new URL(request.url()).pathname.endsWith(".cncweb")) archiveRequests.push(request.url());
    });

    await page.goto("./", { waitUntil: "domcontentloaded" });
    await expect(page.getByText("EA has not endorsed and does not support this product.", { exact: true })).toBeVisible();
    const opfsAvailable = await page.evaluate(() => Boolean(navigator.storage?.getDirectory));
    if (!opfsAvailable) {
      testInfo.annotations.push({
        type: "browser-capability",
        description: "This Playwright browser port does not expose the Origin Private File System",
      });
      await expect(page.locator(".notice-strip")).toHaveText("Private browser storage is unavailable · running explicit demo fallback");
      await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
      await expect.poll(() => page.evaluate(() => window.__cncwebRuntimeMetrics?.snapshot(1_000))).toMatchObject({
        core: "demo",
        missionId: "demo",
        running: true,
      });
      expect(archiveRequests).toHaveLength(0);
      expect(errors).toEqual([]);
      return;
    }
    await waitForRealMission(page);
    expect(archiveRequests).toHaveLength(1);
    await expectCompositedBattlefield(
      page,
      page.getByLabel("Real-time strategy battlefield"),
      testInfo,
      `classic-freeware-${testInfo.project.name}-battlefield.png`,
    );

    await page.getByRole("button", { name: "Pause", exact: true }).click();
    await expect(page.getByRole("button", { name: "Resume", exact: true })).toBeEnabled();
    const save = page.getByRole("button", { name: "Save", exact: true });
    await expect(save).toBeEnabled();
    await save.click();
    await expect(page.locator(".notice-strip")).toContainText("Manual save saved locally for this exact content revision");

    await page.reload({ waitUntil: "domcontentloaded" });
    await waitForRealMission(page);
    expect(archiveRequests).toHaveLength(1);
    expect(errors).toEqual([]);
    await testInfo.attach("classic-freeware-browser-matrix.json", {
      body: JSON.stringify({ project: testInfo.project.name, archiveRequests: archiveRequests.length }),
      contentType: "application/json",
    });
  });
});
