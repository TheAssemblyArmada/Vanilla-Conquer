import { CNC_WEB_MAGIC_MESSAGE, MessageKind, SIMULATION_PROTOCOL_VERSION } from "./protocol";

export const SNAPSHOT_HEADER_BYTES = 40;
export const SECTION_HEADER_BYTES = 16;
export const OBJECT_RECORD_BYTES = 472;
export const SIDEBAR_RECORD_BYTES = 128;
export const MAP_CELL_PIXELS = 24;
export const MAP_CELL_STRIDE = 128;
const STATIC_MAP_FIXED_BYTES = 304;
const STATIC_CELL_RECORD_BYTES = 36;
const DYNAMIC_MAP_FIXED_BYTES = 20;
const DYNAMIC_MAP_RECORD_BYTES = 48;
const SIDEBAR_FIXED_BYTES = 60;
const PLAYER_FIXED_BYTES = 504;
const MAX_MAP_CELLS = MAP_CELL_STRIDE * MAP_CELL_STRIDE;
const CLASSIC_FULL_HEADER_BYTES = 16;
const CLASSIC_DIRTY_HEADER_BYTES = 32;
const CLASSIC_MAXIMUM_WIDTH = 128 * 24;
const CLASSIC_MAXIMUM_HEIGHT = 128 * 24;

export enum SnapshotSectionKind {
  StaticMap = 1,
  DynamicMap = 2,
  Objects = 3,
  Sidebar = 4,
  Placement = 5,
  Shroud = 6,
  Occupiers = 7,
  Player = 8,
  ClassicSurface = 9,
  Palette = 10,
  Camera = 11,
}

export enum SpriteFlags {
  None = 0,
  Selected = 1 << 0,
  Shadow = 1 << 1,
  Translucent = 1 << 2,
  FlipX = 1 << 3,
  FlipY = 1 << 4,
}

export enum SnapshotObjectType {
  Unknown = 0,
  Infantry = 1,
  Unit = 2,
  Aircraft = 3,
  Building = 4,
  Terrain = 5,
  Animation = 6,
  Bullet = 7,
  Overlay = 8,
  Smudge = 9,
  Object = 10,
  Special = 11,
  InfantryType = 12,
  UnitType = 13,
  AircraftType = 14,
  BuildingType = 15,
  Vessel = 16,
  VesselType = 17,
}

/** Legacy TechnoClass cloak states exported on each object record. */
export enum SnapshotCloakState {
  Uncloaked = 0,
  Cloaking = 1,
  Cloaked = 2,
  Uncloaking = 3,
}

/** Contextual cursor/action values exported by the original TD selection logic. */
export enum SnapshotContextualAction {
  None = 0,
  Move = 1,
  NoMove = 2,
  Enter = 3,
  Self = 4,
  Attack = 5,
  AttackOutOfRange = 6,
  Guard = 7,
  Select = 8,
  Capture = 9,
  Sabotage = 10,
  Heal = 11,
  Damage = 12,
  TogglePrimary = 13,
  CannotDeploy = 14,
  Repair = 15,
  CannotRepair = 16,
}

export interface SnapshotSprite {
  x: number;
  y: number;
  width: number;
  height: number;
  u0: number;
  v0: number;
  u1: number;
  v1: number;
  atlasPage: number;
  flags: number;
  sortKey: number;
  tint: number;
  teamColor: number;
}

export interface SnapshotSidebar {
  leftEntries: number;
  rightEntries: number;
  credits: number;
  creditsCounter: number;
  tiberium: number;
  maxTiberium: number;
  powerProduced: number;
  powerDrained: number;
  missionTimer: number;
  unitsKilled: number;
  buildingsKilled: number;
  unitsLost: number;
  buildingsLost: number;
  harvestedCredits: number;
  repairEnabled: boolean;
  sellEnabled: boolean;
  radarActive: boolean;
  entries: readonly SnapshotSidebarEntry[];
  leftColumn: readonly SnapshotSidebarEntry[];
  rightColumn: readonly SnapshotSidebarEntry[];
}

export interface SnapshotSidebarEntry {
  column: 0 | 1;
  columnIndex: number;
  assetName: string;
  buildableType: number;
  buildableId: number;
  objectType: SnapshotObjectType;
  superweaponType: number;
  cost: number;
  powerDelta: number;
  buildTime: number;
  progress: number;
  placementOffsets: readonly number[];
  completed: boolean;
  constructing: boolean;
  onHold: boolean;
  busy: boolean;
  viaCapture: boolean;
  fake: boolean;
}

export interface SnapshotStaticMap {
  cellX: number;
  cellY: number;
  width: number;
  height: number;
  originalCellX: number;
  originalCellY: number;
  originalWidth: number;
  originalHeight: number;
  theater: number;
  scenarioName: string;
  cellCount: number;
  /** True when the immutable cell records are retained from the prior full snapshot. */
  retained: boolean;
}

export interface SnapshotPlayer {
  name: string;
  house: number;
  homeCellX: number;
  homeCellY: number;
  colorIndex: number;
  playerId: bigint;
  team: number;
  startLocation: number;
  ai: boolean;
  defeated: boolean;
  radarJammed: boolean;
  /** Houses this player currently treats as allies, using one bit per house. */
  allyFlags: number;
  actions: SnapshotContextualActionGrid;
}

/** Contextual action values for the scenario's unexpanded, original map bounds. */
export class SnapshotContextualActionGrid {
  readonly originCellX: number;
  readonly originCellY: number;
  readonly width: number;
  readonly height: number;
  readonly count: number;
  private readonly values: Uint8Array;

  constructor(originCellX: number, originCellY: number, width: number, height: number, values: Uint8Array) {
    this.originCellX = originCellX;
    this.originCellY = originCellY;
    this.width = width;
    this.height = height;
    this.count = values.length;
    this.values = values.slice();
  }

  atMapCell(mapCellX: number, mapCellY: number): SnapshotContextualAction | undefined {
    if (!Number.isInteger(mapCellX) || !Number.isInteger(mapCellY)) return undefined;
    const x = mapCellX - this.originCellX;
    const y = mapCellY - this.originCellY;
    if (x < 0 || y < 0 || x >= this.width || y >= this.height) return undefined;
    const index = y * this.width + x;
    if (index >= this.count) return undefined;
    return this.values[index] as SnapshotContextualAction;
  }

  atWorldPoint(worldX: number, worldY: number): SnapshotContextualAction | undefined {
    return this.atMapCell(Math.floor(worldX / MAP_CELL_PIXELS), Math.floor(worldY / MAP_CELL_PIXELS));
  }
}

export interface SnapshotShroudCell {
  readonly mapCellX: number;
  readonly mapCellY: number;
  readonly shadow: number;
  readonly visible: boolean;
  readonly mapped: boolean;
  readonly jammed: boolean;
}

/** Immutable shroud values for the expanded static-map bounds. */
export class SnapshotShroudGrid {
  readonly originCellX: number;
  readonly originCellY: number;
  readonly width: number;
  readonly height: number;
  readonly count: number;
  private readonly shadows: Int8Array;
  private readonly flags: Uint8Array;

  constructor(originCellX: number, originCellY: number, width: number, height: number, entries: Uint8Array) {
    this.originCellX = originCellX;
    this.originCellY = originCellY;
    this.width = width;
    this.height = height;
    this.count = entries.length / 2;
    this.shadows = new Int8Array(this.count);
    this.flags = new Uint8Array(this.count);
    for (let index = 0; index < this.count; index += 1) {
      this.shadows[index] = (entries[index * 2] << 24) >> 24;
      this.flags[index] = entries[index * 2 + 1];
    }
  }

  cellAtMapCell(mapCellX: number, mapCellY: number): SnapshotShroudCell | undefined {
    if (!Number.isInteger(mapCellX) || !Number.isInteger(mapCellY)) return undefined;
    const x = mapCellX - this.originCellX;
    const y = mapCellY - this.originCellY;
    if (x < 0 || y < 0 || x >= this.width || y >= this.height) return undefined;
    const index = y * this.width + x;
    const flags = this.flags[index];
    return {
      mapCellX,
      mapCellY,
      shadow: this.shadows[index],
      visible: Boolean(flags & 1),
      mapped: Boolean(flags & 2),
      jammed: Boolean(flags & 4),
    };
  }

  cellAtWorldPoint(worldX: number, worldY: number): SnapshotShroudCell | undefined {
    return this.cellAtMapCell(Math.floor(worldX / MAP_CELL_PIXELS), Math.floor(worldY / MAP_CELL_PIXELS));
  }

  isVisibleAtMapCell(mapCellX: number, mapCellY: number): boolean {
    if (!Number.isInteger(mapCellX) || !Number.isInteger(mapCellY)) return false;
    const x = mapCellX - this.originCellX;
    const y = mapCellY - this.originCellY;
    return x >= 0 && y >= 0 && x < this.width && y < this.height
      && Boolean(this.flags[y * this.width + x] & 1);
  }

  isVisibleAtWorldPoint(worldX: number, worldY: number): boolean {
    return this.isVisibleAtMapCell(Math.floor(worldX / MAP_CELL_PIXELS), Math.floor(worldY / MAP_CELL_PIXELS));
  }
}

export interface SnapshotOccupier {
  readonly type: SnapshotObjectType;
  readonly id: number;
}

/**
 * Cell-linked object identities in the exact order exported by Cell_Occupier.
 * The visitor API avoids allocating short-lived arrays during pointer hover.
 */
export class SnapshotOccupierGrid {
  readonly originCellX: number;
  readonly originCellY: number;
  readonly width: number;
  readonly height: number;
  readonly count: number;
  private readonly data: DataView;
  private readonly cellOffsets: Uint32Array;

  constructor(
    originCellX: number,
    originCellY: number,
    width: number,
    height: number,
    data: DataView,
    cellOffsets: Uint32Array,
  ) {
    this.originCellX = originCellX;
    this.originCellY = originCellY;
    this.width = width;
    this.height = height;
    this.count = cellOffsets.length;
    this.data = data;
    this.cellOffsets = cellOffsets;
  }

  forEachAtMapCell(
    mapCellX: number,
    mapCellY: number,
    visitor: (type: SnapshotObjectType, id: number) => void,
  ): void {
    if (!Number.isInteger(mapCellX) || !Number.isInteger(mapCellY)) return;
    const x = mapCellX - this.originCellX;
    const y = mapCellY - this.originCellY;
    if (x < 0 || y < 0 || x >= this.width || y >= this.height) return;
    let offset = this.cellOffsets[y * this.width + x];
    const count = this.data.getUint32(offset, true);
    offset += 4;
    for (let index = 0; index < count; index += 1, offset += 8) {
      visitor(this.data.getInt32(offset, true) as SnapshotObjectType, this.data.getInt32(offset + 4, true));
    }
  }

  atMapCell(mapCellX: number, mapCellY: number): SnapshotOccupier[] {
    const occupiers: SnapshotOccupier[] = [];
    this.forEachAtMapCell(mapCellX, mapCellY, (type, id) => occupiers.push({ type, id }));
    return occupiers;
  }
}

export interface SnapshotPlacementCell {
  requestCellX: number;
  requestCellY: number;
  mapCellX: number;
  mapCellY: number;
  passesProximityCheck: boolean;
  generallyClear: boolean;
}

export class SnapshotPlacementGrid {
  readonly originCellX: number;
  readonly originCellY: number;
  readonly width: number;
  readonly height: number;
  readonly count: number;
  private readonly flags: Uint8Array;

  constructor(originCellX: number, originCellY: number, width: number, height: number, flags: Uint8Array) {
    this.originCellX = originCellX;
    this.originCellY = originCellY;
    this.width = width;
    this.height = height;
    this.count = flags.length;
    this.flags = flags.slice();
  }

  cell(requestCellX: number, requestCellY: number): SnapshotPlacementCell | undefined {
    if (!Number.isInteger(requestCellX) || !Number.isInteger(requestCellY)
      || requestCellX < 0 || requestCellY < 0 || requestCellX >= this.width || requestCellY >= this.height) return undefined;
    const flags = this.flags[requestCellY * this.width + requestCellX];
    return {
      requestCellX,
      requestCellY,
      mapCellX: this.originCellX + requestCellX,
      mapCellY: this.originCellY + requestCellY,
      passesProximityCheck: Boolean(flags & 1),
      generallyClear: Boolean(flags & 2),
    };
  }

  cellAtMapCell(mapCellX: number, mapCellY: number): SnapshotPlacementCell | undefined {
    return this.cell(mapCellX - this.originCellX, mapCellY - this.originCellY);
  }

  canPlace(entry: SnapshotSidebarEntry, requestCellX: number, requestCellY: number): boolean {
    const anchor = this.cell(requestCellX, requestCellY);
    if (!anchor?.passesProximityCheck) return false;
    const anchorIndex = anchor.mapCellY * MAP_CELL_STRIDE + anchor.mapCellX;
    const offsets = entry.placementOffsets.length > 0 ? entry.placementOffsets : [0];
    return offsets.every((offset) => {
      const cellIndex = anchorIndex + offset;
      if (cellIndex < 0 || cellIndex >= MAX_MAP_CELLS) return false;
      return this.cellAtMapCell(cellIndex % MAP_CELL_STRIDE, Math.floor(cellIndex / MAP_CELL_STRIDE))?.generallyClear === true;
    });
  }
}

export interface SnapshotObject {
  index: number;
  typeName: string;
  assetName: string;
  type: SnapshotObjectType;
  id: number;
  baseId: number;
  baseType: SnapshotObjectType;
  x: number;
  y: number;
  width: number;
  height: number;
  /** Aircraft altitude in legacy leptons. */
  altitude: number;
  sortOrder: number;
  drawFlags: number;
  maxStrength: number;
  strength: number;
  cellX: number;
  cellY: number;
  /** Native Center_Coord components in legacy leptons. */
  centerCoordX: number;
  centerCoordY: number;
  owner: number;
  subObject: number;
  cloak: SnapshotCloakState;
  /** Zero-based legacy group, or undefined when this object is unassigned. */
  controlGroup: number | undefined;
  selectedMask: number;
  visibleFlags: number;
  actionWithSelected: readonly SnapshotContextualAction[];
  occupyOffsets: readonly number[];
  selectable: boolean;
  repairing: boolean;
  canRepair: boolean;
  canDemolish: boolean;
  factory: boolean;
  primaryFactory: boolean;
  fixedWing: boolean;
  root: boolean;
}

export interface SnapshotDynamicMapEntry {
  index: number;
  assetName: string;
  x: number;
  y: number;
  width: number;
  height: number;
  drawFlags: number;
  type: number;
  owner: number;
  cellX: number;
  cellY: number;
  overlay: boolean;
  sellable: boolean;
}

export interface BuildingHitTestOptions {
  owner?: number;
  capability?: "repair" | "sell";
}

export type SnapshotSellTarget =
  | { kind: "building"; building: SnapshotObject }
  | { kind: "wall"; wall: SnapshotDynamicMapEntry };

export interface SnapshotClassicSurface {
  /** 1 is a complete baseline; 2 is a dirty-rectangle update. */
  format: 1 | 2;
  width: number;
  height: number;
  rectX: number;
  rectY: number;
  rectWidth: number;
  rectHeight: number;
  /** Tightly packed rows. Wire-format row padding, if present, is removed. */
  pixels: Uint8Array;
}

export interface SnapshotLayout {
  tick: number;
  worldWidth: number;
  worldHeight: number;
  classicWidth: number;
  classicHeight: number;
  classicPixels: Uint8Array;
  /** Browser RGBA palette. It is converted to the engine wire RGB triplets. */
  palette: Uint8Array;
  sprites: readonly SnapshotSprite[];
  cameraX: number;
  cameraY: number;
  zoom: number;
}

interface SnapshotSection {
  kind: SnapshotSectionKind;
  flags: number;
  count: number;
  offset: number;
  length: number;
}

const utf8Decoder = new TextDecoder("utf-8", { fatal: true });

function fixedString(buffer: ArrayBuffer, offset: number, length: number, label: string): string {
  const bytes = new Uint8Array(buffer, offset, length);
  const terminator = bytes.indexOf(0);
  const textLength = terminator < 0 ? length : terminator;
  if (terminator >= 0 && bytes.subarray(terminator).some((value) => value !== 0)) {
    throw new Error(`${label} has nonzero bytes after its terminator`);
  }
  try {
    return utf8Decoder.decode(bytes.subarray(0, textLength));
  } catch {
    throw new Error(`${label} is not valid UTF-8`);
  }
}

function renderedBounds(x: number, y: number, width: number, height: number, drawFlags: number) {
  let left = x;
  let top = y;
  if (drawFlags & 0x20) {
    left -= width / 2;
    top -= height / 2;
  } else if (drawFlags & 0x40) {
    top -= height;
  }
  return { left, top, right: left + width, bottom: top + height };
}

function pointInRenderedBounds(
  x: number,
  y: number,
  width: number,
  height: number,
  drawFlags: number,
  worldX: number,
  worldY: number,
): boolean {
  const bounds = renderedBounds(x, y, Math.max(1, width), Math.max(1, height), drawFlags);
  return worldX >= bounds.left && worldX < bounds.right && worldY >= bounds.top && worldY < bounds.bottom;
}

function checkedEnd(offset: number, length: number, total: number, label: string): number {
  const end = offset + length;
  if (!Number.isSafeInteger(end) || offset < 0 || length < 0 || end > total) throw new Error(`${label} range is outside the snapshot`);
  return end;
}

function packedColor(red: number, green: number, blue: number, alpha = 255): number {
  return (red | (green << 8) | (blue << 16) | (alpha << 24)) >>> 0;
}

function objectTint(owner: number, remap: number): number {
  const colors = [
    packedColor(201, 133, 84),
    packedColor(104, 164, 110),
    packedColor(202, 167, 122),
    packedColor(181, 145, 91),
    packedColor(188, 105, 100),
    packedColor(149, 119, 173),
    packedColor(99, 143, 179),
    packedColor(202, 190, 105),
  ];
  return colors[(remap === 0xff ? owner : remap) & 7];
}

export class SnapshotView {
  readonly buffer: ArrayBuffer;
  readonly byteLength: number;
  readonly tick: number;
  readonly baseTick: number;
  readonly stateHash: bigint;
  readonly flags: number;
  readonly terminal: boolean;
  /** True when at least one section must be applied to the preceding materialized snapshot. */
  readonly requiresBaseline: boolean;
  readonly cameraX: number;
  readonly cameraY: number;
  readonly zoom = 1;
  readonly worldWidth: number;
  readonly worldHeight: number;
  readonly classicWidth: number;
  readonly classicHeight: number;
  readonly classicFormat: 0 | 1 | 2;
  readonly classicRectX: number;
  readonly classicRectY: number;
  readonly classicRectWidth: number;
  readonly classicRectHeight: number;
  readonly classicOriginX: number;
  readonly classicOriginY: number;
  readonly spriteCount: number;
  readonly staticMap: SnapshotStaticMap | undefined;
  readonly player: SnapshotPlayer | undefined;
  readonly placement: SnapshotPlacementGrid | undefined;
  readonly shroud: SnapshotShroudGrid | undefined;
  readonly occupiers: SnapshotOccupierGrid | undefined;

  private readonly data: DataView;
  private readonly sections = new Map<SnapshotSectionKind, SnapshotSection>();
  private classicCache?: SnapshotClassicSurface;
  private paletteCache?: Uint8Array;
  private sidebarCache?: SnapshotSidebar;
  private objectCache?: Array<SnapshotObject | undefined>;
  private dynamicMapCache?: SnapshotDynamicMapEntry[];

  constructor(buffer: ArrayBuffer) {
    if (buffer.byteLength < SNAPSHOT_HEADER_BYTES) throw new Error("Snapshot header is truncated");
    this.buffer = buffer;
    this.data = new DataView(buffer);
    if (this.data.getUint32(0, true) !== CNC_WEB_MAGIC_MESSAGE) throw new Error("Snapshot CNCW magic is invalid");
    if (this.data.getUint16(4, true) !== SIMULATION_PROTOCOL_VERSION || this.data.getUint16(6, true) !== MessageKind.Snapshot) throw new Error("Snapshot protocol is unsupported");
    this.byteLength = this.data.getUint32(8, true);
    if (this.byteLength < SNAPSHOT_HEADER_BYTES || this.byteLength > buffer.byteLength) throw new Error("Snapshot byte length is invalid");
    const headerCount = this.data.getUint32(12, true);
    this.tick = this.data.getUint32(16, true);
    this.baseTick = this.data.getUint32(20, true);
    this.stateHash = this.data.getBigUint64(24, true);
    const sectionCount = this.data.getUint32(32, true);
    this.flags = this.data.getUint32(36, true);
    this.terminal = Boolean(this.flags & 1);
    if (sectionCount !== headerCount || sectionCount > 64) throw new Error("Snapshot section count is invalid");

    let offset = SNAPSHOT_HEADER_BYTES;
    for (let index = 0; index < sectionCount; index += 1) {
      checkedEnd(offset, SECTION_HEADER_BYTES, this.byteLength, "Section header");
      const kind = this.data.getUint16(offset, true) as SnapshotSectionKind;
      const flags = this.data.getUint16(offset + 2, true);
      const length = this.data.getUint32(offset + 4, true);
      const count = this.data.getUint32(offset + 8, true);
      if (this.data.getUint32(offset + 12, true) !== 0) throw new Error("Snapshot section reserved field is not zero");
      const payloadOffset = offset + SECTION_HEADER_BYTES;
      offset = checkedEnd(payloadOffset, length, this.byteLength, "Snapshot section");
      if (kind < SnapshotSectionKind.StaticMap || kind > SnapshotSectionKind.Camera || this.sections.has(kind)) throw new Error("Snapshot section kind is invalid or duplicated");
      this.sections.set(kind, { kind, flags, count, offset: payloadOffset, length });
    }
    if (offset !== this.byteLength) throw new Error("Snapshot contains trailing data inside its declared length");

    const classic = this.sections.get(SnapshotSectionKind.ClassicSurface);
    if (classic) {
      if (classic.length < CLASSIC_FULL_HEADER_BYTES) throw new Error("Classic surface header is truncated");
      this.classicWidth = this.data.getUint32(classic.offset, true);
      this.classicHeight = this.data.getUint32(classic.offset + 4, true);
      const pitch = this.data.getUint32(classic.offset + 8, true);
      const format = this.data.getUint32(classic.offset + 12, true);
      if (this.classicWidth === 0 || this.classicHeight === 0
        || this.classicWidth > CLASSIC_MAXIMUM_WIDTH || this.classicHeight > CLASSIC_MAXIMUM_HEIGHT) throw new Error("Classic surface dimensions are invalid");
      if (format === 1) {
        const pixelBytes = classic.length - CLASSIC_FULL_HEADER_BYTES;
        if (pitch < this.classicWidth || pitch * this.classicHeight !== pixelBytes || classic.count !== pixelBytes) throw new Error("Classic surface layout is invalid");
        this.classicFormat = 1;
        this.classicRectX = 0;
        this.classicRectY = 0;
        this.classicRectWidth = this.classicWidth;
        this.classicRectHeight = this.classicHeight;
      } else if (format === 2) {
        if (classic.length < CLASSIC_DIRTY_HEADER_BYTES) throw new Error("Classic dirty surface header is truncated");
        this.classicRectX = this.data.getUint32(classic.offset + 16, true);
        this.classicRectY = this.data.getUint32(classic.offset + 20, true);
        this.classicRectWidth = this.data.getUint32(classic.offset + 24, true);
        this.classicRectHeight = this.data.getUint32(classic.offset + 28, true);
        const pixelBytes = classic.length - CLASSIC_DIRTY_HEADER_BYTES;
        const empty = this.classicRectWidth === 0 && this.classicRectHeight === 0;
        const rectangleValid = empty
          ? pitch === 0 && pixelBytes === 0 && classic.count === 0
            && this.classicRectX <= this.classicWidth && this.classicRectY <= this.classicHeight
          : this.classicRectWidth > 0 && this.classicRectHeight > 0
            && this.classicRectX + this.classicRectWidth <= this.classicWidth
            && this.classicRectY + this.classicRectHeight <= this.classicHeight
            && pitch >= this.classicRectWidth
            && pitch * this.classicRectHeight === pixelBytes
            && classic.count === pixelBytes;
        if (!rectangleValid) throw new Error("Classic dirty surface layout is invalid");
        this.classicFormat = 2;
      } else {
        throw new Error("Classic surface format is unsupported");
      }
    } else {
      this.classicWidth = 0;
      this.classicHeight = 0;
      this.classicFormat = 0;
      this.classicRectX = 0;
      this.classicRectY = 0;
      this.classicRectWidth = 0;
      this.classicRectHeight = 0;
    }

    const staticMapSection = this.sections.get(SnapshotSectionKind.StaticMap);
    this.staticMap = staticMapSection ? this.readStaticMap(staticMapSection) : undefined;
    if (this.staticMap) {
      // The indexed surface begins at the scenario's original map bounds.
      this.classicOriginX = this.staticMap.originalCellX * MAP_CELL_PIXELS;
      this.classicOriginY = this.staticMap.originalCellY * MAP_CELL_PIXELS;
    } else {
      this.classicOriginX = 0;
      this.classicOriginY = 0;
    }

    const objects = this.sections.get(SnapshotSectionKind.Objects);
    if (objects) this.validateObjects(objects);
    this.spriteCount = objects?.count ?? 0;
    const palette = this.sections.get(SnapshotSectionKind.Palette);
    if (palette && (palette.length !== 256 * 3 || palette.count !== 256)) throw new Error("Palette section layout is invalid");
    const sidebar = this.sections.get(SnapshotSectionKind.Sidebar);
    this.sidebarCache = sidebar ? this.readSidebar(sidebar) : undefined;

    const dynamicMap = this.sections.get(SnapshotSectionKind.DynamicMap);
    if (dynamicMap) this.validateDynamicMap(dynamicMap);

    const player = this.sections.get(SnapshotSectionKind.Player);
    this.player = player ? this.readPlayer(player) : undefined;

    const placement = this.sections.get(SnapshotSectionKind.Placement);
    this.placement = placement ? this.readPlacement(placement) : undefined;

    const shroud = this.sections.get(SnapshotSectionKind.Shroud);
    this.shroud = shroud ? this.readShroud(shroud) : undefined;

    const occupiers = this.sections.get(SnapshotSectionKind.Occupiers);
    this.occupiers = occupiers ? this.readOccupiers(occupiers) : undefined;

    const camera = this.sections.get(SnapshotSectionKind.Camera);
    if (camera) {
      if (camera.length !== 24 || camera.count !== 1) throw new Error("Camera section layout is invalid");
      this.cameraX = this.data.getInt32(camera.offset, true);
      this.cameraY = this.data.getInt32(camera.offset + 4, true);
      this.worldWidth = Math.max(1, this.data.getInt32(camera.offset + 8, true));
      this.worldHeight = Math.max(1, this.data.getInt32(camera.offset + 12, true));
    } else {
      this.cameraX = 0;
      this.cameraY = 0;
      this.worldWidth = Math.max(1, this.classicWidth);
      this.worldHeight = Math.max(1, this.classicHeight);
    }

    this.requiresBaseline = this.staticMap?.retained === true || this.classicFormat === 2;
  }

  private readStaticMap(section: SnapshotSection): SnapshotStaticMap {
    const fullLength = STATIC_MAP_FIXED_BYTES + section.count * STATIC_CELL_RECORD_BYTES;
    const retained = section.length === STATIC_MAP_FIXED_BYTES;
    if (section.flags !== 0 || section.count > MAX_MAP_CELLS
      || (!retained && section.length !== fullLength)) {
      throw new Error("Static map section layout is invalid");
    }
    const cellX = this.data.getInt32(section.offset, true);
    const cellY = this.data.getInt32(section.offset + 4, true);
    const width = this.data.getInt32(section.offset + 8, true);
    const height = this.data.getInt32(section.offset + 12, true);
    const originalCellX = this.data.getInt32(section.offset + 16, true);
    const originalCellY = this.data.getInt32(section.offset + 20, true);
    const originalWidth = this.data.getInt32(section.offset + 24, true);
    const originalHeight = this.data.getInt32(section.offset + 28, true);
    const theater = this.data.getInt32(section.offset + 32, true);
    const repeatedCount = this.data.getUint32(section.offset + 300, true);
    const validBounds = (x: number, y: number, w: number, h: number) => Number.isInteger(x) && Number.isInteger(y)
      && Number.isInteger(w) && Number.isInteger(h) && x >= 0 && y >= 0 && w > 0 && h > 0
      && w <= MAP_CELL_STRIDE && h <= MAP_CELL_STRIDE && x + w <= MAP_CELL_STRIDE && y + h <= MAP_CELL_STRIDE;
    if (!validBounds(cellX, cellY, width, height) || !validBounds(originalCellX, originalCellY, originalWidth, originalHeight)
      || width * height !== section.count || repeatedCount !== section.count || theater < -1 || theater > 3) {
      throw new Error("Static map metadata is invalid");
    }
    return {
      cellX,
      cellY,
      width,
      height,
      originalCellX,
      originalCellY,
      originalWidth,
      originalHeight,
      theater,
      scenarioName: fixedString(this.buffer, section.offset + 36, 264, "Static map scenario name"),
      cellCount: section.count,
      retained,
    };
  }

  private validateObjects(section: SnapshotSection): void {
    if (section.flags !== 0 || section.count > 100_000 || section.length !== section.count * OBJECT_RECORD_BYTES) {
      throw new Error("Object section layout is invalid");
    }
    const knownFlags = 0x00ff_ffff & ~((1 << 11) | (1 << 19));
    for (let index = 0; index < section.count; index += 1) {
      const offset = section.offset + index * OBJECT_RECORD_BYTES;
      const type = this.data.getInt32(offset + 112, true);
      const baseType = this.data.getInt32(offset + 124, true);
      const objectFlags = this.data.getUint32(offset + 204, true);
      const cloak = this.data.getUint8(offset + 185);
      const controlGroup = this.data.getUint8(offset + 186);
      let actionsValid = true;
      for (let house = 0; house < 32; house += 1) {
        if (this.data.getUint8(offset + 440 + house) > SnapshotContextualAction.CannotRepair) {
          actionsValid = false;
          break;
        }
      }
      if (this.data.getUint8(offset + 187) !== 0 || type < SnapshotObjectType.Unknown || type > SnapshotObjectType.VesselType
        || baseType < SnapshotObjectType.Unknown || baseType > SnapshotObjectType.VesselType || (objectFlags & ~knownFlags) !== 0
        || !actionsValid
        || cloak > SnapshotCloakState.Uncloaking
        || (controlGroup > 9 && controlGroup !== 0xff)
        || this.data.getUint16(offset + 216, true) > 36 || this.data.getUint16(offset + 218, true) > 18
        || this.data.getUint16(offset + 220, true) > 18 || this.data.getUint16(offset + 222, true) > 3) {
        throw new Error("Object record layout is invalid");
      }
    }
  }

  private readSidebar(section: SnapshotSection): SnapshotSidebar {
    if (section.flags !== 0 || section.count > 4096
      || section.length !== SIDEBAR_FIXED_BYTES + section.count * SIDEBAR_RECORD_BYTES) {
      throw new Error("Sidebar section layout is invalid");
    }
    const leftEntries = this.data.getInt32(section.offset, true);
    const rightEntries = this.data.getInt32(section.offset + 4, true);
    const flags = this.data.getUint32(section.offset + 56, true);
    if (leftEntries < 0 || rightEntries < 0 || leftEntries + rightEntries !== section.count || (flags & ~7) !== 0) {
      throw new Error("Sidebar entry counts or flags are invalid");
    }
    const entries: SnapshotSidebarEntry[] = [];
    for (let index = 0; index < section.count; index += 1) {
      const offset = section.offset + SIDEBAR_FIXED_BYTES + index * SIDEBAR_RECORD_BYTES;
      const objectType = this.data.getInt32(offset + 24, true);
      const progress = this.data.getFloat32(offset + 44, true);
      const placementCount = this.data.getUint32(offset + 48, true);
      const entryFlags = this.data.getUint32(offset + 52, true);
      if (objectType < SnapshotObjectType.Unknown || objectType > SnapshotObjectType.VesselType
        || !Number.isFinite(progress) || progress < 0 || progress > 1 || placementCount > 36 || (entryFlags & ~0x3f) !== 0) {
        throw new Error("Sidebar entry metadata is invalid");
      }
      const placementOffsets: number[] = [];
      for (let cell = 0; cell < 36; cell += 1) {
        const value = this.data.getInt16(offset + 56 + cell * 2, true);
        if (cell < placementCount) placementOffsets.push(value);
        else if (value !== 0) throw new Error("Sidebar entry has nonzero placement padding");
      }
      const column: 0 | 1 = index < leftEntries ? 0 : 1;
      entries.push({
        column,
        columnIndex: column === 0 ? index : index - leftEntries,
        assetName: fixedString(this.buffer, offset, 16, "Sidebar asset name"),
        buildableType: this.data.getInt32(offset + 16, true),
        buildableId: this.data.getInt32(offset + 20, true),
        objectType: objectType as SnapshotObjectType,
        superweaponType: this.data.getInt32(offset + 28, true),
        cost: this.data.getInt32(offset + 32, true),
        powerDelta: this.data.getInt32(offset + 36, true),
        buildTime: this.data.getInt32(offset + 40, true),
        progress,
        placementOffsets,
        completed: Boolean(entryFlags & 1),
        constructing: Boolean(entryFlags & 2),
        onHold: Boolean(entryFlags & 4),
        busy: Boolean(entryFlags & 8),
        viaCapture: Boolean(entryFlags & 16),
        fake: Boolean(entryFlags & 32),
      });
    }
    return {
      leftEntries,
      rightEntries,
      credits: this.data.getInt32(section.offset + 8, true),
      creditsCounter: this.data.getInt32(section.offset + 12, true),
      tiberium: this.data.getInt32(section.offset + 16, true),
      maxTiberium: this.data.getInt32(section.offset + 20, true),
      powerProduced: this.data.getInt32(section.offset + 24, true),
      powerDrained: this.data.getInt32(section.offset + 28, true),
      missionTimer: this.data.getInt32(section.offset + 32, true),
      unitsKilled: this.data.getUint32(section.offset + 36, true),
      buildingsKilled: this.data.getUint32(section.offset + 40, true),
      unitsLost: this.data.getUint32(section.offset + 44, true),
      buildingsLost: this.data.getUint32(section.offset + 48, true),
      harvestedCredits: this.data.getUint32(section.offset + 52, true),
      repairEnabled: Boolean(flags & 1),
      sellEnabled: Boolean(flags & 2),
      radarActive: Boolean(flags & 4),
      entries,
      leftColumn: entries.slice(0, leftEntries),
      rightColumn: entries.slice(leftEntries),
    };
  }

  private validateDynamicMap(section: SnapshotSection): void {
    if (section.flags !== 0 || section.count > 100_000
      || section.length !== DYNAMIC_MAP_FIXED_BYTES + section.count * DYNAMIC_MAP_RECORD_BYTES
      || this.data.getUint32(section.offset, true) > 1) throw new Error("Dynamic map section layout is invalid");
    for (let index = 0; index < section.count; index += 1) {
      const offset = section.offset + DYNAMIC_MAP_FIXED_BYTES + index * DYNAMIC_MAP_RECORD_BYTES;
      if ((this.data.getUint16(offset + 42, true) & ~0x3f) !== 0 || this.data.getUint32(offset + 44, true) !== 0) {
        throw new Error("Dynamic map record layout is invalid");
      }
    }
  }

  private readPlayer(section: SnapshotSection): SnapshotPlayer {
    if (section.flags !== 0 || section.count !== 1 || section.length < PLAYER_FIXED_BYTES) {
      throw new Error("Player section layout is invalid");
    }
    const actionCount = this.data.getUint32(section.offset + 116, true);
    const flags = this.data.getUint32(section.offset + 88, true);
    if (actionCount > MAX_MAP_CELLS || section.length !== PLAYER_FIXED_BYTES + actionCount
      || this.data.getUint8(section.offset + 67) !== 0 || (flags & ~7) !== 0) {
      throw new Error("Player section metadata is invalid");
    }
    if (actionCount !== 0 && (!this.staticMap
      || actionCount !== this.staticMap.originalWidth * this.staticMap.originalHeight)) {
      throw new Error("Player action grid dimensions do not match the original static map");
    }
    const actionValues = new Uint8Array(this.buffer, section.offset + PLAYER_FIXED_BYTES, actionCount);
    if (actionValues.some((value) => value > SnapshotContextualAction.CannotRepair)) {
      throw new Error("Player action grid contains an invalid contextual action");
    }
    const actions = new SnapshotContextualActionGrid(
      this.staticMap?.originalCellX ?? 0,
      this.staticMap?.originalCellY ?? 0,
      this.staticMap?.originalWidth ?? 0,
      this.staticMap?.originalHeight ?? 0,
      actionValues,
    );
    return {
      name: fixedString(this.buffer, section.offset, 64, "Player name"),
      house: this.data.getUint8(section.offset + 64),
      homeCellX: this.data.getUint8(section.offset + 65),
      homeCellY: this.data.getUint8(section.offset + 66),
      colorIndex: this.data.getInt32(section.offset + 68, true),
      playerId: this.data.getBigUint64(section.offset + 72, true),
      team: this.data.getInt32(section.offset + 80, true),
      startLocation: this.data.getInt32(section.offset + 84, true),
      ai: Boolean(flags & 1),
      defeated: Boolean(flags & 2),
      radarJammed: Boolean(flags & 4),
      allyFlags: this.data.getUint32(section.offset + 92, true),
      actions,
    };
  }

  private readPlacement(section: SnapshotSection): SnapshotPlacementGrid {
    if (section.flags !== 0 || section.count === 0 || section.count > MAX_MAP_CELLS || section.length !== section.count) {
      throw new Error("Placement section layout is invalid");
    }
    if (!this.staticMap) throw new Error("Placement section requires static map metadata");
    /* STATIC_MAP already carries the same one-cell expansion performed by the
     * legacy placement exporter. Expanding it again rejects real maps. */
    if (this.staticMap.width * this.staticMap.height !== section.count) throw new Error("Placement grid dimensions do not match the static map");
    const flags = new Uint8Array(this.buffer, section.offset, section.count);
    if (flags.some((value) => (value & ~3) !== 0)) throw new Error("Placement cell flags are invalid");
    return new SnapshotPlacementGrid(this.staticMap.cellX, this.staticMap.cellY, this.staticMap.width, this.staticMap.height, flags);
  }

  private readShroud(section: SnapshotSection): SnapshotShroudGrid {
    if (section.flags !== 0 || section.count === 0 || section.count > MAX_MAP_CELLS
      || section.length !== section.count * 2) {
      throw new Error("Shroud section layout is invalid");
    }
    if (!this.staticMap) throw new Error("Shroud section requires static map metadata");
    if (section.count !== this.staticMap.width * this.staticMap.height) {
      throw new Error("Shroud grid dimensions do not match the expanded static map");
    }
    const entries = new Uint8Array(this.buffer, section.offset, section.length);
    for (let index = 0; index < section.count; index += 1) {
      if ((entries[index * 2 + 1] & ~7) !== 0) throw new Error("Shroud cell flags are invalid");
    }
    return new SnapshotShroudGrid(this.staticMap.cellX, this.staticMap.cellY, this.staticMap.width, this.staticMap.height, entries);
  }

  private readOccupiers(section: SnapshotSection): SnapshotOccupierGrid {
    if (section.flags !== 0 || section.count === 0 || section.count > MAX_MAP_CELLS || !this.staticMap
      || section.count !== this.staticMap.width * this.staticMap.height) {
      throw new Error("Occupier grid dimensions do not match the expanded static map");
    }
    const offsets = new Uint32Array(section.count);
    const end = section.offset + section.length;
    let offset = section.offset;
    for (let cell = 0; cell < section.count; cell += 1) {
      checkedEnd(offset, 4, end, "Occupier cell header");
      offsets[cell] = offset;
      const count = this.data.getUint32(offset, true);
      if (count > 4096) throw new Error("Occupier cell count is invalid");
      offset = checkedEnd(offset + 4, count * 8, end, "Occupier cell objects");
      const firstObject = offsets[cell] + 4;
      for (let index = 0; index < count; index += 1) {
        const type = this.data.getInt32(firstObject + index * 8, true);
        if (type < SnapshotObjectType.Unknown || type > SnapshotObjectType.VesselType) {
          throw new Error("Occupier object type is invalid");
        }
      }
    }
    if (offset !== end) throw new Error("Occupier section contains trailing data");
    return new SnapshotOccupierGrid(
      this.staticMap.cellX,
      this.staticMap.cellY,
      this.staticMap.width,
      this.staticMap.height,
      this.data,
      offsets,
    );
  }

  get classicPixels(): Uint8Array | undefined {
    return this.classicSurface?.pixels;
  }

  get classicSurface(): SnapshotClassicSurface | undefined {
    const section = this.sections.get(SnapshotSectionKind.ClassicSurface);
    if (!section) return undefined;
    if (this.classicCache) return this.classicCache;
    const pitch = this.data.getUint32(section.offset + 8, true);
    const pixelsOffset = section.offset + (this.classicFormat === 2 ? CLASSIC_DIRTY_HEADER_BYTES : CLASSIC_FULL_HEADER_BYTES);
    const packedLength = this.classicRectWidth * this.classicRectHeight;
    let pixels: Uint8Array;
    if (pitch === this.classicRectWidth) {
      pixels = new Uint8Array(this.buffer, pixelsOffset, packedLength);
    } else {
      pixels = new Uint8Array(packedLength);
      for (let y = 0; y < this.classicRectHeight; y += 1) {
        pixels.set(new Uint8Array(this.buffer, pixelsOffset + y * pitch, this.classicRectWidth), y * this.classicRectWidth);
      }
    }
    this.classicCache = {
      format: this.classicFormat as 1 | 2,
      width: this.classicWidth,
      height: this.classicHeight,
      rectX: this.classicRectX,
      rectY: this.classicRectY,
      rectWidth: this.classicRectWidth,
      rectHeight: this.classicRectHeight,
      pixels,
    };
    return this.classicCache;
  }

  get palette(): Uint8Array | undefined {
    const section = this.sections.get(SnapshotSectionKind.Palette);
    if (!section) return undefined;
    if (this.paletteCache) return this.paletteCache;
    const source = new Uint8Array(this.buffer, section.offset, section.length);
    const sixBit = source.every((value) => value <= 63);
    const output = new Uint8Array(256 * 4);
    for (let index = 0; index < 256; index += 1) {
      output[index * 4] = Math.min(255, source[index * 3] * (sixBit ? 4 : 1));
      output[index * 4 + 1] = Math.min(255, source[index * 3 + 1] * (sixBit ? 4 : 1));
      output[index * 4 + 2] = Math.min(255, source[index * 3 + 2] * (sixBit ? 4 : 1));
      output[index * 4 + 3] = 255;
    }
    this.paletteCache = output;
    return output;
  }

  get sidebar(): SnapshotSidebar | undefined {
    return this.sidebarCache;
  }

  sidebarEntry(buildableType: number, buildableId: number): SnapshotSidebarEntry | undefined {
    return this.sidebar?.entries.find((entry) => entry.buildableType === buildableType && entry.buildableId === buildableId);
  }

  canPlaceSidebarEntry(entry: SnapshotSidebarEntry, requestCellX: number, requestCellY: number): boolean {
    return this.placement?.canPlace(entry, requestCellX, requestCellY) ?? false;
  }

  object(index: number): SnapshotObject {
    const section = this.sections.get(SnapshotSectionKind.Objects);
    if (!section || !Number.isInteger(index) || index < 0 || index >= section.count) throw new RangeError("Object index is out of range");
    this.objectCache ??= new Array(section.count);
    const cached = this.objectCache[index];
    if (cached) return cached;
    const offset = section.offset + index * OBJECT_RECORD_BYTES;
    const objectFlags = this.data.getUint32(offset + 204, true);
    const occupyCount = this.data.getUint16(offset + 216, true);
    const occupyOffsets = Array.from({ length: occupyCount }, (_, cell) => this.data.getInt16(offset + 224 + cell * 2, true));
    const subObject = this.data.getUint8(offset + 184);
    const object: SnapshotObject = {
      index,
      typeName: fixedString(this.buffer, offset, 16, "Object type name"),
      assetName: fixedString(this.buffer, offset + 16, 16, "Object asset name"),
      type: this.data.getInt32(offset + 112, true) as SnapshotObjectType,
      id: this.data.getInt32(offset + 116, true),
      baseId: this.data.getInt32(offset + 120, true),
      baseType: this.data.getInt32(offset + 124, true) as SnapshotObjectType,
      x: this.data.getInt32(offset + 128, true),
      y: this.data.getInt32(offset + 132, true),
      width: this.data.getInt32(offset + 136, true),
      height: this.data.getInt32(offset + 140, true),
      altitude: this.data.getInt32(offset + 144, true),
      sortOrder: this.data.getInt32(offset + 148, true),
      drawFlags: this.data.getInt32(offset + 156, true),
      maxStrength: this.data.getInt16(offset + 160, true),
      strength: this.data.getInt16(offset + 162, true),
      cellX: this.data.getUint16(offset + 166, true),
      cellY: this.data.getUint16(offset + 168, true),
      centerCoordX: this.data.getUint16(offset + 170, true),
      centerCoordY: this.data.getUint16(offset + 172, true),
      owner: this.data.getUint8(offset + 182),
      subObject,
      cloak: this.data.getUint8(offset + 185) as SnapshotCloakState,
      controlGroup: this.data.getUint8(offset + 186) === 0xff ? undefined : this.data.getUint8(offset + 186),
      selectedMask: this.data.getUint32(offset + 188, true),
      visibleFlags: this.data.getUint32(offset + 196, true),
      actionWithSelected: Array.from(
        { length: 32 },
        (_, house) => this.data.getUint8(offset + 440 + house) as SnapshotContextualAction,
      ),
      occupyOffsets,
      selectable: Boolean(objectFlags & (1 << 0)),
      repairing: Boolean(objectFlags & (1 << 1)),
      canRepair: Boolean(objectFlags & (1 << 4)),
      canDemolish: Boolean(objectFlags & (1 << 5)),
      factory: Boolean(objectFlags & (1 << 9)),
      primaryFactory: Boolean(objectFlags & (1 << 10)),
      fixedWing: Boolean(objectFlags & (1 << 22)),
      root: subObject === 0,
    };
    this.objectCache[index] = object;
    return object;
  }

  objects(): SnapshotObject[] {
    const section = this.sections.get(SnapshotSectionKind.Objects);
    return section ? Array.from({ length: section.count }, (_, index) => this.object(index)) : [];
  }

  dynamicMapEntries(): SnapshotDynamicMapEntry[] {
    const section = this.sections.get(SnapshotSectionKind.DynamicMap);
    if (!section) return [];
    if (this.dynamicMapCache) return this.dynamicMapCache;
    this.dynamicMapCache = Array.from({ length: section.count }, (_, index) => {
      const offset = section.offset + DYNAMIC_MAP_FIXED_BYTES + index * DYNAMIC_MAP_RECORD_BYTES;
      const flags = this.data.getUint16(offset + 42, true);
      return {
        index,
        assetName: fixedString(this.buffer, offset, 16, "Dynamic map asset name"),
        x: this.data.getInt32(offset + 16, true),
        y: this.data.getInt32(offset + 20, true),
        width: this.data.getInt32(offset + 24, true),
        height: this.data.getInt32(offset + 28, true),
        drawFlags: this.data.getInt32(offset + 32, true),
        type: this.data.getInt16(offset + 36, true),
        owner: this.data.getUint8(offset + 38),
        cellX: this.data.getUint8(offset + 40),
        cellY: this.data.getUint8(offset + 41),
        overlay: Boolean(flags & 2),
        sellable: Boolean(flags & 8),
      };
    });
    return this.dynamicMapCache;
  }

  findBuildingAtWorldPoint(worldX: number, worldY: number, options: BuildingHitTestOptions = {}): SnapshotObject | undefined {
    if (!Number.isFinite(worldX) || !Number.isFinite(worldY)) return undefined;
    const owner = options.owner ?? this.player?.house;
    if (owner === undefined) return undefined;
    const all = this.objects();
    const roots = all.filter((object) => object.root && object.type === SnapshotObjectType.Building && object.owner === owner
      && (options.capability !== "repair" || object.canRepair)
      && (options.capability !== "sell" || object.canDemolish))
      .sort((left, right) => right.sortOrder - left.sortOrder);
    const mapCellX = Math.floor(worldX / MAP_CELL_PIXELS);
    const mapCellY = Math.floor(worldY / MAP_CELL_PIXELS);
    const mapCell = mapCellY * MAP_CELL_STRIDE + mapCellX;
    for (const root of roots) {
      const visualHit = all.some((part) => part.type === SnapshotObjectType.Building && part.id === root.id
        && pointInRenderedBounds(part.x, part.y, part.width, part.height, part.drawFlags, worldX, worldY));
      const anchorCell = root.cellY * MAP_CELL_STRIDE + root.cellX;
      const footprintHit = mapCellX >= 0 && mapCellY >= 0
        && (root.occupyOffsets.length > 0 ? root.occupyOffsets : [0]).some((offset) => anchorCell + offset === mapCell);
      if (visualHit || footprintHit) return root;
    }
    return undefined;
  }

  findSellableWallAtWorldPoint(worldX: number, worldY: number, owner = this.player?.house): SnapshotDynamicMapEntry | undefined {
    if (owner === undefined || !Number.isFinite(worldX) || !Number.isFinite(worldY)) return undefined;
    const mapCellX = Math.floor(worldX / MAP_CELL_PIXELS);
    const mapCellY = Math.floor(worldY / MAP_CELL_PIXELS);
    return this.dynamicMapEntries().find((entry) => entry.overlay && entry.sellable && entry.owner === owner
      && ((entry.cellX === mapCellX && entry.cellY === mapCellY)
        || pointInRenderedBounds(entry.x, entry.y, entry.width, entry.height, entry.drawFlags, worldX, worldY)));
  }

  findSellTargetAtWorldPoint(worldX: number, worldY: number, owner = this.player?.house): SnapshotSellTarget | undefined {
    if (owner === undefined) return undefined;
    const building = this.findBuildingAtWorldPoint(worldX, worldY, { owner, capability: "sell" });
    if (building) return { kind: "building", building };
    const wall = this.findSellableWallAtWorldPoint(worldX, worldY, owner);
    return wall ? { kind: "wall", wall } : undefined;
  }

  sprite(index: number): SnapshotSprite {
    const section = this.sections.get(SnapshotSectionKind.Objects);
    if (!section || !Number.isInteger(index) || index < 0 || index >= section.count) throw new RangeError("Sprite index is out of range");
    const offset = section.offset + index * OBJECT_RECORD_BYTES;
    const drawFlags = this.data.getInt32(offset + 156, true);
    const selected = this.data.getUint32(offset + 188, true) !== 0;
    const cloak = this.data.getUint8(offset + 185);
    const owner = this.data.getUint8(offset + 182);
    const remap = this.data.getUint8(offset + 183);
    const tint = objectTint(owner, remap);
    const width = Math.max(1, this.data.getInt32(offset + 136, true));
    const height = Math.max(1, this.data.getInt32(offset + 140, true));
    let x = this.data.getInt32(offset + 128, true);
    let y = this.data.getInt32(offset + 132, true);
    if (drawFlags & 0x20) { x -= width / 2; y -= height / 2; }
    else if (drawFlags & 0x40) y -= height;
    return {
      x,
      y,
      width,
      height,
      u0: 0,
      v0: 0,
      u1: 1,
      v1: 1,
      atlasPage: 0,
      flags: (selected ? SpriteFlags.Selected : 0)
        | (drawFlags & 0x2000 ? SpriteFlags.Shadow : 0)
        | (drawFlags & 0x1000 || cloak > 0 ? SpriteFlags.Translucent : 0)
        | (drawFlags & 0x1 ? SpriteFlags.FlipX : 0)
        | (drawFlags & 0x2 ? SpriteFlags.FlipY : 0),
      sortKey: this.data.getInt32(offset + 148, true),
      tint,
      teamColor: tint,
    };
  }

  sprites(): SnapshotSprite[] {
    return Array.from({ length: this.spriteCount }, (_, index) => this.sprite(index));
  }
}

export function snapshotByteLength(classicWidth: number, classicHeight: number, spriteCount: number): number {
  const payloads = spriteCount * OBJECT_RECORD_BYTES + (16 + classicWidth * classicHeight) + 256 * 3 + 24;
  return SNAPSHOT_HEADER_BYTES + 4 * SECTION_HEADER_BYTES + payloads;
}

function writeSectionHeader(view: DataView, offset: number, kind: SnapshotSectionKind, length: number, count: number): number {
  view.setUint16(offset, kind, true);
  view.setUint16(offset + 2, 0, true);
  view.setUint32(offset + 4, length, true);
  view.setUint32(offset + 8, count, true);
  view.setUint32(offset + 12, 0, true);
  return offset + SECTION_HEADER_BYTES;
}

export function writeSnapshot(buffer: ArrayBuffer, layout: SnapshotLayout): number {
  const required = snapshotByteLength(layout.classicWidth, layout.classicHeight, layout.sprites.length);
  if (buffer.byteLength < required) throw new RangeError(`Snapshot buffer needs ${required} bytes`);
  if (layout.classicPixels.length !== layout.classicWidth * layout.classicHeight) throw new Error("Classic pixels have invalid dimensions");
  if (layout.palette.length !== 256 * 4) throw new Error("Snapshot palette must contain exactly 256 RGBA colors");
  const view = new DataView(buffer);
  view.setUint32(0, CNC_WEB_MAGIC_MESSAGE, true);
  view.setUint16(4, SIMULATION_PROTOCOL_VERSION, true);
  view.setUint16(6, MessageKind.Snapshot, true);
  view.setUint32(8, required, true);
  view.setUint32(12, 4, true);
  view.setUint32(16, layout.tick, true);
  view.setUint32(20, layout.tick, true);
  view.setBigUint64(24, 0n, true);
  view.setUint32(32, 4, true);
  view.setUint32(36, 0, true);
  let offset = SNAPSHOT_HEADER_BYTES;

  offset = writeSectionHeader(view, offset, SnapshotSectionKind.Objects, layout.sprites.length * OBJECT_RECORD_BYTES, layout.sprites.length);
  for (let index = 0; index < layout.sprites.length; index += 1) {
    const sprite = layout.sprites[index];
    const record = offset + index * OBJECT_RECORD_BYTES;
    new Uint8Array(buffer, record, OBJECT_RECORD_BYTES).fill(0);
    new Uint8Array(buffer, record + 16, 4).set(new TextEncoder().encode("demo"));
    view.setInt32(record + 116, index + 1, true);
    view.setInt32(record + 128, Math.round(sprite.x), true);
    view.setInt32(record + 132, Math.round(sprite.y), true);
    view.setInt32(record + 136, Math.round(sprite.width), true);
    view.setInt32(record + 140, Math.round(sprite.height), true);
    view.setInt32(record + 148, sprite.sortKey, true);
    view.setInt32(record + 152, 1, true);
    view.setInt32(record + 156, (sprite.flags & SpriteFlags.Shadow ? 0x2000 : 0) | (sprite.flags & SpriteFlags.FlipX ? 0x1 : 0) | (sprite.flags & SpriteFlags.FlipY ? 0x2 : 0), true);
    view.setInt16(record + 160, 100, true);
    view.setInt16(record + 162, 100, true);
    view.setUint8(record + 183, index & 7);
    view.setUint8(record + 185, sprite.flags & SpriteFlags.Translucent ? 1 : 0);
    view.setUint8(record + 186, 0xff);
    view.setUint32(record + 188, sprite.flags & SpriteFlags.Selected ? 1 : 0, true);
  }
  offset += layout.sprites.length * OBJECT_RECORD_BYTES;

  const classicLength = 16 + layout.classicPixels.length;
  offset = writeSectionHeader(view, offset, SnapshotSectionKind.ClassicSurface, classicLength, layout.classicPixels.length);
  view.setUint32(offset, layout.classicWidth, true);
  view.setUint32(offset + 4, layout.classicHeight, true);
  view.setUint32(offset + 8, layout.classicWidth, true);
  view.setUint32(offset + 12, 1, true);
  new Uint8Array(buffer, offset + 16, layout.classicPixels.length).set(layout.classicPixels);
  offset += classicLength;

  offset = writeSectionHeader(view, offset, SnapshotSectionKind.Palette, 256 * 3, 256);
  const wirePalette = new Uint8Array(buffer, offset, 256 * 3);
  for (let index = 0; index < 256; index += 1) wirePalette.set(layout.palette.subarray(index * 4, index * 4 + 3), index * 3);
  offset += 256 * 3;

  offset = writeSectionHeader(view, offset, SnapshotSectionKind.Camera, 24, 1);
  view.setInt32(offset, Math.round(layout.cameraX), true);
  view.setInt32(offset + 4, Math.round(layout.cameraY), true);
  view.setInt32(offset + 8, layout.worldWidth, true);
  view.setInt32(offset + 12, layout.worldHeight, true);
  view.setInt32(offset + 16, 0, true);
  view.setInt32(offset + 20, 0, true);
  return required;
}
