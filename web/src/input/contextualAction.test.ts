import { describe, expect, it } from "vitest";
import { SnapshotCloakState, SnapshotContextualAction, SnapshotObjectType } from "../simulation/snapshot";
import {
  EXPLORE_CONTEXTUAL_ACTION,
  SELECT_UNITS_CONTEXTUAL_ACTION,
  contextualObjectIdentity,
  findNativeContextualTarget,
  isContextualObjectVisibleToPlayer,
  resolveContextualAction,
  type ContextualActionInput,
  type ContextualOccupierLookup,
  type ContextualRootObject,
} from "./contextualAction";

function actions(action: SnapshotContextualAction, house = 2): SnapshotContextualAction[] {
  const values = Array<SnapshotContextualAction>(32).fill(SnapshotContextualAction.None);
  values[house] = action;
  return values;
}

function object(overrides: Partial<ContextualRootObject> = {}): ContextualRootObject {
  return {
    type: SnapshotObjectType.Unit,
    id: 1,
    x: 10,
    y: 20,
    width: 8,
    height: 10,
    drawFlags: 0,
    sortOrder: 1,
    altitude: 0,
    fixedWing: false,
    centerCoordX: 128,
    centerCoordY: 128,
    owner: 1,
    cloak: SnapshotCloakState.Uncloaked,
    actionWithSelected: actions(SnapshotContextualAction.Attack),
    selectable: true,
    ...overrides,
  };
}

function occupiers(cells: Readonly<Record<string, readonly ContextualRootObject[]>>): ContextualOccupierLookup {
  return {
    forEachAtMapCell(mapCellX, mapCellY, visitor) {
      for (const entry of cells[`${mapCellX},${mapCellY}`] ?? []) visitor(entry.type, entry.id);
    },
  };
}

function nativeTarget(
  point: { x: number; y: number },
  roots: readonly ContextualRootObject[],
  cells: Readonly<Record<string, readonly ContextualRootObject[]>>,
  playerAllyFlags = 0,
): ContextualRootObject | undefined {
  return findNativeContextualTarget({
    point,
    rootObjects: roots,
    objectsByIdentity: new Map(roots.map((entry) => [contextualObjectIdentity(entry.type, entry.id), entry])),
    occupiers: occupiers(cells),
    map: { originalCellX: 0, originalCellY: 0, originalWidth: 10, originalHeight: 10 },
    playerHouse: 2,
    playerAllyFlags,
  });
}

function input(overrides: Partial<ContextualActionInput> = {}): ContextualActionInput {
  return {
    point: { x: 12, y: 22 },
    rootObjects: [],
    playerHouse: 2,
    cellVisible: true,
    terrainAction: SnapshotContextualAction.Move,
    ...overrides,
  };
}

describe("contextual action presentation", () => {
  it.each([
    [SnapshotContextualAction.Attack, "Attack", "attack", "crosshair"],
    [SnapshotContextualAction.AttackOutOfRange, "Attack · approach", "attack", "crosshair"],
    [SnapshotContextualAction.Move, "Move", "move", "move"],
    [SnapshotContextualAction.NoMove, "Cannot move", "blocked", "not-allowed"],
    [SnapshotContextualAction.Select, "Select", "select", "pointer"],
    [SnapshotContextualAction.Enter, "Enter", "interact", "pointer"],
    [SnapshotContextualAction.Capture, "Capture", "interact", "pointer"],
    [SnapshotContextualAction.Sabotage, "Sabotage", "attack", "crosshair"],
    [SnapshotContextualAction.Guard, "Guard", "support", "pointer"],
    [SnapshotContextualAction.Self, "Deploy", "interact", "pointer"],
    [SnapshotContextualAction.Repair, "Repair", "support", "cell"],
    [SnapshotContextualAction.None, "No action", "blocked", "not-allowed"],
    [SnapshotContextualAction.CannotDeploy, "Deployment blocked", "blocked", "not-allowed"],
    [SnapshotContextualAction.CannotRepair, "Repair blocked", "blocked", "not-allowed"],
  ] as const)("maps action %s to stable player presentation", (action, label, tone, cursor) => {
    expect(resolveContextualAction(input({ terrainAction: action }))).toEqual({
      action,
      source: "terrain",
      label,
      tone,
      cursor,
    });
  });

  it("prefers the topmost visible object and does not fall through its blocked action", () => {
    const bottom = object({ sortOrder: 10, actionWithSelected: actions(SnapshotContextualAction.Attack) });
    const top = object({ sortOrder: 20, actionWithSelected: actions(SnapshotContextualAction.Capture) });
    expect(resolveContextualAction(input({ rootObjects: [bottom, top] }))).toMatchObject({
      action: SnapshotContextualAction.Capture,
      source: "object",
      label: "Capture",
    });

    const blockedTop = object({ sortOrder: 30, actionWithSelected: actions(SnapshotContextualAction.None) });
    expect(resolveContextualAction(input({ rootObjects: [bottom, blockedTop], terrainAction: SnapshotContextualAction.Move }))).toMatchObject({
      action: SnapshotContextualAction.None,
      source: "object",
      tone: "blocked",
    });
  });

  it("uses the later render record when overlapping objects have equal sort order", () => {
    expect(resolveContextualAction(input({
      rootObjects: [
        object({ sortOrder: 5, actionWithSelected: actions(SnapshotContextualAction.Guard) }),
        object({ sortOrder: 5, actionWithSelected: actions(SnapshotContextualAction.Repair) }),
      ],
    }))).toMatchObject({ action: SnapshotContextualAction.Repair, source: "object" });
  });

  it("falls back to terrain when no root record is hit", () => {
    expect(resolveContextualAction(input({
      point: { x: 100, y: 100 },
      rootObjects: [object()],
      terrainAction: SnapshotContextualAction.NoMove,
    }))).toMatchObject({ action: SnapshotContextualAction.NoMove, source: "terrain", label: "Cannot move" });
  });

  it.each([
    [SnapshotContextualAction.Attack, false],
    [SnapshotContextualAction.None, true],
  ] as const)("treats a fully cloaked non-player object as absent for action %s", (objectAction, selectObjectWhenIdle) => {
    expect(resolveContextualAction(input({
      rootObjects: [object({
        owner: 1,
        cloak: SnapshotCloakState.Cloaked,
        actionWithSelected: actions(objectAction),
      })],
      selectObjectWhenIdle,
      terrainAction: SnapshotContextualAction.Move,
    }))).toEqual({
      action: SnapshotContextualAction.Move,
      source: "terrain",
      label: "Move",
      tone: "move",
      cursor: "move",
    });
  });

  it("keeps player-owned cloaks and partial cloak transitions hoverable", () => {
    expect(isContextualObjectVisibleToPlayer(
      object({ owner: 2, cloak: SnapshotCloakState.Cloaked }),
      2,
    )).toBe(true);
    expect(isContextualObjectVisibleToPlayer(
      object({ owner: 1, cloak: SnapshotCloakState.Cloaking }),
      2,
    )).toBe(true);
    expect(isContextualObjectVisibleToPlayer(
      object({ owner: 1, cloak: SnapshotCloakState.Uncloaking }),
      2,
    )).toBe(true);
    expect(isContextualObjectVisibleToPlayer(
      object({ owner: 2, cloak: SnapshotCloakState.Cloaked }),
      -1,
    )).toBe(false);
    expect(isContextualObjectVisibleToPlayer(
      object({ owner: 40, cloak: SnapshotCloakState.Cloaked }),
      40,
    )).toBe(false);
    expect(isContextualObjectVisibleToPlayer(
      object({ owner: 1, cloak: SnapshotCloakState.Cloaked }),
      2,
      1 << 1,
    )).toBe(true);
  });

  it("mirrors native occupier order and building-cell distance instead of sprite stacking", () => {
    const current = object({ type: SnapshotObjectType.Building, id: 10, centerCoordX: 2_000, centerCoordY: 2_000 });
    const left = object({ type: SnapshotObjectType.Building, id: 11, centerCoordX: 512, centerCoordY: 512 });
    // At the boundary both occupied cell centers are 128 leptons away. Native
    // scanning starts with the current cell and keeps the first equal distance.
    expect(nativeTarget({ x: 48, y: 60 }, [left, current], {
      "2,2": [current],
      "1,2": [left],
    })).toBe(current);
  });

  it("chooses the closest adjacent non-building center and enforces the quarter-cell radius", () => {
    const far = object({ id: 20, centerCoordX: 900, centerCoordY: 640 });
    const close = object({ id: 21, centerCoordX: 650, centerCoordY: 640 });
    expect(nativeTarget({ x: 60, y: 60 }, [far, close], { "2,2": [far], "3,2": [close] })).toBe(close);
    expect(nativeTarget({ x: 60, y: 60 }, [far], { "2,2": [far] })).toBeUndefined();
  });

  it("uses the same world-pixel rounding as the dispatched command at the 192-lepton cutoff", () => {
    const edge = object({ id: 22, centerCoordX: 448, centerCoordY: 256 });
    expect(nativeTarget({ x: 23.6, y: 24 }, [edge], { "1,1": [edge] })).toBe(edge);
    expect(nativeTarget({ x: 23.49, y: 24 }, [edge], { "0,1": [edge] })).toBeUndefined();
  });

  it("uses the native Dragon-Strike diagonal metric at 192 and 193 leptons", () => {
    const atCutoff = object({ id: 23, centerCoordX: 768, centerCoordY: 768 });
    const pastCutoff = object({ id: 24, centerCoordX: 769, centerCoordY: 768 });
    expect(nativeTarget({ x: 60, y: 60 }, [atCutoff], { "2,2": [atCutoff] })).toBe(atCutoff);
    expect(nativeTarget({ x: 60, y: 60 }, [pastCutoff], { "2,2": [pastCutoff] })).toBeUndefined();
  });

  it("targets airborne aircraft at their altitude-adjusted center", () => {
    const aircraft = object({
      type: SnapshotObjectType.Aircraft,
      id: 30,
      altitude: 256,
      centerCoordX: 640,
      centerCoordY: 896,
    });
    expect(nativeTarget({ x: 60, y: 60 }, [aircraft], {})).toBe(aircraft);

    const landingHelicopter = object({
      type: SnapshotObjectType.Aircraft,
      id: 31,
      altitude: 170,
      centerCoordX: 640,
      centerCoordY: 810,
    });
    expect(nativeTarget({ x: 60, y: 60 }, [landingHelicopter], {})).toBeUndefined();

    const fixedWing = object({
      type: SnapshotObjectType.Aircraft,
      id: 32,
      fixedWing: true,
      centerCoordX: 640,
      centerCoordY: 640,
    });
    expect(nativeTarget({ x: 60, y: 60 }, [fixedWing], {})).toBe(fixedWing);
  });

  it("keeps the ground candidate when an airborne aircraft has equal distance", () => {
    const ground = object({ id: 33, centerCoordX: 768, centerCoordY: 640 });
    const aircraft = object({
      type: SnapshotObjectType.Aircraft,
      id: 34,
      fixedWing: true,
      centerCoordX: 768,
      centerCoordY: 640,
    });
    expect(nativeTarget({ x: 60, y: 60 }, [ground, aircraft], { "2,2": [ground] })).toBe(ground);
  });

  it("excludes enemy cloaks but permits player-allied cloaks during native picking", () => {
    const cloaked = object({ id: 40, owner: 1, cloak: SnapshotCloakState.Cloaked, centerCoordX: 640, centerCoordY: 640 });
    expect(nativeTarget({ x: 60, y: 60 }, [cloaked], { "2,2": [cloaked] })).toBeUndefined();
    expect(nativeTarget({ x: 60, y: 60 }, [cloaked], { "2,2": [cloaked] }, 1 << 1)).toBe(cloaked);
  });

  it("does not scan the occupier grid's expanded border outside original map bounds", () => {
    const border = object({ id: 41, centerCoordX: 2_688, centerCoordY: 1_408 });
    expect(nativeTarget({ x: 239, y: 132 }, [border], { "10,5": [border] })).toBeUndefined();
  });

  it("treats a native terrain result as authoritative over legacy sprite bounds", () => {
    expect(resolveContextualAction(input({
      targetObject: null,
      rootObjects: [object()],
      terrainAction: SnapshotContextualAction.Move,
    }))).toMatchObject({ action: SnapshotContextualAction.Move, source: "terrain" });
  });

  it("presents Select for an idle selection cursor only on selectable objects", () => {
    expect(resolveContextualAction(input({
      rootObjects: [object({ actionWithSelected: actions(SnapshotContextualAction.None) })],
      selectObjectWhenIdle: true,
    }))).toMatchObject({ action: SnapshotContextualAction.Select, source: "object", label: "Select" });
    expect(resolveContextualAction(input({
      rootObjects: [object({ actionWithSelected: actions(SnapshotContextualAction.None), selectable: false })],
      selectObjectWhenIdle: true,
    }))).toMatchObject({ action: SnapshotContextualAction.None, source: "object", label: "No action" });
  });

  it("prompts for a selection without inspecting hidden battlefield state", () => {
    const hidden = {
      cellVisible: false,
      hasSelection: false,
      get point(): never { throw new Error("point leaked"); },
      get rootObjects(): never { throw new Error("objects leaked"); },
      get playerHouse(): never { throw new Error("player leaked"); },
      get terrainAction(): never { throw new Error("terrain leaked"); },
    } as ContextualActionInput;
    expect(resolveContextualAction(hidden)).toEqual({
      action: SELECT_UNITS_CONTEXTUAL_ACTION,
      source: "selection",
      label: "Select units first",
      tone: "blocked",
      cursor: "not-allowed",
    });
  });

  it("prompts before visible terrain orders but still describes an idle selectable object", () => {
    expect(resolveContextualAction(input({ hasSelection: false }))).toMatchObject({
      action: SELECT_UNITS_CONTEXTUAL_ACTION,
      source: "selection",
      label: "Select units first",
    });
    expect(resolveContextualAction(input({
      hasSelection: false,
      rootObjects: [object({ actionWithSelected: actions(SnapshotContextualAction.None) })],
      selectObjectWhenIdle: true,
    }))).toMatchObject({ action: SnapshotContextualAction.Select, source: "object", label: "Select" });
  });

  it("mirrors top-left, centered, and bottom-anchored render bounds", () => {
    const attack = actions(SnapshotContextualAction.Attack);
    const resolveAt = (point: { x: number; y: number }, target: ContextualRootObject) => resolveContextualAction(input({
      point,
      rootObjects: [target],
      terrainAction: SnapshotContextualAction.Move,
    })).source;

    const topLeft = object({ x: 10, y: 20, width: 8, height: 10, drawFlags: 0, actionWithSelected: attack });
    expect(resolveAt({ x: 10, y: 20 }, topLeft)).toBe("object");
    expect(resolveAt({ x: 17.999, y: 29.999 }, topLeft)).toBe("object");
    expect(resolveAt({ x: 18, y: 25 }, topLeft)).toBe("terrain");
    expect(resolveAt({ x: 14, y: 30 }, topLeft)).toBe("terrain");

    const centered = object({ x: 10, y: 20, width: 8, height: 10, drawFlags: 0x20, actionWithSelected: attack });
    expect(resolveAt({ x: 6, y: 15 }, centered)).toBe("object");
    expect(resolveAt({ x: 14, y: 20 }, centered)).toBe("terrain");

    const bottom = object({ x: 10, y: 20, width: 8, height: 10, drawFlags: 0x40, actionWithSelected: attack });
    expect(resolveAt({ x: 10, y: 10 }, bottom)).toBe("object");
    expect(resolveAt({ x: 12, y: 20 }, bottom)).toBe("terrain");

    const minimumSize = object({ x: 4, y: 5, width: 0, height: -2, drawFlags: 0, actionWithSelected: attack });
    expect(resolveAt({ x: 4.5, y: 5.5 }, minimumSize)).toBe("object");
  });

  it("returns Explore before inspecting any hidden-cell state", () => {
    const hidden = {
      cellVisible: false,
      get point(): never { throw new Error("point leaked"); },
      get rootObjects(): never { throw new Error("objects leaked"); },
      get playerHouse(): never { throw new Error("player leaked"); },
      get terrainAction(): never { throw new Error("terrain leaked"); },
    } as ContextualActionInput;
    expect(resolveContextualAction(hidden)).toEqual({
      action: EXPLORE_CONTEXTUAL_ACTION,
      source: "explore",
      label: "Explore",
      tone: "explore",
      cursor: "move",
    });
  });

  it("fails closed when the player has no action slot", () => {
    expect(resolveContextualAction(input({
      playerHouse: 40,
      rootObjects: [object({ actionWithSelected: actions(SnapshotContextualAction.Attack) })],
    }))).toMatchObject({ action: SnapshotContextualAction.None, source: "object", tone: "blocked" });
  });
});
