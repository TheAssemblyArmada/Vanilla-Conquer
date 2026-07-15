import {
  CommandType,
  ControlGroupRequest,
  GameRequest,
  InputRequest,
  SidebarRequest,
  StructureRequest,
  SuperweaponRequest,
  UnitRequest,
  type SimulationCommand,
} from "../simulation/protocol";
import type { ScreenPoint } from "./TouchController";

export type InteractionMode = "select" | "order";

/** Stable identity copied verbatim from a SidebarV1 entry. */
export interface SidebarIdentity {
  buildableType: number;
  buildableId: number;
}

export interface MapCellPoint {
  x: number;
  y: number;
}

const EMPTY_ARGS = [0, 0, 0, 0, 0, 0, 0] as const;
const SIGNED_INT32_MIN = -0x8000_0000;
const SIGNED_INT32_MAX = 0x7fff_ffff;
const SIGNED_INT16_MIN = -0x8000;
const SIGNED_INT16_MAX = 0x7fff;
// Mirrors WorldPointCanNormalize in the TD adapter, leaving conversion
// headroom for its current camera and tactical-window offset.
const WORLD_COORDINATE_LIMIT = 0x3fff_ffff;

function signedInt32(value: number, label: string): number {
  if (!Number.isSafeInteger(value) || value < SIGNED_INT32_MIN || value > SIGNED_INT32_MAX) {
    throw new RangeError(`${label} must be a signed 32-bit integer`);
  }
  return value;
}

function roundedInRange(value: number, minimum: number, maximum: number, label: string): number {
  if (!Number.isFinite(value)) throw new RangeError(`${label} must be finite`);
  const rounded = Math.round(value);
  if (!Number.isSafeInteger(rounded) || rounded < minimum || rounded > maximum) {
    throw new RangeError(`${label} is outside the supported range`);
  }
  return rounded === 0 ? 0 : rounded;
}

/** Integer world pixel the engine will receive for a pointer coordinate. */
export function canonicalWorldCoordinate(value: number, label = "World coordinate"): number {
  return roundedInRange(value, -WORLD_COORDINATE_LIMIT, WORLD_COORDINATE_LIMIT, label);
}

function worldPoint(point: ScreenPoint, label: string): [number, number] {
  return [
    canonicalWorldCoordinate(point.x, `${label} x`),
    canonicalWorldCoordinate(point.y, `${label} y`),
  ];
}

function mapCell(point: MapCellPoint): [number, number] {
  return [
    roundedInRange(point.x, SIGNED_INT16_MIN, SIGNED_INT16_MAX, "Placement cell x"),
    roundedInRange(point.y, SIGNED_INT16_MIN, SIGNED_INT16_MAX, "Placement cell y"),
  ];
}

function sidebarIdentity(identity: SidebarIdentity): [number, number] {
  return [
    signedInt32(identity.buildableType, "Sidebar buildable type"),
    signedInt32(identity.buildableId, "Sidebar buildable ID"),
  ];
}

function sidebarCommand(request: SidebarRequest, identity: SidebarIdentity, cell?: MapCellPoint): SimulationCommand {
  const [buildableType, buildableId] = sidebarIdentity(identity);
  const [cellX, cellY] = cell ? mapCell(cell) : [0, 0];
  return { type: CommandType.Sidebar, args: [request, buildableType, buildableId, cellX, cellY, 0, 0] };
}

function structureCommand(request: StructureRequest, objectId = 0): SimulationCommand {
  return {
    type: CommandType.Structure,
    args: [request, signedInt32(objectId, "Structure object ID"), 0, 0, 0, 0, 0],
  };
}

function controlGroupIndex(index: number): number {
  if (!Number.isInteger(index) || index < 0 || index > 9) {
    throw new RangeError("Control group index must be an integer from 0 through 9");
  }
  return index;
}

export function pointCommand(mode: InteractionMode, point: ScreenPoint, alternate = false): SimulationCommand {
  const request = alternate || mode === "order" ? InputRequest.CommandAtPosition : InputRequest.SelectAtPosition;
  const [x, y] = worldPoint(point, "World point");
  return { type: CommandType.Input, args: [request, x, y, 0, 0, 0, 0] };
}

export function boxSelectCommand(start: ScreenPoint, end: ScreenPoint): SimulationCommand {
  const [startX, startY] = worldPoint(start, "Selection start");
  const [endX, endY] = worldPoint(end, "Selection end");
  return { type: CommandType.Input, args: [InputRequest.MouseArea, startX, startY, endX, endY, 0, 0] };
}

export function stopSelectedCommand(): SimulationCommand {
  return { type: CommandType.Unit, args: [UnitRequest.Stop, ...EMPTY_ARGS.slice(1)] as [number, number, number, number, number, number, number] };
}

/** Replaces a legacy control group with the currently selected mobile units. */
export function createControlGroupCommand(index: number): SimulationCommand {
  return { type: CommandType.ControlGroup, args: [ControlGroupRequest.Create, controlGroupIndex(index), 0, 0, 0, 0, 0] };
}

/** Selects a legacy control group, optionally preserving the existing selection. */
export function selectControlGroupCommand(index: number, additive = false): SimulationCommand {
  return {
    type: CommandType.ControlGroup,
    args: [additive ? ControlGroupRequest.AdditiveSelection : ControlGroupRequest.Toggle, controlGroupIndex(index), 0, 0, 0, 0, 0],
  };
}

/** Acknowledges an omitted movie so the legacy engine can leave its presentation wait state. */
export function movieDoneCommand(): SimulationCommand {
  return { type: CommandType.Game, args: [GameRequest.MovieDone, ...EMPTY_ARGS.slice(1)] as [number, number, number, number, number, number, number] };
}

export function startProductionCommand(identity: SidebarIdentity): SimulationCommand {
  return sidebarCommand(SidebarRequest.StartConstruction, identity);
}

export function holdProductionCommand(identity: SidebarIdentity): SimulationCommand {
  return sidebarCommand(SidebarRequest.HoldConstruction, identity);
}

export function cancelProductionCommand(identity: SidebarIdentity): SimulationCommand {
  return sidebarCommand(SidebarRequest.CancelConstruction, identity);
}

export function startPlacementCommand(identity: SidebarIdentity): SimulationCommand {
  return sidebarCommand(SidebarRequest.StartPlacement, identity);
}

export function placeProductionCommand(identity: SidebarIdentity, cell: MapCellPoint): SimulationCommand {
  return sidebarCommand(SidebarRequest.Place, identity, cell);
}

export function cancelPlacementCommand(identity: SidebarIdentity): SimulationCommand {
  return sidebarCommand(SidebarRequest.CancelPlacement, identity);
}

export function startRepairCommand(): SimulationCommand {
  return structureCommand(StructureRequest.RepairStart);
}

export function repairStructureCommand(objectId: number): SimulationCommand {
  return structureCommand(StructureRequest.Repair, objectId);
}

export function startSellCommand(): SimulationCommand {
  return structureCommand(StructureRequest.SellStart);
}

export function sellStructureCommand(objectId: number): SimulationCommand {
  return structureCommand(StructureRequest.Sell, objectId);
}

export function cancelStructureActionCommand(): SimulationCommand {
  return structureCommand(StructureRequest.Cancel);
}

/** Sells a wall/overlay through the legacy position-based input path. */
export function sellWallAtWorldCommand(point: ScreenPoint): SimulationCommand {
  const [x, y] = worldPoint(point, "Wall world point");
  return { type: CommandType.Input, args: [InputRequest.SellAtPosition, x, y, 0, 0, 0, 0] };
}

/** Activates a sidebar superweapon at an absolute map-world position. */
export function targetSuperweaponCommand(identity: SidebarIdentity, point: ScreenPoint): SimulationCommand {
  const [buildableType, buildableId] = sidebarIdentity(identity);
  const [x, y] = worldPoint(point, "Superweapon world point");
  return {
    type: CommandType.Superweapon,
    args: [SuperweaponRequest.Place, buildableType, buildableId, x, y, 0, 0],
  };
}
