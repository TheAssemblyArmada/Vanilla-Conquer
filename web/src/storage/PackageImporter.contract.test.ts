// @vitest-environment node
import { readFile } from "node:fs/promises";
import { describe, expect, it } from "vitest";
import { DEFAULT_PACKAGE_LIMITS, packageInstallQuotaBytes, type PackageImportLimits } from "./PackageImporter";

interface BrowserPackageProfileFixture {
  profile: string;
  version: number;
  limits: PackageImportLimits;
}

describe("browser package profile contract", () => {
  it("matches the Rust packer's browser-v1 defaults", async () => {
    const fixtureUrl = new URL("../../../tools/content-packer/fixtures/browser-package-profile-v1.json", import.meta.url);
    const fixture = JSON.parse(await readFile(fixtureUrl, "utf8")) as BrowserPackageProfileFixture;

    expect(fixture.profile).toBe("cncweb-browser-import");
    expect(fixture.version).toBe(1);
    expect(DEFAULT_PACKAGE_LIMITS).toEqual(fixture.limits);
  });

  it("reserves only the candidate's expanded content and manifest", () => {
    expect(packageInstallQuotaBytes(640 * 1024 * 1024, 128 * 1024)).toBe((640 * 1024 * 1024) + (128 * 1024));
    expect(() => packageInstallQuotaBytes(Number.MAX_SAFE_INTEGER, 1)).toThrow("unsupported");
  });
});
