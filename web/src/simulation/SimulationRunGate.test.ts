import { describe, expect, it } from "vitest";
import { SimulationRunGate } from "./SimulationRunGate";

describe("SimulationRunGate", () => {
  it("does not permit a first tick before a deferred resume save loads", () => {
    const gate = new SimulationRunGate();
    gate.begin(false, true);

    expect(gate.awaitingDeferredLoad).toBe(true);
    expect(gate.running).toBe(false);
    gate.requestRunning(true, false);
    expect(gate.running).toBe(false);

    expect(gate.completeLoad(false)).toBe(true);
    expect(gate.running).toBe(true);
  });

  it("preserves an explicit pause across the deferred load", () => {
    const gate = new SimulationRunGate();
    gate.begin(false, true);
    gate.requestRunning(false, false);
    gate.completeLoad(false);
    expect(gate.running).toBe(false);
  });

  it("resumes fresh state after a nonfatal load failure but stops after a fatal one", () => {
    const recoverable = new SimulationRunGate();
    recoverable.begin(false, true);
    expect(recoverable.failLoad(false, false)).toBe(true);
    expect(recoverable.running).toBe(true);

    const fatal = new SimulationRunGate();
    fatal.begin(false, true);
    expect(fatal.failLoad(false, true)).toBe(true);
    expect(fatal.running).toBe(false);
  });
});
