import {
  SnapshotCloakState,
  SnapshotContextualAction,
  SnapshotObjectType,
} from "../simulation/snapshot";
import { canonicalWorldCoordinate } from "./gameCommands";

export interface ContextualWorldPoint {
  x: number;
  y: number;
}

/** A root render record. Child/subobject records must be removed by the caller. */
export interface ContextualRootObject {
  type: SnapshotObjectType;
  id: number;
  x: number;
  y: number;
  width: number;
  height: number;
  drawFlags: number;
  sortOrder: number;
  altitude: number;
  fixedWing: boolean;
  centerCoordX: number;
  centerCoordY: number;
  owner: number;
  cloak: SnapshotCloakState;
  actionWithSelected: readonly SnapshotContextualAction[];
  selectable?: boolean;
}

export type ContextualActionTone = "attack" | "move" | "select" | "interact" | "support" | "blocked" | "explore";
export type ContextualActionCursor = "crosshair" | "move" | "pointer" | "cell" | "not-allowed";
export type ContextualActionSource = "object" | "terrain" | "explore" | "selection";
export const EXPLORE_CONTEXTUAL_ACTION = "explore" as const;
export const SELECT_UNITS_CONTEXTUAL_ACTION = "select-units" as const;

export interface ContextualActionPresentation {
  label: string;
  tone: ContextualActionTone;
  cursor: ContextualActionCursor;
}

export interface ResolvedContextualAction extends ContextualActionPresentation {
  action: SnapshotContextualAction | typeof EXPLORE_CONTEXTUAL_ACTION | typeof SELECT_UNITS_CONTEXTUAL_ACTION;
  source: ContextualActionSource;
}

export interface ContextualActionInput {
  point: ContextualWorldPoint;
  rootObjects: readonly ContextualRootObject[];
  /** null means native occupier resolution ran and found terrain. */
  targetObject?: ContextualRootObject | null;
  playerHouse: number;
  playerAllyFlags?: number;
  cellVisible: boolean;
  terrainAction: SnapshotContextualAction;
  /** Player-visible selection state; omitted callers retain legacy behavior. */
  hasSelection?: boolean;
  /** Lets an idle selection cursor describe a selectable object honestly. */
  selectObjectWhenIdle?: boolean;
}

export interface ContextualMapBounds {
  originalCellX: number;
  originalCellY: number;
  originalWidth: number;
  originalHeight: number;
}

export interface ContextualOccupierLookup {
  forEachAtMapCell(
    mapCellX: number,
    mapCellY: number,
    visitor: (type: SnapshotObjectType, id: number) => void,
  ): void;
}

export interface NativeContextualTargetInput {
  point: ContextualWorldPoint;
  rootObjects: readonly ContextualRootObject[];
  objectsByIdentity: ReadonlyMap<number, ContextualRootObject>;
  occupiers: ContextualOccupierLookup;
  map: ContextualMapBounds;
  playerHouse: number;
  playerAllyFlags?: number;
}

const ACTION_PRESENTATIONS: Readonly<Record<SnapshotContextualAction, ContextualActionPresentation>> = Object.freeze({
  [SnapshotContextualAction.None]: { label: "No action", tone: "blocked", cursor: "not-allowed" },
  [SnapshotContextualAction.Move]: { label: "Move", tone: "move", cursor: "move" },
  [SnapshotContextualAction.NoMove]: { label: "Cannot move", tone: "blocked", cursor: "not-allowed" },
  [SnapshotContextualAction.Enter]: { label: "Enter", tone: "interact", cursor: "pointer" },
  [SnapshotContextualAction.Self]: { label: "Deploy", tone: "interact", cursor: "pointer" },
  [SnapshotContextualAction.Attack]: { label: "Attack", tone: "attack", cursor: "crosshair" },
  [SnapshotContextualAction.AttackOutOfRange]: { label: "Attack · approach", tone: "attack", cursor: "crosshair" },
  [SnapshotContextualAction.Guard]: { label: "Guard", tone: "support", cursor: "pointer" },
  [SnapshotContextualAction.Select]: { label: "Select", tone: "select", cursor: "pointer" },
  [SnapshotContextualAction.Capture]: { label: "Capture", tone: "interact", cursor: "pointer" },
  [SnapshotContextualAction.Sabotage]: { label: "Sabotage", tone: "attack", cursor: "crosshair" },
  [SnapshotContextualAction.Heal]: { label: "Heal", tone: "support", cursor: "cell" },
  [SnapshotContextualAction.Damage]: { label: "Damage", tone: "attack", cursor: "crosshair" },
  [SnapshotContextualAction.TogglePrimary]: { label: "Make primary", tone: "interact", cursor: "pointer" },
  [SnapshotContextualAction.CannotDeploy]: { label: "Deployment blocked", tone: "blocked", cursor: "not-allowed" },
  [SnapshotContextualAction.Repair]: { label: "Repair", tone: "support", cursor: "cell" },
  [SnapshotContextualAction.CannotRepair]: { label: "Repair blocked", tone: "blocked", cursor: "not-allowed" },
});

const EXPLORE_PRESENTATION: ContextualActionPresentation = Object.freeze({
  label: "Explore",
  tone: "explore",
  cursor: "move",
});

const SELECT_UNITS_PRESENTATION: ContextualActionPresentation = Object.freeze({
  label: "Select units first",
  tone: "blocked",
  cursor: "not-allowed",
});

function presentationFor(action: SnapshotContextualAction): ContextualActionPresentation {
  return ACTION_PRESENTATIONS[action] ?? ACTION_PRESENTATIONS[SnapshotContextualAction.None];
}

function pointInRenderedBounds(object: ContextualRootObject, point: ContextualWorldPoint): boolean {
  const width = Math.max(1, object.width);
  const height = Math.max(1, object.height);
  let left = object.x;
  let top = object.y;
  if (object.drawFlags & 0x20) {
    left -= width / 2;
    top -= height / 2;
  } else if (object.drawFlags & 0x40) {
    top -= height;
  }
  return point.x >= left && point.x < left + width && point.y >= top && point.y < top + height;
}

/**
 * Mirrors the native hover visibility rule for completely cloaked objects.
 * Ownership or player-side alliance must be positively known before a fully
 * cloaked record can participate, so invalid house data fails closed.
 */
export function isContextualObjectVisibleToPlayer(
  object: Pick<ContextualRootObject, "owner" | "cloak">,
  playerHouse: number,
  playerAllyFlags = 0,
): boolean {
  if (object.cloak !== SnapshotCloakState.Cloaked) return true;
  if (!Number.isInteger(playerHouse) || playerHouse < 0 || playerHouse >= 32
    || !Number.isInteger(object.owner) || object.owner < 0 || object.owner >= 32) return false;
  return object.owner === playerHouse || ((playerAllyFlags >>> object.owner) & 1) !== 0;
}

export function contextualObjectIdentity(type: SnapshotObjectType, id: number): number {
  return type * 0x1_0000_0000 + (id >>> 0);
}

function pixelToLepton(value: number): number {
  const pixel = canonicalWorldCoordinate(value);
  return Math.trunc((pixel * 256 + 12 - (pixel < 0 ? 23 : 0)) / 24);
}

function nativeDistance(x1: number, y1: number, x2: number, y2: number): number {
  const dx = Math.abs(x1 - x2);
  const dy = Math.abs(y1 - y2);
  return Math.max(dx, dy) + Math.floor(Math.min(dx, dy) / 2);
}

function airborne(object: ContextualRootObject): boolean {
  // AircraftClass::In_Which_Layer: fixed-wing is always top; helicopters
  // enter the top layer at 16 pixels (171 leptons after native rounding).
  return object.fixedWing || object.altitude >= 171;
}

const CLOSE_OBJECT_CELL_OFFSETS = [
  [0, 0], [-1, 0], [1, 0], [0, -1], [0, 1], [-1, 1], [1, 1], [1, -1], [-1, -1],
] as const;

/**
 * Mirrors MapClass::Close_Object: current/adjacent cell occupiers in native
 * list order, building-cell centers, then airborne aircraft. Only root
 * records already admitted by the caller's visibility policy can resolve.
 */
export function findNativeContextualTarget(input: NativeContextualTargetInput): ContextualRootObject | undefined {
  if (!Number.isFinite(input.point.x) || !Number.isFinite(input.point.y)) return undefined;
  const coordX = pixelToLepton(input.point.x);
  const coordY = pixelToLepton(input.point.y);
  const cellX = Math.floor(coordX / 256);
  const cellY = Math.floor(coordY / 256);
  const left = input.map.originalCellX;
  const top = input.map.originalCellY;
  const right = left + input.map.originalWidth;
  const bottom = top + input.map.originalHeight;
  let closest: ContextualRootObject | undefined;
  let closestDistance = 0;
  let scanX = 0;
  let scanY = 0;
  const visitOccupier = (type: SnapshotObjectType, id: number) => {
    const object = input.objectsByIdentity.get(contextualObjectIdentity(type, id));
    if (!object || !isContextualObjectVisibleToPlayer(object, input.playerHouse, input.playerAllyFlags)
      || (object.type === SnapshotObjectType.Aircraft && airborne(object))) return;
    const distance = object.type === SnapshotObjectType.Building
      ? nativeDistance(coordX, coordY, scanX * 256 + 128, scanY * 256 + 128)
      : nativeDistance(coordX, coordY, object.centerCoordX, object.centerCoordY);
    if (distance <= 0xc0 && (!closest || distance < closestDistance)) {
      closest = object;
      closestDistance = distance;
    }
  };
  for (const [offsetX, offsetY] of CLOSE_OBJECT_CELL_OFFSETS) {
    const x = cellX + offsetX;
    const y = cellY + offsetY;
    if (x < left || y < top || x >= right || y >= bottom) continue;
    scanX = x;
    scanY = y;
    input.occupiers.forEachAtMapCell(x, y, visitOccupier);
  }

  for (const aircraft of input.rootObjects) {
    if (aircraft.type !== SnapshotObjectType.Aircraft || !airborne(aircraft)
      || !isContextualObjectVisibleToPlayer(aircraft, input.playerHouse, input.playerAllyFlags)) continue;
    const distance = nativeDistance(
      coordX,
      coordY,
      aircraft.centerCoordX,
      aircraft.centerCoordY - aircraft.altitude,
    );
    if (distance <= 0xc0 && (!closest || distance < closestDistance)) {
      closest = aircraft;
      closestDistance = distance;
    }
  }
  return closest;
}

/**
 * Resolves only information a player can act on. An unrevealed cell exits
 * before point, object, player, or terrain fields are inspected, preventing a
 * hover/cursor path from becoming a hidden-state oracle.
 */
export function resolveContextualAction(input: ContextualActionInput): ResolvedContextualAction {
  if (!input.cellVisible) {
    if (input.hasSelection === false) {
      return { action: SELECT_UNITS_CONTEXTUAL_ACTION, source: "selection", ...SELECT_UNITS_PRESENTATION };
    }
    return { action: EXPLORE_CONTEXTUAL_ACTION, source: "explore", ...EXPLORE_PRESENTATION };
  }

  const point = input.point;
  let topmost = input.targetObject ?? undefined;
  if (input.targetObject === undefined && Number.isFinite(point.x) && Number.isFinite(point.y)) {
    for (const object of input.rootObjects) {
      if (!isContextualObjectVisibleToPlayer(object, input.playerHouse, input.playerAllyFlags)) continue;
      if (!pointInRenderedBounds(object, point)) continue;
      // Equal sort order follows render/input order: the later record is on top.
      if (!topmost || object.sortOrder >= topmost.sortOrder) topmost = object;
    }
  }

  if (topmost) {
    if (input.hasSelection === false && !(input.selectObjectWhenIdle && topmost.selectable)) {
      return { action: SELECT_UNITS_CONTEXTUAL_ACTION, source: "selection", ...SELECT_UNITS_PRESENTATION };
    }
    let action = Number.isInteger(input.playerHouse) && input.playerHouse >= 0
      ? topmost.actionWithSelected[input.playerHouse] ?? SnapshotContextualAction.None
      : SnapshotContextualAction.None;
    if (action === SnapshotContextualAction.None && input.selectObjectWhenIdle && topmost.selectable) {
      action = SnapshotContextualAction.Select;
    }
    return { action, source: "object", ...presentationFor(action) };
  }

  if (input.hasSelection === false) {
    return { action: SELECT_UNITS_CONTEXTUAL_ACTION, source: "selection", ...SELECT_UNITS_PRESENTATION };
  }
  const action = input.terrainAction;
  return { action, source: "terrain", ...presentationFor(action) };
}
