/**
 * Keeps DOM-facing state near 5 Hz without assuming the worker delivers every
 * simulation tick. Snapshots may skip ticks under backpressure.
 */
export function domTelemetryRefreshDue(
  tick: number,
  lastRefreshTick: number | undefined,
  force = false,
): boolean {
  return force
    || lastRefreshTick === undefined
    || tick < lastRefreshTick
    || tick - lastRefreshTick >= 3;
}

/**
 * The radar is presentation-only and considerably more expensive than text
 * telemetry because it downsamples and paints the complete indexed surface.
 * Keep it live at roughly 1 Hz while simulation and WebGL stay full cadence.
 */
export function minimapRefreshDue(
  tick: number,
  lastRefreshTick: number | undefined,
  force = false,
): boolean {
  return force
    || lastRefreshTick === undefined
    || tick < lastRefreshTick
    || tick - lastRefreshTick >= 15;
}
