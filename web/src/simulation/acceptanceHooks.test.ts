import { describe, expect, it } from "vitest";
import { localAcceptanceSession } from "./acceptanceHooks";

const session = "ab".repeat(16);

describe("loopback acceptance hooks", () => {
  it.each([
    `http://127.0.0.1:4173/?acceptance=${session}`,
    `https://localhost/?acceptance=${session}`,
    `http://[::1]:4173/?acceptance=${session}`,
  ])("accepts an intentional loopback session at %s", (href) => {
    expect(localAcceptanceSession({ href } as Location)).toBe(session);
  });

  it.each([
    `https://example.test/?acceptance=${session}`,
    `http://127.0.0.1:4173/play?acceptance=${session}`,
    `http://127.0.0.1:4173/?acceptance=${session}&extra=1`,
    `http://127.0.0.1:4173/?acceptance=short`,
    `http://127.0.0.1:4173/?acceptance=${session}#fragment`,
    "javascript:alert(1)",
  ])("rejects a non-local or ambiguous URL at %s", (href) => {
    expect(localAcceptanceSession({ href } as Location)).toBeUndefined();
  });
});
