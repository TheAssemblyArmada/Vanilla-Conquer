import { afterEach, describe, expect, it, vi } from "vitest";
import {
  ORDER_COACH_SESSION_KEY,
  rememberOrderCoachDismissal,
  shouldShowOrderCoach,
  wasOrderCoachDismissed,
} from "./orderCoach";

describe("orderCoach", () => {
  afterEach(() => {
    window.sessionStorage.clear();
    vi.restoreAllMocks();
  });

  it("shows only for coarse pointer with a selection in select mode", () => {
    expect(shouldShowOrderCoach({
      coarsePointer: true,
      dismissed: false,
      selectionCount: 2,
      interactionMode: "select",
      hasBattlefieldTool: false,
    })).toBe(true);
    expect(shouldShowOrderCoach({
      coarsePointer: false,
      dismissed: false,
      selectionCount: 2,
      interactionMode: "select",
      hasBattlefieldTool: false,
    })).toBe(false);
    expect(shouldShowOrderCoach({
      coarsePointer: true,
      dismissed: true,
      selectionCount: 2,
      interactionMode: "select",
      hasBattlefieldTool: false,
    })).toBe(false);
    expect(shouldShowOrderCoach({
      coarsePointer: true,
      dismissed: false,
      selectionCount: 0,
      interactionMode: "select",
      hasBattlefieldTool: false,
    })).toBe(false);
    expect(shouldShowOrderCoach({
      coarsePointer: true,
      dismissed: false,
      selectionCount: 2,
      interactionMode: "order",
      hasBattlefieldTool: false,
    })).toBe(false);
    expect(shouldShowOrderCoach({
      coarsePointer: true,
      dismissed: false,
      selectionCount: 2,
      interactionMode: "select",
      hasBattlefieldTool: true,
    })).toBe(false);
  });

  it("persists dismissal in session storage and tolerates storage failures", () => {
    expect(wasOrderCoachDismissed()).toBe(false);
    rememberOrderCoachDismissal();
    expect(window.sessionStorage.getItem(ORDER_COACH_SESSION_KEY)).toBe("dismissed");
    expect(wasOrderCoachDismissed()).toBe(true);

    vi.spyOn(Storage.prototype, "getItem").mockImplementation(() => { throw new DOMException("blocked"); });
    vi.spyOn(Storage.prototype, "setItem").mockImplementation(() => { throw new DOMException("blocked"); });
    expect(wasOrderCoachDismissed()).toBe(false);
    expect(() => rememberOrderCoachDismissal()).not.toThrow();
  });
});
