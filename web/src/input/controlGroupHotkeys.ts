export interface ControlGroupHotkeyLike {
  code: string;
  ctrlKey?: boolean;
  metaKey?: boolean;
  altKey?: boolean;
  shiftKey?: boolean;
}

export interface ControlGroupHotkeyAction {
  index: number;
  action: "create" | "select" | "additive";
  focus: boolean;
}

/** Decodes both keyboard rows while preserving the original TD modifier priority. */
export function controlGroupHotkey(event: ControlGroupHotkeyLike): ControlGroupHotkeyAction | undefined {
  const match = /^(?:Digit|Numpad)([0-9])$/.exec(event.code);
  if (!match) return undefined;
  const digit = Number(match[1]);
  const index = digit === 0 ? 9 : digit - 1;
  if (event.ctrlKey || event.metaKey) return { index, action: "create", focus: false };
  if (event.altKey) return { index, action: "select", focus: true };
  if (event.shiftKey) return { index, action: "additive", focus: false };
  return { index, action: "select", focus: false };
}
