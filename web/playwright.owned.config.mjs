import { defineConfig, devices } from "@playwright/test";
import { loadOwnedBrowserPreflightEnvironment } from "./scripts/owned-browser-preflight-env.mjs";

const owned = loadOwnedBrowserPreflightEnvironment();

export default defineConfig({
  testDir: "./e2e",
  testMatch: "owned-content.preflight.spec.ts",
  fullyParallel: false,
  forbidOnly: true,
  retries: 0,
  workers: 1,
  reporter: [["./scripts/owned-browser-preflight-reporter.mjs"]],
  outputDir: owned.outputDir,
  timeout: 15 * 60_000,
  expect: { timeout: 2 * 60_000 },
  use: {
    ...devices["Desktop Chrome"],
    baseURL: owned.baseURL,
    serviceWorkers: "allow",
    trace: "off",
    screenshot: "off",
    video: "off",
    viewport: { width: 1440, height: 900 },
  },
  projects: [{ name: "private-owned-chromium" }],
  // Deliberately no webServer: this preflight must reuse the private harness
  // preview and cannot start against a default or repository-local fixture.
});
