import { expect, test, type Page } from "@playwright/test";
import { createHash } from "node:crypto";
import { BlobWriter, TextReader, ZipWriter } from "@zip.js/zip.js";
import { probeEngineInDedicatedWorker, type EngineProbeResult } from "./engineProbe";

const demoReadyNotice = "Demo simulation · no compatible campaign content loaded";
const requireEngineProbe = process.env.REQUIRE_BROWSER_ENGINE_E2E === "1";

function tickStatus(page: Page) {
  return page.locator(".runtime-status");
}

async function currentTick(page: Page): Promise<number> {
  const status = await tickStatus(page).innerText();
  const match = /tick\s+([\d,]+)/i.exec(status);
  if (!match) throw new Error(`Runtime status did not contain a tick: ${status}`);
  return Number(match[1].replaceAll(",", ""));
}

async function cameraState(page: Page): Promise<{ x: number; y: number; zoom: number }> {
  return page.getByLabel("Real-time strategy battlefield").evaluate((element) => ({
    x: Number((element as HTMLCanvasElement).dataset.cameraX),
    y: Number((element as HTMLCanvasElement).dataset.cameraY),
    zoom: Number((element as HTMLCanvasElement).dataset.cameraZoom),
  }));
}

async function middleDragCamera(page: Page): Promise<void> {
  const bounds = await page.getByLabel("Real-time strategy battlefield").boundingBox();
  if (!bounds) throw new Error("Battlefield has no desktop pointer bounds");
  await page.mouse.move(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
  await page.mouse.down({ button: "middle" });
  await page.mouse.move(bounds.x + bounds.width / 2 - 80, bounds.y + bounds.height / 2 - 40, { steps: 4 });
  await page.mouse.up({ button: "middle" });
}

async function launchFreshDemo(page: Page): Promise<void> {
  await page.goto("/");
  await expect(page.getByText(demoReadyNotice, { exact: true })).toBeVisible();
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(1);
}

async function installControlGroupHarness(page: Page): Promise<void> {
  await page.addInitScript(() => {
    interface RecordedControlGroupCommand { request: number; index: number }
    const commands: RecordedControlGroupCommand[] = [];
    let groupSelected = false;
    Object.defineProperty(window, "__e2eControlGroupCommands", { configurable: true, value: commands });

    const NativeWorker = window.Worker;
    const nativePostMessage = NativeWorker.prototype.postMessage;
    NativeWorker.prototype.postMessage = function instrumentCommands(message: unknown, transferOrOptions?: Transferable[] | StructuredSerializeOptions): void {
      const candidate = message as { type?: unknown; buffer?: unknown } | null;
      if (candidate?.type === "commands" && candidate.buffer instanceof ArrayBuffer && candidate.buffer.byteLength >= 64) {
        const view = new DataView(candidate.buffer);
        const count = view.getUint32(12, true);
        const recordSize = view.getUint16(20, true);
        for (let index = 0; index < count; index += 1) {
          const offset = 32 + index * recordSize;
          if (view.getUint16(offset, true) !== 6) continue;
          const request = view.getInt32(offset + 4, true);
          const groupIndex = view.getInt32(offset + 8, true);
          commands.push({ request, index: groupIndex });
          if (groupIndex === 0 && (request === 1 || request === 2)) groupSelected = true;
        }
      }
      Reflect.apply(nativePostMessage, this, transferOrOptions === undefined ? [message] : [message, transferOrOptions]);
    };

    function addSyntheticControlGroup(source: ArrayBuffer): ArrayBuffer {
      const sourceView = new DataView(source);
      const declaredLength = sourceView.getUint32(8, true);
      const originalSectionCount = sourceView.getUint32(32, true);
      const playerSectionBytes = 16 + 504;
      const target = new ArrayBuffer(declaredLength + playerSectionBytes);
      new Uint8Array(target).set(new Uint8Array(source, 0, declaredLength));
      const view = new DataView(target);
      view.setUint32(8, target.byteLength, true);
      view.setUint32(12, originalSectionCount + 1, true);
      view.setUint32(32, originalSectionCount + 1, true);

      let sectionOffset = 40;
      for (let section = 0; section < originalSectionCount; section += 1) {
        const kind = view.getUint16(sectionOffset, true);
        const length = view.getUint32(sectionOffset + 4, true);
        const count = view.getUint32(sectionOffset + 8, true);
        const payloadOffset = sectionOffset + 16;
        if (kind === 3) {
          for (let object = 0; object < count; object += 1) {
            const record = payloadOffset + object * 472;
            if (object === 0) view.setInt32(record + 112, 1, true);
            view.setUint8(record + 182, object === 0 ? 0 : 1);
            view.setUint8(record + 184, 0);
            view.setUint8(record + 186, object === 0 ? 0 : 0xff);
            view.setUint32(record + 188, object === 0 && groupSelected ? 1 : 0, true);
          }
        }
        sectionOffset = payloadOffset + length;
      }

      const playerHeader = declaredLength;
      view.setUint16(playerHeader, 8, true);
      view.setUint16(playerHeader + 2, 0, true);
      view.setUint32(playerHeader + 4, 504, true);
      view.setUint32(playerHeader + 8, 1, true);
      view.setUint32(playerHeader + 12, 0, true);
      const player = playerHeader + 16;
      view.setUint8(player + 64, 0);
      view.setUint8(player + 67, 0);
      view.setUint32(player + 88, 0, true);
      view.setUint32(player + 116, 0, true);
      return target;
    }

    class ControlGroupWorker extends NativeWorker {
      constructor(scriptURL: string | URL, options?: WorkerOptions) {
        super(scriptURL, options);
        let messageHandler: ((this: Worker, event: MessageEvent) => unknown) | null = null;
        this.addEventListener("message", (event) => {
          if (!messageHandler) return;
          const data = event.data as { type?: unknown; buffer?: unknown } | null;
          if (data?.type === "snapshot" && data.buffer instanceof ArrayBuffer) {
            messageHandler.call(this, new MessageEvent("message", {
              data: { ...data, buffer: addSyntheticControlGroup(data.buffer) },
            }));
          } else {
            messageHandler.call(this, event);
          }
        });
        Object.defineProperty(this, "onmessage", {
          configurable: true,
          get: () => messageHandler,
          set: (value: ((this: Worker, event: MessageEvent) => unknown) | null) => { messageHandler = value; },
        });
      }
    }
    Object.defineProperty(window, "Worker", { configurable: true, value: ControlGroupWorker });
  });
}

async function installWaitingUpdate(page: Page): Promise<void> {
  await page.evaluate(async () => {
    const registration = await navigator.serviceWorker.ready;
    await navigator.serviceWorker.register(
      new URL(`sw.js?e2e-update=${Date.now()}`, registration.scope),
      { scope: registration.scope },
    );
  });
  await expect(page.locator(".update-toast")).toContainText("An offline update is ready.");
}

async function pauseAtStableTick(page: Page): Promise<number> {
  await page.getByRole("button", { name: "Pause", exact: true }).click();
  await expect(page.getByRole("button", { name: "Resume", exact: true })).toBeEnabled();
  await page.waitForTimeout(250);
  const pausedTick = await currentTick(page);
  await page.waitForTimeout(500);
  expect(await currentTick(page)).toBe(pausedTick);
  return pausedTick;
}

async function markCachedShell(page: Page): Promise<void> {
  await page.evaluate(async () => {
    const registration = await navigator.serviceWorker.ready;
    const cacheName = (await caches.keys()).find((key) => key.startsWith("theater-shell-"));
    if (!cacheName) throw new Error("Current application shell cache is missing");
    const cache = await caches.open(cacheName);
    const indexUrl = new URL("index.html", registration.scope).href;
    const cachedIndex = await cache.match(indexUrl);
    if (!cachedIndex) throw new Error("Cached application index is missing");
    const markedHtml = (await cachedIndex.text()).replace("<html ", '<html data-e2e-shell="active-cache" ');
    await cache.delete(registration.scope);
    await cache.put(indexUrl, new Response(markedHtml, {
      status: cachedIndex.status,
      statusText: cachedIndex.statusText,
      headers: cachedIndex.headers,
    }));
  });
}

async function manifestOnlyPackage(packageId: string): Promise<Buffer> {
  const manifest = JSON.stringify({
    format: "cncweb-content",
    version: 1,
    package_id: packageId,
    created_at_unix_ms: 0,
    source: {
      product: "cnc-remastered-collection",
      provider: "unknown",
      install_fingerprint_sha256: "0".repeat(64),
    },
    content: { games: ["tiberian-dawn"], locales: ["en"] },
    content_sha256: createHash("sha256").update("CNCWEB-CONTENT-MANIFEST-V1\0").digest("hex"),
    files: [],
  });
  const output = new BlobWriter("application/zip");
  const zip = new ZipWriter(output, { useWebWorkers: false });
  await zip.add("manifest.json", new TextReader(manifest));
  return Buffer.from(await (await zip.close()).arrayBuffer());
}

function expectValidEngineProbe(result: EngineProbeResult): void {
  expect(result.inDedicatedWorker).toBe(true);
  expect(result.abi).toBe(2);
  expect(result.createStatus).toBe(0);
  expect(result.handle).toBeGreaterThan(0);
  expect(result.startStatus).toBe(4);
  expect(result.eventQueryStatus).toBe(0);
  expect(result.eventSize).toBeGreaterThanOrEqual(64);
  expect(result.pollStatus).toBe(0);
  expect(result.eventWritten).toBe(result.eventSize);
  expect(result.eventMagic).toBe(0x57434e43);
  expect(result.eventProtocol).toBe(1);
  expect(result.eventKind).toBe(4);
  expect(result.eventType).toBe(14);
  expect(result.diagnosticFlags & 2).toBe(2);
  expect(result.diagnosticCode).toBe(3);
  expect(result.diagnosticStatus).toBe(4);
  expect(result.diagnosticId).toBe("engine.content.invalid");
  expect(result.diagnosticDetail).toMatch(/missing/i);
  expect(result.destroyStatus).toBe(0);
  expect(result.memoryBytes).toBeGreaterThanOrEqual(64 * 1024 * 1024);
}

test("launches the explicit asset-free demo through a worker and WebGL2", async ({ page }) => {
  const pageErrors: string[] = [];
  page.on("pageerror", (error) => pageErrors.push(error.message));
  const workerStarted = page.waitForEvent("worker");

  await launchFreshDemo(page);
  expect((await workerStarted).url()).toMatch(/assets\/simulation\.worker-[\w-]+\.js$/);
  await expect(page.getByRole("heading", { name: "Foundation range" })).toBeVisible();
  await expect(page.getByText("LOCAL DEMO", { exact: true })).toBeVisible();

  const graphics = await page.getByLabel("Real-time strategy battlefield").evaluate((element) => {
    const canvas = element as HTMLCanvasElement;
    const context = canvas.getContext("webgl2");
    return {
      isWebGl2: context instanceof WebGL2RenderingContext,
      width: context?.drawingBufferWidth ?? 0,
      height: context?.drawingBufferHeight ?? 0,
    };
  });
  expect(graphics.isWebGl2).toBe(true);
  expect(graphics.width).toBeGreaterThan(0);
  expect(graphics.height).toBeGreaterThan(0);

  const pausedTick = await pauseAtStableTick(page);
  await page.getByRole("button", { name: "Resume", exact: true }).click();
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(pausedTick + 2);
  expect(pageErrors).toEqual([]);
});

test("discloses import consequences and restores the active launch after an invalid package", async ({ page }) => {
  await launchFreshDemo(page);
  const fileInput = page.locator('input[type="file"][accept*=".cncweb"]');
  await fileInput.setInputFiles({
    name: "invalid-local-package.cncweb",
    mimeType: "application/zip",
    buffer: Buffer.from("not a zip archive", "utf8"),
  });
  const dialog = page.getByRole("dialog", { name: /Install invalid-local-package\.cncweb/ });
  await expect(dialog).toBeVisible();
  await expect(dialog).toContainText("stored only in this browser profile");
  await expect(dialog).toContainText("Clearing this site’s browser data removes imported content and local saves");
  await expect(dialog).toContainText("Installing ends the current simulation");
  await dialog.getByRole("button", { name: "Validate & install" }).click();
  await expect(page.getByRole("alert")).toBeVisible();
  await expect(page.locator(".notice-strip")).toHaveText(demoReadyNotice);
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(1);
});

test("disables stale simulation controls while a package commit is waiting", async ({ page }) => {
  const packageId = "e2e-waiting-import";
  await launchFreshDemo(page);
  await page.evaluate(async (lockName) => {
    let release!: () => void;
    const lifetime = new Promise<void>((resolve) => { release = resolve; });
    (window as typeof window & { __releaseImportLock?: () => void }).__releaseImportLock = release;
    await new Promise<void>((resolve, reject) => {
      void navigator.locks.request(lockName, { mode: "shared" }, async () => {
        resolve();
        await lifetime;
      }).catch(reject);
    });
  }, `theater-content:${packageId}`);

  await page.locator('input[type="file"][accept*=".cncweb"]').setInputFiles({
    name: `${packageId}.cncweb`,
    mimeType: "application/zip",
    buffer: await manifestOnlyPackage(packageId),
  });
  await page.getByRole("dialog", { name: `Install ${packageId}.cncweb` })
    .getByRole("button", { name: "Validate & install" }).click();

  const overlay = page.locator(".launch-overlay");
  await expect(overlay).toContainText("Validating local content");
  await expect(overlay).toContainText("The previous simulation is stopped");
  await expect(page.getByRole("button", { name: "Importing…" })).toBeDisabled();
  await expect(page.getByRole("button", { name: /^(Pause|Resume)$/ })).toBeDisabled();
  await expect(page.getByRole("button", { name: "Stop selected units" })).toBeDisabled();
  await expect(page.getByRole("button", { name: "Save", exact: true })).toBeDisabled();
  await expect(page.getByRole("button", { name: "Zoom in" })).toBeDisabled();
  await expect(page.getByRole("button", { name: "Zoom out" })).toBeDisabled();
  await expect(page.getByRole("button", { name: /Reset camera view/ })).toBeDisabled();

  await page.evaluate(() => {
    const target = window as typeof window & { __releaseImportLock?: () => void };
    target.__releaseImportLock?.();
    delete target.__releaseImportLock;
  });
  await expect(page.getByRole("alert")).toContainText("Runtime catalog is missing");
  await expect(page.locator(".notice-strip")).toHaveText(demoReadyNotice);
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(1);
});

test("keeps keyboard focus contained in dialogs and restores play afterward", async ({ page }) => {
  await launchFreshDemo(page);
  const battlefield = page.getByLabel("Real-time strategy battlefield");
  await expect(battlefield).toHaveAttribute("tabindex", "0");
  await expect(battlefield).toHaveAttribute("aria-describedby", "battlefield-help");

  const collapse = page.getByRole("button", { name: "Collapse mission panel" });
  await collapse.click();
  await expect(page.locator("#mission-panel-content")).toHaveAttribute("aria-hidden", "true");
  await expect(page.locator("#mission-panel-content")).toHaveAttribute("inert", "");
  await page.getByRole("button", { name: "Expand mission panel" }).click();

  const about = page.getByRole("button", { name: "About & legal" });
  await about.click();
  const dialog = page.getByRole("dialog", { name: "Theater Runtime" });
  await expect(dialog).toBeVisible();
  await expect(dialog.getByRole("button", { name: "Close" })).toBeFocused();
  await expect(page.locator(".hud-button.compact").filter({ hasText: "Resume" })).toBeVisible();

  await page.keyboard.press("Shift+Tab");
  await expect(dialog.getByRole("link", { name: "GPL and additional terms" })).toBeFocused();
  await page.keyboard.press("Tab");
  await expect(dialog.getByRole("button", { name: "Close" })).toBeFocused();
  await page.keyboard.press("Escape");
  await expect(dialog).toBeHidden();
  await expect(about).toBeFocused();
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();

  await battlefield.dispatchEvent("webglcontextlost", { cancelable: true });
  await expect(page.getByRole("button", { name: "Resume", exact: true })).toBeEnabled();
  await about.click();
  const contextLossTick = await currentTick(page);
  await battlefield.dispatchEvent("webglcontextrestored");
  await page.waitForTimeout(350);
  expect(await currentTick(page)).toBe(contextLossTick);
  await page.keyboard.press("Escape");
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(contextLossTick);
});

test("maps control-group hotkeys to commands, announcements, focus guards, and repeat centering", async ({ page }) => {
  await installControlGroupHarness(page);
  await launchFreshDemo(page);

  const battlefield = page.getByLabel("Real-time strategy battlefield");
  await battlefield.focus();
  await expect(page.getByRole("button", { name: "Control group 1, 1 object" })).toBeEnabled();

  await page.keyboard.press("1");
  await expect(page.locator(".notice-strip")).toHaveText("Selected control group 1 · 1 object");
  await expect(page.getByRole("button", { name: "Control group 1, 1 object, selected" })).toHaveAttribute("aria-pressed", "true");

  await page.keyboard.press("=");
  await page.keyboard.press("ArrowRight");
  await page.keyboard.press("ArrowDown");
  const displacedCamera = await cameraState(page);
  expect(displacedCamera.zoom).toBeGreaterThan(1);
  expect(displacedCamera.x + displacedCamera.y).toBeGreaterThan(0);

  await page.keyboard.press("1");
  await expect(page.locator(".notice-strip")).toHaveText("Selected control group 1 · 1 object · camera centered");
  const centeredCamera = await cameraState(page);
  expect(centeredCamera.x + centeredCamera.y).toBeLessThan(displacedCamera.x + displacedCamera.y);

  await page.keyboard.press("Control+2");
  await expect(page.locator(".notice-strip")).toHaveText("Assigned 1 selected mobile unit to control group 2");
  await page.keyboard.press("Shift+1");
  await expect(page.locator(".notice-strip")).toHaveText("Added control group 1 · 1 object");

  const accepted = await page.evaluate(() => (
    window as typeof window & { __e2eControlGroupCommands: Array<{ request: number; index: number }> }
  ).__e2eControlGroupCommands);
  expect(accepted).toEqual([
    { request: 1, index: 0 },
    { request: 1, index: 0 },
    { request: 0, index: 1 },
    { request: 2, index: 0 },
  ]);

  const editable = page.locator("#e2e-control-group-editable");
  await page.evaluate(() => {
    const input = document.createElement("input");
    input.id = "e2e-control-group-editable";
    input.setAttribute("aria-label", "E2E editable control");
    document.body.append(input);
  });
  await editable.focus();
  await page.keyboard.press("Control+3");

  await page.getByRole("button", { name: "About & legal" }).click();
  const dialog = page.getByRole("dialog", { name: "Theater Runtime" });
  await expect(dialog).toBeVisible();
  await page.keyboard.press("Control+4");
  expect(await page.evaluate(() => (
    window as typeof window & { __e2eControlGroupCommands: unknown[] }
  ).__e2eControlGroupCommands.length)).toBe(accepted.length);
  await dialog.getByRole("button", { name: "Close" }).click();
});

test("keeps a load paused through graphics loss and blocks an update while loading", async ({ page }) => {
  await page.addInitScript(() => {
    let delayNextLoad = false;
    const postMessage = Worker.prototype.postMessage;
    Worker.prototype.postMessage = function delayedSimulationLoad(message: unknown, transfer: Transferable[] = []) {
      if (delayNextLoad && (message as { type?: unknown } | null)?.type === "load") {
        delayNextLoad = false;
        window.setTimeout(() => postMessage.call(this, message, transfer), 500);
        return;
      }
      postMessage.call(this, message, transfer);
    };
    Object.defineProperty(window, "delayNextSimulationLoad", {
      configurable: true,
      value: () => { delayNextLoad = true; },
    });
  });

  await launchFreshDemo(page);
  const savedTick = await pauseAtStableTick(page);
  await page.getByRole("button", { name: "Save", exact: true }).click();
  await expect(page.getByText("Manual save saved locally for this exact content revision", { exact: true })).toBeVisible();
  await page.getByRole("button", { name: "Resume", exact: true }).click();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(savedTick + 4);

  await installWaitingUpdate(page);
  await expect(page.getByRole("button", { name: "Save & update", exact: true })).toBeEnabled();

  const battlefield = page.getByLabel("Real-time strategy battlefield");
  await battlefield.dispatchEvent("webglcontextlost", { cancelable: true });
  await expect(page.getByRole("button", { name: "Resume", exact: true })).toBeEnabled();
  await expect(page.getByRole("button", { name: "Save & update", exact: true })).toBeEnabled();

  await page.evaluate(() => (
    window as typeof window & { delayNextSimulationLoad: () => void }
  ).delayNextSimulationLoad());
  await page.getByRole("button", { name: "Load (1)", exact: true }).click();
  await expect(page.getByRole("button", { name: "Loading…", exact: true })).toBeDisabled();
  await expect(page.getByRole("button", { name: "Operation active", exact: true })).toBeDisabled();
  await expect(page.getByRole("button", { name: "Save", exact: true })).toBeDisabled();

  await expect(page.locator(".notice-strip")).toContainText(`Loaded Manual save from tick ${savedTick.toLocaleString()}`);
  await expect(page.getByRole("button", { name: "Save & update", exact: true })).toBeEnabled();
  const loadedTick = await currentTick(page);
  await page.waitForTimeout(500);
  expect(await currentTick(page)).toBe(loadedTick);

  await battlefield.dispatchEvent("webglcontextrestored");
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect(page.getByRole("button", { name: "Save & update", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(loadedTick);
});

test("pauses and autosaves a running game before activating an offline update", async ({ page }) => {
  await page.addInitScript(() => {
    const key = "cncweb:e2e-update-navigation-count";
    sessionStorage.setItem(key, String(Number(sessionStorage.getItem(key) ?? "0") + 1));
  });
  await launchFreshDemo(page);
  await installWaitingUpdate(page);
  const tickBeforeUpdate = await currentTick(page);

  await Promise.all([
    page.waitForNavigation(),
    page.getByRole("button", { name: "Save & update", exact: true }).evaluate((button) => {
      (button as HTMLButtonElement).click();
      (button as HTMLButtonElement).click();
    }),
  ]);

  await expect(page.getByText("Resumed local demo", { exact: true })).toBeVisible();
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect(page.getByRole("button", { name: "Load (1)", exact: true })).toBeEnabled();
  await page.getByRole("button", { name: "Pause", exact: true }).click();
  await page.getByRole("button", { name: "Load (1)", exact: true }).click();
  const loadedNotice = page.locator(".notice-strip");
  await expect(loadedNotice).toContainText("Loaded Update autosave from tick");
  const loadedTick = Number((/tick ([\d,]+)/.exec(await loadedNotice.innerText())?.[1] ?? "0").replaceAll(",", ""));
  expect(loadedTick).toBeGreaterThanOrEqual(tickBeforeUpdate);
  expect(await page.evaluate(() => Number(sessionStorage.getItem("cncweb:e2e-update-navigation-count")))).toBe(2);
  await page.waitForTimeout(500);
  expect(await page.evaluate(() => Number(sessionStorage.getItem("cncweb:e2e-update-navigation-count")))).toBe(2);
});

test("resumes a safely saved game when an update never takes control", async ({ page }) => {
  await page.addInitScript(() => {
    const nativePostMessage = ServiceWorker.prototype.postMessage;
    ServiceWorker.prototype.postMessage = function suppressActivation(message: unknown, transfer: Transferable[] = []) {
      if ((message as { type?: unknown } | null)?.type === "SKIP_WAITING") {
        Object.defineProperty(window, "__e2eSuppressedUpdateActivation", { configurable: true, value: true });
        return;
      }
      nativePostMessage.call(this, message, transfer);
    };
  });
  await launchFreshDemo(page);
  await installWaitingUpdate(page);
  const tickBeforeUpdate = await currentTick(page);
  await page.getByRole("button", { name: "Save & update", exact: true }).click();

  await expect(page.locator(".update-toast")).toContainText("did not take control within 10 seconds", { timeout: 15_000 });
  await expect(page.getByRole("button", { name: "Retry update", exact: true })).toBeEnabled();
  expect(await page.evaluate(() => Boolean((window as typeof window & { __e2eSuppressedUpdateActivation?: boolean }).__e2eSuppressedUpdateActivation))).toBe(true);
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(tickBeforeUpdate);
});

test("does not activate an update when an active game cannot be preserved", async ({ page }) => {
  await page.addInitScript(() => {
    Object.defineProperty(StorageManager.prototype, "getDirectory", { configurable: true, value: undefined });
    const nativePostMessage = ServiceWorker.prototype.postMessage;
    Object.defineProperty(window, "__e2eSkipWaitingRequests", { configurable: true, writable: true, value: 0 });
    ServiceWorker.prototype.postMessage = function countActivation(message: unknown, transfer: Transferable[] = []) {
      if ((message as { type?: unknown } | null)?.type === "SKIP_WAITING") {
        (window as typeof window & { __e2eSkipWaitingRequests: number }).__e2eSkipWaitingRequests += 1;
      }
      nativePostMessage.call(this, message, transfer);
    };
  });
  await page.goto("/");
  await expect(page.locator(".notice-strip")).toHaveText("Private browser storage is unavailable · running explicit demo fallback");
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(1);
  await installWaitingUpdate(page);
  const tickBeforeUpdate = await currentTick(page);

  await page.getByRole("button", { name: "Save & update", exact: true }).click();
  await expect(page.locator(".error-banner")).toContainText("active game could not be preserved");
  await expect(page.locator(".notice-strip")).toContainText("cannot be saved safely");
  expect(await page.evaluate(() => (window as typeof window & { __e2eSkipWaitingRequests: number }).__e2eSkipWaitingRequests)).toBe(0);
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(tickBeforeUpdate);
});

test("detects deployment build drift through cache-busted metadata and forces a worker update check", async ({ page }) => {
  await page.addInitScript(() => {
    const nativeUpdate = ServiceWorkerRegistration.prototype.update;
    Object.defineProperty(window, "__e2eWorkerUpdateChecks", { configurable: true, writable: true, value: 0 });
    ServiceWorkerRegistration.prototype.update = function trackedUpdate() {
      (window as typeof window & { __e2eWorkerUpdateChecks: number }).__e2eWorkerUpdateChecks += 1;
      return nativeUpdate.call(this);
    };
  });
  await launchFreshDemo(page);
  await page.evaluate(async () => navigator.serviceWorker.ready);
  await expect.poll(() => page.evaluate(() => navigator.serviceWorker.controller?.scriptURL ?? "")).toContain("/sw.js");

  const requests: string[] = [];
  await page.route("**/build-v1.json?update-check=*", async (route) => {
    requests.push(route.request().url());
    await route.fulfill({
      contentType: "application/json",
      body: JSON.stringify({ format: "cncweb-build", version: 1, id: "fedcba9876543210" }),
    });
  });
  await page.reload();

  await expect(page.locator(".update-toast")).toContainText("service worker was not updated");
  await expect(page.getByRole("button", { name: "Retry update", exact: true })).toBeEnabled();
  await expect.poll(() => page.evaluate(() => (
    window as typeof window & { __e2eWorkerUpdateChecks: number }
  ).__e2eWorkerUpdateChecks)).toBeGreaterThan(0);
  expect(requests.length).toBeGreaterThan(0);
  expect(new URL(requests[0]).searchParams.get("update-check")).toBeTruthy();
});

test("remains playable in a coarse-pointer portrait viewport", async ({ browser }) => {
  const context = await browser.newContext({ viewport: { width: 390, height: 844 }, hasTouch: true, isMobile: true });
  const page = await context.newPage();
  try {
    await launchFreshDemo(page);
    await expect(page.locator(".portrait-blocker")).toHaveCount(0);
    const battlefield = page.getByLabel("Real-time strategy battlefield");
    const battlefieldBounds = await battlefield.boundingBox();
    expect(battlefieldBounds?.width).toBeGreaterThanOrEqual(388);
    const topControls = page.locator(".viewport-chrome button");
    for (let index = 0; index < await topControls.count(); index += 1) {
      const bounds = await topControls.nth(index).boundingBox();
      expect(bounds?.width).toBeGreaterThanOrEqual(44);
      expect(bounds?.height).toBeGreaterThanOrEqual(44);
      expect(bounds!.x).toBeGreaterThanOrEqual(0);
      expect(bounds!.x + bounds!.width).toBeLessThanOrEqual(390);
    }
    expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBeLessThanOrEqual(390);
    await page.getByRole("button", { name: "Collapse mission panel" }).click();
    const sidebarToggleBounds = await page.getByRole("button", { name: "Expand mission panel" }).boundingBox();
    const zoomInBounds = await page.getByRole("button", { name: "Zoom in" }).boundingBox();
    expect(sidebarToggleBounds!.y).toBeGreaterThanOrEqual(zoomInBounds!.y + zoomInBounds!.height);
    const bounds = await battlefield.boundingBox();
    await page.touchscreen.tap(bounds!.x + bounds!.width / 2, bounds!.y + bounds!.height / 2);
    await expect(battlefield).toBeFocused();
  } finally {
    await context.close();
  }
});

test("stays paused when launch completes in a hidden document and resumes on foreground", async ({ page }) => {
  await page.addInitScript(() => {
    let visibility: DocumentVisibilityState = "visible";
    Object.defineProperty(document, "visibilityState", { configurable: true, get: () => visibility });
    Object.defineProperty(window, "setE2eVisibility", {
      configurable: true,
      value: (next: DocumentVisibilityState) => {
        visibility = next;
        document.dispatchEvent(new Event("visibilitychange"));
      },
    });
  });
  await page.route(/\/assets\/simulation\.worker-[\w-]+\.js$/, async (route) => {
    await new Promise((resolve) => setTimeout(resolve, 500));
    await route.continue();
  });
  await page.goto("/");
  await page.getByLabel("Real-time strategy battlefield").waitFor();
  await page.evaluate(() => (window as typeof window & { setE2eVisibility: (state: DocumentVisibilityState) => void }).setE2eVisibility("hidden"));
  await expect(page.getByText(demoReadyNotice, { exact: true })).toBeAttached();
  await expect(page.getByRole("button", { name: "Resume", exact: true })).toBeEnabled();
  const hiddenTick = await currentTick(page);
  await page.waitForTimeout(500);
  expect(await currentTick(page)).toBe(hiddenTick);
  await page.evaluate(() => (window as typeof window & { setE2eVisibility: (state: DocumentVisibilityState) => void }).setE2eVisibility("visible"));
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(hiddenTick);
});

test("saves while paused, loads the earlier state, and resumes it after refresh", async ({ page }) => {
  await launchFreshDemo(page);
  const savedTick = await pauseAtStableTick(page);

  await page.getByRole("button", { name: "Zoom in", exact: true }).click();
  await page.getByRole("button", { name: "Zoom in", exact: true }).click();
  await middleDragCamera(page);
  const savedCamera = await cameraState(page);
  expect(savedCamera.zoom).toBeGreaterThan(1);

  const save = page.getByRole("button", { name: "Save", exact: true });
  await expect(save).toBeEnabled();
  await save.click();
  await expect(page.getByText("Manual save saved locally for this exact content revision", { exact: true })).toBeVisible();
  const load = page.getByRole("button", { name: "Load (1)", exact: true });
  await expect(load).toBeEnabled();
  await page.getByRole("button", { name: /Reset camera view/ }).click();
  expect((await cameraState(page)).zoom).toBe(1);

  await page.getByRole("button", { name: "Resume", exact: true }).click();
  await expect.poll(() => currentTick(page)).toBeGreaterThan(savedTick + 12);
  const advancedTick = await pauseAtStableTick(page);

  await load.click();
  const loadedNotice = page.locator(".notice-strip");
  await expect(loadedNotice).toContainText("Loaded Manual save from tick");
  const notice = await loadedNotice.innerText();
  const loadedMetadataTick = Number(/tick\s+([\d,]+)/.exec(notice)?.[1].replaceAll(",", ""));
  expect(loadedMetadataTick).toBe(savedTick);
  await expect.poll(() => cameraState(page)).toEqual(savedCamera);
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  const reloadedTick = await pauseAtStableTick(page);
  expect(reloadedTick).toBeLessThan(advancedTick);
  expect(reloadedTick).toBeLessThanOrEqual(loadedMetadataTick + 4);

  await page.reload();
  await expect(page.getByText("Resumed local demo", { exact: true })).toBeVisible();
  await expect(page.getByRole("button", { name: "Pause", exact: true })).toBeEnabled();
  const resumedTick = await pauseAtStableTick(page);
  expect(resumedTick).toBeGreaterThanOrEqual(loadedMetadataTick);
  expect(resumedTick).toBeLessThan(advancedTick);
  await expect(page.getByRole("button", { name: "Load (1)", exact: true })).toBeEnabled();
});

test("is service-worker controlled and launches after an offline navigation", async ({ context, page }) => {
  await launchFreshDemo(page);
  await page.evaluate(async () => navigator.serviceWorker.ready);
  await expect.poll(() => page.evaluate(() => navigator.serviceWorker.controller?.scriptURL ?? "")).toContain("/sw.js");
  await expect(page.locator(".update-toast")).toHaveCount(0);
  await expect.poll(() => page.evaluate(async () => (await caches.keys()).some((key) => key.startsWith("theater-shell-")))).toBe(true);
  await markCachedShell(page);

  const controlledReload = await page.reload();
  expect(controlledReload?.fromServiceWorker()).toBe(true);
  await expect(page.locator("html")).toHaveAttribute("data-e2e-shell", "active-cache");
  await expect(page.locator(".notice-strip")).toContainText(/Demo simulation|Resumed local demo/);

  await context.setOffline(true);
  try {
    const offlineNavigation = await page.goto("/");
    expect(offlineNavigation?.fromServiceWorker()).toBe(true);
    await expect(page.locator(".notice-strip")).toContainText(/Demo simulation|Resumed local demo/);
    await expect.poll(() => currentTick(page)).toBeGreaterThan(1);
    const cachedStaticShell = await page.evaluate(async () => Promise.all(
      ["manifest.webmanifest", "icon.svg", "legal.html", "build-v1.json"].map(async (path) => {
        const response = await fetch(path);
        return { path, ok: response.ok, bytes: (await response.arrayBuffer()).byteLength };
      }),
    ));
    expect(cachedStaticShell).toEqual([
      { path: "manifest.webmanifest", ok: true, bytes: expect.any(Number) },
      { path: "icon.svg", ok: true, bytes: expect.any(Number) },
      { path: "legal.html", ok: true, bytes: expect.any(Number) },
      { path: "build-v1.json", ok: true, bytes: expect.any(Number) },
    ]);
    expect(cachedStaticShell.every(({ bytes }) => bytes > 0)).toBe(true);
  } finally {
    await context.setOffline(false);
  }
});

test("instantiates the real Emscripten engine in a dedicated worker online and offline", async ({ context, page }) => {
  test.skip(!requireEngineProbe, "The integrated Wasm bundle is tested in the WebAssembly CI job");

  await launchFreshDemo(page);
  expectValidEngineProbe(await probeEngineInDedicatedWorker(page));

  await page.evaluate(async () => navigator.serviceWorker.ready);
  await expect.poll(() => page.evaluate(() => navigator.serviceWorker.controller?.scriptURL ?? "")).toContain("/sw.js");
  const controlledReload = await page.reload();
  expect(controlledReload?.fromServiceWorker()).toBe(true);
  await expect(page.locator(".notice-strip")).toContainText(/Demo simulation|Resumed local demo/);

  const engineResponses: Array<{ path: string; fromServiceWorker: boolean }> = [];
  page.on("response", (response) => {
    const path = new URL(response.url()).pathname;
    if (/\/engine\/tiberiandawn\.(?:js|wasm)$/.test(path)) {
      engineResponses.push({ path, fromServiceWorker: response.fromServiceWorker() });
    }
  });

  await context.setOffline(true);
  try {
    const offlineReload = await page.reload();
    expect(offlineReload?.fromServiceWorker()).toBe(true);
    await expect(page.locator(".notice-strip")).toContainText(/Demo simulation|Resumed local demo/);
    expectValidEngineProbe(await probeEngineInDedicatedWorker(page));
    await expect.poll(() => new Set(engineResponses.filter((entry) => entry.fromServiceWorker).map((entry) => entry.path)).size).toBe(2);
  } finally {
    await context.setOffline(false);
  }
});
