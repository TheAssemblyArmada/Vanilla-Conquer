import { describe, expect, it } from "vitest";
import { domTelemetryRefreshDue, minimapRefreshDue } from "./domTelemetryCadence";

describe("DOM telemetry cadence", () => {
  it("refreshes after three elapsed ticks even when intermediate snapshots were skipped", () => {
    expect(domTelemetryRefreshDue(1, undefined)).toBe(true);
    expect(domTelemetryRefreshDue(2, 1)).toBe(false);
    expect(domTelemetryRefreshDue(3, 1)).toBe(false);
    expect(domTelemetryRefreshDue(4, 1)).toBe(true);
    expect(domTelemetryRefreshDue(7, 4)).toBe(true);
  });

  it("refreshes when a launch or load moves the tick backwards and when forced", () => {
    expect(domTelemetryRefreshDue(1, 120)).toBe(true);
    expect(domTelemetryRefreshDue(120, 120)).toBe(false);
    expect(domTelemetryRefreshDue(120, 120, true)).toBe(true);
  });

  it("keeps the minimap near one hertz and refreshes discontinuities immediately", () => {
    expect(minimapRefreshDue(1, undefined)).toBe(true);
    expect(minimapRefreshDue(15, 1)).toBe(false);
    expect(minimapRefreshDue(16, 1)).toBe(true);
    expect(minimapRefreshDue(120, 120)).toBe(false);
    expect(minimapRefreshDue(120, 120, true)).toBe(true);
    expect(minimapRefreshDue(3, 120)).toBe(true);
  });
});
