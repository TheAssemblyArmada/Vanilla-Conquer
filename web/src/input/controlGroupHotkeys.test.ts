import { describe, expect, it } from "vitest";
import { controlGroupHotkey } from "./controlGroupHotkeys";

describe("control-group hotkeys", () => {
  it("maps the visible digits to all ten zero-based engine groups", () => {
    expect(controlGroupHotkey({ code: "Digit1" })).toEqual({ index: 0, action: "select", focus: false });
    expect(controlGroupHotkey({ code: "Numpad9" })).toEqual({ index: 8, action: "select", focus: false });
    expect(controlGroupHotkey({ code: "Digit0" })).toEqual({ index: 9, action: "select", focus: false });
    expect(controlGroupHotkey({ code: "Key1" })).toBeUndefined();
  });

  it("matches classic Ctrl, Alt, and Shift behavior and priority", () => {
    expect(controlGroupHotkey({ code: "Digit2", shiftKey: true })).toEqual({ index: 1, action: "additive", focus: false });
    expect(controlGroupHotkey({ code: "Digit2", altKey: true, shiftKey: true })).toEqual({ index: 1, action: "select", focus: true });
    expect(controlGroupHotkey({ code: "Digit2", ctrlKey: true, altKey: true, shiftKey: true })).toEqual({ index: 1, action: "create", focus: false });
    expect(controlGroupHotkey({ code: "Digit2", metaKey: true })).toEqual({ index: 1, action: "create", focus: false });
  });
});
