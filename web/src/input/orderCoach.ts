export const ORDER_COACH_SESSION_KEY = "cncweb:order-coach:v1";

export function wasOrderCoachDismissed(storage: Storage | undefined = typeof window === "undefined" ? undefined : window.sessionStorage): boolean {
  try {
    return storage?.getItem(ORDER_COACH_SESSION_KEY) === "dismissed";
  } catch {
    return false;
  }
}

export function rememberOrderCoachDismissal(storage: Storage | undefined = typeof window === "undefined" ? undefined : window.sessionStorage): void {
  try {
    storage?.setItem(ORDER_COACH_SESSION_KEY, "dismissed");
  } catch {
    // Coaching must never depend on session storage availability.
  }
}

export function shouldShowOrderCoach(options: {
  coarsePointer: boolean;
  dismissed: boolean;
  selectionCount: number;
  interactionMode: "select" | "order";
  hasBattlefieldTool: boolean;
}): boolean {
  return options.coarsePointer
    && !options.dismissed
    && options.selectionCount > 0
    && options.interactionMode === "select"
    && !options.hasBattlefieldTool;
}
