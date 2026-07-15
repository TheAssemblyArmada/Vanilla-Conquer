const MINIMUM_TIMER_DELAY_MS = 1;

/** Returns the delay until the next simulation deadline. */
export function nextSimulationStepDelay(accumulatorMs: number, stepMs: number): number {
  if (!Number.isFinite(accumulatorMs) || accumulatorMs < 0) throw new RangeError("accumulatorMs must be a non-negative finite number");
  if (!Number.isFinite(stepMs) || stepMs <= 0) throw new RangeError("stepMs must be a positive finite number");
  return Math.max(MINIMUM_TIMER_DELAY_MS, stepMs - accumulatorMs);
}
