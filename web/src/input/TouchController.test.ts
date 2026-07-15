import { describe, expect, it, vi } from "vitest";
import { TouchController } from "./TouchController";

function pointer(type: string, id: number, x: number, y: number, button = 0, pointerType = "touch"): Event {
  const event = new MouseEvent(type, { bubbles: true, cancelable: true, clientX: x, clientY: y, button });
  Object.defineProperties(event, { pointerId: { value: id }, pointerType: { value: pointerType } });
  return event;
}

describe("TouchController", () => {
  it("maps taps and drag selection without relying on click events", () => {
    const element = document.createElement("div");
    vi.spyOn(element, "getBoundingClientRect").mockReturnValue({ x: 10, y: 20, left: 10, top: 20, right: 210, bottom: 120, width: 200, height: 100, toJSON: () => ({}) });
    const onTap = vi.fn();
    const onBoxSelect = vi.fn();
    const controller = new TouchController(element, { onTap, onBoxSelect, onPan: vi.fn(), onZoom: vi.fn() });
    element.dispatchEvent(pointer("pointerdown", 1, 30, 40));
    element.dispatchEvent(pointer("pointerup", 1, 31, 41));
    expect(onTap).toHaveBeenCalledWith({ x: 21, y: 21 }, false);
    element.dispatchEvent(pointer("pointerdown", 2, 40, 50));
    element.dispatchEvent(pointer("pointermove", 2, 90, 90));
    element.dispatchEvent(pointer("pointerup", 2, 90, 90));
    expect(onBoxSelect).toHaveBeenCalledWith({ x: 30, y: 30 }, { x: 80, y: 70 });
    controller.destroy();
  });

  it("turns a two-pointer gesture into pan and incremental zoom", () => {
    const element = document.createElement("div");
    vi.spyOn(element, "getBoundingClientRect").mockReturnValue({ x: 0, y: 0, left: 0, top: 0, right: 200, bottom: 100, width: 200, height: 100, toJSON: () => ({}) });
    const onPan = vi.fn();
    const onZoom = vi.fn();
    const controller = new TouchController(element, { onTap: vi.fn(), onBoxSelect: vi.fn(), onPan, onZoom });
    element.dispatchEvent(pointer("pointerdown", 1, 20, 20));
    element.dispatchEvent(pointer("pointerdown", 2, 60, 20));
    element.dispatchEvent(pointer("pointermove", 2, 80, 30));
    expect(onPan).toHaveBeenCalledWith({ x: 10, y: 5 });
    expect(onZoom.mock.calls[0][0]).toBeGreaterThan(1);
    controller.destroy();
  });

  it("supports desktop middle-drag pan and pointer-centered wheel zoom", () => {
    const element = document.createElement("div");
    vi.spyOn(element, "getBoundingClientRect").mockReturnValue({ x: 10, y: 20, left: 10, top: 20, right: 210, bottom: 120, width: 200, height: 100, toJSON: () => ({}) });
    const onPan = vi.fn();
    const onZoom = vi.fn();
    const controller = new TouchController(element, { onTap: vi.fn(), onBoxSelect: vi.fn(), onPan, onZoom });
    element.dispatchEvent(pointer("pointerdown", 1, 50, 60, 1, "mouse"));
    element.dispatchEvent(pointer("pointermove", 1, 70, 75, 1, "mouse"));
    element.dispatchEvent(pointer("pointerup", 1, 70, 75, 1, "mouse"));
    expect(onPan).toHaveBeenCalledWith({ x: 20, y: 15 });
    element.dispatchEvent(new WheelEvent("wheel", { bubbles: true, cancelable: true, clientX: 110, clientY: 70, deltaY: -100 }));
    expect(onZoom.mock.calls[0][0]).toBeGreaterThan(1);
    expect(onZoom.mock.calls[0][1]).toEqual({ x: 100, y: 50 });
    controller.destroy();
  });

  it("reports passive pointer movement for tool previews", () => {
    const element = document.createElement("div");
    vi.spyOn(element, "getBoundingClientRect").mockReturnValue({ x: 10, y: 20, left: 10, top: 20, right: 210, bottom: 120, width: 200, height: 100, toJSON: () => ({}) });
    const onHover = vi.fn();
    const controller = new TouchController(element, { onTap: vi.fn(), onHover, onBoxSelect: vi.fn(), onPan: vi.fn(), onZoom: vi.fn() });

    element.dispatchEvent(pointer("pointermove", 9, 42, 63, 0, "mouse"));

    expect(onHover).toHaveBeenCalledWith({ x: 32, y: 43 });
    controller.destroy();
  });

  it("does not turn a stationary middle click into a selection tap", () => {
    const element = document.createElement("div");
    vi.spyOn(element, "getBoundingClientRect").mockReturnValue({ x: 0, y: 0, left: 0, top: 0, right: 200, bottom: 100, width: 200, height: 100, toJSON: () => ({}) });
    const onTap = vi.fn();
    const onBoxCancel = vi.fn();
    const controller = new TouchController(element, { onTap, onBoxSelect: vi.fn(), onBoxCancel, onPan: vi.fn(), onZoom: vi.fn() });

    element.dispatchEvent(pointer("pointerdown", 1, 80, 50, 1, "mouse"));
    element.dispatchEvent(pointer("pointerup", 1, 80, 50, 1, "mouse"));

    expect(onTap).not.toHaveBeenCalled();
    expect(onBoxCancel).toHaveBeenCalledTimes(1);
    controller.destroy();
  });

  it("cancels selection previews for alternate drags and canceled pointers", () => {
    const element = document.createElement("div");
    element.tabIndex = 0;
    document.body.append(element);
    vi.spyOn(element, "getBoundingClientRect").mockReturnValue({ x: 0, y: 0, left: 0, top: 0, right: 200, bottom: 100, width: 200, height: 100, toJSON: () => ({}) });
    const onTap = vi.fn();
    const onBoxPreview = vi.fn();
    const onBoxCancel = vi.fn();
    const controller = new TouchController(element, { onTap, onBoxSelect: vi.fn(), onBoxPreview, onBoxCancel, onPan: vi.fn(), onZoom: vi.fn() });

    element.dispatchEvent(pointer("pointerdown", 1, 20, 20, 2, "mouse"));
    element.dispatchEvent(pointer("pointermove", 1, 80, 60, 2, "mouse"));
    element.dispatchEvent(pointer("pointerup", 1, 80, 60, 2, "mouse"));
    expect(onBoxPreview).not.toHaveBeenCalled();
    expect(onBoxCancel).toHaveBeenCalledTimes(1);
    expect(onTap).toHaveBeenCalledWith({ x: 80, y: 60 }, true);
    expect(document.activeElement).toBe(element);

    element.dispatchEvent(pointer("pointerdown", 2, 20, 20));
    element.dispatchEvent(pointer("pointermove", 2, 80, 60));
    expect(onBoxPreview).toHaveBeenCalledTimes(1);
    element.dispatchEvent(pointer("pointercancel", 2, 80, 60));
    expect(onBoxCancel).toHaveBeenCalledTimes(2);
    controller.destroy();
    element.remove();
  });

  it("consumes every pointer participating in a multi-touch gesture", () => {
    const element = document.createElement("div");
    vi.spyOn(element, "getBoundingClientRect").mockReturnValue({ x: 0, y: 0, left: 0, top: 0, right: 200, bottom: 100, width: 200, height: 100, toJSON: () => ({}) });
    const onTap = vi.fn();
    const onBoxCancel = vi.fn();
    const onBoxPreview = vi.fn();
    const controller = new TouchController(element, { onTap, onBoxSelect: vi.fn(), onBoxPreview, onBoxCancel, onPan: vi.fn(), onZoom: vi.fn() });
    element.dispatchEvent(pointer("pointerdown", 1, 20, 20));
    element.dispatchEvent(pointer("pointerdown", 2, 60, 20));
    element.dispatchEvent(pointer("pointerdown", 3, 100, 20));
    element.dispatchEvent(pointer("pointerup", 3, 100, 20));
    element.dispatchEvent(pointer("pointerup", 2, 60, 20));
    element.dispatchEvent(pointer("pointermove", 1, 90, 70));
    element.dispatchEvent(pointer("pointerup", 1, 20, 20));
    expect(onTap).not.toHaveBeenCalled();
    expect(onBoxPreview).not.toHaveBeenCalled();
    expect(onBoxCancel).toHaveBeenCalledTimes(1);
    controller.destroy();
  });
});
