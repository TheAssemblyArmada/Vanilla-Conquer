import { expect, test, type Page } from "@playwright/test";
import { dismissBattlefieldGuide, expectCompositedBattlefield } from "./compositedPixels";

const enabled = process.env.CNCWEB_CLASSIC_FREEWARE_PREFLIGHT === "1";
const packageId = "classic-freeware-gdi-v1";

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

async function serviceWorkerControlled(page: Page): Promise<boolean> {
  return page.evaluate(async () => {
    const registration = await navigator.serviceWorker.ready;
    return Boolean(registration.active && navigator.serviceWorker.controller);
  });
}

async function waitForFreewareMission(page: Page, missionId = "gdi-01-east-a", title = "GDI Mission 1"): Promise<void> {
  await expect(page.locator(".mission-picker select").first()).toHaveValue(packageId, { timeout: 3 * 60_000 });
  await expect(page.locator(".mission-picker select").nth(1)).toHaveValue(missionId);
  await expect(page.getByRole("heading", { name: title, exact: true })).toBeVisible();
  await expect(page.locator(".minimap span")).toHaveText(missionId.toUpperCase());
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled({ timeout: 3 * 60_000 });
  await expect.poll(() => currentTick(page), { timeout: 3 * 60_000 }).toBeGreaterThan(1);
  await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
}

async function selectMissionThreeMcv(page: Page): Promise<void> {
  const battlefield = page.getByLabel("Real-time strategy battlefield");
  const bounds = await battlefield.boundingBox();
  expect(bounds).not.toBeNull();

  // Classic aspect-fit letterboxing shifts this lower-edge unit slightly
  // between the desktop project viewport and a taller local viewport.
  for (const x of [0.25, 0.23, 0.24, 0.26, 0.22]) {
    await battlefield.click({ position: { x: bounds!.width * x, y: bounds!.height * 0.96 }, force: true });
    await page.waitForTimeout(250);
    const selectionLabels = await page.locator(".selection-status").allTextContents();
    if (selectionLabels.some((label) => label.includes("Mobile Construction Vehicle selected"))
      && await page.getByRole("button", { name: "Deploy selected unit", exact: true }).count() === 1) return;
  }

  throw new Error("Mission 3's visible starting MCV could not be selected through the battlefield");
}

interface ExpectedMissionObjective {
  label: string;
  description: string;
  progress: string | RegExp;
}

async function expectMissionObjectives(page: Page, expected: readonly ExpectedMissionObjective[]): Promise<void> {
  const objectives = page.locator(".mission-objectives");
  await expect(objectives).toBeVisible();
  await expect(objectives.getByRole("heading", { name: "Operation orders", exact: true })).toBeVisible();
  await expect(objectives.getByText("In progress", { exact: true })).toBeVisible();
  const items = objectives.getByRole("listitem");
  await expect(items).toHaveCount(expected.length);
  for (let index = 0; index < expected.length; index += 1) {
    const item = items.nth(index);
    await expect(item.getByText(expected[index].label, { exact: true })).toBeVisible();
    await expect(item.getByText(expected[index].description, { exact: true })).toBeVisible();
    await expect(item.locator("small")).toHaveText(expected[index].progress);
  }
}

async function selectMissionFourWestAApc(page: Page): Promise<void> {
  const battlefield = page.getByLabel("Real-time strategy battlefield");
  const bounds = await battlefield.boundingBox();
  expect(bounds).not.toBeNull();

  // The recovery convoy starts at the southeast edge. Try a few points across
  // the two rendered APC footprints so small aspect-fit shifts stay harmless.
  const candidates = [
    { x: 0.86, y: 0.90 },
    { x: 0.82, y: 0.92 },
    { x: 0.90, y: 0.90 },
    { x: 0.85, y: 0.92 },
    { x: 0.85, y: 0.94 },
    { x: 0.87, y: 0.94 },
  ];
  for (const candidate of candidates) {
    await battlefield.click({
      position: { x: bounds!.width * candidate.x, y: bounds!.height * candidate.y },
      force: true,
    });
    await page.waitForTimeout(250);
    const selectionLabels = await page.locator(".selection-status").allTextContents();
    if (selectionLabels.some((label) => label.includes("APC selected"))) return;
  }

  throw new Error("Mission 4 West A's visible starting APC could not be selected through the battlefield");
}

async function selectMissionFiveEastAE2(page: Page): Promise<void> {
  const battlefield = page.getByLabel("Real-time strategy battlefield");
  const bounds = await battlefield.boundingBox();
  expect(bounds).not.toBeNull();

  // The damaged-base relief group opens in the southern tactical view. Its
  // infantry can shift slightly as the native formation settles, so probe a
  // compact set of visible points around the authored E2 formation.
  const candidates = [0.82, 0.85, 0.88].flatMap((y) => (
    [0.12, 0.13, 0.14, 0.15, 0.16, 0.17, 0.18, 0.19, 0.2, 0.21, 0.22]
      .map((x) => ({ x, y }))
  ));
  for (const candidate of candidates) {
    await battlefield.click({
      position: { x: bounds!.width * candidate.x, y: bounds!.height * candidate.y },
      force: true,
    });
    // The real classic surface can run at only a few rendered frames per
    // second under software WebGL; wait for selection telemetry before the
    // next probe can replace a valid click.
    await page.waitForTimeout(750);
    const selectionLabels = await page.locator(".selection-status").allTextContents();
    if (selectionLabels.some((label) => label.includes("E2 selected"))) return;
  }

  throw new Error("Mission 5 East A's visible starting E2 could not be selected through the battlefield");
}

async function missionFiveVisibleMoveTarget(page: Page): Promise<{ x: number; y: number }> {
  const battlefield = page.getByLabel("Real-time strategy battlefield");
  const bounds = await battlefield.boundingBox();
  expect(bounds).not.toBeNull();
  const candidates = [
    { x: 0.13, y: 0.78 },
    { x: 0.24, y: 0.8 },
    { x: 0.13, y: 0.92 },
    { x: 0.25, y: 0.92 },
  ];
  for (const candidate of candidates) {
    const point = { x: bounds!.width * candidate.x, y: bounds!.height * candidate.y };
    await battlefield.hover({ position: point, force: true });
    await page.waitForTimeout(150);
    if (await battlefield.getAttribute("data-contextual-action") === "1"
      && await page.locator(".contextual-order-status").textContent() === "Right-click · Move") return point;
  }
  throw new Error("Mission 5 East A exposed no visible move target near its starting relief group");
}

test.describe("real classic-freeware bootstrap", () => {
  test.skip(!enabled, "Set CNCWEB_CLASSIC_FREEWARE_PREFLIGHT=1 after building the real sidecar into web/dist");
  test.setTimeout(8 * 60_000);

  test("downloads once, launches Wasm, and reloads from OPFS online and offline", async ({ context, page }) => {
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));
    const archiveRequests: string[] = [];
    page.on("request", (request) => {
      if (new URL(request.url()).pathname.endsWith(".cncweb")) archiveRequests.push(request.url());
    });

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await expect(page.getByText("EA has not endorsed and does not support this product.", { exact: true })).toBeVisible();
    await waitForFreewareMission(page);
    expect(archiveRequests).toHaveLength(1);

    const cachedContentUrls = await page.evaluate(async () => {
      const urls: string[] = [];
      for (const name of await caches.keys()) {
        const cache = await caches.open(name);
        urls.push(...(await cache.keys()).map((request) => request.url));
      }
      return urls.filter((url) => /classic-freeware|\.cncweb(?:$|[?#])/i.test(url));
    });
    expect(cachedContentUrls).toEqual([]);

    await page.locator(".mission-picker select").nth(1).selectOption("gdi-08-east-a");
    await page.getByRole("button", { name: "Start new mission", exact: true }).click();
    await waitForFreewareMission(page, "gdi-08-east-a", "GDI Mission 8 (East A)");

    await page.reload({ waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page, "gdi-08-east-a", "GDI Mission 8 (East A)");
    expect(archiveRequests).toHaveLength(1);

    await expect.poll(() => serviceWorkerControlled(page), { timeout: 2 * 60_000 }).toBe(true);

    await context.setOffline(true);
    try {
      const response = await page.reload({ waitUntil: "domcontentloaded" });
      expect(response?.fromServiceWorker()).toBe(true);
      await waitForFreewareMission(page, "gdi-08-east-a", "GDI Mission 8 (East A)");
      expect(archiveRequests).toHaveLength(1);
    } finally {
      await context.setOffline(false);
    }

    expect(pageErrors).toEqual([]);
  });

  test("shows exact Mission 1 rules and deploys the MCV through visible controls", async ({ page }) => {
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page);

    const objectives = page.locator(".mission-objectives");
    await expect(objectives).toBeVisible();
    await expect(objectives.getByText("In progress", { exact: true })).toBeVisible();
    await expect(objectives.getByText("Eliminate the Nod force", { exact: true })).toBeVisible();
    await expect(objectives.getByText("Keep a GDI ground force operational", { exact: true })).toBeVisible();
    await expect(objectives).toContainText(/\d+ units and \d+ structures destroyed/);
    await expect(objectives).toContainText(/\d+ losses recorded/);

    await dismissBattlefieldGuide(page);
    const battlefield = page.getByLabel("Real-time strategy battlefield");
    const bounds = await battlefield.boundingBox();
    expect(bounds).not.toBeNull();
    await battlefield.click({ position: { x: bounds!.width * 0.725, y: bounds!.height * 0.63 } });
    await expect(page.locator(".selection-status")).toContainText("Mobile Construction Vehicle selected", { timeout: 15_000 });

    const deploy = page.getByRole("button", { name: "Deploy selected unit", exact: true });
    await expect(deploy).toBeEnabled();
    await deploy.click();
    await expect(page.locator(".notice-strip")).toHaveText("Deploy order sent");
    await expect(page.getByLabel("Construction and production")).toContainText("Power Plant", { timeout: 30_000 });
    await expect(page.getByLabel("Construction and production")).not.toContainText("NUKE");
    await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    expect(pageErrors).toEqual([]);
  });

  test("shows exact Mission 2 rules and trains a unit through visible production controls", async ({ page }) => {
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page);
    await page.locator(".mission-picker select").nth(1).selectOption("gdi-02-east-a");
    await page.getByRole("button", { name: "Start new mission", exact: true }).click();
    await waitForFreewareMission(page, "gdi-02-east-a", "GDI Mission 2 (East A)");

    const objectives = page.locator(".mission-objectives");
    await expect(objectives).toBeVisible();
    await expect(objectives.getByText("Eliminate the Nod occupation", { exact: true })).toBeVisible();
    await expect(objectives.getByText("Keep a GDI force operational", { exact: true })).toBeVisible();
    await expect(objectives).toContainText(/\d+ units and \d+ structures destroyed/);
    await expect(objectives).toContainText(/\d+ losses recorded/);

    const production = page.getByLabel("Construction and production");
    await expect(production).toBeVisible();
    await expect(production).toContainText("Minigunner");
    const build = page.getByRole("button", { name: "Build Minigunner", exact: true });
    await expect(build).toBeEnabled();
    const creditsBefore = Number((await production.locator(".production-heading strong").textContent() ?? "0").replace(/\D/g, ""));
    await build.click();
    await expect(page.locator(".notice-strip")).toHaveText("Building Minigunner");

    const pause = page.getByRole("button", { name: "Pause Minigunner", exact: true });
    await expect(pause).toBeEnabled({ timeout: 10_000 });
    await pause.click();
    await expect(page.locator(".notice-strip")).toHaveText("Pausing Minigunner");
    const resume = page.getByRole("button", { name: "Resume Minigunner", exact: true });
    await expect(resume).toBeEnabled({ timeout: 10_000 });
    await resume.click();
    await expect(page.locator(".notice-strip")).toHaveText("Resuming Minigunner");
    await expect(build).toBeEnabled({ timeout: 45_000 });
    await expect.poll(async () => Number((await production.locator(".production-heading strong").textContent() ?? "0").replace(/\D/g, ""))).toBeLessThan(creditsBefore);

    await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    expect(pageErrors).toEqual([]);
  });

  test("shows exact Mission 3 rules and establishes a base through visible controls", async ({ page }) => {
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page);
    await page.locator(".mission-picker select").nth(1).selectOption("gdi-03-east-a");
    await page.getByRole("button", { name: "Start new mission", exact: true }).click();
    await waitForFreewareMission(page, "gdi-03-east-a", "GDI Mission 3 (East A)");

    const objectives = page.locator(".mission-objectives");
    await expect(objectives).toBeVisible();
    await expect(objectives.getByText("Eliminate the Nod force", { exact: true })).toBeVisible();
    await expect(objectives.getByText("Keep GDI operational", { exact: true })).toBeVisible();
    await expect(objectives.getByText("Destroy every counted Nod unit and structure in the operation area. Nod production, rebuilt structures, and attack teams can add targets.", { exact: true })).toBeVisible();
    await expect(objectives.getByText("The operation fails if no counted GDI structure, infantry, or ground vehicle remains.", { exact: true })).toBeVisible();

    await dismissBattlefieldGuide(page);
    const battlefield = page.getByLabel("Real-time strategy battlefield");
    await selectMissionThreeMcv(page);
    await expect(page.locator(".selection-status")).toContainText("Mobile Construction Vehicle selected", { timeout: 15_000 });

    const deploy = page.getByRole("button", { name: "Deploy selected unit", exact: true });
    await expect(deploy).toBeEnabled();
    await deploy.click();
    await expect(page.locator(".notice-strip")).toHaveText("Deploy order sent");

    const production = page.getByLabel("Construction and production");
    await expect(production).toContainText("Power Plant", { timeout: 30_000 });
    const build = page.getByRole("button", { name: "Build Power Plant", exact: true });
    await expect(build).toBeEnabled();
    const creditsBefore = Number((await production.locator(".production-heading strong").textContent() ?? "0").replace(/\D/g, ""));
    await build.click();
    await expect(page.locator(".notice-strip")).toHaveText("Building Power Plant");

    const place = page.getByRole("button", { name: "Place Power Plant", exact: true });
    await expect(place).toBeEnabled({ timeout: 90_000 });
    await expect.poll(async () => Number((await production.locator(".production-heading strong").textContent() ?? "0").replace(/\D/g, ""))).toBeLessThan(creditsBefore);
    await place.click();
    await expect(page.locator(".notice-strip")).toContainText(/(?:Placing|Placement restored for) Power Plant/);
    await expect(page.locator(".battlefield-tool-status.placement")).toContainText("Place Power Plant", { timeout: 15_000 });

    const notice = page.locator(".notice-strip");
    const quickPlace = page.getByRole("button", { name: "Quick-place Power Plant at a legal site", exact: true });
    let structurePlaced = false;
    for (let attempt = 0; attempt < 5 && !structurePlaced; attempt += 1) {
      await expect(quickPlace).toBeEnabled({ timeout: 15_000 });
      await quickPlace.click();
      await expect.poll(
        () => notice.textContent(),
        { timeout: 30_000 },
      ).toMatch(/^(?:Structure placed|That footprint is blocked · choose another green location)$/);
      structurePlaced = await quickPlace.count() === 0;
    }
    expect(structurePlaced, "Quick place exhausted five currently legal Power Plant sites").toBe(true);

    await page.getByRole("button", { name: "Expand mission panel", exact: true }).click();
    await expect(page.getByLabel("Construction and production")).toContainText("Barracks", { timeout: 30_000 });
    await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    expect(pageErrors).toEqual([]);
  });

  test("shows all Mission 4 variant rules and routes the West A recovery APC through visible controls", async ({ page }) => {
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page);

    const variants = [
      {
        id: "gdi-04-west-a",
        title: "GDI Mission 4 (West A)",
        objectives: [
          {
            label: "Recover the GDI crate",
            description: "Reach the marked recovery area. The operation completes when a GDI unit enters the crate cell; destroying Nod is not required.",
            progress: "Crate recovery objective active",
          },
          {
            label: "Keep the recovery force operational",
            description: "The operation fails if every counted GDI infantry unit and ground vehicle is destroyed. A transport aircraft alone does not prevent defeat.",
            progress: "Recovery force condition active",
          },
        ],
      },
      {
        id: "gdi-04-west-b",
        title: "GDI Mission 4 (West B)",
        objectives: [
          {
            label: "Eliminate the Nod force",
            description: "Destroy every counted Nod unit in the operation area. Triggered Nod assault groups become additional targets.",
            progress: "Nod elimination objective active",
          },
          {
            label: "Preserve the protected village",
            description: "The operation fails if all four protected village structures are destroyed.",
            progress: "Village protection condition active",
          },
          {
            label: "Keep GDI operational",
            description: "The operation fails if every counted GDI infantry unit and ground vehicle is destroyed.",
            progress: "GDI survival condition active",
          },
        ],
      },
      {
        id: "gdi-04-east-a",
        title: "GDI Mission 4 (East A)",
        objectives: [
          {
            label: "Recover the GDI crate",
            description: "Reach the marked recovery area. The operation completes when a GDI unit enters the crate cell; destroying Nod is not required.",
            progress: "Crate recovery objective active",
          },
          {
            label: "Keep the recovery force operational",
            description: "The operation fails if every counted GDI infantry unit and ground vehicle is destroyed. A transport aircraft alone does not prevent defeat.",
            progress: "Recovery force condition active",
          },
        ],
      },
    ] as const;

    // The legacy nested label includes every option in its computed name, so
    // target the same stable mission select used by the bootstrap helpers.
    const missionPicker = page.locator(".mission-picker select").nth(1);
    for (const variant of variants) {
      await missionPicker.selectOption(variant.id);
      await page.getByRole("button", { name: "Start new mission", exact: true }).click();
      await waitForFreewareMission(page, variant.id, variant.title);
      await expectMissionObjectives(page, variant.objectives);

      if (variant.id !== "gdi-04-west-a") continue;
      await dismissBattlefieldGuide(page);
      const battlefield = page.getByLabel("Real-time strategy battlefield");
      await selectMissionFourWestAApc(page);
      await expect(page.locator(".selection-status")).toContainText("APC selected", { timeout: 15_000 });

      // Clamp the host camera at the northwest map edge using the documented
      // pan controls. This exposes the authored recovery crate at cell 846.
      for (let step = 0; step < 24; step += 1) {
        await page.keyboard.press("Shift+KeyA");
        await page.keyboard.press("Shift+KeyW");
      }
      const bounds = await battlefield.boundingBox();
      expect(bounds).not.toBeNull();
      const recoveryCrate = { x: bounds!.width * 0.72, y: bounds!.height * 0.30 };
      await battlefield.hover({ position: recoveryCrate, force: true });
      await expect(page.locator(".contextual-order-status")).toHaveText("Right-click · Explore");
      await expect(battlefield).toHaveAttribute("data-contextual-action", "explore");
      await battlefield.click({ position: recoveryCrate, button: "right", force: true });
      await expect(page.locator(".notice-strip")).toHaveText("Explore order issued");
    }

    await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    expect(pageErrors).toEqual([]);
  });

  test("shows all Mission 5 variant rules and issues an East A relief-force move order", async ({ page }) => {
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page);

    const objectives = [
      {
        label: "Eliminate the Nod force",
        description: "Destroy every counted Nod unit and structure in the operation area. Nod production, rebuilt structures, patrols, and timed attack teams can add targets.",
        progress:
          /^\d+ (?:unit|units) and \d+ (?:structure|structures) destroyed$/,
      },
      {
        label: "Relieve the separated GDI base",
        description: "Move GDI units through both authored relief zones. Until each zone is crossed, losing the last member of its protected starting group—field force or base structures—immediately fails the operation.",
        progress: "Base-relief conditions active",
      },
      {
        label: "Keep GDI operational",
        description: "The operation also fails if every counted GDI unit and structure is destroyed.",
        progress: "GDI survival condition active",
      },
    ] as const;
    const variants = [
      { id: "gdi-05-east-a", title: "GDI Mission 5 (East A)" },
      { id: "gdi-05-west-a", title: "GDI Mission 5 (West A)" },
      { id: "gdi-05-west-b", title: "GDI Mission 5 (West B)" },
    ] as const;
    const missionPicker = page.locator(".mission-picker select").nth(1);

    for (const variant of variants) {
      await missionPicker.selectOption(variant.id);
      await page.getByRole("button", { name: "Start new mission", exact: true }).click();
      await waitForFreewareMission(page, variant.id, variant.title);
      await expectMissionObjectives(page, objectives);

      if (variant.id === "gdi-05-east-a") {
        await dismissBattlefieldGuide(page);
        const battlefield = page.getByLabel("Real-time strategy battlefield");
        await selectMissionFiveEastAE2(page);
        await expect(page.locator(".selection-status")).toContainText("E2 selected", { timeout: 15_000 });

        const moveTarget = await missionFiveVisibleMoveTarget(page);
        await expect(page.locator(".contextual-order-status")).toHaveText("Right-click · Move");
        await expect(battlefield).toHaveAttribute("data-contextual-action", "1");
        await battlefield.click({ position: moveTarget, button: "right", force: true });
        await expect(page.locator(".notice-strip")).toHaveText("Move order issued");
      }

      await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    }

    expect(pageErrors).toEqual([]);
  });

  test("presents fog-safe contextual actions through the rendered battlefield", async ({ page }) => {
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page);
    await dismissBattlefieldGuide(page);

    const battlefield = page.getByLabel("Real-time strategy battlefield");
    const bounds = await battlefield.boundingBox();
    expect(bounds).not.toBeNull();
    const mcv = { x: bounds!.width * 0.725, y: bounds!.height * 0.63 };
    await battlefield.hover({ position: mcv });
    await expect(page.locator(".contextual-order-status")).toHaveText("Select");
    await expect(battlefield).toHaveAttribute("data-contextual-action", String(8));
    await expect(battlefield).toHaveCSS("cursor", "pointer");

    await battlefield.click({ position: mcv });
    await expect(page.locator(".selection-status")).toContainText("Mobile Construction Vehicle selected", { timeout: 15_000 });

    const hiddenCandidates = [
      { x: bounds!.width * 0.22, y: bounds!.height * 0.28 },
      { x: bounds!.width * 0.32, y: bounds!.height * 0.42 },
      { x: bounds!.width * 0.44, y: bounds!.height * 0.22 },
    ];
    let hidden: { x: number; y: number } | undefined;
    for (const point of hiddenCandidates) {
      await battlefield.hover({ position: point });
      if (await page.locator(".contextual-order-status").textContent() === "Right-click · Explore") {
        hidden = point;
        break;
      }
    }
    expect(hidden, "Mission 1 should expose an unrevealed cell away from viewport chrome").toBeDefined();
    await expect(page.locator(".contextual-order-status")).toHaveText("Right-click · Explore");
    await expect(battlefield).toHaveAttribute("data-contextual-action", "explore");
    await expect(battlefield).toHaveCSS("cursor", "move");

    await battlefield.click({ position: hidden!, button: "right" });
    await expect(page.locator(".notice-strip")).toHaveText("Explore order issued");
    await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    expect(pageErrors).toEqual([]);
  });

  test("saves, loads, and resumes the exact Wasm mission online and offline", async ({ context, page }) => {
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));
    const archiveRequests: string[] = [];
    page.on("request", (request) => {
      if (new URL(request.url()).pathname.endsWith(".cncweb")) archiveRequests.push(request.url());
    });

    await page.goto("/", { waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page);
    expect(archiveRequests).toHaveLength(1);

    const savedTick = await pauseAtStableTick(page);
    const save = page.getByRole("button", { name: "Save", exact: true });
    await expect(save).toBeEnabled();
    await save.click();
    await expect(page.locator(".notice-strip")).toContainText("Manual save saved locally for this exact content revision");

    await page.getByRole("button", { name: "Resume", exact: true }).click();
    // Make the pre-load timeline visibly distinct. The loaded mission resumes
    // immediately, so a ten-tick gap can be consumed by browser/UI scheduling
    // before the next visible Pause click on a busy release runner.
    await expect.poll(() => currentTick(page)).toBeGreaterThan(savedTick + 60);
    const advancedTick = await pauseAtStableTick(page);

    const load = page.getByRole("button", { name: "Load (1)", exact: true });
    await expect(load).toBeEnabled();
    await load.click();
    await expect(page.locator(".notice-strip")).toContainText(`Loaded Manual save from tick ${savedTick.toLocaleString()}`);
    await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
    const loadedTick = await pauseAtStableTick(page);
    expect(loadedTick).toBeGreaterThanOrEqual(savedTick);
    expect(loadedTick).toBeLessThan(advancedTick);
    // The load notice above identifies the exact serialized tick. The next
    // observed paused tick also includes variable browser/UI scheduling time,
    // so compare it with the deliberately separated pre-load timeline rather
    // than imposing a fixed post-load drift allowance.

    const onlineReload = await page.reload({ waitUntil: "domcontentloaded" });
    expect(onlineReload).not.toBeNull();
    await waitForFreewareMission(page);
    const onlineResumedTick = await pauseAtStableTick(page);
    expect(onlineResumedTick).toBeGreaterThanOrEqual(savedTick);
    expect(onlineResumedTick).toBeLessThan(advancedTick);
    expect(archiveRequests).toHaveLength(1);

    await expect.poll(() => serviceWorkerControlled(page), { timeout: 2 * 60_000 }).toBe(true);
    await context.setOffline(true);
    try {
      const offlineReload = await page.reload({ waitUntil: "domcontentloaded" });
      expect(offlineReload?.fromServiceWorker()).toBe(true);
      await waitForFreewareMission(page);
      const offlineResumedTick = await pauseAtStableTick(page);
      expect(offlineResumedTick).toBeGreaterThanOrEqual(savedTick);
      expect(offlineResumedTick).toBeLessThan(advancedTick);
      expect(archiveRequests).toHaveLength(1);
    } finally {
      await context.setOffline(false);
    }

    expect(pageErrors).toEqual([]);
  });

  test("emits a genuine terminal result and preserves the canonical continuation offline", async ({ context, page }) => {
    const acceptanceSession = "ac".repeat(16);
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));
    const archiveRequests: string[] = [];
    page.on("request", (request) => {
      if (new URL(request.url()).pathname.endsWith(".cncweb")) archiveRequests.push(request.url());
    });

    await page.goto(`/?acceptance=${acceptanceSession}`, { waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page);
    expect(archiveRequests).toHaveLength(1);
    await pauseAtStableTick(page);
    await page.evaluate(async () => {
      if (!window.__cncwebAcceptance) throw new Error("Loopback release-acceptance API is unavailable");
      await window.__cncwebAcceptance.forceVictory();
    });
    await page.getByRole("button", { name: "Resume", exact: true }).click();

    const result = page.locator(".game-over");
    await expect(result).toBeVisible({ timeout: 2 * 60_000 });
    await expect(result.getByRole("heading", { name: "Victory", exact: true })).toBeVisible();
    await expect(result.getByText("The next operation is ready.", { exact: true })).toBeVisible();
    await expect(page.locator(".mission-objectives-state")).toHaveText("Complete");
    const continuation = result.getByRole("button", { name: "GDI Mission 2 (East A)", exact: true });
    await expect(continuation).toBeEnabled();
    await expect(result.locator(".campaign-choices button")).toHaveCount(1);

    const pendingSession = await page.evaluate(() => JSON.parse(localStorage.getItem("theater.runtime-session.v1") ?? "null") as {
      missionId?: string;
      runId?: string;
      pendingVictory?: { gameOver?: { tick?: number }; outcome?: { tick?: number; scenarioRoot?: string; scenario?: number; house?: number } };
    });
    expect(pendingSession).toMatchObject({
      missionId: "gdi-01-east-a",
      runId: expect.stringMatching(/^campaign-/),
      pendingVictory: {
        gameOver: { tick: expect.any(Number) },
        outcome: { tick: expect.any(Number), scenarioRoot: "SCG01EA", scenario: 1, house: 0 },
      },
    });
    expect(pendingSession.pendingVictory?.gameOver?.tick).toBe(pendingSession.pendingVictory?.outcome?.tick);

    await page.reload({ waitUntil: "domcontentloaded" });
    await expect(page.locator(".mission-picker select").first()).toHaveValue(packageId, { timeout: 3 * 60_000 });
    await expect(page.locator(".mission-picker select").nth(1)).toHaveValue("gdi-01-east-a");
    await expect(page.locator(".notice-strip")).toHaveText("GDI Mission 1 victory restored · choose the next operation");
    await expect(page.locator(".game-over").getByRole("heading", { name: "Victory", exact: true })).toBeVisible();
    expect(archiveRequests).toHaveLength(1);

    await page.locator(".game-over").getByRole("button", { name: "GDI Mission 2 (East A)", exact: true }).click();
    await waitForFreewareMission(page, "gdi-02-east-a", "GDI Mission 2 (East A)");
    await expect.poll(() => page.evaluate(() => window.__cncwebRuntimeMetrics?.snapshot(1_000))).toMatchObject({
      core: "wasm",
      missionId: "gdi-02-east-a",
      running: true,
    });
    const continuedSession = await page.evaluate(() => JSON.parse(localStorage.getItem("theater.runtime-session.v1") ?? "null") as Record<string, unknown>);
    expect(continuedSession).toMatchObject({
      version: 2,
      mode: "mission",
      missionId: "gdi-02-east-a",
      runId: pendingSession.runId,
      incomingTransition: {
        carryOverCredits: expect.any(Number),
        nukePieces: expect.any(Number),
        sabotagedStructure: -1,
      },
    });
    expect(continuedSession).not.toHaveProperty("pendingVictory");
    expect(archiveRequests).toHaveLength(1);

    await page.reload({ waitUntil: "domcontentloaded" });
    await waitForFreewareMission(page, "gdi-02-east-a", "GDI Mission 2 (East A)");
    expect(archiveRequests).toHaveLength(1);
    await expect.poll(() => serviceWorkerControlled(page), { timeout: 2 * 60_000 }).toBe(true);
    await context.setOffline(true);
    try {
      const offlineReload = await page.reload({ waitUntil: "domcontentloaded" });
      expect(offlineReload?.fromServiceWorker()).toBe(true);
      await waitForFreewareMission(page, "gdi-02-east-a", "GDI Mission 2 (East A)");
      expect(archiveRequests).toHaveLength(1);

      await pauseAtStableTick(page);
      await page.evaluate(async () => {
        if (!window.__cncwebAcceptance) throw new Error("Loopback release-acceptance API is unavailable");
        await window.__cncwebAcceptance.forceVictory();
      });
      await page.getByRole("button", { name: "Resume", exact: true }).click();
      const missionTwoResult = page.locator(".game-over");
      await expect(missionTwoResult.getByRole("heading", { name: "Victory", exact: true })).toBeVisible({ timeout: 2 * 60_000 });
      const missionThree = missionTwoResult.getByRole("button", { name: "GDI Mission 3 (East A)", exact: true });
      await expect(missionThree).toBeEnabled();

      // Reproduce the long-session failure mode in the window realm only. The
      // engine worker keeps normal OPFS access; four failures exhaust the
      // bounded read retry and prove optional audio cannot gate mission start.
      await page.evaluate(() => {
        const prototype = FileSystemDirectoryHandle.prototype;
        const getFileHandle = prototype.getFileHandle;
        let failures = 0;
        Object.defineProperty(prototype, "getFileHandle", {
          configurable: true,
          writable: true,
          async value(this: FileSystemDirectoryHandle, name: string, options?: FileSystemGetFileOptions) {
            if (name === "audio-v1.json" && failures < 4) {
              failures += 1;
              document.documentElement.dataset.audioOpfsFaults = String(failures);
              throw new DOMException("A requested file or directory could not be found at the time an operation was processed.", "NotFoundError");
            }
            return getFileHandle.call(this, name, options);
          },
        });
      });
      await missionThree.click();
      await waitForFreewareMission(page, "gdi-03-east-a", "GDI Mission 3 (East A)");
      await expect(page.locator("html")).toHaveAttribute("data-audio-opfs-faults", "4");
      await expect(page.locator(".notice-strip")).toContainText("audio unavailable");
      expect(archiveRequests).toHaveLength(1);
    } finally {
      await context.setOffline(false);
    }
    await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    expect(pageErrors).toEqual([]);
  });

  test("keeps the real winter mission playable in a coarse-pointer portrait viewport", async ({ browser }, testInfo) => {
    const context = await browser.newContext({
      viewport: { width: 390, height: 844 },
      hasTouch: true,
      isMobile: true,
    });
    const page = await context.newPage();
    const pageErrors: string[] = [];
    page.on("pageerror", (error) => pageErrors.push(error.message));
    try {
      await page.goto("/", { waitUntil: "domcontentloaded" });
      await waitForFreewareMission(page);
      await page.locator(".mission-picker select").nth(1).selectOption("gdi-08-east-a");
      await page.getByRole("button", { name: "Start new mission", exact: true }).click();
      await waitForFreewareMission(page, "gdi-08-east-a", "GDI Mission 8 (East A)");

      await expect(page.locator(".portrait-blocker")).toHaveCount(0);
      expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBeLessThanOrEqual(390);
      await page.getByRole("button", { name: "Collapse mission panel", exact: true }).click();
      const guide = page.locator(".battlefield-guide");
      await expect(guide).toBeVisible();
      const guideBounds = await guide.boundingBox();
      expect(guideBounds?.x).toBeGreaterThanOrEqual(0);
      expect(guideBounds!.x + guideBounds!.width).toBeLessThanOrEqual(390);
      const guideDismiss = page.getByRole("button", { name: "Dismiss battlefield controls guide", exact: true });
      const guideDismissBounds = await guideDismiss.boundingBox();
      expect(guideDismissBounds?.width).toBeGreaterThanOrEqual(44);
      expect(guideDismissBounds?.height).toBeGreaterThanOrEqual(44);
      await guideDismiss.tap();
      await expect(guide).toHaveCount(0);
      expect(await page.evaluate((key) => localStorage.getItem(key), "cncweb:battlefield-onboarding:v1")).toBe("dismissed");
      await page.reload({ waitUntil: "domcontentloaded" });
      await waitForFreewareMission(page, "gdi-08-east-a", "GDI Mission 8 (East A)");
      await expect(guide).toHaveCount(0);
      const guideLauncher = page.getByRole("button", { name: "Open battlefield controls guide", exact: true });
      await expect(guideLauncher).toBeVisible();
      await page.getByRole("button", { name: "Collapse mission panel", exact: true }).click();
      const guideLauncherBounds = await guideLauncher.boundingBox();
      expect(guideLauncherBounds?.height).toBeGreaterThanOrEqual(44);
      await guideLauncher.tap();
      await expect(guide).toBeVisible();
      await dismissBattlefieldGuide(page);
      const battlefield = page.getByLabel("Real-time strategy battlefield");
      const bounds = await battlefield.boundingBox();
      expect(bounds?.width).toBeGreaterThanOrEqual(388);
      const topControls = page.locator(".viewport-chrome button");
      for (let index = 0; index < await topControls.count(); index += 1) {
        const controlBounds = await topControls.nth(index).boundingBox();
        expect(controlBounds?.width).toBeGreaterThanOrEqual(44);
        expect(controlBounds?.height).toBeGreaterThanOrEqual(44);
        expect(controlBounds!.x).toBeGreaterThanOrEqual(0);
        expect(controlBounds!.x + controlBounds!.width).toBeLessThanOrEqual(390);
      }

      await page.getByRole("button", { name: "Select units on next tap", exact: true }).tap();
      await page.touchscreen.tap(bounds!.x + bounds!.width / 2, bounds!.y + bounds!.height / 2);
      await expect(battlefield).toBeFocused();
      await expectCompositedBattlefield(page, battlefield, testInfo, "classic-freeware-gdi-08-mobile.png");
      await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
      expect(pageErrors).toEqual([]);
    } finally {
      await context.close();
    }
  });
});
