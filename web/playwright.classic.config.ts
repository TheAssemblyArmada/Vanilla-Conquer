import { defineConfig, devices } from "@playwright/test";
import baseConfig from "./playwright.config";
import { classicFreewarePlaywrightTarget } from "./scripts/classic-freeware-base-url.mjs";

const targetConfig = classicFreewarePlaywrightTarget(baseConfig);

export default defineConfig({
  ...targetConfig,
  testMatch: "classic-freeware.cross-browser.spec.ts",
  projects: [
    { name: "chromium", use: { ...devices["Desktop Chrome"] } },
    {
      name: "firefox",
      use: {
        ...devices["Desktop Firefox"],
        headless: false,
        launchOptions: {
          firefoxUserPrefs: {
            "webgl.disabled": false,
            "webgl.force-enabled": true,
            "gfx.webrender.software": true,
          },
        },
      },
    },
    { name: "webkit", use: { ...devices["Desktop Safari"] } },
  ],
});
