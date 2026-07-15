import { describe, expect, it } from "vitest";
import { nextSimulationStepDelay } from "./tickTiming";

describe("nextSimulationStepDelay", () => {
  it("waits until the next simulation deadline", () => {
    expect(nextSimulationStepDelay(0, 1000 / 15)).toBeCloseTo(1000 / 15);
    expect(nextSimulationStepDelay(25, 50)).toBe(25);
  });

  it("uses a short timer when a tick is already due", () => {
    expect(nextSimulationStepDelay(50, 50)).toBe(1);
    expect(nextSimulationStepDelay(250, 50)).toBe(1);
  });

  it("rejects invalid timing state", () => {
    expect(() => nextSimulationStepDelay(-1, 50)).toThrow(/accumulatorMs/);
    expect(() => nextSimulationStepDelay(0, 0)).toThrow(/stepMs/);
  });
});
