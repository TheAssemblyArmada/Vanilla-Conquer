export interface ScreenPoint {
  x: number;
  y: number;
}

export interface TouchControllerCallbacks {
  onTap: (point: ScreenPoint, alternate: boolean) => void;
  /** Reports pointer movement for lightweight battlefield previews. */
  onHover?: (point: ScreenPoint) => void;
  onBoxSelect: (start: ScreenPoint, end: ScreenPoint) => void;
  onBoxPreview?: (start: ScreenPoint, end: ScreenPoint) => void;
  onBoxCancel?: () => void;
  onPan: (delta: ScreenPoint) => void;
  onZoom: (factor: number, center: ScreenPoint) => void;
}

interface PointerState {
  start: ScreenPoint;
  current: ScreenPoint;
  alternate: boolean;
  consumed: boolean;
  panOnly: boolean;
}

interface GestureState {
  center: ScreenPoint;
  distance: number;
}

function distance(left: ScreenPoint, right: ScreenPoint): number {
  return Math.hypot(right.x - left.x, right.y - left.y);
}

function center(left: ScreenPoint, right: ScreenPoint): ScreenPoint {
  return { x: (left.x + right.x) / 2, y: (left.y + right.y) / 2 };
}

/** Pointer Events controller shared by touch, pen, and mouse. */
export class TouchController {
  private readonly element: HTMLElement;
  private readonly callbacks: TouchControllerCallbacks;
  private readonly pointers = new Map<number, PointerState>();
  private gesture?: GestureState;
  private readonly dragThreshold: number;

  constructor(element: HTMLElement, callbacks: TouchControllerCallbacks, dragThreshold = 8) {
    this.element = element;
    this.callbacks = callbacks;
    this.dragThreshold = dragThreshold;
    element.style.touchAction = "none";
    element.addEventListener("pointerdown", this.pointerDown);
    element.addEventListener("pointermove", this.pointerMove);
    element.addEventListener("pointerup", this.pointerUp);
    element.addEventListener("pointercancel", this.pointerCancel);
    element.addEventListener("contextmenu", this.contextMenu);
    element.addEventListener("wheel", this.wheel, { passive: false });
  }

  destroy(): void {
    this.element.removeEventListener("pointerdown", this.pointerDown);
    this.element.removeEventListener("pointermove", this.pointerMove);
    this.element.removeEventListener("pointerup", this.pointerUp);
    this.element.removeEventListener("pointercancel", this.pointerCancel);
    this.element.removeEventListener("contextmenu", this.contextMenu);
    this.element.removeEventListener("wheel", this.wheel);
    this.pointers.clear();
    this.gesture = undefined;
  }

  private localPoint(event: PointerEvent): ScreenPoint {
    const bounds = this.element.getBoundingClientRect();
    return { x: event.clientX - bounds.left, y: event.clientY - bounds.top };
  }

  private beginGestureIfNeeded(): void {
    if (this.pointers.size < 2) {
      this.gesture = undefined;
      return;
    }
    if (!this.gesture) this.callbacks.onBoxCancel?.();
    const [first, second] = [...this.pointers.values()].slice(0, 2);
    for (const pointer of this.pointers.values()) pointer.consumed = true;
    this.gesture = { center: center(first.current, second.current), distance: Math.max(1, distance(first.current, second.current)) };
  }

  private readonly pointerDown = (event: PointerEvent): void => {
    if (event.pointerType === "mouse" && event.button !== 0 && event.button !== 1 && event.button !== 2) return;
    event.preventDefault();
    this.element.focus({ preventScroll: true });
    const point = this.localPoint(event);
    this.callbacks.onHover?.(point);
    this.pointers.set(event.pointerId, {
      start: point,
      current: point,
      alternate: event.button === 2,
      consumed: false,
      panOnly: event.pointerType === "mouse" && event.button === 1,
    });
    this.element.setPointerCapture?.(event.pointerId);
    this.beginGestureIfNeeded();
  };

  private readonly pointerMove = (event: PointerEvent): void => {
    const point = this.localPoint(event);
    this.callbacks.onHover?.(point);
    const pointer = this.pointers.get(event.pointerId);
    if (!pointer) return;
    event.preventDefault();
    const previous = pointer.current;
    pointer.current = point;
    if (this.pointers.size >= 2) {
      const [first, second] = [...this.pointers.values()].slice(0, 2);
      const nextCenter = center(first.current, second.current);
      const nextDistance = Math.max(1, distance(first.current, second.current));
      if (this.gesture) {
        this.callbacks.onPan({ x: nextCenter.x - this.gesture.center.x, y: nextCenter.y - this.gesture.center.y });
        this.callbacks.onZoom(nextDistance / this.gesture.distance, nextCenter);
      }
      this.gesture = { center: nextCenter, distance: nextDistance };
      first.consumed = true;
      second.consumed = true;
      return;
    }
    if (pointer.panOnly) {
      this.callbacks.onPan({ x: pointer.current.x - previous.x, y: pointer.current.y - previous.y });
      pointer.consumed = true;
      return;
    }
    if (pointer.consumed) return;
    if (!pointer.alternate && distance(pointer.start, pointer.current) >= this.dragThreshold) {
      this.callbacks.onBoxPreview?.(pointer.start, pointer.current);
    }
  };

  private readonly pointerUp = (event: PointerEvent): void => {
    const pointer = this.pointers.get(event.pointerId);
    if (!pointer) return;
    event.preventDefault();
    pointer.current = this.localPoint(event);
    this.pointers.delete(event.pointerId);
    if (pointer.panOnly) {
      this.callbacks.onBoxCancel?.();
    } else if (!pointer.consumed) {
      if (distance(pointer.start, pointer.current) >= this.dragThreshold && !pointer.alternate) {
        this.callbacks.onBoxSelect(pointer.start, pointer.current);
      } else {
        this.callbacks.onBoxCancel?.();
        this.callbacks.onTap(pointer.current, pointer.alternate);
      }
    }
    this.beginGestureIfNeeded();
  };

  private readonly pointerCancel = (event: PointerEvent): void => {
    if (this.pointers.delete(event.pointerId)) this.callbacks.onBoxCancel?.();
    this.beginGestureIfNeeded();
  };

  private readonly contextMenu = (event: Event): void => event.preventDefault();

  private readonly wheel = (event: WheelEvent): void => {
    event.preventDefault();
    const factor = Math.exp(-Math.max(-240, Math.min(240, event.deltaY)) * 0.0025);
    this.callbacks.onZoom(factor, this.localPoint(event as unknown as PointerEvent));
  };
}
