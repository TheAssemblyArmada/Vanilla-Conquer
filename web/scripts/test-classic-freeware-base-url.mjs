import assert from "node:assert/strict";
import test from "node:test";

import {
  CLASSIC_FREEWARE_BASE_URL_ENV,
  classicFreewareBaseURLFromEnvironment,
  classicFreewarePlaywrightTarget,
  resolveClassicFreewareBaseURL,
} from "./classic-freeware-base-url.mjs";

test("leaves the local Playwright target unchanged when the staging URL is absent", () => {
  assert.equal(resolveClassicFreewareBaseURL(undefined), undefined);
  assert.equal(classicFreewareBaseURLFromEnvironment({}), undefined);
  const local = { use: { baseURL: "http://127.0.0.1:4173/", trace: "on" }, webServer: { command: "preview" } };
  assert.equal(classicFreewarePlaywrightTarget(local, {}), local);
});

test("accepts HTTPS staging origins and normalizes their URLs", () => {
  assert.equal(resolveClassicFreewareBaseURL("https://staging.example.test"), "https://staging.example.test/");
  assert.equal(resolveClassicFreewareBaseURL("https://staging.example.test/releases/candidate"), "https://staging.example.test/releases/candidate/");
  assert.equal(
    classicFreewareBaseURLFromEnvironment({ [CLASSIC_FREEWARE_BASE_URL_ENV]: "https://staging.example.test/release/" }),
    "https://staging.example.test/release/",
  );
});

test("permits HTTP only for explicit loopback hosts", () => {
  for (const value of [
    "http://127.0.0.1:4173",
    "http://127.0.0.2:4173",
    "http://localhost:4173",
    "http://[::1]:4173",
  ]) {
    assert.equal(resolveClassicFreewareBaseURL(value), `${value}/`);
  }
  assert.throws(() => resolveClassicFreewareBaseURL("http://staging.example.test"), /HTTPS for non-loopback/);
});

test("uses the remote base URL without inheriting the local preview server", () => {
  const local = {
    timeout: 30_000,
    use: { baseURL: "http://127.0.0.1:4173/", trace: "on" },
    webServer: { command: "pnpm preview", reuseExistingServer: true },
  };
  const remote = classicFreewarePlaywrightTarget(local, {
    [CLASSIC_FREEWARE_BASE_URL_ENV]: "https://staging.example.test/releases/candidate",
  });
  assert.deepEqual(remote, {
    timeout: 30_000,
    use: { baseURL: "https://staging.example.test/releases/candidate/", trace: "on" },
  });
  assert.deepEqual(local.webServer, { command: "pnpm preview", reuseExistingServer: true });
});

test("rejects credentials, queries, fragments, non-web schemes, and ambiguous values", () => {
  for (const value of [
    "https://user@staging.example.test/",
    "https://@staging.example.test/",
    "https://staging.example.test/?token=secret",
    "https://staging.example.test/?",
    "https://staging.example.test/#release",
    "https://staging.example.test/#",
    "file:///tmp/staging",
    " staging.example.test ",
    "",
  ]) {
    assert.throws(() => resolveClassicFreewareBaseURL(value));
  }
});
