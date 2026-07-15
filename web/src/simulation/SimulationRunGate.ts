/** Keeps a resumed launch paused until its save has replaced the fresh state. */
export class SimulationRunGate {
  private requestedRunning = false;
  private deferredLoad = false;
  private active = false;

  get running(): boolean { return this.active; }
  get awaitingDeferredLoad(): boolean { return this.deferredLoad; }

  begin(terminal: boolean, deferRunningUntilLoad: boolean): void {
    this.requestedRunning = !terminal;
    this.deferredLoad = this.requestedRunning && deferRunningUntilLoad;
    this.active = this.requestedRunning && !this.deferredLoad;
  }

  requestRunning(requested: boolean, terminal: boolean): void {
    this.requestedRunning = requested && !terminal;
    this.active = this.requestedRunning && !this.deferredLoad;
  }

  completeLoad(terminal: boolean): boolean {
    const wasDeferred = this.deferredLoad;
    this.deferredLoad = false;
    this.active = this.requestedRunning && !terminal;
    return wasDeferred;
  }

  failLoad(terminal: boolean, fatal: boolean): boolean {
    const wasDeferred = this.deferredLoad;
    this.deferredLoad = false;
    this.active = !fatal && this.requestedRunning && !terminal;
    return wasDeferred;
  }

  stop(): void {
    this.requestedRunning = false;
    this.deferredLoad = false;
    this.active = false;
  }
}
