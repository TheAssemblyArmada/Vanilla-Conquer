import { act } from "react";
import { createRoot, type Root } from "react-dom/client";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { BATTLEFIELD_ONBOARDING_STORAGE_KEY, BattlefieldOnboarding } from "./BattlefieldOnboarding";

(globalThis as typeof globalThis & { IS_REACT_ACT_ENVIRONMENT: boolean }).IS_REACT_ACT_ENVIRONMENT = true;

describe("BattlefieldOnboarding", () => {
  let container: HTMLDivElement;
  let root: Root;

  beforeEach(() => {
    window.localStorage.clear();
    container = document.createElement("div");
    document.body.append(container);
    root = createRoot(container);
  });

  afterEach(() => {
    act(() => root.unmount());
    container.remove();
    window.localStorage.clear();
  });

  function render(active = true): void {
    act(() => root.render(<BattlefieldOnboarding active={active} />));
  }

  function button(label: string): HTMLButtonElement {
    const match = container.querySelector(`button[aria-label="${label}"]`);
    if (!(match instanceof HTMLButtonElement)) throw new Error(`Button not found: ${label}`);
    return match;
  }

  it("waits for a mission, then explains shroud and every primary input path", () => {
    render(false);
    expect(container.innerHTML).toBe("");

    render(true);
    const guide = container.querySelector("aside");
    expect(guide?.getAttribute("aria-labelledby")).toBe("battlefield-guide-title");
    expect(guide?.textContent).toContain("unexplored shroud, not a graphics failure");
    expect(guide?.textContent).toContain("toward the lower right");
    expect(guide?.textContent).toContain("Middle-button drag");
    expect(guide?.textContent).toContain("W A S D");
    expect(guide?.textContent).toContain("two-finger drag");
    expect(guide?.textContent).toContain("Wheel, pinch");
    expect(guide?.textContent).toContain("Click or tap a unit");
    expect(guide?.textContent).toContain("Right-click to order");
  });

  it("persists dismissal, restores focus, and leaves an accessible way to reopen the guide", () => {
    render();
    const dismiss = button("Dismiss battlefield controls guide");
    dismiss.focus();
    expect(document.activeElement).toBe(dismiss);
    act(() => dismiss.click());

    expect(window.localStorage.getItem(BATTLEFIELD_ONBOARDING_STORAGE_KEY)).toBe("dismissed");
    expect(container.querySelector("aside")).toBeNull();
    const launcher = button("Open battlefield controls guide");
    expect(launcher.textContent).toBe("Controls");
    expect(document.activeElement).toBe(launcher);

    act(() => launcher.click());
    expect(document.activeElement).toBe(button("Dismiss battlefield controls guide"));
    act(() => button("Dismiss battlefield controls guide").click());

    act(() => root.unmount());
    root = createRoot(container);
    render();
    expect(container.querySelector("aside")).toBeNull();

    act(() => button("Open battlefield controls guide").click());
    expect(container.querySelector("aside")).not.toBeNull();
    expect(document.activeElement).toBe(button("Dismiss battlefield controls guide"));
  });

  it("remains dismissible when browser storage is unavailable", () => {
    vi.spyOn(Storage.prototype, "getItem").mockImplementation(() => { throw new DOMException("blocked"); });
    vi.spyOn(Storage.prototype, "setItem").mockImplementation(() => { throw new DOMException("blocked"); });

    render();
    expect(container.querySelector("aside")).not.toBeNull();
    act(() => button("Dismiss battlefield controls guide").click());
    expect(container.querySelector("aside")).toBeNull();
    expect(button("Open battlefield controls guide")).toBeInstanceOf(HTMLButtonElement);
  });
});
