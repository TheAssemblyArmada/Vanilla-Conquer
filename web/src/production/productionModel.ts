/** Pure production and host-tool state shared by the DOM controls and tests. */

export const TD_MAP_STRIDE = 128;

export const ProductionObjectType = {
  Special: 11,
  Infantry: 12,
  Unit: 13,
  Aircraft: 14,
  Building: 15,
  Vessel: 17,
} as const;

export interface ProductionEntryLike {
  assetName: string;
  buildableType: number;
  buildableId: number;
  objectType: number;
  progress: number;
  placementOffsets: readonly number[];
  completed: boolean;
  constructing: boolean;
  onHold: boolean;
  busy: boolean;
}

export type ProductionCategory = "infantry" | "vehicle" | "aircraft" | "structure" | "special" | "unknown";
export type ProductionStatus = "available" | "constructing" | "on-hold" | "ready" | "busy" | "unavailable";
export type ProductionPrimaryAction = "start" | "hold" | "resume" | "place" | "disabled";

export interface ProductionEntryDescription {
  key: string;
  category: ProductionCategory;
  label: string;
  status: ProductionStatus;
  progress: number;
  primaryAction: ProductionPrimaryAction;
}

function requireInteger(value: number, label: string): number {
  if (!Number.isSafeInteger(value)) throw new RangeError(`${label} must be an integer`);
  return value;
}

export function productionEntryKey(entry: Pick<ProductionEntryLike, "buildableType" | "buildableId">): string {
  return `production:${requireInteger(entry.buildableType, "buildableType")}:${requireInteger(entry.buildableId, "buildableId")}`;
}

export function productionEntryCategory(entry: Pick<ProductionEntryLike, "objectType">): ProductionCategory {
  switch (entry.objectType) {
    case ProductionObjectType.Infantry: return "infantry";
    case ProductionObjectType.Unit:
    case ProductionObjectType.Vessel: return "vehicle";
    case ProductionObjectType.Aircraft: return "aircraft";
    case ProductionObjectType.Building: return "structure";
    case ProductionObjectType.Special: return "special";
    default: return "unknown";
  }
}

const SPECIAL_LABELS: Readonly<Record<string, string>> = {
  SW_ION: "Ion Cannon",
  SW_NUKE: "Nuclear Strike",
  SW_AIRSTRIKE: "Air Strike",
};

const TD_ASSET_LABELS: Readonly<Record<string, string>> = {
  E1: "Minigunner",
  JEEP: "Humvee",
  NUKE: "Power Plant",
  FACT: "Construction Yard",
  PYLE: "Barracks",
};

export function productionEntryLabel(entry: Pick<ProductionEntryLike, "assetName" | "buildableId" | "objectType">): string {
  const asset = entry.assetName.split("\0", 1)[0].trim();
  const special = SPECIAL_LABELS[asset.replace(/[\s-]+/g, "_").toUpperCase()];
  if (special) return special;
  const classic = TD_ASSET_LABELS[asset.toUpperCase()];
  if (classic) return classic;
  if (asset) return asset.replace(/[_-]+/g, " ").replace(/\s+/g, " ");
  const category = productionEntryCategory(entry);
  const noun = category === "unknown" ? "Item" : category[0].toUpperCase() + category.slice(1);
  return `${noun} ${requireInteger(entry.buildableId, "buildableId")}`;
}

export function normalizeProductionProgress(value: number, completed = false): number {
  if (completed) return 1;
  if (!Number.isFinite(value)) return 0;
  return Math.min(1, Math.max(0, value));
}

export function describeProductionEntry(entry: ProductionEntryLike): ProductionEntryDescription {
  const key = productionEntryKey(entry);
  const category = productionEntryCategory(entry);
  let status: ProductionStatus;
  if (entry.completed) status = "ready";
  else if (entry.onHold) status = "on-hold";
  else if (entry.constructing) status = "constructing";
  else if (entry.busy) status = "busy";
  else if (category === "unknown" || category === "special") status = "unavailable";
  else status = "available";

  let primaryAction: ProductionPrimaryAction;
  if (status === "ready") primaryAction = category === "structure" ? "place" : "disabled";
  else if (status === "on-hold") primaryAction = "resume";
  else if (status === "constructing") primaryAction = category === "special" ? "disabled" : "hold";
  else if (status === "available") primaryAction = "start";
  else primaryAction = "disabled";

  const progress = status === "ready"
    ? 1
    : status === "constructing" || status === "on-hold"
      ? normalizeProductionProgress(entry.progress)
      : 0;
  return { key, category, label: productionEntryLabel(entry), status, progress, primaryAction };
}

export interface CellPoint { x: number; y: number }
export interface CellBounds extends CellPoint { width: number; height: number }

export interface PlacementSearchGrid<TEntry extends ProductionEntryLike> {
  width: number;
  height: number;
  canPlace(entry: TEntry, requestCellX: number, requestCellY: number): boolean;
}

/** Returns the first currently legal public-grid cell for an optional quick-place action. */
export function firstLegalPlacementCell<TEntry extends ProductionEntryLike>(
  grid: PlacementSearchGrid<TEntry>,
  entry: TEntry,
  startIndex = 0,
): CellPoint | undefined {
  const count = grid.width * grid.height;
  if (!Number.isSafeInteger(count) || count <= 0) return undefined;
  const start = ((Math.trunc(startIndex) % count) + count) % count;
  for (let offset = 0; offset < count; offset += 1) {
    const index = (start + offset) % count;
    const x = index % grid.width;
    const y = Math.floor(index / grid.width);
    if (grid.canPlace(entry, x, y)) return { x, y };
  }
  return undefined;
}

export interface PlacementGridGeometry extends CellBounds {
  stride: typeof TD_MAP_STRIDE;
  count: number;
}

/** Mirrors the one-cell expansion used by TD's exported placement grid. */
export function expandPlacementGrid(map: CellBounds): PlacementGridGeometry {
  let x = requireInteger(map.x, "map.x");
  let y = requireInteger(map.y, "map.y");
  let width = requireInteger(map.width, "map.width");
  let height = requireInteger(map.height, "map.height");
  if (x < 0 || y < 0 || x >= TD_MAP_STRIDE || y >= TD_MAP_STRIDE
    || width <= 0 || height <= 0 || width > TD_MAP_STRIDE || height > TD_MAP_STRIDE
    || x + width > TD_MAP_STRIDE || y + height > TD_MAP_STRIDE) {
    throw new RangeError("Map cell bounds are outside the 128 by 128 TD grid");
  }
  if (x > 0) { x -= 1; width += 1; }
  if (width < TD_MAP_STRIDE) width += 1;
  if (y > 0) { y -= 1; height += 1; }
  if (height < TD_MAP_STRIDE) height += 1;
  const count = width * height;
  if (!Number.isSafeInteger(count) || count > TD_MAP_STRIDE * TD_MAP_STRIDE) {
    throw new RangeError("Expanded placement grid is too large");
  }
  return { x, y, width, height, stride: TD_MAP_STRIDE, count };
}

/** Decodes TD's signed linear cell offset while keeping the small X delta. */
export function decodePlacementOffset(offset: number): CellPoint {
  requireInteger(offset, "placement offset");
  if (offset < -0x8000 || offset > 0x7fff) throw new RangeError("Placement offset must fit a signed 16-bit value");
  const y = Math.floor((offset + TD_MAP_STRIDE / 2) / TD_MAP_STRIDE);
  return { x: offset - y * TD_MAP_STRIDE, y };
}

export type PlacementRejection = "not-placeable" | "outside-grid" | "missing-footprint" | "no-proximity" | "footprint-outside" | "obstructed";

export interface PlacementFootprintCell extends CellPoint {
  index: number;
  clear: boolean;
}

export interface PlacementCandidate {
  anchor: CellPoint;
  commandCell: CellPoint;
  footprint: readonly PlacementFootprintCell[];
  proximity: boolean;
  legal: boolean;
  rejection?: PlacementRejection;
}

function gridIndex(grid: PlacementGridGeometry, cell: CellPoint): number | undefined {
  if (!Number.isInteger(cell.x) || !Number.isInteger(cell.y)
    || cell.x < 0 || cell.y < 0 || cell.x >= grid.stride || cell.y >= grid.stride
    || cell.x < grid.x || cell.y < grid.y || cell.x >= grid.x + grid.width || cell.y >= grid.y + grid.height) return undefined;
  return (cell.y - grid.y) * grid.width + cell.x - grid.x;
}

export function evaluatePlacementCandidate(
  grid: PlacementGridGeometry,
  flags: ArrayLike<number>,
  entry: ProductionEntryLike,
  anchor: CellPoint,
): PlacementCandidate {
  if (flags.length !== grid.count) throw new RangeError("Placement flags do not match the expanded grid");
  const commandCell = { x: anchor.x - grid.x, y: anchor.y - grid.y };
  const rejected = (rejection: PlacementRejection, proximity = false, footprint: readonly PlacementFootprintCell[] = []): PlacementCandidate => ({
    anchor: { x: anchor.x, y: anchor.y }, commandCell, footprint, proximity, legal: false, rejection,
  });
  if (describeProductionEntry(entry).primaryAction !== "place") return rejected("not-placeable");
  const anchorIndex = gridIndex(grid, anchor);
  if (anchorIndex === undefined) return rejected("outside-grid");
  if (entry.placementOffsets.length === 0) return rejected("missing-footprint");
  const proximity = (flags[anchorIndex] & 1) !== 0;
  if (!proximity) return rejected("no-proximity");

  const footprint: PlacementFootprintCell[] = [];
  const seen = new Set<number>();
  for (const rawOffset of entry.placementOffsets) {
    const offset = decodePlacementOffset(rawOffset);
    const cell = { x: anchor.x + offset.x, y: anchor.y + offset.y };
    const index = gridIndex(grid, cell);
    if (index === undefined) return rejected("footprint-outside", proximity, footprint);
    if (seen.has(index)) continue;
    seen.add(index);
    footprint.push({ ...cell, index, clear: (flags[index] & 2) !== 0 });
  }
  if (footprint.some((cell) => !cell.clear)) return rejected("obstructed", proximity, footprint);
  return { anchor: { x: anchor.x, y: anchor.y }, commandCell, footprint, proximity, legal: true };
}

export type HostBaseTool = "select" | "order" | "repair" | "sell";
export type HostToolState =
  | { mode: HostBaseTool }
  | { mode: "placement"; entryKey: string; buildableType: number; buildableId: number; phase: "requesting" | "active" };

export type HostToolAction =
  | { type: "activate"; mode: HostBaseTool }
  | { type: "begin-placement"; entry: ProductionEntryLike }
  | { type: "cancel" }
  | { type: "reconcile"; entries: readonly ProductionEntryLike[]; placementActive: boolean; repairEnabled: boolean; sellEnabled: boolean };

export function initialHostToolState(): HostToolState { return { mode: "select" }; }

export function reduceHostTool(state: HostToolState, action: HostToolAction): HostToolState {
  switch (action.type) {
    case "activate":
      return state.mode === action.mode && action.mode !== "select" ? initialHostToolState() : { mode: action.mode };
    case "begin-placement": {
      const description = describeProductionEntry(action.entry);
      if (description.primaryAction !== "place") return state;
      return {
        mode: "placement",
        entryKey: description.key,
        buildableType: action.entry.buildableType,
        buildableId: action.entry.buildableId,
        phase: "requesting",
      };
    }
    case "cancel":
      return initialHostToolState();
    case "reconcile": {
      if (state.mode === "repair") return action.repairEnabled ? state : initialHostToolState();
      if (state.mode === "sell") return action.sellEnabled ? state : initialHostToolState();
      if (state.mode !== "placement") return state;
      const entry = action.entries.find((candidate) => productionEntryKey(candidate) === state.entryKey);
      if (!entry || describeProductionEntry(entry).primaryAction !== "place") return initialHostToolState();
      if (action.placementActive) return { ...state, phase: "active" };
      return state.phase === "active" ? initialHostToolState() : state;
    }
  }
}

export function leavesPlacementTool(previous: HostToolState, next: HostToolState): boolean {
  return previous.mode === "placement"
    && (next.mode !== "placement" || next.entryKey !== previous.entryKey);
}
