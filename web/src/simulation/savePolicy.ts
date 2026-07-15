export function assertSimulationSaveAllowed(terminal: boolean): void {
  if (terminal) throw new Error("Terminal simulation state cannot be saved; load an earlier save or restart the mission");
}
