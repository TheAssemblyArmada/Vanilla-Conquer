import { useEffect, useRef, useState } from "react";
import "./battlefieldOnboarding.css";

export const BATTLEFIELD_ONBOARDING_STORAGE_KEY = "cncweb:battlefield-onboarding:v1";

function wasDismissed(): boolean {
  try {
    return window.localStorage.getItem(BATTLEFIELD_ONBOARDING_STORAGE_KEY) === "dismissed";
  } catch {
    // Storage can be unavailable in private or embedded browsing contexts. The
    // guide remains useful there and can still be closed for the current page.
    return false;
  }
}

function rememberDismissal(): void {
  try {
    window.localStorage.setItem(BATTLEFIELD_ONBOARDING_STORAGE_KEY, "dismissed");
  } catch {
    // Closing the non-blocking guide must never depend on persistent storage.
  }
}

export interface BattlefieldOnboardingProps {
  active: boolean;
}

/** A first-run, non-modal battlefield guide with a persistent compact launcher. */
export function BattlefieldOnboarding({ active }: BattlefieldOnboardingProps) {
  const [open, setOpen] = useState(() => !wasDismissed());
  const launcherRef = useRef<HTMLButtonElement>(null);
  const dismissRef = useRef<HTMLButtonElement>(null);
  const focusAfterTransitionRef = useRef<"launcher" | "dismiss" | undefined>(undefined);

  useEffect(() => {
    const target = focusAfterTransitionRef.current;
    if (!target) return;
    focusAfterTransitionRef.current = undefined;
    (target === "dismiss" ? dismissRef : launcherRef).current?.focus({ preventScroll: true });
  }, [open]);

  if (!active) return null;

  if (!open) {
    return <button
      ref={launcherRef}
      type="button"
      className="battlefield-guide-launcher"
      aria-label="Open battlefield controls guide"
      onClick={() => {
        focusAfterTransitionRef.current = "dismiss";
        setOpen(true);
      }}
    >Controls</button>;
  }

  const dismiss = (): void => {
    rememberDismissal();
    focusAfterTransitionRef.current = "launcher";
    setOpen(false);
  };

  return <aside className="battlefield-guide" aria-labelledby="battlefield-guide-title">
    <div className="battlefield-guide-heading">
      <div>
        <span>First run</span>
        <h2 id="battlefield-guide-title">Battlefield basics</h2>
      </div>
      <button ref={dismissRef} type="button" onClick={dismiss} aria-label="Dismiss battlefield controls guide">Done</button>
    </div>
    <p className="battlefield-guide-shroud"><strong>Black map?</strong> That is unexplored shroud, not a graphics failure. Explore to reveal it. In the first GDI mission, your revealed start and units are toward the lower right.</p>
    <dl>
      <div>
        <dt>Pan</dt>
        <dd>Middle-button drag, <kbd>W A S D</kbd> or arrow keys; two-finger drag on touch.</dd>
      </div>
      <div>
        <dt>Zoom</dt>
        <dd>Wheel, pinch, the <kbd>+</kbd>/<kbd>−</kbd> buttons, or keyboard.</dd>
      </div>
      <div>
        <dt>Select &amp; order</dt>
        <dd>Click or tap a unit; drag to box-select. Right-click to order, or choose <strong>Order</strong> then tap.</dd>
      </div>
    </dl>
  </aside>;
}
