import { expect, test, type Locator, type Page, type TestInfo } from "@playwright/test";
import { dismissBattlefieldGuide } from "./compositedPixels";

const enabled = process.env.CNCWEB_CLASSIC_FREEWARE_GENUINE_VICTORY === "1";
const missionTwoEnabled = process.env.CNCWEB_CLASSIC_FREEWARE_GENUINE_MISSION_TWO === "1";
const packageId = "classic-freeware-gdi-v1";

test.use({ screenshot: "off", trace: "off", video: "off", viewport: { width: 1280, height: 720 } });

interface BattlefieldPoint { x: number; y: number; waitTicks?: number }

const COMBAT_ORDERS = new Set(["Attack", "Attack · approach", "Sabotage", "Damage"]);
const TRAVEL_ORDERS = new Set(["Move", "Explore", ...COMBAT_ORDERS]);

function recordProgress(progress: string[], message: string): void {
  progress.push(message);
  console.log(`[genuine-victory] ${message}`);
}

const EXPLORATION_ROUTE: readonly BattlefieldPoint[] = [
  // Clear cells adjacent to every authored infantry cluster. Keeping the
  // three gun approaches last prevents a reinforcement from obscuring an
  // original guard before the mission's All Destr trigger is evaluated.
  { x: 0.781440, y: 0.326087, waitTicks: 510 }, // cell 58,46
  { x: 0.697008, y: 0.152174, waitTicks: 300 }, // cell 55,42
  { x: 0.528144, y: 0.152174, waitTicks: 330 }, // cell 49,42
  { x: 0.331136, y: 0.065217, waitTicks: 420 }, // cell 42,40
  { x: 0.246704, y: 0.239130, waitTicks: 300 }, // cell 39,44
  { x: 0.500000, y: 0.326087, waitTicks: 510 }, // cell 48,46
  { x: 0.471856, y: 0.456522, waitTicks: 210 }, // cell 47,49
  { x: 0.556288, y: 0.673913, waitTicks: 330 }, // cell 50,54
  { x: 0.471856, y: 0.673913, waitTicks: 300 }, // cell 47,54
  { x: 0.331136, y: 0.673913, waitTicks: 300 }, // cell 42,54
];

const FINAL_PATROL: readonly BattlefieldPoint[] = [
  { x: 0.162272, y: 0.065217, waitTicks: 810 }, // cell 36,40
  { x: 0.359280, y: 0.065217, waitTicks: 390 }, // cell 43,40
  { x: 0.443712, y: 0.065217, waitTicks: 300 }, // cell 46,40
  { x: 0.640720, y: 0.065217, waitTicks: 390 }, // cell 53,40
  { x: 0.837728, y: 0.065217, waitTicks: 390 }, // cell 60,40
];

const MISSION_TWO_HOME_PATROL: readonly BattlefieldPoint[] = [
  { x: 0.67, y: 0.54, waitTicks: 450 }, // Southern approach near cell 54,48.
  { x: 0.70, y: 0.28, waitTicks: 600 }, // Central approach near cell 55,42.
  { x: 0.84, y: 0.20, waitTicks: 450 },
  { x: 0.56, y: 0.16, waitTicks: 450 },
  { x: 0.28, y: 0.12, waitTicks: 600 },
  { x: 0.16, y: 0.12, waitTicks: 450 },
  { x: 0.56, y: 0.08, waitTicks: 600 },
  { x: 0.84, y: 0.12, waitTicks: 450 },
];

const MISSION_TWO_NORTH_PATROL: readonly BattlefieldPoint[] = [
  // The camera is moved north three keyboard steps before this phase. These
  // points cover the revealed base and both northern infantry flanks.
  { x: 0.84, y: 0.25, waitTicks: 600 },
  { x: 0.78, y: 0.17, waitTicks: 600 },
  { x: 0.61, y: 0.17, waitTicks: 600 },
  { x: 0.28, y: 0.17, waitTicks: 600 },
  { x: 0.16, y: 0.17, waitTicks: 600 },
  { x: 0.50, y: 0.25, waitTicks: 600 },
];

const MISSION_TWO_CENTERED_PATROL: readonly BattlefieldPoint[] = [
  { x: 0.50, y: 0.24, waitTicks: 300 },
  { x: 0.78, y: 0.40, waitTicks: 300 },
  { x: 0.78, y: 0.68, waitTicks: 300 },
  { x: 0.50, y: 0.76, waitTicks: 300 },
  { x: 0.22, y: 0.68, waitTicks: 300 },
  { x: 0.22, y: 0.40, waitTicks: 300 },
];

async function currentTick(page: Page): Promise<number> {
  const label = await page.locator(".runtime-status").textContent();
  const match = /tick\s+([\d,]+)/i.exec(label ?? "");
  if (!match) throw new Error("Runtime status did not expose a numeric tick");
  return Number(match[1].replaceAll(",", ""));
}

async function visibleContextualOrder(page: Page): Promise<string | undefined> {
  const labels = await page.locator(".contextual-order-status:visible").allTextContents();
  return labels[0]?.trim().replace(/^Right-click\s*·\s*/i, "") || undefined;
}

async function confirmedCombatOrderAt(page: Page, point: BattlefieldPoint): Promise<string | undefined> {
  await page.mouse.move(point.x, point.y);
  const deadline = Date.now() + 750;
  while (Date.now() < deadline) {
    const label = await visibleContextualOrder(page);
    if (label && COMBAT_ORDERS.has(label)) return label;
    await page.waitForTimeout(25);
  }
  return undefined;
}

async function waitForMission(page: Page, missionId = "gdi-01-east-a", title = "GDI Mission 1"): Promise<void> {
  await expect(page.locator(".mission-picker select").first()).toHaveValue(packageId, { timeout: 3 * 60_000 });
  await expect(page.locator(".mission-picker select").nth(1)).toHaveValue(missionId);
  await expect(page.getByRole("heading", { name: title, exact: true })).toBeVisible();
  await expect(page.locator(".minimap span")).toHaveText(missionId.toUpperCase());
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled({ timeout: 3 * 60_000 });
  await expect.poll(() => currentTick(page), { timeout: 3 * 60_000 }).toBeGreaterThan(1);
}

async function pointInBattlefield(battlefield: Locator, normalized: BattlefieldPoint): Promise<BattlefieldPoint> {
  const bounds = await battlefield.boundingBox();
  if (!bounds) throw new Error("Battlefield has no rendered bounds");
  return { x: bounds.x + bounds.width * normalized.x, y: bounds.y + bounds.height * normalized.y };
}

async function boxSelectMobileForce(page: Page, battlefield: Locator): Promise<string> {
  // Stay inside the 4:3 classic presentation rect; the canvas itself can be
  // wider, and starting a drag in its letterbox is intentionally rejected.
  // This southeast band also leaves the reserve MCV outside the selection.
  const start = await pointInBattlefield(battlefield, { x: 0.50, y: 0.65 });
  const end = await pointInBattlefield(battlefield, { x: 0.85, y: 0.82 });
  await page.mouse.move(start.x, start.y);
  await page.mouse.down();
  await page.mouse.move(end.x, end.y, { steps: 8 });
  await page.mouse.up();
  const selection = page.locator(".selection-status");
  await expect(selection).toBeVisible({ timeout: 2_500 }).catch(() => undefined);
  return await selection.textContent({ timeout: 500 }).catch(() => "") ?? "";
}

async function ensureMobileForceSelected(page: Page, battlefield: Locator): Promise<string> {
  for (let attempt = 0; attempt < 5; attempt += 1) {
    const label = await boxSelectMobileForce(page, battlefield);
    if (/Minigunner|Humvee/i.test(label)) return label;
    await page.waitForTimeout(1_000);
  }
  throw new Error("Visible box selection did not find a surviving mobile GDI force");
}

async function refreshControlGroup(page: Page, battlefield: Locator): Promise<string> {
  const reinforcements = await boxSelectMobileForce(page, battlefield);
  if (/Minigunner|Humvee/i.test(reinforcements)) {
    await battlefield.press("Shift+1");
    await page.waitForTimeout(120);
    const combined = await page.locator(".selection-status").textContent({ timeout: 1_000 }).catch(() => reinforcements) ?? reinforcements;
    await battlefield.press("Control+1");
    return combined;
  }
  await battlefield.press("1");
  const selected = await page.locator(".selection-status").textContent({ timeout: 1_000 }).catch(() => "") ?? "";
  if (/Minigunner|Humvee/i.test(selected)) return selected;
  const recovered = await ensureMobileForceSelected(page, battlefield);
  await battlefield.press("Control+1");
  return recovered;
}

async function destroyedCount(page: Page): Promise<number> {
  const labels = await page.locator(".telemetry-grid span").allTextContents();
  const destroyed = labels.find((label) => /^Destroyed/i.test(label));
  const match = /Destroyed\s*([\d,]+)/i.exec(destroyed ?? "");
  return match ? Number(match[1].replaceAll(",", "")) : 0;
}

async function warmVisiblePixelCandidates(page: Page, battlefield: Locator, limit = 16): Promise<BattlefieldPoint[]> {
  const bounds = await battlefield.boundingBox();
  if (!bounds) throw new Error("Battlefield has no rendered bounds");
  await page.mouse.move(4, 4);
  await expect(page.locator(".contextual-order-status")).toHaveCount(0, { timeout: 1_000 });
  const png = await page.screenshot({
    clip: bounds,
    scale: "css",
  });
  return page.evaluate(async ({ encoded, limit }) => {
    const image = new Image();
    image.src = `data:image/png;base64,${encoded}`;
    await image.decode();
    const canvas = document.createElement("canvas");
    canvas.width = image.naturalWidth;
    canvas.height = image.naturalHeight;
    const context = canvas.getContext("2d", { willReadFrequently: true });
    if (!context) throw new Error("Could not inspect the player-visible battlefield screenshot");
    context.drawImage(image, 0, 0);
    const pixels = context.getImageData(0, 0, canvas.width, canvas.height).data;
    const warm = new Uint8Array(canvas.width * canvas.height);
    const scores = new Uint16Array(canvas.width * canvas.height);
    const xStart = Math.floor(canvas.width * 0.10);
    const xEnd = Math.ceil(canvas.width * 0.94);
    const yStart = Math.floor(canvas.height * 0.02);
    const yEnd = Math.ceil(canvas.height * 0.88);
    for (let y = yStart; y < yEnd; y += 1) {
      for (let x = xStart; x < xEnd; x += 1) {
        const offset = (y * canvas.width + x) * 4;
        const red = pixels[offset];
        const green = pixels[offset + 1];
        const blue = pixels[offset + 2];
        if (red < 108 || green > 100 || blue > 80 || red - green < 58 || red - blue < 68) continue;
        const index = y * canvas.width + x;
        warm[index] = 1;
        scores[index] = (red - green) * 3 + red - blue;
      }
    }

    const components: { x: number; y: number; score: number }[] = [];
    const stack: number[] = [];
    for (let y = yStart; y < yEnd; y += 1) {
      for (let x = xStart; x < xEnd; x += 1) {
        const seed = y * canvas.width + x;
        if (!warm[seed]) continue;
        warm[seed] = 0;
        stack.length = 0;
        stack.push(seed);
        let count = 0;
        let best = seed;
        let bestScore = scores[seed];
        while (stack.length) {
          const index = stack.pop()!;
          const componentX = index % canvas.width;
          const componentY = Math.floor(index / canvas.width);
          count += 1;
          if (scores[index] > bestScore) {
            best = index;
            bestScore = scores[index];
          }
          for (let offsetY = -1; offsetY <= 1; offsetY += 1) {
            for (let offsetX = -1; offsetX <= 1; offsetX += 1) {
              if (offsetX === 0 && offsetY === 0) continue;
              const neighborX = componentX + offsetX;
              const neighborY = componentY + offsetY;
              if (neighborX < xStart || neighborX >= xEnd || neighborY < yStart || neighborY >= yEnd) continue;
              const neighbor = neighborY * canvas.width + neighborX;
              if (!warm[neighbor]) continue;
              warm[neighbor] = 0;
              stack.push(neighbor);
            }
          }
        }
        components.push({
          x: best % canvas.width,
          y: Math.floor(best / canvas.width),
          score: bestScore + Math.min(count, 16) * 12,
        });
      }
    }

    const candidates: { x: number; y: number; score: number }[] = [];
    for (const component of components
      .sort((left, right) => right.score - left.score)
    ) {
      if (candidates.some((candidate) => (candidate.x - component.x) ** 2 + (candidate.y - component.y) ** 2 < 100)) continue;
      candidates.push(component);
      if (candidates.length === limit) break;
    }
    return candidates.map(({ x, y }) => ({ x: x / canvas.width, y: y / canvas.height }));
  }, { encoded: png.toString("base64"), limit });
}

async function pauseSimulation(page: Page): Promise<boolean> {
  if (await victoryVisible(page) || await defeatVisible(page)) return false;
  const resume = page.getByRole("button", { name: "Resume", exact: true });
  if (await resume.isVisible().catch(() => false)) return true;
  const pause = page.getByRole("button", { name: "Pause", exact: true });
  await pause.click({ timeout: 2_000 }).catch(() => undefined);
  if (!await resume.isVisible().catch(() => false)) await page.keyboard.press("Escape");
  await expect(resume).toBeEnabled({ timeout: 3_000 });
  return true;
}

async function resumeSimulation(page: Page): Promise<void> {
  if (await victoryVisible(page) || await defeatVisible(page)) return;
  const resume = page.getByRole("button", { name: "Resume", exact: true });
  if (await resume.isVisible().catch(() => false)) {
    await resume.click({ timeout: 2_000 }).catch(() => undefined);
    if (await resume.isVisible().catch(() => false)) await page.keyboard.press("Escape");
  }
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled({ timeout: 3_000 });
}

async function findVisibleAttack(page: Page, battlefield: Locator, candidateLimit = 16): Promise<(BattlefieldPoint & { label: string }) | undefined> {
  const bounds = await battlefield.boundingBox();
  if (!bounds) throw new Error("Battlefield has no rendered bounds");
  for (const candidate of await warmVisiblePixelCandidates(page, battlefield, candidateLimit)) {
    // Infantry remap pixels are often on the head while the root interaction
    // bounds are centered lower on the body/feet. Probe only a tight visible
    // neighborhood around that rendered pixel.
    for (const offset of [{ x: 0, y: 0 }, { x: 0, y: 6 }, { x: 0, y: 11 }, { x: -5, y: 8 }, { x: 5, y: 8 }]) {
      const x = bounds.x + candidate.x * bounds.width + offset.x;
      const y = bounds.y + candidate.y * bounds.height + offset.y;
      await page.mouse.move(x, y);
      await page.waitForTimeout(20);
      const label = await visibleContextualOrder(page);
      if (label && COMBAT_ORDERS.has(label)) return { x, y, label };
    }
  }
  return undefined;
}

async function victoryVisible(page: Page): Promise<boolean> {
  return page.locator(".game-over.won").isVisible().catch(() => false);
}

async function defeatVisible(page: Page): Promise<boolean> {
  return page.locator(".game-over.lost").isVisible().catch(() => false);
}

async function waitForVisibleTicks(page: Page, delta: number, timeout: number): Promise<number> {
  const start = await currentTick(page);
  const deadline = Date.now() + timeout;
  let tick = start;
  while (Date.now() < deadline) {
    if (await victoryVisible(page) || await defeatVisible(page)) return tick - start;
    tick = await currentTick(page);
    if (tick - start >= delta) return tick - start;
    await page.waitForTimeout(250);
  }
  return (await currentTick(page)) - start;
}

async function waitForCombatProgress(
  page: Page,
  destroyedBefore: number,
  tickBefore: number,
  settleTicks = 120,
  noDestructionTicks = 150,
): Promise<void> {
  const deadline = Date.now() + 30_000;
  let progressTick: number | undefined;
  while (Date.now() < deadline) {
    if (await victoryVisible(page) || await defeatVisible(page)) return;
    const tick = await currentTick(page);
    if (await destroyedCount(page) > destroyedBefore) progressTick ??= tick;
    // Mission house triggers are evaluated on a cadence. Keep real gameplay
    // running after a kill so a pause used for the next visual scan cannot
    // indefinitely postpone the visible Victory result.
    if (progressTick !== undefined && tick - progressTick >= settleTicks) return;
    if (progressTick === undefined && tick - tickBefore >= noDestructionTicks) return;
    await page.waitForTimeout(250);
  }
}

async function orderableWaypoint(
  page: Page,
  battlefield: Locator,
  normalized: BattlefieldPoint,
): Promise<(BattlefieldPoint & { label: string }) | undefined> {
  const bounds = await battlefield.boundingBox();
  if (!bounds) throw new Error("Battlefield has no rendered bounds");
  const base = await pointInBattlefield(battlefield, normalized);
  for (const offset of [
    { x: 0, y: 0 },
    { x: 0, y: 20 },
    { x: 0, y: 32 },
    { x: 0, y: 48 },
    { x: 20, y: 0 },
    { x: -20, y: 0 },
    { x: 0, y: -20 },
    { x: 20, y: 20 },
    { x: -20, y: 20 },
    { x: 40, y: 32 },
    { x: -40, y: 32 },
    { x: 80, y: 32 },
    { x: -80, y: 32 },
  ]) {
    const x = Math.min(bounds.x + bounds.width - 2, Math.max(bounds.x + 2, base.x + offset.x));
    const y = Math.min(bounds.y + bounds.height - 2, Math.max(bounds.y + 2, base.y + offset.y));
    const canvasOwnsPoint = await battlefield.evaluate((canvas, point) => document.elementFromPoint(point.x, point.y) === canvas, { x, y });
    if (!canvasOwnsPoint) continue;
    await page.mouse.move(x, y);
    await page.waitForTimeout(20);
    const label = await visibleContextualOrder(page);
    if (label && TRAVEL_ORDERS.has(label)) return { x, y, label };
  }
  return undefined;
}

async function engageVisibleTargets(
  page: Page,
  battlefield: Locator,
  progress: string[],
  candidateLimit = 16,
  settleTicks = 120,
  noDestructionTicks = 150,
): Promise<void> {
  for (let attempt = 0; attempt < 2 && !await victoryVisible(page); attempt += 1) {
    if (await defeatVisible(page)) throw new Error("The player-visible result reported defeat");
    await pauseSimulation(page);
    const target = await findVisibleAttack(page, battlefield, candidateLimit);
    if (!target) {
      await resumeSimulation(page);
      return;
    }
    const before = await destroyedCount(page);
    const tickBefore = await currentTick(page);
    await resumeSimulation(page);
    const confirmedLabel = await confirmedCombatOrderAt(page, target);
    if (!confirmedLabel) continue;
    await page.mouse.click(target.x, target.y, { button: "right" });
    recordProgress(progress, `tick ${await currentTick(page)}: ${confirmedLabel} at a player-visible target; destroyed ${before}`);
    await waitForCombatProgress(page, before, tickBefore, settleTicks, noDestructionTicks);
  }
}

async function runVisiblePatrol(
  page: Page,
  battlefield: Locator,
  route: readonly BattlefieldPoint[],
  name: string,
  progress: string[],
): Promise<void> {
  for (let index = 0; index < route.length && !await victoryVisible(page); index += 1) {
    if (await defeatVisible(page)) throw new Error("The player-visible result reported defeat");
    const selected = await refreshControlGroup(page, battlefield);
    recordProgress(progress, `tick ${await currentTick(page)}: selected ${selected}`);
    let remainingTicks = route[index].waitTicks ?? 120;
    let firstSlice = true;
    while (remainingTicks > 0 && !await victoryVisible(page)) {
      const destination = await orderableWaypoint(page, battlefield, route[index]);
      if (destination) {
        await page.mouse.click(destination.x, destination.y, { button: "right" });
        if (firstSlice || COMBAT_ORDERS.has(destination.label)) {
          recordProgress(progress, `tick ${await currentTick(page)}: ${destination.label} toward ${name} ${index + 1}`);
        }
      } else if (firstSlice) {
        recordProgress(progress, `tick ${await currentTick(page)}: blocked ${name} ${index + 1}; retaining the current visible order`);
      }
      firstSlice = false;
      // Destination cells are guaranteed adjacent to the authored encounter,
      // so a 120-tick visual check remains bounded without repeatedly scanning
      // empty terrain on every few cells of travel.
      const slice = Math.min(120, remainingTicks);
      const advanced = await waitForVisibleTicks(page, slice, Math.max(8_000, slice * 100));
      if (advanced <= 0 && !await victoryVisible(page) && !await defeatVisible(page)) {
        throw new Error(`The visible runtime tick stalled while traveling toward ${name} ${index + 1}`);
      }
      remainingTicks -= advanced;
      await engageVisibleTargets(page, battlefield, progress);
      if (await defeatVisible(page)) throw new Error("The player-visible result reported defeat");
    }
    if (!await victoryVisible(page) && await destroyedCount(page) >= 13) {
      recordProgress(progress, `tick ${await currentTick(page)}: thorough visible-target scan after ${await destroyedCount(page)} destructions`);
      await engageVisibleTargets(page, battlefield, progress, 64);
    }
  }
}

async function startMinigunnerIfAvailable(page: Page, progress: string[]): Promise<boolean> {
  const build = page.getByRole("button", { name: "Build Minigunner", exact: true });
  if (!await build.isVisible().catch(() => false) || !await build.isEnabled().catch(() => false)) return false;
  await build.click();
  recordProgress(progress, `tick ${await currentTick(page)}: started visible Minigunner production`);
  return true;
}

async function trainMissionTwoForce(page: Page, targetTick: number, progress: string[]): Promise<void> {
  let lastProgressTick = await currentTick(page);
  let lastProgressAt = Date.now();
  while (await currentTick(page) < targetTick) {
    if (await defeatVisible(page)) throw new Error("Mission 2 reported defeat while the starting force defended the base");
    await startMinigunnerIfAvailable(page, progress);
    await page.waitForTimeout(750);
    const tick = await currentTick(page);
    if (tick > lastProgressTick) {
      lastProgressTick = tick;
      lastProgressAt = Date.now();
    } else if (Date.now() - lastProgressAt > 10_000) {
      throw new Error(`Mission 2 stopped advancing at visible tick ${tick}`);
    }
  }
}

async function boxSelectMissionTwoBaseForce(page: Page, battlefield: Locator): Promise<string> {
  // Keep both corners inside Mission 2's 31-cell map. The canvas extends
  // beyond the east map edge at the home camera, where box selection is
  // intentionally rejected even though the letterboxed canvas is visible.
  const start = await pointInBattlefield(battlefield, { x: 0.20, y: 0.15 });
  const end = await pointInBattlefield(battlefield, { x: 0.80, y: 0.82 });
  await page.mouse.move(start.x, start.y);
  await page.mouse.down();
  await page.mouse.move(end.x, end.y, { steps: 12 });
  await page.mouse.up();
  await page.waitForTimeout(750);
  return await page.locator(".selection-status").textContent({ timeout: 1_000 }).catch(() => "") ?? "";
}

async function createMissionTwoControlGroup(page: Page, battlefield: Locator, progress: string[]): Promise<void> {
  await battlefield.press("Home");
  await page.waitForTimeout(150);
  const selected = await boxSelectMissionTwoBaseForce(page, battlefield);
  if (!/Minigunner|Humvee|objects selected/i.test(selected)) {
    throw new Error(`Visible Mission 2 box selection found no combat force: ${selected || "no selection"}`);
  }
  await battlefield.press("Control+1");
  const group = page.getByRole("button", { name: /^Control group 1, \d+ objects(?:, selected)?$/ });
  await expect.poll(async () => {
    const label = await group.getAttribute("aria-label").catch(() => "");
    return Number(/Control group 1, (\d+) objects/.exec(label ?? "")?.[1] ?? 0);
  }, { timeout: 5_000 }).toBeGreaterThanOrEqual(20);
  recordProgress(progress, `tick ${await currentTick(page)}: assigned ${await group.getAttribute("aria-label")} from ${selected}`);
}

async function mergeMissionTwoReinforcements(page: Page, battlefield: Locator, progress: string[]): Promise<void> {
  await battlefield.press("Home");
  await page.waitForTimeout(150);
  const nearby = await boxSelectMissionTwoBaseForce(page, battlefield);
  await battlefield.press("Shift+1");
  await page.waitForTimeout(250);
  const combined = await page.locator(".selection-status").textContent({ timeout: 1_000 }).catch(() => nearby) ?? nearby;
  await battlefield.press("Control+1");
  await page.waitForTimeout(250);
  if (!/Minigunner|Humvee|objects selected/i.test(combined)) {
    throw new Error(`Control group 1 could not recover a visible combat force: ${combined || "no selection"}`);
  }
  recordProgress(progress, `tick ${await currentTick(page)}: refreshed control group 1 · ${combined}`);
}

async function runMissionTwoFixedPatrol(
  page: Page,
  battlefield: Locator,
  route: readonly BattlefieldPoint[],
  name: string,
  progress: string[],
  refreshAtHome: boolean,
): Promise<void> {
  for (let index = 0; index < route.length && !await victoryVisible(page); index += 1) {
    if (await defeatVisible(page)) throw new Error("The player-visible Mission 2 result reported defeat");
    if (refreshAtHome) {
      await startMinigunnerIfAvailable(page, progress);
      await mergeMissionTwoReinforcements(page, battlefield, progress);
    }
    await engageVisibleTargets(page, battlefield, progress, 48, 15, 60);
    if (await victoryVisible(page)) break;
    const destination = await orderableWaypoint(page, battlefield, route[index]);
    if (!destination) {
      recordProgress(progress, `tick ${await currentTick(page)}: blocked ${name} ${index + 1}; continuing the visible patrol`);
      const advanced = await waitForVisibleTicks(page, 60, 12_000);
      if (advanced <= 0 && !await victoryVisible(page) && !await defeatVisible(page)) {
        throw new Error(`Mission 2 stalled at blocked ${name} ${index + 1}`);
      }
      await engageVisibleTargets(page, battlefield, progress, 64, 15, 60);
      continue;
    }
    await page.mouse.click(destination.x, destination.y, { button: "right" });
    recordProgress(progress, `tick ${await currentTick(page)}: ${destination.label} toward ${name} ${index + 1}`);
    let remaining = route[index].waitTicks ?? 300;
    while (remaining > 0 && !await victoryVisible(page)) {
      const slice = Math.min(60, remaining);
      const advanced = await waitForVisibleTicks(page, slice, Math.max(8_000, slice * 100));
      if (advanced <= 0 && !await victoryVisible(page) && !await defeatVisible(page)) {
        throw new Error(`Mission 2 stalled while traveling toward ${name} ${index + 1}`);
      }
      remaining -= advanced;
      await engageVisibleTargets(page, battlefield, progress, 48, 15, 60);
      if (await defeatVisible(page)) throw new Error("The player-visible Mission 2 result reported defeat");
    }
  }
}

async function runMissionTwoCenteredPatrol(page: Page, battlefield: Locator, progress: string[]): Promise<void> {
  for (let cycle = 0; cycle < 2 && !await victoryVisible(page); cycle += 1) {
    for (let index = 0; index < MISSION_TWO_CENTERED_PATROL.length && !await victoryVisible(page); index += 1) {
      if (await defeatVisible(page)) throw new Error("The player-visible Mission 2 result reported defeat");
      await battlefield.press("Alt+1");
      await page.waitForTimeout(200);
      await engageVisibleTargets(page, battlefield, progress, 64, 15, 60);
      if (await victoryVisible(page)) break;
      const destination = await orderableWaypoint(page, battlefield, MISSION_TWO_CENTERED_PATROL[index]);
      if (!destination) continue;
      await page.mouse.click(destination.x, destination.y, { button: "right" });
      recordProgress(progress, `tick ${await currentTick(page)}: ${destination.label} toward centered patrol ${cycle + 1}.${index + 1}`);
      const advanced = await waitForVisibleTicks(page, MISSION_TWO_CENTERED_PATROL[index].waitTicks ?? 300, 35_000);
      if (advanced <= 0 && !await victoryVisible(page) && !await defeatVisible(page)) {
        throw new Error("Mission 2 stalled during the centered final patrol");
      }
      await engageVisibleTargets(page, battlefield, progress, 64, 15, 60);
    }
  }
}

test.describe("genuine classic-freeware Mission 1 victory", () => {
  test.skip(!enabled, "Set CNCWEB_CLASSIC_FREEWARE_GENUINE_VICTORY=1 for the long visible-control acceptance");
  test.setTimeout(15 * 60_000);

  test("wins through a scenario-scripted patrol, visible target acquisition, and ordinary controls", async ({ page }, testInfo: TestInfo) => {
    const pageErrors: string[] = [];
    const progress: string[] = [];
    let terminalEvidenceAttached = false;
    page.on("pageerror", (error) => pageErrors.push(error.message));
    try {
      await page.goto("/", { waitUntil: "domcontentloaded" });
      await waitForMission(page);
      await dismissBattlefieldGuide(page);
      const battlefield = page.getByLabel("Real-time strategy battlefield");
      await expect(page.getByRole("button", { name: "Collapse mission panel", exact: true })).toHaveAttribute("aria-expanded", "true");
      await expect(page.getByRole("button", { name: /Reset camera view \(100%\)/ })).toBeVisible();
      const initialBounds = await battlefield.boundingBox();
      expect(initialBounds).not.toBeNull();
      expect(initialBounds!.width).toBeGreaterThan(900);
      expect(initialBounds!.height).toBeGreaterThan(590);

      await page.waitForTimeout(4_000);
      recordProgress(progress, `tick ${await currentTick(page)}: ${await ensureMobileForceSelected(page, battlefield)}`);
      await battlefield.press("Control+1");

      await runVisiblePatrol(page, battlefield, EXPLORATION_ROUTE, "clear-cell waypoint", progress);
      if (!await victoryVisible(page)) {
        recordProgress(progress, `tick ${await currentTick(page)}: allowing the visible mission result to settle after ${await destroyedCount(page)} destructions`);
        await resumeSimulation(page);
        const settledTicks = await waitForVisibleTicks(page, 120, 30_000);
        if (settledTicks < 120 && !await victoryVisible(page) && !await defeatVisible(page)) {
          throw new Error(`The visible runtime advanced only ${settledTicks} of 120 settlement ticks`);
        }
      }
      if (!await victoryVisible(page)) {
        recordProgress(progress, `tick ${await currentTick(page)}: north patrol after ${await destroyedCount(page)} visible destructions`);
        await runVisiblePatrol(page, battlefield, FINAL_PATROL, "north patrol waypoint", progress);
      }

      await expect(page.locator(".game-over.won"), `Visible-control progress:\n${progress.join("\n")}`).toBeVisible({ timeout: 30_000 });
      const result = page.locator(".game-over.won");
      await expect(result.getByRole("heading", { name: "Victory", exact: true })).toBeVisible();
      await expect(page.locator(".mission-objectives-state")).toHaveText("Complete");
      await expect(page.locator(".mission-objectives")).toContainText("Engine-confirmed objective complete");
      const continuation = result.getByRole("button", { name: "GDI Mission 2 (East A)", exact: true });
      await expect(continuation).toBeEnabled();
      expect(pageErrors).toEqual([]);

      const victoryPng = await page.screenshot({ fullPage: true });
      await testInfo.attach("genuine-mission-1-victory.png", { body: victoryPng, contentType: "image/png" });
      terminalEvidenceAttached = true;

      await continuation.click();
      await waitForMission(page, "gdi-02-east-a", "GDI Mission 2 (East A)");
      await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    } finally {
      await testInfo.attach("genuine-mission-1-progress.txt", { body: progress.join("\n"), contentType: "text/plain" });
      if (!terminalEvidenceAttached) {
        const finalPng = await page.screenshot({ fullPage: true }).catch(() => undefined);
        if (finalPng) await testInfo.attach("genuine-mission-1-final-state.png", { body: finalPng, contentType: "image/png" });
      }
    }
  });
});

test.describe("genuine classic-freeware Mission 2 victory", () => {
  test.skip(!missionTwoEnabled, "Set CNCWEB_CLASSIC_FREEWARE_GENUINE_MISSION_TWO=1 for the long Mission 2 visible-control acceptance");
  test.setTimeout(45 * 60_000);

  test("defends, produces a force, wins through visible targets, and continues to Mission 3", async ({ page }, testInfo: TestInfo) => {
    const pageErrors: string[] = [];
    const progress: string[] = [];
    let terminalEvidenceAttached = false;
    page.on("pageerror", (error) => pageErrors.push(error.message));
    try {
      await page.goto("/", { waitUntil: "domcontentloaded" });
      await waitForMission(page);
      await page.locator(".mission-picker select").nth(1).selectOption("gdi-02-east-a");
      await page.getByRole("button", { name: "Start new mission", exact: true }).click();
      await waitForMission(page, "gdi-02-east-a", "GDI Mission 2 (East A)");
      await dismissBattlefieldGuide(page);
      const battlefield = page.getByLabel("Real-time strategy battlefield");
      await expect(page.locator(".mission-objectives")).toContainText("Eliminate the Nod occupation");
      await expect(page.getByLabel("Construction and production")).toContainText("Minigunner");

      await trainMissionTwoForce(page, 12_000, progress);
      recordProgress(progress, `tick ${await currentTick(page)}: beginning the rendered south-to-north sweep after ${await destroyedCount(page)} automatic-defense destructions`);
      await createMissionTwoControlGroup(page, battlefield, progress);
      await runMissionTwoFixedPatrol(page, battlefield, MISSION_TWO_HOME_PATROL, "home-camera patrol", progress, false);

      if (!await victoryVisible(page)) {
        await battlefield.press("KeyW");
        await battlefield.press("KeyW");
        await battlefield.press("KeyW");
        await page.waitForTimeout(200);
        recordProgress(progress, `tick ${await currentTick(page)}: panned north through visible keyboard controls · camera ${await battlefield.getAttribute("data-camera-y")}`);
        await runMissionTwoFixedPatrol(page, battlefield, MISSION_TWO_NORTH_PATROL, "north patrol", progress, false);
      }
      if (!await victoryVisible(page)) await runMissionTwoCenteredPatrol(page, battlefield, progress);

      if (!await victoryVisible(page)) {
        await resumeSimulation(page);
        const settled = await waitForVisibleTicks(page, 180, 30_000);
        if (settled < 180 && !await victoryVisible(page) && !await defeatVisible(page)) {
          throw new Error(`Mission 2 advanced only ${settled} of 180 settlement ticks`);
        }
      }

      await expect(page.locator(".game-over.won"), `Visible-control progress:\n${progress.join("\n")}`).toBeVisible({ timeout: 30_000 });
      await expect(page.locator(".mission-objectives-state")).toHaveText("Complete");
      await expect(page.locator(".mission-objectives")).toContainText("Engine-confirmed objective complete");
      const continuation = page.locator(".game-over.won").getByRole("button", { name: "GDI Mission 3 (East A)", exact: true });
      await expect(continuation).toBeEnabled();
      expect(pageErrors).toEqual([]);

      const victoryPng = await page.screenshot({ fullPage: true });
      await testInfo.attach("genuine-mission-2-victory.png", { body: victoryPng, contentType: "image/png" });
      terminalEvidenceAttached = true;
      await continuation.click();
      await waitForMission(page, "gdi-03-east-a", "GDI Mission 3 (East A)");
      await expect(page.locator(".error-banner, .diagnostic-error")).toHaveCount(0);
    } finally {
      await testInfo.attach("genuine-mission-2-progress.txt", { body: progress.join("\n"), contentType: "text/plain" });
      if (!terminalEvidenceAttached) {
        const finalPng = await page.screenshot({ fullPage: true }).catch(() => undefined);
        if (finalPng) await testInfo.attach("genuine-mission-2-final-state.png", { body: finalPng, contentType: "image/png" });
      }
    }
  });
});
