import { describe, expect, it } from "vitest";
import { assertSimulationSaveAllowed } from "./savePolicy";

describe("simulation save policy", () => {
  it("rejects worker-side save requests after the terminal tick", () => {
    expect(() => assertSimulationSaveAllowed(false)).not.toThrow();
    expect(() => assertSimulationSaveAllowed(true)).toThrow("Terminal simulation state cannot be saved");
  });
});
