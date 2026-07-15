export const SIMULATION_PROTOCOL_VERSION = 1 as const;
export const CNC_WEB_ABI_VERSION = 2 as const;
export const CNC_WEB_MAGIC_MESSAGE = 0x57434e43;
export const MESSAGE_HEADER_BYTES = 16;

export enum MessageKind {
  Start = 1,
  CommandBatch = 2,
  Snapshot = 3,
  Event = 4,
  Save = 5,
}

export type GameId = "tiberian-dawn" | "red-alert" | "demo";
export type CoreKind = "demo" | "wasm";

export enum Faction {
  Gdi = 1,
  Nod = 2,
  Jurassic = 3,
}

export enum GameMode {
  Campaign = 1,
  Skirmish = 2,
}

export interface CampaignTransition {
  carryOverCredits: number;
  nukePieces: number;
}

export interface StartConfiguration {
  game: GameId;
  seed: number;
  scenario: number;
  variation: number;
  direction: number;
  buildLevel: number;
  sabotagedStructure: number;
  faction: Faction;
  gameMode: GameMode;
  playerId: bigint;
  contentDirectory: string;
  overrideMapName: string;
  contentIdHash: bigint;
  /** Applied before a continued campaign mission starts; not part of StartV1. */
  campaignTransition?: CampaignTransition;
}

export interface ContentMountFile {
  path: string;
  size: number;
  sha256: string;
}

export interface ContentMountRequest {
  packageId: string;
  revision: string;
  storageKey: string;
  files: ContentMountFile[];
}

export interface ContentMountProgress {
  phase: "opening" | "verifying" | "mounting" | "complete";
  packageId: string;
  completedFiles: number;
  totalFiles: number;
  completedBytes: number;
  totalBytes: number;
  currentPath?: string;
}

export type MainToWorkerMessage =
  | { type: "initialize"; protocolVersion: number; core: "demo" }
  | { type: "initialize"; protocolVersion: number; core: "wasm"; emscriptenModuleUrl: string; contentMount: ContentMountRequest; acceptanceSession?: string }
  | { type: "start"; requestId: number; configuration: StartConfiguration; deferRunningUntilLoad: boolean }
  | { type: "commands"; buffer: ArrayBuffer }
  | { type: "recycle"; buffer: ArrayBuffer }
  | { type: "set-running"; running: boolean }
  | { type: "cancel-deferred-load" }
  | { type: "save"; requestId: number }
  | { type: "load"; requestId: number; buffer: ArrayBuffer }
  | { type: "acceptance-force-victory"; requestId: number }
  | { type: "shutdown"; requestId: number };

export interface SimulationCapabilities {
  saves: boolean;
  stateHashes: boolean;
  tickRate: number;
  protocolVersion: number;
}

export type WorkerToMainMessage =
  | { type: "ready"; capabilities: SimulationCapabilities }
  | { type: "started"; requestId: number }
  | { type: "snapshot"; buffer: ArrayBuffer }
  | { type: "saved"; requestId: number; buffer: ArrayBuffer }
  | { type: "loaded"; requestId: number }
  | { type: "acceptance-victory-forced"; requestId: number }
  | { type: "shutdown-complete"; requestId: number }
  | { type: "running"; running: boolean }
  | { type: "event"; event: SimulationEvent }
  | { type: "mount-progress"; progress: ContentMountProgress }
  | { type: "error"; requestId?: number; fatal: boolean; message: string };

export type SimulationEvent =
  | { kind: "message"; tick: number; text: string; timeoutSeconds: number; messageType: number; parameter: bigint }
  | { kind: "debug"; tick: number; text: string }
  | { kind: "sound"; tick: number; assetId: number; name: string; variation: number; x: number; y: number; priority: number; context: number }
  | { kind: "speech"; tick: number; assetId: number; name: string }
  | { kind: "movie"; tick: number; name: string; immediate: boolean; theme: number }
  | { kind: "camera"; tick: number; x: number; y: number }
  | { kind: "ping"; tick: number; x: number; y: number }
  | { kind: "game-over"; tick: number; multiplayer: boolean; human: boolean; won: boolean; score: number; leadership: number; efficiency: number; remainingCredits: number; sabotagedStructure: number; timerRemaining: number; movieName: string; afterScoreMovieName: string }
  | { kind: "campaign-outcome"; tick: number; carryOverCredits: number; nukePieces: number; sabotagedStructure: number; randomSeed: number; scenario: number; house: 0 | 1; scenarioRoot: string }
  | { kind: "diagnostic"; tick: number; warning: boolean; error: boolean; code: number; status: number; scenario: number; variation: number; direction: number; buildLevel: number; id: string; detail: string }
  | { kind: "engine"; tick: number; eventType: number; flags: number; playerId: bigint; args: readonly number[]; text1: string; text2: string };

/** Categories from cnc_web_protocol.h. Type-specific data lives in args[0..6]. */
export enum CommandType {
  Input = 1,
  Structure = 2,
  Unit = 3,
  Sidebar = 4,
  Superweapon = 5,
  ControlGroup = 6,
  Game = 7,
  ClearSelection = 8,
  SelectObject = 9,
}

/** Mirrors ControlGroupRequestEnum in the legacy DLL interface. */
export enum ControlGroupRequest {
  Create = 0,
  Toggle = 1,
  AdditiveSelection = 2,
}

export enum GameRequest {
  MovieDone = 0,
  LoadingDone = 1,
}

export enum InputRequest {
  None = 0,
  MouseMove = 1,
  MouseLeftClick = 2,
  MouseRightDown = 3,
  MouseRightClick = 4,
  MouseArea = 5,
  MouseAreaAdditive = 6,
  SellAtPosition = 7,
  SelectAtPosition = 8,
  CommandAtPosition = 9,
  SpecialKeys = 10,
  ModCommand1AtPosition = 11,
  ModCommand2AtPosition = 12,
  ModCommand3AtPosition = 13,
  ModCommand4AtPosition = 14,
}

/** StructureRequestEnum from the Tiberian Dawn DLL boundary. */
export enum StructureRequest {
  None = 0,
  RepairStart = 1,
  Repair = 2,
  SellStart = 3,
  Sell = 4,
  Cancel = 5,
}

/** SidebarRequestEnum from the Tiberian Dawn DLL boundary. */
export enum SidebarRequest {
  StartConstruction = 0,
  HoldConstruction = 1,
  CancelConstruction = 2,
  StartPlacement = 3,
  Place = 4,
  CancelPlacement = 5,
  ClickRepair = 6,
  EnableQueue = 7,
  DisableQueue = 8,
  StartConstructionMulti = 9,
  CancelConstructionMulti = 10,
}

/** SuperWeaponRequestEnum from the Tiberian Dawn DLL boundary. */
export enum SuperweaponRequest {
  Place = 0,
}

/** DllObjectTypeEnum values carried by object and sidebar snapshot records. */
export enum DllObjectType {
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

export enum UnitRequest {
  None = 0,
  Scatter = 1,
  SelectNext = 2,
  SelectPrevious = 3,
  GuardMode = 4,
  Stop = 5,
  FormationToggle = 6,
  QueuedMovementOn = 7,
  QueuedMovementOff = 8,
}

export enum ModifierFlags {
  Ctrl = 1 << 0,
  Alt = 1 << 1,
  Shift = 1 << 2,
}

export interface SimulationCommand {
  type: CommandType;
  flags?: number;
  args: readonly [number, number, number, number, number, number, number];
}

export interface DecodedCommandBatch {
  targetTick: number;
  playerId: bigint;
  commands: SimulationCommand[];
}

export function resolveImmediateCommandTick(batch: DecodedCommandBatch, currentTick: number): DecodedCommandBatch {
  if (!Number.isSafeInteger(currentTick) || currentTick < 0 || currentTick > 0xffff_ffff) throw new RangeError("currentTick must be an unsigned 32-bit integer");
  if (batch.targetTick !== 0) return batch;
  if (currentTick === 0xffff_ffff) throw new RangeError("Cannot schedule a command after the final tick");
  return { ...batch, targetTick: currentTick + 1 };
}

const START_FIXED_BYTES = 72;
const COMMAND_BATCH_FIXED_BYTES = 32;
const COMMAND_RECORD_BYTES = 32;

function writeHeader(view: DataView, kind: MessageKind, byteLength: number, count: number): void {
  view.setUint32(0, CNC_WEB_MAGIC_MESSAGE, true);
  view.setUint16(4, SIMULATION_PROTOCOL_VERSION, true);
  view.setUint16(6, kind, true);
  view.setUint32(8, byteLength, true);
  view.setUint32(12, count, true);
}

export function validateMessageHeader(view: DataView, expectedKind: MessageKind): { byteLength: number; count: number } {
  if (view.byteLength < MESSAGE_HEADER_BYTES) throw new Error("CNCW message header is truncated");
  if (view.getUint32(0, true) !== CNC_WEB_MAGIC_MESSAGE) throw new Error("CNCW message magic is invalid");
  if (view.getUint16(4, true) !== SIMULATION_PROTOCOL_VERSION) throw new Error("CNCW protocol version is unsupported");
  if (view.getUint16(6, true) !== expectedKind) throw new Error("CNCW message kind is invalid");
  const byteLength = view.getUint32(8, true);
  if (byteLength !== view.byteLength) throw new Error("CNCW message length is invalid");
  return { byteLength, count: view.getUint32(12, true) };
}

export function encodeStartConfiguration(configuration: StartConfiguration): ArrayBuffer {
  const encoder = new TextEncoder();
  const content = encoder.encode(configuration.contentDirectory);
  const overrideMap = encoder.encode(configuration.overrideMapName);
  if (content.byteLength < 1 || content.byteLength > 4096) throw new RangeError("Content directory must contain 1 to 4096 UTF-8 bytes");
  if (overrideMap.byteLength > 255) throw new RangeError("Override map name cannot exceed 255 UTF-8 bytes");
  if (configuration.contentIdHash <= 0n || configuration.contentIdHash > 0xffff_ffff_ffff_ffffn) throw new RangeError("contentIdHash must be a non-zero unsigned 64-bit integer");
  if (content.includes(0) || overrideMap.includes(0)) throw new Error("Start strings cannot contain NUL bytes");
  const buffer = new ArrayBuffer(START_FIXED_BYTES + content.byteLength + overrideMap.byteLength);
  const view = new DataView(buffer);
  writeHeader(view, MessageKind.Start, buffer.byteLength, 1);
  view.setUint32(16, configuration.seed, true);
  view.setInt32(20, configuration.scenario, true);
  view.setInt32(24, configuration.variation, true);
  view.setInt32(28, configuration.direction, true);
  view.setInt32(32, configuration.buildLevel, true);
  view.setInt32(36, configuration.sabotagedStructure, true);
  view.setUint32(40, configuration.faction, true);
  view.setUint32(44, configuration.gameMode, true);
  view.setBigUint64(48, configuration.playerId, true);
  view.setUint32(56, content.byteLength, true);
  view.setUint32(60, overrideMap.byteLength, true);
  view.setBigUint64(64, configuration.contentIdHash, true);
  new Uint8Array(buffer, START_FIXED_BYTES).set(content);
  new Uint8Array(buffer, START_FIXED_BYTES + content.byteLength).set(overrideMap);
  return buffer;
}

export function encodeCommandBatch(targetTick: number, commands: readonly SimulationCommand[], playerId = 0n): ArrayBuffer {
  if (!Number.isSafeInteger(targetTick) || targetTick < 0 || targetTick > 0xffff_ffff) throw new RangeError("targetTick must be an unsigned 32-bit integer");
  if (commands.length > 4096) throw new RangeError("A command batch cannot contain more than 4096 commands");
  if (playerId < 0n || playerId > 0xffff_ffff_ffff_ffffn) throw new RangeError("playerId must be an unsigned 64-bit integer");
  const buffer = new ArrayBuffer(COMMAND_BATCH_FIXED_BYTES + commands.length * COMMAND_RECORD_BYTES);
  const view = new DataView(buffer);
  writeHeader(view, MessageKind.CommandBatch, buffer.byteLength, commands.length);
  view.setUint32(16, targetTick, true);
  view.setUint16(20, COMMAND_RECORD_BYTES, true);
  view.setUint16(22, 0, true);
  view.setBigUint64(24, playerId, true);
  commands.forEach((command, index) => {
    if (command.type < CommandType.Input || command.type > CommandType.SelectObject) throw new RangeError("Command type is invalid");
    if (command.args.length !== 7) throw new RangeError("Commands require exactly seven signed arguments");
    const offset = COMMAND_BATCH_FIXED_BYTES + index * COMMAND_RECORD_BYTES;
    view.setUint16(offset, command.type, true);
    view.setUint16(offset + 2, command.flags ?? 0, true);
    command.args.forEach((argument, argumentIndex) => view.setInt32(offset + 4 + argumentIndex * 4, argument, true));
  });
  return buffer;
}

export function decodeCommandBatch(buffer: ArrayBuffer): DecodedCommandBatch {
  if (buffer.byteLength < COMMAND_BATCH_FIXED_BYTES) throw new Error("Command batch is truncated");
  const view = new DataView(buffer);
  const header = validateMessageHeader(view, MessageKind.CommandBatch);
  const recordSize = view.getUint16(20, true);
  if (recordSize !== COMMAND_RECORD_BYTES || view.getUint16(22, true) !== 0 || header.byteLength !== COMMAND_BATCH_FIXED_BYTES + header.count * recordSize || header.count > 4096) {
    throw new Error("Command batch layout is invalid");
  }
  const commands: SimulationCommand[] = [];
  for (let index = 0; index < header.count; index += 1) {
    const offset = COMMAND_BATCH_FIXED_BYTES + index * COMMAND_RECORD_BYTES;
    const type = view.getUint16(offset, true) as CommandType;
    if (type < CommandType.Input || type > CommandType.SelectObject) throw new Error("Command type is invalid");
    commands.push({
      type,
      flags: view.getUint16(offset + 2, true),
      args: [0, 1, 2, 3, 4, 5, 6].map((argument) => view.getInt32(offset + 4 + argument * 4, true)) as [number, number, number, number, number, number, number],
    });
  }
  return { targetTick: view.getUint32(16, true), playerId: view.getBigUint64(24, true), commands };
}
