import { useCallback, useEffect, useRef, useState, type InputHTMLAttributes, type RefObject } from "react";
import { RuntimeAudio } from "./audio/RuntimeAudio";
import { bootstrapClassicFreeware } from "./bootstrap/classicFreewareBootstrap";
import { isTdCampaignFinalMission, nextTdCampaignMissions, tdCampaignCarryState } from "./campaign/tdCampaign";
import { TouchController, type ScreenPoint } from "./input/TouchController";
import {
  contextualObjectIdentity,
  findNativeContextualTarget,
  resolveContextualAction,
  type ResolvedContextualAction,
} from "./input/contextualAction";
import { controlGroupHotkey } from "./input/controlGroupHotkeys";
import {
  boxSelectCommand,
  cancelPlacementCommand,
  cancelProductionCommand,
  cancelStructureActionCommand,
  createControlGroupCommand,
  holdProductionCommand,
  movieDoneCommand,
  placeProductionCommand,
  pointCommand,
  repairStructureCommand,
  selectControlGroupCommand,
  sellStructureCommand,
  sellWallAtWorldCommand,
  startPlacementCommand,
  startProductionCommand,
  startRepairCommand,
  startSellCommand,
  stopSelectedCommand,
  targetSuperweaponCommand,
  type InteractionMode,
} from "./input/gameCommands";
import { battlefieldSelectionPresentation, type BattlefieldSelectionPresentation } from "./input/selectionModel";
import { BattlefieldOnboarding } from "./onboarding/BattlefieldOnboarding";
import { MissionObjectives } from "./objectives/MissionObjectives";
import { runtimePerformanceMetrics } from "./performance/runtimeMetrics";
import { ProductionPanel, type ProductionEntryPresentation, type ProductionPrimaryAction } from "./production/ProductionPanel";
import { decodePlacementOffset, describeProductionEntry, firstLegalPlacementCell, productionEntryKey } from "./production/productionModel";
import {
  registerServiceWorker,
  type ServiceWorkerRegistrationController,
  type ServiceWorkerUpdateState,
} from "./pwa/registerServiceWorker";
import { ClassicSurfaceAccumulator, type AccumulatedClassicSurface } from "./render/ClassicSurfaceAccumulator";
import { WebGLRenderer, type CameraTransform, type GraphicsMode } from "./render/WebGLRenderer";
import { buildMinimapImage } from "./render/minimap";
import { clampCameraTransform, focusCameraTransform, pointToWorld, presentationViewport, visibleWorldRect } from "./render/viewport";
import { SimulationClient } from "./simulation/SimulationClient";
import { localAcceptanceSession } from "./simulation/acceptanceHooks";
import { domTelemetryRefreshDue, minimapRefreshDue } from "./simulation/domTelemetryCadence";
import { Faction, GameMode, type CampaignTransition, type ContentMountProgress, type SimulationEvent, type StartConfiguration } from "./simulation/protocol";
import { loadRuntimeLibrary, type CompatibleRuntimePack, type RuntimeLibrary } from "./simulation/runtimeLibrary";
import { missionStartConfiguration, type RuntimeMissionV1 } from "./simulation/runtimeCatalog";
import { MAP_CELL_PIXELS, SnapshotContextualAction, SnapshotObjectType, type SnapshotObject, type SnapshotSidebar, type SnapshotSidebarEntry, type SnapshotView } from "./simulation/snapshot";
import { ContentStore, type ContentFileSource, type ContentManifest } from "./storage/ContentStore";
import { OpfsBinaryStore } from "./storage/OpfsBinaryStore";
import { DEFAULT_PACKAGE_LIMITS, importCncwebPackage } from "./storage/PackageImporter";
import { SaveStore, type SaveMetadata, type StoredSave } from "./storage/SaveStore";
import { checkStorageReadiness, type StorageReadiness } from "./storage/helpers";
import {
  loadPersistedSession,
  savePersistedSession,
  type PersistedCampaignTransitionV2,
  type PersistedPendingVictoryV2,
  type PersistedSession,
} from "./storage/session";

interface SelectionBox { left: number; top: number; width: number; height: number }
interface CommandMarker extends ScreenPoint { id: number; alternate: boolean; ping?: boolean }

type BattlefieldTool =
  | { kind: "placement"; entry: SnapshotSidebarEntry; entryKey: string; phase: "requesting" | "active" | "placing"; requestedTick: number }
  | { kind: "repair" }
  | { kind: "sell" }
  | { kind: "superweapon"; entry: SnapshotSidebarEntry; entryKey: string };

interface DemoLaunch {
  kind: "demo";
  key: string;
  seed: number;
  start: StartConfiguration;
  resumeSaveId?: string;
}

interface MissionLaunch {
  kind: "mission";
  key: string;
  seed: number;
  pack: CompatibleRuntimePack;
  mission: RuntimeMissionV1;
  start: StartConfiguration;
  resumeSaveId?: string;
  runId?: string;
  incomingTransition?: PersistedCampaignTransitionV2;
  acceptLegacyResume?: boolean;
}

type Launch = DemoLaunch | MissionLaunch;
type GameOverEvent = Extract<SimulationEvent, { kind: "game-over" }>;
type CampaignOutcomeEvent = Extract<SimulationEvent, { kind: "campaign-outcome" }>;
type DiagnosticEvent = Extract<SimulationEvent, { kind: "diagnostic" }>;

let launchSequence = 0;
const EMPTY_SELECTION_PRESENTATION = battlefieldSelectionPresentation([], undefined);

const DIALOG_FOCUSABLE = "button:not(:disabled), a[href], input:not(:disabled), select:not(:disabled), textarea:not(:disabled), [tabindex]:not([tabindex='-1'])";

function useDialogFocus(open: boolean, dialogRef: RefObject<HTMLElement | null>, restoreRef?: RefObject<HTMLElement | null>): void {
  useEffect(() => {
    if (!open || !dialogRef.current) return;
    const dialog = dialogRef.current;
    const previous = restoreRef?.current ?? (document.activeElement instanceof HTMLElement ? document.activeElement : undefined);
    const focusable = (): HTMLElement[] => [...dialog.querySelectorAll<HTMLElement>(DIALOG_FOCUSABLE)]
      .filter((element) => element.getAttribute("aria-hidden") !== "true");
    const focusFrame = requestAnimationFrame(() => (dialog.querySelector<HTMLElement>("[autofocus]") ?? focusable()[0] ?? dialog).focus({ preventScroll: true }));
    const trapFocus = (event: KeyboardEvent): void => {
      if (event.key !== "Tab") return;
      const entries = focusable();
      if (!entries.length) {
        event.preventDefault();
        dialog.focus({ preventScroll: true });
        return;
      }
      const first = entries[0];
      const last = entries.at(-1)!;
      const active = document.activeElement;
      if (event.shiftKey && (active === first || !dialog.contains(active))) {
        event.preventDefault();
        last.focus({ preventScroll: true });
      } else if (!event.shiftKey && (active === last || !dialog.contains(active))) {
        event.preventDefault();
        first.focus({ preventScroll: true });
      }
    };
    document.addEventListener("keydown", trapFocus);
    return () => {
      cancelAnimationFrame(focusFrame);
      document.removeEventListener("keydown", trapFocus);
      if (previous?.isConnected) previous.focus({ preventScroll: true });
    };
  }, [dialogRef, open, restoreRef]);
}

function randomSeed(): number {
  const value = crypto.getRandomValues(new Uint32Array(1))[0];
  return value || 1;
}

function newCampaignRunId(): string {
  return `campaign-${crypto.randomUUID().toLowerCase()}`;
}

function demoLaunch(seed = randomSeed(), resumeSaveId?: string): DemoLaunch {
  return {
    kind: "demo",
    key: `demo-${++launchSequence}`,
    seed,
    resumeSaveId,
    start: {
      game: "demo",
      seed,
      scenario: 1,
      variation: 0,
      direction: 0,
      buildLevel: 1,
      sabotagedStructure: -1,
      faction: Faction.Gdi,
      gameMode: GameMode.Campaign,
      playerId: 0n,
      contentDirectory: "/demo",
      overrideMapName: "",
      contentIdHash: 1n,
    },
  };
}

interface MissionLaunchOptions {
  seed?: number;
  resumeSaveId?: string;
  runId?: string;
  incomingTransition?: PersistedCampaignTransitionV2;
  acceptLegacyResume?: boolean;
}

function missionLaunch(pack: CompatibleRuntimePack, mission: RuntimeMissionV1, options: MissionLaunchOptions = {}): MissionLaunch {
  const seed = options.seed ?? randomSeed();
  const start = missionStartConfiguration(pack.descriptor, pack.catalog, mission, seed);
  if (options.incomingTransition) {
    const campaignTransition: CampaignTransition = {
      carryOverCredits: options.incomingTransition.carryOverCredits,
      nukePieces: options.incomingTransition.nukePieces,
    };
    // A continued campaign preserves the terminal RNG bits, including zero.
    start.seed = seed >>> 0;
    start.sabotagedStructure = options.incomingTransition.sabotagedStructure;
    start.campaignTransition = campaignTransition;
  }
  return {
    kind: "mission",
    key: `${pack.descriptor.id}-${pack.descriptor.revision.slice(0, 12)}-${mission.id}-${++launchSequence}`,
    seed,
    pack,
    mission,
    resumeSaveId: options.resumeSaveId,
    runId: options.runId ?? newCampaignRunId(),
    incomingTransition: options.incomingTransition,
    acceptLegacyResume: options.acceptLegacyResume,
    start,
  };
}

function persistedSession(launch: Launch, resumeSaveId?: string, pendingVictory?: PersistedPendingVictoryV2): PersistedSession {
  if (launch.kind === "demo") return { version: 1, mode: "demo", seed: launch.seed, ...(resumeSaveId ? { resumeSaveId } : {}) };
  if (!launch.runId) return {
    version: 1,
    mode: "mission",
    packageId: launch.pack.descriptor.id,
    revision: launch.pack.descriptor.revision,
    missionId: launch.mission.id,
    seed: launch.seed || 1,
    ...(resumeSaveId ? { resumeSaveId } : {}),
  };
  return {
    version: 2,
    mode: "mission",
    packageId: launch.pack.descriptor.id,
    revision: launch.pack.descriptor.revision,
    missionId: launch.mission.id,
    seed: launch.seed,
    runId: launch.runId,
    ...(resumeSaveId ? { resumeSaveId } : {}),
    ...(launch.acceptLegacyResume && resumeSaveId === launch.resumeSaveId ? { legacyResume: true as const } : {}),
    ...(launch.incomingTransition ? { incomingTransition: launch.incomingTransition } : {}),
    ...(pendingVictory ? { pendingVictory } : {}),
  };
}

function rememberSession(launch: Launch, resumeSaveId?: string, pendingVictory?: PersistedPendingVictoryV2): void {
  try { savePersistedSession(persistedSession(launch, resumeSaveId, pendingVictory)); }
  catch (error) { console.warn("Runtime session could not be persisted", error); }
}

function saveMatchesLaunch(save: StoredSave, launch: Launch): boolean {
  if (launch.kind === "demo") return save.game === "demo" && save.contentPackageId === undefined;
  return save.game === "tiberian-dawn"
    && save.scenario === launch.mission.id
    && save.contentPackageId === launch.pack.descriptor.id
    && save.contentRevision === launch.pack.descriptor.revision
    && save.missionId === launch.mission.id
    && (save.runId === launch.runId
      || Boolean(launch.acceptLegacyResume && save.id === launch.resumeSaveId && save.runId === undefined));
}

function saveMatchesMissionIdentity(save: StoredSave, pack: CompatibleRuntimePack, mission: RuntimeMissionV1): boolean {
  return save.game === "tiberian-dawn"
    && save.scenario === mission.id
    && save.contentPackageId === pack.descriptor.id
    && save.contentRevision === pack.descriptor.revision
    && save.missionId === mission.id;
}

function correlateCampaignOutcome(active: Launch | undefined, gameOver: GameOverEvent, outcome: CampaignOutcomeEvent): active is MissionLaunch {
  if (!active || active.kind !== "mission" || !gameOver.won || gameOver.multiplayer || !gameOver.human) return false;
  const expectedHouse = active.mission.faction === "gdi" ? 0 : 1;
  return outcome.tick === gameOver.tick
    && outcome.sabotagedStructure === gameOver.sabotagedStructure
    && outcome.scenario === active.mission.scenario
    && outcome.scenarioRoot === active.mission.scenarioRoot
    && outcome.house === expectedHouse;
}

function persistedVictory(gameOver: GameOverEvent, outcome: CampaignOutcomeEvent): PersistedPendingVictoryV2 {
  const { kind: _kind, ...persistedOutcome } = outcome;
  return {
    gameOver: {
      tick: gameOver.tick,
      score: gameOver.score,
      leadership: gameOver.leadership,
      efficiency: gameOver.efficiency,
      remainingCredits: gameOver.remainingCredits,
      sabotagedStructure: gameOver.sabotagedStructure,
      movieName: gameOver.movieName,
      afterScoreMovieName: gameOver.afterScoreMovieName,
    },
    outcome: persistedOutcome,
  };
}

function restoredVictory(pending: PersistedPendingVictoryV2): { gameOver: GameOverEvent; outcome: CampaignOutcomeEvent } {
  return {
    gameOver: {
      kind: "game-over",
      multiplayer: false,
      human: true,
      won: true,
      timerRemaining: -1,
      ...pending.gameOver,
    },
    outcome: { kind: "campaign-outcome", ...pending.outcome },
  };
}

function fileRelativePath(file: File): string {
  const path = file.webkitRelativePath || file.name;
  const segments = path.split("/");
  return segments.length > 1 ? segments.slice(1).join("/") : path;
}

function screenToWorld(point: ScreenPoint, canvas: HTMLCanvasElement, snapshot: SnapshotView | undefined, camera: CameraTransform, mode: GraphicsMode): ScreenPoint | undefined {
  if (!snapshot) return undefined;
  const bounds = canvas.getBoundingClientRect();
  const world = visibleWorldRect(snapshot, mode, camera);
  const viewport = presentationViewport(bounds.width, bounds.height, world, mode);
  return pointToWorld(point, viewport, world);
}

interface ContextualSnapshotObjects {
  roots: readonly SnapshotObject[];
  byIdentity: ReadonlyMap<number, SnapshotObject>;
}

const contextualRootCache = new WeakMap<SnapshotView, ContextualSnapshotObjects>();

function contextualRootObjects(snapshot: SnapshotView): ContextualSnapshotObjects {
  const cached = contextualRootCache.get(snapshot);
  if (cached) return cached;
  const shroud = snapshot.shroud;
  const roots = snapshot.objects().filter((object) => object.root
    && shroud?.isVisibleAtMapCell(object.cellX, object.cellY) === true);
  const byIdentity = new Map<number, SnapshotObject>();
  for (const object of roots) byIdentity.set(contextualObjectIdentity(object.type, object.id), object);
  const context = {
    roots,
    byIdentity,
  };
  contextualRootCache.set(snapshot, context);
  return context;
}

function contextualActionAtWorld(snapshot: SnapshotView, point: ScreenPoint, selectObjectInSelectMode = false): ResolvedContextualAction {
  const shroud = snapshot.shroud;
  const playerHouse = snapshot.player?.house ?? -1;
  const hasSelection = (snapshot.player?.actions.count ?? 0) > 0;
  const cellVisible = shroud?.isVisibleAtWorldPoint(point.x, point.y) === true;
  if (!cellVisible) {
    // Only player-owned selection state is derived before this branch. Hidden
    // object and terrain fields never participate in the result.
    return resolveContextualAction({
      point,
      rootObjects: [],
      playerHouse: -1,
      cellVisible: false,
      terrainAction: SnapshotContextualAction.None,
      hasSelection,
    });
  }

  const context = contextualRootObjects(snapshot);
  const nativeTargeting = snapshot.occupiers !== undefined && snapshot.staticMap !== undefined;
  const targetObject = nativeTargeting ? findNativeContextualTarget({
    point,
    rootObjects: context.roots,
    objectsByIdentity: context.byIdentity,
    occupiers: snapshot.occupiers!,
    map: snapshot.staticMap!,
    playerHouse,
    playerAllyFlags: snapshot.player?.allyFlags,
  }) ?? null : undefined;

  return resolveContextualAction({
    point,
    rootObjects: nativeTargeting ? [] : context.roots,
    targetObject,
    playerHouse,
    playerAllyFlags: snapshot.player?.allyFlags,
    cellVisible: true,
    terrainAction: snapshot.player?.actions.atWorldPoint(point.x, point.y) ?? SnapshotContextualAction.None,
    selectObjectWhenIdle: selectObjectInSelectMode && !hasSelection,
    hasSelection,
  });
}

function contextualOrderNotice(action: ResolvedContextualAction): string {
  if (action.action === SnapshotContextualAction.AttackOutOfRange) return "Attack order issued · selected units will approach";
  if (action.tone === "blocked") return action.label;
  return `${action.label} order issued`;
}

function worldToScreen(point: ScreenPoint, canvas: HTMLCanvasElement, snapshot: SnapshotView | undefined, camera: CameraTransform, mode: GraphicsMode): ScreenPoint | undefined {
  if (!snapshot) return undefined;
  const bounds = canvas.getBoundingClientRect();
  const world = visibleWorldRect(snapshot, mode, camera);
  if (point.x < world.x || point.y < world.y || point.x > world.x + world.width || point.y > world.y + world.height) return undefined;
  const viewport = presentationViewport(bounds.width, bounds.height, world, mode);
  return {
    x: viewport.x + ((point.x - world.x) / world.width) * viewport.width,
    y: viewport.y + ((point.y - world.y) / world.height) * viewport.height,
  };
}

function progressText(progress: ContentMountProgress | undefined): string {
  if (!progress) return "Loading browser engine…";
  if (progress.phase === "mounting") return "Mounting verified content read-only…";
  if (progress.phase === "complete") return "Starting mission engine…";
  const file = progress.currentPath ? ` · ${progress.currentPath}` : "";
  const mib = (progress.completedBytes / 1024 / 1024).toFixed(1);
  return `${progress.phase === "opening" ? "Opening" : "Verifying"} ${progress.completedFiles}/${progress.totalFiles} files · ${mib} MiB${file}`;
}

function drawMinimap(canvas: HTMLCanvasElement | null, surface: AccumulatedClassicSurface | undefined, palette: Uint8Array | undefined): void {
  if (!canvas || !surface || !palette) return;
  const image = buildMinimapImage(surface.pixels, palette, surface.width, surface.height);
  if (canvas.width !== image.width) canvas.width = image.width;
  if (canvas.height !== image.height) canvas.height = image.height;
  const context = canvas.getContext("2d", { alpha: false });
  if (!context) return;
  const frame = context.createImageData(image.width, image.height);
  frame.data.set(image.rgba);
  context.putImageData(frame, 0, 0);
}

interface PlacementPreview {
  legal: boolean;
  requestCell: { x: number; y: number };
  mapCells: readonly { x: number; y: number }[];
}

function placementPreviewAtWorld(snapshot: SnapshotView, entry: SnapshotSidebarEntry, world: ScreenPoint): PlacementPreview | undefined {
  const grid = snapshot.placement;
  if (!grid) return undefined;
  const mapCellX = Math.floor(world.x / MAP_CELL_PIXELS);
  const mapCellY = Math.floor(world.y / MAP_CELL_PIXELS);
  const anchor = grid.cellAtMapCell(mapCellX, mapCellY);
  if (!anchor) return undefined;
  const offsets = entry.placementOffsets.length ? entry.placementOffsets : [0];
  const mapCells = offsets.map((offset) => {
    const delta = decodePlacementOffset(offset);
    return { x: mapCellX + delta.x, y: mapCellY + delta.y };
  });
  const uniqueCells = [...new Map(mapCells.map((cell) => [`${cell.x}:${cell.y}`, cell])).values()];
  return {
    legal: grid.canPlace(entry, anchor.requestCellX, anchor.requestCellY),
    requestCell: { x: anchor.requestCellX, y: anchor.requestCellY },
    mapCells: uniqueCells,
  };
}

function drawPlacementOverlay(
  overlay: HTMLCanvasElement | null,
  battlefield: HTMLCanvasElement,
  snapshot: SnapshotView | undefined,
  camera: CameraTransform,
  mode: GraphicsMode,
  tool: BattlefieldTool | undefined,
  hover: ScreenPoint | undefined,
): void {
  if (!overlay) return;
  const active = Boolean(snapshot && hover && tool
    && ((tool.kind === "placement" && tool.phase !== "placing") || tool.kind === "repair" || tool.kind === "sell"));
  if (!active) {
    if (overlay.dataset.active !== "true") return;
    const context = overlay.getContext("2d");
    if (context) {
      context.setTransform(1, 0, 0, 1, 0, 0);
      context.clearRect(0, 0, overlay.width, overlay.height);
    }
    overlay.dataset.active = "false";
    delete overlay.dataset.frameKey;
    delete overlay.dataset.legal;
    return;
  }
  const frameKey = `${Math.floor((snapshot?.tick ?? 0) / 3)}:${camera.x}:${camera.y}:${camera.zoom}:${hover?.x}:${hover?.y}:${tool?.kind}:${battlefield.clientWidth}:${battlefield.clientHeight}`;
  if (overlay.dataset.frameKey === frameKey) return;
  overlay.dataset.frameKey = frameKey;
  const bounds = battlefield.getBoundingClientRect();
  const ratio = Math.min(2, Math.max(1, window.devicePixelRatio || 1));
  const width = Math.max(1, Math.round(bounds.width * ratio));
  const height = Math.max(1, Math.round(bounds.height * ratio));
  if (overlay.width !== width) overlay.width = width;
  if (overlay.height !== height) overlay.height = height;
  const context = overlay.getContext("2d");
  if (!context) return;
  context.setTransform(ratio, 0, 0, ratio, 0, 0);
  context.clearRect(0, 0, bounds.width, bounds.height);
  /* Narrowed by the active check above. */
  if (!snapshot || !tool || !hover) return;
  const worldPoint = screenToWorld(hover, battlefield, snapshot, camera, mode);
  const world = visibleWorldRect(snapshot, mode, camera);
  const viewport = presentationViewport(bounds.width, bounds.height, world, mode);
  const project = (point: ScreenPoint): ScreenPoint => ({
    x: viewport.x + ((point.x - world.x) / world.width) * viewport.width,
    y: viewport.y + ((point.y - world.y) / world.height) * viewport.height,
  });
  if (tool.kind === "repair" || tool.kind === "sell") {
    const target = worldPoint
      ? tool.kind === "repair"
        ? snapshot.findBuildingAtWorldPoint(worldPoint.x, worldPoint.y, { capability: "repair" })
        : snapshot.findSellTargetAtWorldPoint(worldPoint.x, worldPoint.y)
      : undefined;
    const record = target && "kind" in target ? (target.kind === "building" ? target.building : target.wall) : target;
    if (!record) {
      overlay.dataset.legal = "false";
      overlay.dataset.active = "true";
      return;
    }
    let left = record.x;
    let top = record.y;
    const targetWidth = Math.max(1, record.width);
    const targetHeight = Math.max(1, record.height);
    if (record.drawFlags & 0x20) { left -= targetWidth / 2; top -= targetHeight / 2; }
    else if (record.drawFlags & 0x40) top -= targetHeight;
    const topLeft = project({ x: left, y: top });
    const bottomRight = project({ x: left + targetWidth, y: top + targetHeight });
    context.lineWidth = 2;
    context.fillStyle = tool.kind === "sell" ? "rgba(226, 101, 83, .20)" : "rgba(164, 221, 124, .18)";
    context.strokeStyle = tool.kind === "sell" ? "rgba(255, 154, 137, .95)" : "rgba(205, 245, 171, .95)";
    context.fillRect(topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y);
    context.strokeRect(topLeft.x + 1, topLeft.y + 1, Math.max(0, bottomRight.x - topLeft.x - 2), Math.max(0, bottomRight.y - topLeft.y - 2));
    overlay.dataset.legal = "true";
    overlay.dataset.active = "true";
    return;
  }
  if (tool.kind !== "placement") return;
  const preview = worldPoint ? placementPreviewAtWorld(snapshot, tool.entry, worldPoint) : undefined;
  if (!preview) {
    overlay.dataset.legal = "false";
    overlay.dataset.active = "true";
    return;
  }
  context.lineWidth = 1.5;
  context.fillStyle = preview.legal ? "rgba(164, 221, 124, .30)" : "rgba(226, 101, 83, .32)";
  context.strokeStyle = preview.legal ? "rgba(205, 245, 171, .95)" : "rgba(255, 170, 153, .95)";
  for (const cell of preview.mapCells) {
    const topLeft = project({ x: cell.x * MAP_CELL_PIXELS, y: cell.y * MAP_CELL_PIXELS });
    const bottomRight = project({ x: (cell.x + 1) * MAP_CELL_PIXELS, y: (cell.y + 1) * MAP_CELL_PIXELS });
    const cellWidth = bottomRight.x - topLeft.x;
    const cellHeight = bottomRight.y - topLeft.y;
    context.fillRect(topLeft.x, topLeft.y, cellWidth, cellHeight);
    context.strokeRect(topLeft.x + .75, topLeft.y + .75, Math.max(0, cellWidth - 1.5), Math.max(0, cellHeight - 1.5));
  }
  overlay.dataset.legal = String(preview.legal);
  overlay.dataset.active = "true";
}

export default function App() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const importRef = useRef<HTMLInputElement>(null);
  const developmentImportRef = useRef<HTMLInputElement>(null);
  const importButtonRef = useRef<HTMLButtonElement>(null);
  const aboutButtonRef = useRef<HTMLButtonElement>(null);
  const importDialogRef = useRef<HTMLElement>(null);
  const aboutDialogRef = useRef<HTMLElement>(null);
  const gameOverDialogRef = useRef<HTMLElement>(null);
  const minimapRef = useRef<HTMLCanvasElement>(null);
  const placementOverlayRef = useRef<HTMLCanvasElement>(null);
  const clientRef = useRef<SimulationClient | undefined>(undefined);
  const snapshotRef = useRef<SnapshotView | undefined>(undefined);
  const modeRef = useRef<GraphicsMode>("classic");
  const cameraRef = useRef<CameraTransform>({ x: 0, y: 0, zoom: 1 });
  const interactionModeRef = useRef<InteractionMode>("select");
  const battlefieldToolRef = useRef<BattlefieldTool | undefined>(undefined);
  const battlefieldHoverRef = useRef<ScreenPoint | undefined>(undefined);
  const quickPlaceCursorRef = useRef(new Map<string, number>());
  const contextualHoverRef = useRef<ResolvedContextualAction | undefined>(undefined);
  const contextualHoverDirtyRef = useRef(false);
  const toolRecoverySuppressedThroughTickRef = useRef(0);
  const missionGraphicsLockedRef = useRef(false);
  const runningRef = useRef(false);
  const contentStoreRef = useRef<ContentStore | undefined>(undefined);
  const saveStoreRef = useRef<SaveStore | undefined>(undefined);
  const activeLaunchRef = useRef<Launch | undefined>(undefined);
  const audioRef = useRef<RuntimeAudio | undefined>(undefined);
  const saveCreatedAtRef = useRef(new Date().toISOString());
  const resumeAfterVisibilityRef = useRef(false);
  const resumeAfterGraphicsRestoreRef = useRef(false);
  const graphicsContextLostRef = useRef(false);
  const terminalRef = useRef(false);
  const campaignOutcomeRef = useRef<CampaignOutcomeEvent | undefined>(undefined);
  const modalWasOpenRef = useRef(false);
  const resumeAfterModalRef = useRef(false);
  const saveInFlightRef = useRef(false);
  const loadInFlightRef = useRef(false);
  const applyingUpdateRef = useRef(false);
  const sessionEpochRef = useRef(0);
  const pendingCameraFocusRef = useRef<ScreenPoint | undefined>(undefined);
  const selectionPresentationRef = useRef<BattlefieldSelectionPresentation>(EMPTY_SELECTION_PRESENTATION);
  const domTelemetryRefreshTickRef = useRef<number | undefined>(undefined);
  const minimapRefreshTickRef = useRef<number | undefined>(undefined);

  const [mode, setMode] = useState<GraphicsMode>("classic");
  const [interactionMode, setInteractionMode] = useState<InteractionMode>("select");
  const [battlefieldTool, setBattlefieldTool] = useState<BattlefieldTool>();
  const [running, setRunning] = useState(false);
  const [tick, setTick] = useState(0);
  const [fps, setFps] = useState(0);
  const [cameraZoom, setCameraZoom] = useState(1);
  const [error, setError] = useState<string>();
  const [notice, setNotice] = useState("Checking classic freeware content…");
  const [aboutOpen, setAboutOpen] = useState(false);
  const [sidebarOpen, setSidebarOpen] = useState(true);
  const [selectionBox, setSelectionBox] = useState<SelectionBox>();
  const [marker, setMarker] = useState<CommandMarker>();
  const [storage, setStorage] = useState<StorageReadiness>();
  const [library, setLibrary] = useState<RuntimeLibrary>({ compatible: [], incompatible: [] });
  const [libraryReady, setLibraryReady] = useState(false);
  const [selectedPackId, setSelectedPackId] = useState("");
  const [selectedMissionId, setSelectedMissionId] = useState("");
  const [launch, setLaunch] = useState<Launch>();
  const [mountProgress, setMountProgress] = useState<ContentMountProgress>();
  const [launching, setLaunching] = useState(false);
  const [diagnostics, setDiagnostics] = useState<DiagnosticEvent[]>([]);
  const [gameOver, setGameOver] = useState<GameOverEvent>();
  const [campaignOutcome, setCampaignOutcome] = useState<CampaignOutcomeEvent>();
  const [saveCount, setSaveCount] = useState(0);
  const [manualSaveCount, setManualSaveCount] = useState(0);
  const [supportsSaves, setSupportsSaves] = useState(false);
  const [missionStats, setMissionStats] = useState<SnapshotSidebar>();
  const [missionStatsLaunchKey, setMissionStatsLaunchKey] = useState<string>();
  const [selectionPresentation, setSelectionPresentation] = useState<BattlefieldSelectionPresentation>(EMPTY_SELECTION_PRESENTATION);
  const [contextualHover, setContextualHover] = useState<ResolvedContextualAction>();
  const [assigningControlGroup, setAssigningControlGroup] = useState(false);
  const [saving, setSaving] = useState(false);
  const [loading, setLoading] = useState(false);
  const [importing, setImporting] = useState(false);
  const [bootstrapping, setBootstrapping] = useState(false);
  const [pendingImport, setPendingImport] = useState<File>();
  const [updateController, setUpdateController] = useState<ServiceWorkerRegistrationController>();
  const [updateState, setUpdateState] = useState<ServiceWorkerUpdateState>({ status: "checking" });
  const [applyingUpdate, setApplyingUpdate] = useState(false);

  useDialogFocus(Boolean(pendingImport), importDialogRef, importButtonRef);
  useDialogFocus(aboutOpen, aboutDialogRef, aboutButtonRef);
  useDialogFocus(Boolean(gameOver), gameOverDialogRef);

  useEffect(() => {
    if (!localAcceptanceSession()) return;
    const api = Object.freeze({
      forceVictory: async (): Promise<void> => {
        const client = clientRef.current;
        if (!client) throw new Error("No active simulation is available for release acceptance");
        await client.forceVictoryForAcceptance();
      },
    });
    Object.defineProperty(window, "__cncwebAcceptance", { configurable: true, value: api });
    return () => { delete window.__cncwebAcceptance; };
  }, []);

  const selectedPack = library.compatible.find((pack) => pack.descriptor.id === selectedPackId);
  const selectedMission = selectedPack?.catalog.missions.find((mission) => mission.id === selectedMissionId) ?? selectedPack?.catalog.missions[0];
  const activeMissionStats = missionStatsLaunchKey === launch?.key ? missionStats : undefined;
  const activeCampaignLaunch = activeLaunchRef.current?.kind === "mission" ? activeLaunchRef.current : undefined;
  const correlatedCampaignResult = gameOver && campaignOutcome && correlateCampaignOutcome(activeCampaignLaunch, gameOver, campaignOutcome);
  const campaignChoices = correlatedCampaignResult
    ? nextTdCampaignMissions(activeCampaignLaunch.pack.catalog, activeCampaignLaunch.mission, campaignOutcome.sabotagedStructure)
    : [];
  const campaignComplete = Boolean(correlatedCampaignResult && isTdCampaignFinalMission(activeCampaignLaunch.mission));
  const contentCount = library.compatible.length + library.incompatible.length;
  const applicationModalOpen = Boolean(pendingImport) || aboutOpen || Boolean(gameOver);
  const battlefieldToolLabel = battlefieldTool?.kind === "placement"
    ? `${battlefieldTool.phase === "requesting" ? "Preparing" : battlefieldTool.phase === "placing" ? "Placing" : "Place"} ${describeProductionEntry(battlefieldTool.entry).label}`
    : battlefieldTool?.kind === "superweapon" ? `Target ${describeProductionEntry(battlefieldTool.entry).label}`
      : battlefieldTool?.kind === "repair" ? "Repair structures" : battlefieldTool?.kind === "sell" ? "Sell structures" : undefined;

  const clearContextualHover = useCallback((): void => {
    contextualHoverDirtyRef.current = false;
    if (contextualHoverRef.current) {
      contextualHoverRef.current = undefined;
      setContextualHover(undefined);
    }
    const canvas = canvasRef.current;
    if (canvas) {
      delete canvas.dataset.contextualAction;
      delete canvas.dataset.contextualCursor;
    }
  }, []);

  const presentContextualHover = useCallback((point: ScreenPoint | undefined, snapshot = snapshotRef.current): void => {
    const canvas = canvasRef.current;
    if (!point || !snapshot || !canvas || battlefieldToolRef.current || terminalRef.current) {
      clearContextualHover();
      return;
    }
    const world = screenToWorld(point, canvas, snapshot, cameraRef.current, modeRef.current);
    if (!world) {
      clearContextualHover();
      return;
    }
    const next = contextualActionAtWorld(
      snapshot,
      world,
      interactionModeRef.current === "select",
    );
    canvas.dataset.contextualAction = String(next.action);
    canvas.dataset.contextualCursor = next.cursor;
    const current = contextualHoverRef.current;
    if (current?.action === next.action && current.source === next.source && current.label === next.label
      && current.tone === next.tone && current.cursor === next.cursor) return;
    contextualHoverRef.current = next;
    setContextualHover(next);
  }, [clearContextualHover]);

  const assignBattlefieldTool = useCallback((next: BattlefieldTool | undefined): void => {
    battlefieldToolRef.current = next;
    setBattlefieldTool(next);
    const canvas = canvasRef.current;
    if (canvas) canvas.dataset.tool = next?.kind ?? interactionModeRef.current;
    clearContextualHover();
    if (!next) battlefieldHoverRef.current = undefined;
  }, [clearContextualHover]);

  const cancelBattlefieldTool = useCallback((announce = true): void => {
    const current = battlefieldToolRef.current;
    if (!current) return;
    if (loadInFlightRef.current) {
      if (announce) setNotice("Wait for the load to finish");
      return;
    }
    const client = clientRef.current;
    if (client) {
      if (current.kind === "placement") client.sendCommands([cancelPlacementCommand(current.entry)]);
      else if (current.kind === "repair" || current.kind === "sell") client.sendCommands([cancelStructureActionCommand()]);
    }
    toolRecoverySuppressedThroughTickRef.current = (snapshotRef.current?.tick ?? 0) + 1;
    assignBattlefieldTool(undefined);
    if (announce) setNotice("Battlefield tool canceled");
  }, [assignBattlefieldTool]);

  const quickPlaceStructure = useCallback((): void => {
    const current = battlefieldToolRef.current;
    const client = clientRef.current;
    const snapshot = snapshotRef.current;
    if (!client || !snapshot || !runningRef.current || snapshot.terminal || loadInFlightRef.current) {
      setNotice("Resume the simulation before placing structures");
      return;
    }
    if (current?.kind !== "placement" || current.phase !== "active" || !snapshot.placement) {
      setNotice("Wait for the placement grid to become ready");
      return;
    }
    const cell = firstLegalPlacementCell(snapshot.placement, current.entry, quickPlaceCursorRef.current.get(current.entryKey) ?? 0);
    if (!cell) {
      setNotice("No legal build site is currently available");
      return;
    }
    quickPlaceCursorRef.current.set(
      current.entryKey,
      (cell.y * snapshot.placement.width + cell.x + 1) % snapshot.placement.count,
    );
    client.sendCommands([placeProductionCommand(current.entry, cell)]);
    assignBattlefieldTool({ ...current, phase: "placing", requestedTick: snapshot.tick });
    setNotice("Placing structure…");
  }, [assignBattlefieldTool]);

  const toggleMode = useCallback(() => {
    if (missionGraphicsLockedRef.current) {
      modeRef.current = "classic";
      setMode("classic");
      setNotice("Enhanced atlases are not present in this mission pack · Classic graphics remain active");
      return;
    }
    setMode((current) => {
      const next = current === "classic" ? "remastered" : "classic";
      modeRef.current = next;
      return next;
    });
  }, []);

  const updateSaveCount = useCallback(async (target = activeLaunchRef.current) => {
    const store = saveStoreRef.current;
    if (!store || !target) return;
    const matches = (await store.list()).filter((save) => saveMatchesLaunch(save, target));
    if (activeLaunchRef.current?.key !== target.key) return;
    setSaveCount(matches.length);
    setManualSaveCount(matches.filter((save) => save.kind === "manual").length);
  }, []);

  const applyCamera = useCallback((candidate: CameraTransform) => {
    const snapshot = snapshotRef.current;
    const next = snapshot ? clampCameraTransform(snapshot, modeRef.current, candidate) : candidate;
    cameraRef.current = next;
    contextualHoverDirtyRef.current = true;
    setCameraZoom(next.zoom);
    if (snapshot && audioRef.current) {
      const view = visibleWorldRect(snapshot, modeRef.current, next);
      audioRef.current.setView(view.x, view.width);
    }
    const canvas = canvasRef.current;
    if (canvas) {
      canvas.dataset.cameraX = String(next.x);
      canvas.dataset.cameraY = String(next.y);
      canvas.dataset.cameraZoom = String(next.zoom);
    }
  }, []);

  const focusCameraOnWorld = useCallback((point: ScreenPoint) => {
    const snapshot = snapshotRef.current;
    if (!snapshot) return;
    applyCamera(focusCameraTransform(snapshot, modeRef.current, cameraRef.current, point));
  }, [applyCamera]);

  const panCamera = useCallback((delta: ScreenPoint) => {
    const snapshot = snapshotRef.current;
    const canvas = canvasRef.current;
    const bounds = canvas?.getBoundingClientRect();
    if (!snapshot || !canvas || !bounds?.width || !bounds.height) return;
    const world = visibleWorldRect(snapshot, modeRef.current, cameraRef.current);
    const viewport = presentationViewport(bounds.width, bounds.height, world, modeRef.current);
    if (!viewport.width || !viewport.height) return;
    applyCamera({
      ...cameraRef.current,
      x: cameraRef.current.x + (-delta.x / viewport.width) * world.width,
      y: cameraRef.current.y + (-delta.y / viewport.height) * world.height,
    });
  }, [applyCamera]);

  const zoomCamera = useCallback((factor: number, center?: ScreenPoint) => {
    const snapshot = snapshotRef.current;
    const canvas = canvasRef.current;
    const bounds = canvas?.getBoundingClientRect();
    if (!snapshot || !canvas || !bounds?.width || !bounds.height || !Number.isFinite(factor) || factor <= 0) return;
    const anchor = center ?? { x: bounds.width / 2, y: bounds.height / 2 };
    const before = screenToWorld(anchor, canvas, snapshot, cameraRef.current, modeRef.current);
    let next = clampCameraTransform(snapshot, modeRef.current, {
      ...cameraRef.current,
      zoom: cameraRef.current.zoom * factor,
    });
    const after = screenToWorld(anchor, canvas, snapshot, next, modeRef.current);
    if (before && after) {
      next = clampCameraTransform(snapshot, modeRef.current, {
        ...next,
        x: next.x + before.x - after.x,
        y: next.y + before.y - after.y,
      });
    }
    applyCamera(next);
  }, [applyCamera]);

  const resetCamera = useCallback(() => applyCamera({ x: 0, y: 0, zoom: 1 }), [applyCamera]);

  const issueControlGroup = useCallback((index: number, action: "create" | "select" | "additive", focus = false): void => {
    const client = clientRef.current;
    const snapshot = snapshotRef.current;
    if (!client || !snapshot || !runningRef.current || snapshot.terminal || loadInFlightRef.current) {
      setNotice("Resume the simulation before using control groups");
      return;
    }
    const group = selectionPresentationRef.current.groups[index];
    if (!group) return;
    if (action === "create") {
      client.sendCommands([createControlGroupCommand(index)]);
      setNotice(selectionPresentationRef.current.assignableCount > 0
        ? `Assigned ${selectionPresentationRef.current.assignableCount} selected ${selectionPresentationRef.current.assignableCount === 1 ? "mobile unit" : "mobile units"} to control group ${group.key}`
        : `Cleared control group ${group.key}`);
      return;
    }
    if (group.count === 0) {
      setNotice(`Control group ${group.key} is empty`);
      return;
    }
    client.sendCommands([selectControlGroupCommand(index, action === "additive")]);
    const shouldFocus = focus || (action === "select" && group.active);
    if (shouldFocus && group.center) focusCameraOnWorld(group.center);
    setNotice(`${action === "additive" ? "Added" : "Selected"} control group ${group.key} · ${group.count} ${group.count === 1 ? "object" : "objects"}${shouldFocus ? " · camera centered" : ""}`);
  }, [focusCameraOnWorld]);

  const restoreSavePresentation = useCallback((metadata: StoredSave, active: Launch) => {
    const presentation = metadata.presentation;
    const graphicsMode = active.kind === "mission" ? "classic" : presentation?.graphicsMode ?? "classic";
    modeRef.current = graphicsMode;
    setMode(graphicsMode);
    applyCamera(presentation
      ? { x: presentation.cameraX, y: presentation.cameraY, zoom: presentation.zoom }
      : { x: 0, y: 0, zoom: 1 });
  }, [applyCamera]);

  const persistSave = useCallback(async (kind: SaveMetadata["kind"], name: string) => {
    const client = clientRef.current;
    const store = saveStoreRef.current;
    const active = activeLaunchRef.current;
    const snapshot = snapshotRef.current;
    if (!client || !store || !active || !snapshot || saveInFlightRef.current || loadInFlightRef.current || snapshot.terminal) return false;
    saveInFlightRef.current = true;
    const sessionEpoch = sessionEpochRef.current;
    setSaving(true);
    try {
      const data = await client.save();
      const now = new Date().toISOString();
      const identity = active.kind === "mission" ? {
        contentPackageId: active.pack.descriptor.id,
        contentRevision: active.pack.descriptor.revision,
        missionId: active.mission.id,
        runId: active.runId,
      } : {};
      const stem = active.kind === "mission"
        ? `${active.pack.descriptor.revision.slice(0, 12)}-${active.mission.id.slice(0, 48)}-${active.runId ?? "legacy"}`
        : "demo";
      const stored = await store.write({
        id: kind === "autosave" ? `${stem}-autosave` : `${stem}-${Date.now().toString(36)}`,
        name,
        game: active.kind === "mission" ? "tiberian-dawn" : "demo",
        scenario: active.kind === "mission" ? active.mission.id : "browser-foundation",
        kind,
        tick: snapshot.tick,
        createdAt: kind === "autosave" ? saveCreatedAtRef.current : now,
        updatedAt: now,
        presentation: {
          cameraX: cameraRef.current.x,
          cameraY: cameraRef.current.y,
          zoom: cameraRef.current.zoom,
          graphicsMode: modeRef.current,
        },
        ...identity,
      }, data);
      if (sessionEpochRef.current === sessionEpoch && activeLaunchRef.current?.key === active.key && !terminalRef.current) {
        rememberSession(active, stored.id);
      }
      await updateSaveCount(active);
      setNotice(`${name} saved locally for this exact content revision`);
      return true;
    } catch (saveError) {
      setError(saveError instanceof Error ? saveError.message : String(saveError));
      return false;
    } finally {
      saveInFlightRef.current = false;
      setSaving(false);
    }
  }, [updateSaveCount]);

  const applyOfflineUpdate = useCallback(async (): Promise<void> => {
    const controller = updateController;
    if (!controller || updateState.status !== "ready" || applyingUpdateRef.current || launching || saving || loading || importing || pendingImport) return;
    const client = clientRef.current;
    const wasRunning = runningRef.current;
    let handedOffToUpdate = false;
    applyingUpdateRef.current = true;
    setApplyingUpdate(true);
    if (wasRunning) client?.setRunning(false);
    try {
      const active = activeLaunchRef.current;
      const snapshot = snapshotRef.current;
      const activeGameNeedsSave = Boolean(active && (!snapshot || !snapshot.terminal));
      if (activeGameNeedsSave && (!supportsSaves || !client || !saveStoreRef.current || !snapshot)) {
        setNotice("Update paused · this active game cannot be saved safely in the current browser");
        setError("The update was not applied because the active game could not be preserved.");
        return;
      }
      if (activeGameNeedsSave && !(await persistSave("autosave", "Update autosave"))) {
        setNotice("Update paused · the current game could not be saved");
        return;
      }
      handedOffToUpdate = await controller.applyUpdate();
      if (!handedOffToUpdate) setError("The installed update is no longer available. Check for it again.");
    } finally {
      if (!handedOffToUpdate) {
        applyingUpdateRef.current = false;
        setApplyingUpdate(false);
        if (wasRunning && clientRef.current === client && !snapshotRef.current?.terminal && document.visibilityState === "visible") {
          client?.setRunning(true);
        }
      }
    }
  }, [importing, launching, loading, pendingImport, persistSave, saving, supportsSaves, updateController, updateState.status]);

  const loadLatest = useCallback(async (preferredKind?: SaveMetadata["kind"]) => {
    const client = clientRef.current;
    const store = saveStoreRef.current;
    const active = activeLaunchRef.current;
    if (!client || !store || !active || loadInFlightRef.current || saveInFlightRef.current) return;
    loadInFlightRef.current = true;
    sessionEpochRef.current += 1;
    setLoading(true);
    setSelectionBox(undefined);
    let pausedForLoad = false;
    let wasRunning = false;
    let previousTool: BattlefieldTool | undefined;
    try {
      const matches = (await store.list()).filter((save) => saveMatchesLaunch(save, active));
      const candidates = preferredKind ? matches.filter((save) => save.kind === preferredKind) : matches;
      const selection = await store.readNewestValid(candidates);
      if (selection.issues.length) {
        console.warn("Unreadable local saves were ignored while loading", selection.issues);
        await updateSaveCount(active);
      }
      if (!selection.save) {
        setNotice(preferredKind === "manual" ? "No readable manual save matches this package revision and mission" : "No readable save matches this package revision and mission");
        return;
      }
      const { data, metadata } = selection.save;
      previousTool = battlefieldToolRef.current;
      wasRunning = runningRef.current;
      assignBattlefieldTool(undefined);
      toolRecoverySuppressedThroughTickRef.current = Number.MAX_SAFE_INTEGER;
      client.setRunning(false);
      pausedForLoad = true;
      await client.load(data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength) as ArrayBuffer);
      toolRecoverySuppressedThroughTickRef.current = 0;
      restoreSavePresentation(metadata, active);
      terminalRef.current = false;
      setGameOver(undefined);
      campaignOutcomeRef.current = undefined;
      setCampaignOutcome(undefined);
      const blockedByVisibility = document.visibilityState !== "visible";
      const blockedByGraphics = graphicsContextLostRef.current;
      const blockedByModal = modalWasOpenRef.current;
      if (blockedByVisibility || blockedByGraphics || blockedByModal) {
        if (blockedByVisibility) resumeAfterVisibilityRef.current = true;
        if (blockedByGraphics) resumeAfterGraphicsRestoreRef.current = true;
        if (blockedByModal) resumeAfterModalRef.current = true;
        client.setRunning(false);
      } else {
        client.setRunning(true);
      }
      rememberSession(active, metadata.id);
      setNotice(`Loaded ${metadata.name} from tick ${metadata.tick.toLocaleString()}`);
    } catch (loadError) {
      if (pausedForLoad && clientRef.current === client && !terminalRef.current) {
        assignBattlefieldTool(previousTool);
        if (wasRunning && document.visibilityState === "visible" && !graphicsContextLostRef.current && !modalWasOpenRef.current) {
          try { client.setRunning(true); } catch { /* The worker error is reported below. */ }
        }
      }
      setError(loadError instanceof Error ? loadError.message : String(loadError));
    } finally {
      toolRecoverySuppressedThroughTickRef.current = 0;
      loadInFlightRef.current = false;
      setLoading(false);
    }
  }, [assignBattlefieldTool, restoreSavePresentation, updateSaveCount]);

  const chooseInteractionMode = (next: InteractionMode): void => {
    if (loadInFlightRef.current) { setNotice("Wait for the load to finish"); return; }
    cancelBattlefieldTool(false);
    interactionModeRef.current = next;
    contextualHoverDirtyRef.current = true;
    setInteractionMode(next);
    if (canvasRef.current) canvasRef.current.dataset.tool = next;
    setNotice(next === "select" ? "Tap a unit to select it; drag to box-select" : "Tap the battlefield to issue a contextual move or attack order");
  };

  const presentProductionEntry = (entry: SnapshotSidebarEntry): ProductionEntryPresentation => {
    const description = describeProductionEntry(entry);
    if (entry.objectType === SnapshotObjectType.Special) {
      if (entry.completed) return { action: "target", actionLabel: "Target", disabled: false, status: "Ready" };
      return { actionLabel: "Recharging", disabled: true, status: `${Math.round(entry.progress * 100)}%` };
    }
    switch (description.primaryAction) {
      case "start": return { action: "start", actionLabel: "Build", disabled: false, status: "Available" };
      case "hold": return { action: "hold", actionLabel: "Pause", disabled: false, status: `${Math.round(description.progress * 100)}%` };
      case "resume": return { action: "resume", actionLabel: "Resume", disabled: false, status: `Paused · ${Math.round(description.progress * 100)}%` };
      case "place": return { action: "place", actionLabel: "Place", disabled: false, status: "Ready" };
      default:
        return { actionLabel: entry.completed ? "Deploying" : entry.busy ? "Busy" : "Unavailable", disabled: true, status: entry.completed ? "Ready" : entry.busy ? "Factory busy" : "Unavailable" };
    }
  };

  const issueProductionAction = (entry: SnapshotSidebarEntry, action: ProductionPrimaryAction): void => {
    const client = clientRef.current;
    const snapshot = snapshotRef.current;
    if (!client || !snapshot || !runningRef.current || snapshot.terminal || loadInFlightRef.current) {
      setNotice("Resume the simulation before using production controls");
      return;
    }
    cancelBattlefieldTool(false);
    const label = describeProductionEntry(entry).label;
    if (action === "start" || action === "resume") {
      client.sendCommands([startProductionCommand(entry)]);
      setNotice(`${action === "resume" ? "Resuming" : "Building"} ${label}`);
    } else if (action === "hold") {
      client.sendCommands([holdProductionCommand(entry)]);
      setNotice(`Pausing ${label}`);
    } else if (action === "place") {
      client.sendCommands([startPlacementCommand(entry)]);
      quickPlaceCursorRef.current.delete(productionEntryKey(entry));
      assignBattlefieldTool({ kind: "placement", entry, entryKey: productionEntryKey(entry), phase: "requesting", requestedTick: snapshot.tick });
      setSidebarOpen(false);
      requestAnimationFrame(() => canvasRef.current?.focus({ preventScroll: true }));
      setNotice(`Placing ${label} · tap a green footprint, or right-click to cancel`);
    } else if (action === "target") {
      assignBattlefieldTool({ kind: "superweapon", entry, entryKey: productionEntryKey(entry) });
      setSidebarOpen(false);
      requestAnimationFrame(() => canvasRef.current?.focus({ preventScroll: true }));
      setNotice(`${label} ready · tap a battlefield target, or right-click to cancel`);
    }
  };

  const cancelProduction = (entry: SnapshotSidebarEntry): void => {
    const client = clientRef.current;
    if (!client || !runningRef.current || loadInFlightRef.current) {
      setNotice("Resume the simulation before canceling production");
      return;
    }
    const active = battlefieldToolRef.current;
    if (active?.kind === "placement" && active.entryKey === productionEntryKey(entry)) {
      toolRecoverySuppressedThroughTickRef.current = (snapshotRef.current?.tick ?? 0) + 1;
      assignBattlefieldTool(undefined);
    }
    client.sendCommands([cancelProductionCommand(entry)]);
    setNotice(`Canceling ${describeProductionEntry(entry).label}`);
  };

  const toggleStructureTool = (kind: "repair" | "sell"): void => {
    const client = clientRef.current;
    if (!client || !runningRef.current || snapshotRef.current?.terminal || loadInFlightRef.current) {
      setNotice("Resume the simulation before using structure tools");
      return;
    }
    if (battlefieldToolRef.current?.kind === kind) {
      cancelBattlefieldTool();
      return;
    }
    cancelBattlefieldTool(false);
    client.sendCommands([kind === "repair" ? startRepairCommand() : startSellCommand()]);
    assignBattlefieldTool({ kind });
    setSidebarOpen(false);
    requestAnimationFrame(() => canvasRef.current?.focus({ preventScroll: true }));
    setNotice(kind === "repair" ? "Repair tool active · tap a damaged friendly structure" : "Sell tool active · tap a sellable friendly structure or wall");
  };

  const stopSelected = (): void => {
    const client = clientRef.current;
    if (!client || !runningRef.current || loadInFlightRef.current) {
      setNotice("Resume the simulation before issuing orders");
      return;
    }
    client.sendCommands([stopSelectedCommand()]);
    setNotice("Stop order sent to selected units");
  };

  const deploySelected = (): void => {
    const client = clientRef.current;
    if (!client || !runningRef.current || loadInFlightRef.current) {
      setNotice("Resume the simulation before deploying a unit");
      return;
    }
    const snapshot = snapshotRef.current;
    const currentPresentation = snapshot
      ? battlefieldSelectionPresentation(snapshot.objects(), snapshot.player?.house)
      : selectionPresentationRef.current;
    selectionPresentationRef.current = currentPresentation;
    setSelectionPresentation(currentPresentation);
    const deployment = currentPresentation.deployment;
    if (!deployment?.available) {
      setNotice("The selected unit cannot deploy at its current location");
      return;
    }
    client.sendCommands([pointCommand("order", deployment.target, true)]);
    setNotice("Deploy order sent");
  };

  const selectPack = (id: string): void => {
    setSelectedPackId(id);
    const pack = library.compatible.find((candidate) => candidate.descriptor.id === id);
    setSelectedMissionId(pack?.catalog.missions[0]?.id ?? "");
  };

  const startSelectedMission = (): void => {
    if (!selectedPack || !selectedMission || loadInFlightRef.current || saveInFlightRef.current) return;
    setError(undefined);
    setDiagnostics([]);
    setGameOver(undefined);
    campaignOutcomeRef.current = undefined;
    setCampaignOutcome(undefined);
    setMissionStats(undefined);
    setMissionStatsLaunchKey(undefined);
    const next = missionLaunch(selectedPack, selectedMission);
    sessionEpochRef.current += 1;
    rememberSession(next);
    missionGraphicsLockedRef.current = true;
    modeRef.current = "classic";
    setMode("classic");
    setLaunch(next);
  };

  const restartMission = (): void => {
    if (loadInFlightRef.current || saveInFlightRef.current) return;
    const active = activeLaunchRef.current;
    terminalRef.current = false;
    setGameOver(undefined);
    campaignOutcomeRef.current = undefined;
    setCampaignOutcome(undefined);
    setDiagnostics([]);
    setError(undefined);
    const next = active?.kind === "mission"
      ? missionLaunch(active.pack, active.mission, {
        seed: active.seed,
        runId: active.runId,
        incomingTransition: active.incomingTransition,
      })
      : demoLaunch(active?.seed);
    sessionEpochRef.current += 1;
    rememberSession(next);
    missionGraphicsLockedRef.current = next.kind === "mission";
    if (next.kind === "mission") { modeRef.current = "classic"; setMode("classic"); }
    setLaunch(next);
  };

  const continueCampaign = (mission: RuntimeMissionV1): void => {
    if (loadInFlightRef.current || saveInFlightRef.current) return;
    const active = activeLaunchRef.current;
    const result = gameOver;
    const outcome = campaignOutcomeRef.current;
    if (!active || active.kind !== "mission" || !result || !outcome || !correlateCampaignOutcome(active, result, outcome)) {
      setError("Campaign continuation state is unavailable or no longer matches this mission");
      return;
    }
    const choices = nextTdCampaignMissions(active.pack.catalog, active.mission, outcome.sabotagedStructure);
    const selected = choices.find((choice) => choice.id === mission.id && choice.scenarioRoot === mission.scenarioRoot);
    if (!selected) {
      setError("That campaign branch is not available in the selected content revision");
      return;
    }
    const incomingTransition: PersistedCampaignTransitionV2 = tdCampaignCarryState(active.mission, selected, outcome);
    const next = missionLaunch(active.pack, selected, {
      seed: outcome.randomSeed,
      runId: active.runId,
      incomingTransition,
    });
    sessionEpochRef.current += 1;
    rememberSession(next);
    terminalRef.current = false;
    campaignOutcomeRef.current = undefined;
    setCampaignOutcome(undefined);
    setGameOver(undefined);
    setDiagnostics([]);
    setError(undefined);
    setSelectedMissionId(selected.id);
    setLaunch(next);
    setNotice(`Continuing campaign · ${selected.title}`);
  };

  const closeCampaignResult = (): void => {
    if (saveInFlightRef.current || loadInFlightRef.current) return;
    const active = activeLaunchRef.current;
    if (active) rememberSession(active);
    setGameOver(undefined);
    campaignOutcomeRef.current = undefined;
    setCampaignOutcome(undefined);
    setNotice(campaignComplete ? "Campaign complete · choose any installed mission" : "Choose a standalone mission from the mission panel");
  };

  useEffect(() => {
    modeRef.current = mode;
    contextualHoverDirtyRef.current = true;
  }, [mode]);
  useEffect(() => { runningRef.current = running; }, [running]);

  useEffect(() => {
    if (applicationModalOpen && !modalWasOpenRef.current) {
      resumeAfterModalRef.current = !gameOver
        && (runningRef.current || resumeAfterVisibilityRef.current || resumeAfterGraphicsRestoreRef.current);
      if (runningRef.current) clientRef.current?.setRunning(false);
    }
    if (!applicationModalOpen && resumeAfterModalRef.current && !importing && !launching && !loading) {
      resumeAfterModalRef.current = false;
      if (!snapshotRef.current?.terminal) {
        if (graphicsContextLostRef.current) resumeAfterGraphicsRestoreRef.current = true;
        else if (document.visibilityState === "visible") {
          clientRef.current?.setRunning(true);
          setNotice("Simulation resumed");
        }
        else resumeAfterVisibilityRef.current = true;
      }
    }
    modalWasOpenRef.current = applicationModalOpen;
  }, [applicationModalOpen, gameOver, importing, launching, loading]);

  useEffect(() => {
    if (!pendingImport && !aboutOpen) return;
    const closeModal = (event: KeyboardEvent): void => {
      if (event.code !== "Escape") return;
      event.preventDefault();
      event.stopPropagation();
      setPendingImport(undefined);
      setAboutOpen(false);
    };
    document.addEventListener("keydown", closeModal);
    return () => document.removeEventListener("keydown", closeModal);
  }, [aboutOpen, pendingImport]);

  useEffect(() => {
    const unlock = () => { void audioRef.current?.unlock(); };
    window.addEventListener("pointerdown", unlock);
    window.addEventListener("keydown", unlock);
    return () => { window.removeEventListener("pointerdown", unlock); window.removeEventListener("keydown", unlock); };
  }, []);

  useEffect(() => {
    let disposed = false;
    let controller: ServiceWorkerRegistrationController | undefined;
    void registerServiceWorker(setUpdateState).then((value) => {
      if (disposed) value.dispose();
      else { controller = value; setUpdateController(value); }
    }).catch(() => undefined);
    return () => { disposed = true; controller?.dispose(); };
  }, []);

  useEffect(() => {
    let disposed = false;
    void (async () => {
      const readiness = await checkStorageReadiness();
      if (disposed) return;
      setStorage(readiness);
      if (!readiness.supported) {
        const fallback = demoLaunch();
        activeLaunchRef.current = fallback;
        setLaunch(fallback);
        setLibraryReady(true);
        setNotice("Private browser storage is unavailable · running explicit demo fallback");
        return;
      }
      try {
        const binary = new OpfsBinaryStore();
        const contentStore = new ContentStore(binary);
        const saveStore = new SaveStore(binary);
        contentStoreRef.current = contentStore;
        saveStoreRef.current = saveStore;
        const saveListingPromise = saveStore.listWithIssues();
        let nextLibrary = await loadRuntimeLibrary(contentStore);
        let freewareBootstrapStatus: "installed" | "already-installed" | undefined;
        let freewareBootstrapError: string | undefined;
        if (!nextLibrary.compatible.length) {
          if (!disposed) setBootstrapping(true);
          try {
            if (!disposed) setNotice("Downloading verified classic freeware content…");
            const bootstrap = await bootstrapClassicFreeware({
              listInstalled: () => contentStore.list(),
              importPackage: (archive, expected) => importCncwebPackage(
                archive,
                contentStore,
                (done, total) => { if (!disposed) setNotice(`Installing classic freeware ${done}/${total}`); },
                DEFAULT_PACKAGE_LIMITS,
                {
                  packageId: expected.id,
                  contentSha256: expected.contentSha256,
                  sourceProduct: expected.source.product,
                  sourceProvider: expected.source.provider,
                },
              ),
            });
            freewareBootstrapStatus = bootstrap.status;
            nextLibrary = await loadRuntimeLibrary(contentStore);
          } catch (bootstrapError) {
            freewareBootstrapError = bootstrapError instanceof Error ? bootstrapError.message : String(bootstrapError);
            console.warn("Classic freeware bootstrap was unavailable", bootstrapError);
          } finally {
            if (!disposed) setBootstrapping(false);
          }
        }
        const saveListing = await saveListingPromise;
        if (disposed) return;
        const saves = saveListing.saves;
        const saveIssues = [...saveListing.issues];
        setLibrary(nextLibrary);
        const remembered = loadPersistedSession();
        let nextLaunch: Launch;
        let startupNotice: string;
        let pendingRestore: { gameOver: GameOverEvent; outcome: CampaignOutcomeEvent } | undefined;
        if (nextLibrary.compatible.length) {
          const rememberedPack = remembered?.mode === "mission"
            ? nextLibrary.compatible.find((pack) => pack.descriptor.id === remembered.packageId && pack.descriptor.revision === remembered.revision)
            : undefined;
          const pack = rememberedPack ?? nextLibrary.compatible[0];
          const rememberedMission = remembered?.mode === "mission" && rememberedPack
            ? pack.catalog.missions.find((mission) => mission.id === remembered.missionId)
            : undefined;
          const mission = rememberedMission ?? pack.catalog.missions[0];
          const seed = rememberedMission && remembered?.mode === "mission" ? remembered.seed : randomSeed();
          const rememberedV2 = rememberedMission && remembered?.mode === "mission" && remembered.version === 2 ? remembered : undefined;
          const probe = missionLaunch(pack, mission, {
            seed,
            resumeSaveId: rememberedV2?.resumeSaveId,
            runId: rememberedV2?.runId,
            incomingTransition: rememberedV2?.incomingTransition,
            acceptLegacyResume: Boolean((rememberedMission && remembered?.version === 1) || rememberedV2?.legacyResume),
          });
          const matchingSaves = rememberedV2
            ? saves.filter((save) => saveMatchesLaunch(save, probe))
            : rememberedMission && remembered?.version === 1
              ? saves.filter((save) => save.runId === undefined && saveMatchesMissionIdentity(save, pack, mission))
              : [];
          const resumeCandidates = rememberedV2
            ? matchingSaves.filter((save) => save.id === rememberedV2.resumeSaveId)
            : matchingSaves;
          const resume = await saveStore.readNewestValid(resumeCandidates);
          if (disposed) return;
          saveIssues.push(...resume.issues);
          const newestSave = resume.save?.metadata;
          nextLaunch = missionLaunch(pack, mission, {
            seed,
            resumeSaveId: newestSave?.id,
            runId: probe.runId,
            incomingTransition: probe.incomingTransition,
            acceptLegacyResume: probe.acceptLegacyResume,
          });
          setSelectedPackId(pack.descriptor.id);
          setSelectedMissionId(mission.id);
          const skippedIds = new Set(resume.issues.flatMap((issue) => issue.id ? [issue.id] : []));
          const retainedSaves = matchingSaves.filter((save) => !skippedIds.has(save.id));
          setSaveCount(retainedSaves.length);
          setManualSaveCount(retainedSaves.filter((save) => save.kind === "manual").length);
          if (rememberedV2?.pendingVictory) {
            const restored = restoredVictory(rememberedV2.pendingVictory);
            if (correlateCampaignOutcome(nextLaunch, restored.gameOver, restored.outcome)) {
              pendingRestore = restored;
              startupNotice = `${mission.title} victory restored · choose the next operation`;
            } else {
              setError("Stored campaign victory did not match its mission and was ignored");
              startupNotice = `Launching ${mission.title} from installed content…`;
            }
          } else if (freewareBootstrapStatus === "installed") {
            startupNotice = `Classic freeware installed · launching ${mission.title}…`;
          } else {
            startupNotice = newestSave ? `Resuming ${mission.title} from local save…` : `Launching ${mission.title} from installed content…`;
          }
        } else {
          const rememberedSeed = remembered?.mode === "demo" ? remembered.seed : randomSeed();
          const probe = demoLaunch(rememberedSeed);
          const matchingSaves = saves.filter((save) => saveMatchesLaunch(save, probe));
          const resume = await saveStore.readNewestValid(matchingSaves);
          if (disposed) return;
          saveIssues.push(...resume.issues);
          const newestSave = resume.save?.metadata;
          nextLaunch = demoLaunch(rememberedSeed, newestSave?.id);
          const skippedIds = new Set(resume.issues.flatMap((issue) => issue.id ? [issue.id] : []));
          const retainedSaves = matchingSaves.filter((save) => !skippedIds.has(save.id));
          setSaveCount(retainedSaves.length);
          setManualSaveCount(retainedSaves.filter((save) => save.kind === "manual").length);
          startupNotice = freewareBootstrapError
            ? "Classic freeware content is unavailable · running explicit demo fallback"
            : nextLibrary.incompatible.length ? "No compatible mission pack · running explicit demo fallback" : "No mission pack installed · running explicit demo fallback";
        }
        if (saveIssues.length) console.warn("Unreadable local saves were ignored during startup", saveIssues);
        setNotice(`${startupNotice}${saveIssues.length ? ` · ignored ${saveIssues.length} unreadable local save${saveIssues.length === 1 ? "" : "s"}` : ""}`);
        activeLaunchRef.current = nextLaunch;
        missionGraphicsLockedRef.current = nextLaunch.kind === "mission";
        if (pendingRestore) {
          terminalRef.current = true;
          campaignOutcomeRef.current = pendingRestore.outcome;
          setCampaignOutcome(pendingRestore.outcome);
          setGameOver(pendingRestore.gameOver);
          setLaunch(undefined);
        } else {
          setLaunch(nextLaunch);
        }
        setLibraryReady(true);
      } catch (storageError) {
        if (disposed) return;
        setError(storageError instanceof Error ? storageError.message : String(storageError));
        const fallback = demoLaunch();
        activeLaunchRef.current = fallback;
        missionGraphicsLockedRef.current = false;
        setLaunch(fallback);
        setLibraryReady(true);
        setNotice("Local library could not be read · running explicit demo fallback");
      }
    })();
    return () => { disposed = true; };
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !launch) return;
    activeLaunchRef.current = launch;
    sessionEpochRef.current += 1;
    void updateSaveCount(launch);
    missionGraphicsLockedRef.current = launch.kind === "mission";
    saveCreatedAtRef.current = new Date().toISOString();
    if (launch.kind === "mission") { modeRef.current = "classic"; setMode("classic"); }
    applyCamera({ x: 0, y: 0, zoom: 1 });
    snapshotRef.current = undefined;
    domTelemetryRefreshTickRef.current = undefined;
    minimapRefreshTickRef.current = undefined;
    assignBattlefieldTool(undefined);
    toolRecoverySuppressedThroughTickRef.current = 0;
    setTick(0);
    setRunning(false);
    setSupportsSaves(false);
    setMountProgress(undefined);
    setLaunching(true);
    terminalRef.current = false;
    setGameOver(undefined);
    campaignOutcomeRef.current = undefined;
    setCampaignOutcome(undefined);
    resumeAfterVisibilityRef.current = false;
    resumeAfterGraphicsRestoreRef.current = false;
    graphicsContextLostRef.current = false;
    pendingCameraFocusRef.current = undefined;
    selectionPresentationRef.current = EMPTY_SELECTION_PRESENTATION;
    setSelectionPresentation(EMPTY_SELECTION_PRESENTATION);
    setMissionStats(undefined);
    setMissionStatsLaunchKey(undefined);
    setAssigningControlGroup(false);
    const classicSurfaceAccumulator = new ClassicSurfaceAccumulator();
    const minimapContext = minimapRef.current?.getContext("2d", { alpha: false });
    if (minimapContext && minimapRef.current) minimapContext.clearRect(0, 0, minimapRef.current.width, minimapRef.current.height);
    let disposed = false;
    let frame = 0;
    let lastFpsTime = performance.now();
    let frameCount = 0;
    let hasStarted = false;
    const client = launch.kind === "mission"
      ? new SimulationClient({
        core: "wasm",
        missionId: launch.mission.id,
        emscriptenModuleUrl: new URL("engine/tiberiandawn.js", document.baseURI).href,
        contentMount: {
          packageId: launch.pack.descriptor.id,
          revision: launch.pack.descriptor.revision,
          storageKey: launch.pack.descriptor.storageKey,
          files: launch.pack.descriptor.manifest.files
            .filter(({ path }) => path.startsWith(`${launch.pack.catalog.engineRoot}/`))
            .map(({ path, size, sha256 }) => ({ path, size, sha256 })),
        },
      })
      : new SimulationClient({ core: "demo" });
    clientRef.current = client;
    let audioPlayer: RuntimeAudio | undefined;
    let audioUnavailable = false;
    const audioLoad = launch.kind === "mission" && contentStoreRef.current
      ? RuntimeAudio.load(contentStoreRef.current, launch.pack.descriptor).catch((audioError: unknown) => {
        audioUnavailable = true;
        console.warn("Runtime audio is unavailable; continuing the mission without audio", audioError);
        return undefined;
      })
      : Promise.resolve(undefined);
    let renderer: WebGLRenderer;
    try {
      renderer = new WebGLRenderer(canvas, {
        maximumDevicePixelRatio: 2,
        // The classic surface was authored for a low-resolution presentation.
        // Keeping its backing buffer near native density avoids wasting fill
        // rate on pixels that nearest-neighbor scaling cannot add detail to.
        resolutionScale: launch.kind === "mission" ? 0.375 : 1,
        onContextLost: () => {
          graphicsContextLostRef.current = true;
          resumeAfterGraphicsRestoreRef.current = runningRef.current || resumeAfterVisibilityRef.current
            || resumeAfterModalRef.current || !hasStarted;
          if (hasStarted) client.setRunning(false);
          setNotice("Graphics context lost; simulation paused");
        },
        onContextRestored: () => {
          graphicsContextLostRef.current = false;
          const retainedSnapshot = snapshotRef.current;
          if (retainedSnapshot) renderer.ingestClassicSurface(retainedSnapshot, classicSurfaceAccumulator.current());
          const blockedByModal = modalWasOpenRef.current;
          if (blockedByModal && resumeAfterGraphicsRestoreRef.current) resumeAfterModalRef.current = true;
          if (document.visibilityState !== "visible" && resumeAfterGraphicsRestoreRef.current) resumeAfterVisibilityRef.current = true;
          const shouldResume = hasStarted && resumeAfterGraphicsRestoreRef.current && !blockedByModal
            && document.visibilityState === "visible" && !snapshotRef.current?.terminal;
          resumeAfterGraphicsRestoreRef.current = false;
          if (shouldResume) client.setRunning(true);
          setNotice(shouldResume
            ? "Graphics restored · simulation resumed"
            : blockedByModal && resumeAfterModalRef.current
              ? "Graphics restored · simulation resumes when the dialog closes"
              : "Graphics restored · simulation remains paused");
        },
      });
    } catch (rendererError) {
      setError(rendererError instanceof Error ? rendererError.message : String(rendererError));
      setLaunching(false);
      client.dispose();
      return;
    }

    const removeSnapshot = client.onSnapshot((snapshot) => {
      const previous = snapshotRef.current;
      snapshotRef.current = snapshot;
      const completeClassicSurface = classicSurfaceAccumulator.apply(snapshot.classicSurface);
      renderer.ingestClassicSurface(snapshot, completeClassicSurface);
      const pendingCameraFocus = pendingCameraFocusRef.current;
      if (pendingCameraFocus) {
        pendingCameraFocusRef.current = undefined;
        focusCameraOnWorld(pendingCameraFocus);
      }
      const audioView = visibleWorldRect(snapshot, modeRef.current, cameraRef.current);
      audioRef.current?.setView(audioView.x, audioView.width);
      if (previous) client.recycle(previous);
      const forceDomTelemetry = loadInFlightRef.current || snapshot.tick === 0 || snapshot.terminal;
      const refreshDomTelemetry = domTelemetryRefreshDue(
        snapshot.tick,
        domTelemetryRefreshTickRef.current,
        forceDomTelemetry,
      );
      if (refreshDomTelemetry) {
        domTelemetryRefreshTickRef.current = snapshot.tick;
        contextualHoverDirtyRef.current = true;
        setTick(snapshot.tick);
        const nextSelectionPresentation = battlefieldSelectionPresentation(snapshot.objects(), snapshot.player?.house);
        selectionPresentationRef.current = nextSelectionPresentation;
        setSelectionPresentation(nextSelectionPresentation);
        if (nextSelectionPresentation.assignableCount === 0) setAssigningControlGroup(false);
        if (minimapRefreshDue(snapshot.tick, minimapRefreshTickRef.current, forceDomTelemetry)) {
          minimapRefreshTickRef.current = snapshot.tick;
          drawMinimap(minimapRef.current, completeClassicSurface, snapshot.palette);
        }
        setMissionStats(snapshot.sidebar);
        setMissionStatsLaunchKey(launch.key);
      }

      const sidebar = snapshot.sidebar;
      const currentTool = battlefieldToolRef.current;
      if (currentTool?.kind === "placement") {
        const entryStillReady = sidebar?.entries.some((entry) => productionEntryKey(entry) === currentTool.entryKey && entry.completed) === true;
        if (!entryStillReady) {
          assignBattlefieldTool(undefined);
          if (currentTool.phase === "placing") setNotice("Structure placed");
        } else if (snapshot.placement) {
          if (currentTool.phase === "requesting") {
            assignBattlefieldTool({ ...currentTool, phase: "active" });
          } else if (currentTool.phase === "placing" && snapshot.tick > currentTool.requestedTick) {
            assignBattlefieldTool({ ...currentTool, phase: "active" });
            setNotice("That footprint is blocked · choose another green location");
          }
        } else if (snapshot.tick > currentTool.requestedTick) {
          assignBattlefieldTool(undefined);
          setNotice(currentTool.phase === "requesting"
            ? "Placement could not start; the completed structure remains available"
            : currentTool.phase === "placing" ? "Structure placed" : "Placement closed");
        }
      } else if (currentTool?.kind === "repair" && !sidebar?.repairEnabled) {
        assignBattlefieldTool(undefined);
      } else if (currentTool?.kind === "sell" && !sidebar?.sellEnabled) {
        assignBattlefieldTool(undefined);
      } else if (currentTool?.kind === "superweapon") {
        const ready = sidebar?.entries.some((entry) => productionEntryKey(entry) === currentTool.entryKey && entry.completed) === true;
        if (!ready) {
          assignBattlefieldTool(undefined);
          setNotice(`${describeProductionEntry(currentTool.entry).label} is no longer ready`);
        }
      } else if (!currentTool && snapshot.placement && snapshot.tick > toolRecoverySuppressedThroughTickRef.current && sidebar) {
        const readyBuildings = sidebar.entries.filter((entry) => entry.completed && entry.objectType === SnapshotObjectType.BuildingType);
        if (readyBuildings.length === 1) {
          const entry = readyBuildings[0];
          assignBattlefieldTool({ kind: "placement", entry, entryKey: productionEntryKey(entry), phase: "active", requestedTick: snapshot.tick });
          setNotice(`Placement restored for ${describeProductionEntry(entry).label}`);
        }
      }
    });
    const removeError = client.onError((simulationError) => { setError(simulationError.message); setLaunching(false); });
    const removeRunning = client.onRunningChange((nextRunning) => {
      runningRef.current = nextRunning;
      setRunning(nextRunning);
      if (!nextRunning) setTick(snapshotRef.current?.tick ?? 0);
      if (nextRunning) void audioRef.current?.setPaused(false);
      else if (!terminalRef.current) void audioRef.current?.setPaused(true);
    });
    const removeProgress = client.onMountProgress((progress) => setMountProgress(progress));
    const removeEvent = client.onEvent((event) => {
      if (event.kind === "message") setNotice(event.text);
      else if (event.kind === "debug") console.debug(`[TD tick ${event.tick}] ${event.text}`);
      else if (event.kind === "diagnostic") {
        setDiagnostics((current) => [...current.slice(-11), event]);
        if (event.detail) setNotice(event.detail);
        if (event.error) setError(`${event.id}: ${event.detail || `engine status ${event.status}`}`);
      } else if (event.kind === "movie") {
        client.sendCommands([movieDoneCommand()]);
        setNotice(`${event.name || "Movie"} omitted (MOVIES.MIX is not part of this local runtime pack)`);
      } else if (event.kind === "camera") {
        // The engine changes TacticalCoord before emitting this callback. Use
        // the following snapshot's camera state so the host offset does not
        // apply the same recenter operation a second time.
        pendingCameraFocusRef.current = { x: event.x, y: event.y };
      } else if (event.kind === "ping") {
        const point = worldToScreen({ x: event.x, y: event.y }, canvas, snapshotRef.current, cameraRef.current, modeRef.current);
        if (point) {
          const id = Date.now();
          setMarker({ ...point, id, alternate: false, ping: true });
          setTimeout(() => setMarker((current) => current?.id === id ? undefined : current), 1_200);
        }
      } else if (event.kind === "sound" || event.kind === "speech") {
        audioRef.current?.handle(event);
      } else if (event.kind === "campaign-outcome") {
        const active = activeLaunchRef.current;
        if (active?.kind === "mission") {
          const expectedHouse = active.mission.faction === "gdi" ? 0 : 1;
          if (event.scenario === active.mission.scenario && event.scenarioRoot === active.mission.scenarioRoot && event.house === expectedHouse) {
            campaignOutcomeRef.current = event;
            setCampaignOutcome(event);
          } else {
            setError("Campaign outcome did not match the active mission; continuation is disabled");
          }
        }
      } else if (event.kind === "game-over") {
        const active = activeLaunchRef.current;
        const outcome = campaignOutcomeRef.current;
        sessionEpochRef.current += 1;
        if (outcome && correlateCampaignOutcome(active, event, outcome)) rememberSession(active, undefined, persistedVictory(event, outcome));
        else if (event.won && active?.kind === "mission") setError("Campaign continuation state was unavailable; restart or load a save to continue safely");
        terminalRef.current = true;
        assignBattlefieldTool(undefined);
        setPendingImport(undefined);
        setAboutOpen(false);
        setGameOver(event);
        setNotice(event.won ? "Mission accomplished" : "Mission failed");
      }
    });

    void client.ready().then((capabilities) => {
      if (!disposed) setSupportsSaves(capabilities.saves);
    }).catch(() => undefined);
    void audioLoad.then(async (player) => {
      if (disposed) { player?.destroy(); return; }
      audioPlayer = player;
      audioRef.current = player;
      let resumeSave: { metadata: StoredSave; data: Uint8Array } | undefined;
      if (launch.resumeSaveId && saveStoreRef.current) {
        try {
          const saved = await saveStoreRef.current.read(launch.resumeSaveId);
          if (!saveMatchesLaunch(saved.metadata, launch)) throw new Error("Save identity does not match the active package revision and mission");
          resumeSave = saved;
        } catch (resumeError) {
          setError(`Could not resume local save: ${resumeError instanceof Error ? resumeError.message : String(resumeError)}`);
        }
      }
      if (disposed) return;
      await client.start(launch.start, { deferRunningUntilLoad: Boolean(resumeSave) });
      hasStarted = true;
      if (disposed) return;
      if (document.visibilityState !== "visible" || modalWasOpenRef.current || graphicsContextLostRef.current) {
        if (document.visibilityState !== "visible") resumeAfterVisibilityRef.current = true;
        if (modalWasOpenRef.current) resumeAfterModalRef.current = true;
        if (graphicsContextLostRef.current) resumeAfterGraphicsRestoreRef.current = true;
        client.setRunning(false);
      }
      let resumed = false;
      if (resumeSave) {
        try {
          await client.load(resumeSave.data.buffer.slice(resumeSave.data.byteOffset, resumeSave.data.byteOffset + resumeSave.data.byteLength) as ArrayBuffer);
          restoreSavePresentation(resumeSave.metadata, launch);
          resumed = true;
        } catch (resumeError) {
          setError(`Could not resume local save: ${resumeError instanceof Error ? resumeError.message : String(resumeError)}`);
        }
      }
      if (disposed) return;
      if (document.visibilityState !== "visible" || modalWasOpenRef.current || graphicsContextLostRef.current) {
        if (document.visibilityState !== "visible") resumeAfterVisibilityRef.current = true;
        if (modalWasOpenRef.current) resumeAfterModalRef.current = true;
        if (graphicsContextLostRef.current) resumeAfterGraphicsRestoreRef.current = true;
        client.setRunning(false);
      }
      rememberSession(launch, resumed ? launch.resumeSaveId : undefined);
      setLaunching(false);
      const audioStatus = audioUnavailable ? " · audio unavailable" : "";
      setNotice((resumed
        ? `Resumed ${launch.kind === "mission" ? launch.mission.title : "local demo"}`
        : launch.kind === "mission" ? `${launch.mission.title} · ${launch.pack.descriptor.id}` : "Demo simulation · no compatible campaign content loaded") + audioStatus);
    }).catch((startError: unknown) => {
      if (!disposed) { setLaunching(false); setError(startError instanceof Error ? startError.message : String(startError)); }
    });

    const render = (time: number): void => {
      runtimePerformanceMetrics.recordFrame(time);
      const snapshot = snapshotRef.current;
      if (contextualHoverDirtyRef.current) {
        contextualHoverDirtyRef.current = false;
        presentContextualHover(battlefieldHoverRef.current, snapshot);
      }
      if (snapshot) renderer.render(snapshot, modeRef.current, cameraRef.current);
      drawPlacementOverlay(placementOverlayRef.current, canvas, snapshot, cameraRef.current, modeRef.current, battlefieldToolRef.current, battlefieldHoverRef.current);
      frameCount += 1;
      if (time - lastFpsTime >= 1000) {
        setFps(Math.round((frameCount * 1000) / (time - lastFpsTime)));
        frameCount = 0;
        lastFpsTime = time;
      }
      frame = requestAnimationFrame(render);
    };
    frame = requestAnimationFrame(render);

    const touch = new TouchController(canvas, {
      onTap: (point, alternate) => {
        if (loadInFlightRef.current) { setNotice("Wait for the load to finish"); return; }
        const activeTool = battlefieldToolRef.current;
        if (activeTool && alternate) {
          cancelBattlefieldTool();
          return;
        }
        if (!runningRef.current) { setNotice("Resume the simulation before issuing orders"); return; }
        const activeSnapshot = snapshotRef.current;
        const world = screenToWorld(point, canvas, activeSnapshot, cameraRef.current, modeRef.current);
        if (!world) { setNotice("Point inside the visible battlefield to issue a command"); return; }
        if (activeTool?.kind === "placement") {
          if (activeTool.phase === "placing") {
            setNotice("Waiting for the engine to confirm placement…");
            return;
          }
          const preview = activeSnapshot ? placementPreviewAtWorld(activeSnapshot, activeTool.entry, world) : undefined;
          if (!activeSnapshot?.placement) {
            setNotice("Preparing the placement grid…");
            return;
          }
          if (!preview?.legal) {
            setNotice("That footprint is blocked or too far from your base");
            return;
          }
          client.sendCommands([placeProductionCommand(activeTool.entry, preview.requestCell)]);
          assignBattlefieldTool({ ...activeTool, phase: "placing", requestedTick: activeSnapshot.tick });
          setNotice("Placing structure…");
          return;
        }
        if (activeTool?.kind === "repair") {
          const building = activeSnapshot?.findBuildingAtWorldPoint(world.x, world.y, { capability: "repair" });
          if (!building) {
            setNotice("Choose a damaged friendly structure that can be repaired");
            return;
          }
          client.sendCommands([repairStructureCommand(building.id)]);
          battlefieldHoverRef.current = undefined;
          setNotice(`${building.repairing ? "Stopping repairs on" : "Repairing"} ${building.assetName || building.typeName || "structure"}`);
          return;
        }
        if (activeTool?.kind === "sell") {
          const target = activeSnapshot?.findSellTargetAtWorldPoint(world.x, world.y);
          if (!target) {
            setNotice("Choose a sellable friendly structure or wall");
            return;
          }
          client.sendCommands([target.kind === "building" ? sellStructureCommand(target.building.id) : sellWallAtWorldCommand(world)]);
          battlefieldHoverRef.current = undefined;
          setNotice(`Selling ${target.kind === "building" ? target.building.assetName || target.building.typeName || "structure" : "wall"}`);
          return;
        }
        if (activeTool?.kind === "superweapon") {
          const readyEntry = activeSnapshot?.sidebarEntry(activeTool.entry.buildableType, activeTool.entry.buildableId);
          if (!readyEntry?.completed) {
            assignBattlefieldTool(undefined);
            setNotice(`${describeProductionEntry(activeTool.entry).label} is no longer ready`);
            return;
          }
          client.sendCommands([targetSuperweaponCommand(activeTool.entry, world)]);
          assignBattlefieldTool(undefined);
          const id = Date.now();
          setMarker({ ...point, id, alternate: true });
          setTimeout(() => setMarker((current) => current?.id === id ? undefined : current), 550);
          setNotice(`${describeProductionEntry(activeTool.entry).label} target designated`);
          return;
        }
        const contextual = alternate || interactionModeRef.current === "order";
        client.sendCommands([pointCommand(interactionModeRef.current, world, alternate)]);
        if (contextual && activeSnapshot) setNotice(contextualOrderNotice(contextualActionAtWorld(activeSnapshot, world)));
        const id = Date.now();
        setMarker({ ...point, id, alternate: contextual });
        setTimeout(() => setMarker((current) => current?.id === id ? undefined : current), 550);
      },
      onHover: (point) => {
        battlefieldHoverRef.current = point;
        if (battlefieldToolRef.current) clearContextualHover();
        else contextualHoverDirtyRef.current = true;
      },
      onBoxPreview: (start, end) => {
        clearContextualHover();
        if (battlefieldToolRef.current) {
          battlefieldHoverRef.current = end;
          setSelectionBox(undefined);
          return;
        }
        setSelectionBox({ left: Math.min(start.x, end.x), top: Math.min(start.y, end.y), width: Math.abs(end.x - start.x), height: Math.abs(end.y - start.y) });
      },
      onBoxCancel: () => {
        setSelectionBox(undefined);
        contextualHoverDirtyRef.current = true;
      },
      onBoxSelect: (start, end) => {
        if (!runningRef.current || loadInFlightRef.current) { setSelectionBox(undefined); setNotice(loadInFlightRef.current ? "Wait for the load to finish" : "Resume the simulation before issuing orders"); return; }
        if (battlefieldToolRef.current) { setSelectionBox(undefined); setNotice("Tap without dragging to use the active battlefield tool"); return; }
        const worldStart = screenToWorld(start, canvas, snapshotRef.current, cameraRef.current, modeRef.current);
        const worldEnd = screenToWorld(end, canvas, snapshotRef.current, cameraRef.current, modeRef.current);
        setSelectionBox(undefined);
        if (!worldStart || !worldEnd) { setNotice("Start and finish box selection inside the visible battlefield"); return; }
        client.sendCommands([boxSelectCommand(worldStart, worldEnd)]);
      },
      onPan: panCamera,
      onZoom: zoomCamera,
    });

    const pointerLeave = (): void => {
      battlefieldHoverRef.current = undefined;
      clearContextualHover();
    };
    canvas.addEventListener("pointerleave", pointerLeave);

    const visibility = (): void => {
      const visible = document.visibilityState === "visible";
      if (!hasStarted) {
        resumeAfterVisibilityRef.current = !visible;
        return;
      }
      if (!visible) {
        const wasRunning = runningRef.current;
        resumeAfterVisibilityRef.current = wasRunning || resumeAfterModalRef.current || resumeAfterGraphicsRestoreRef.current;
        client.setRunning(false);
        void persistSave("autosave", "Lifecycle autosave");
      } else {
        const shouldResume = resumeAfterVisibilityRef.current && !snapshotRef.current?.terminal;
        resumeAfterVisibilityRef.current = false;
        if (shouldResume && modalWasOpenRef.current) resumeAfterModalRef.current = true;
        else if (shouldResume && graphicsContextLostRef.current) resumeAfterGraphicsRestoreRef.current = true;
        else if (shouldResume) client.setRunning(true);
      }
    };
    const pageHide = (): void => {
      if (!hasStarted) {
        resumeAfterVisibilityRef.current = true;
        return;
      }
      resumeAfterVisibilityRef.current = runningRef.current || resumeAfterModalRef.current || resumeAfterGraphicsRestoreRef.current;
      client.setRunning(false);
      void persistSave("autosave", "Lifecycle autosave");
    };
    document.addEventListener("visibilitychange", visibility);
    window.addEventListener("pagehide", pageHide);
    const keyboard = (event: KeyboardEvent): void => {
      const target = event.target as HTMLElement | null;
      if (target?.closest("input, select, textarea, button, a[href], summary, [contenteditable='true'], [role='dialog']")) return;
      if (loadInFlightRef.current && event.code === "Escape") { event.preventDefault(); setNotice("Wait for the load to finish"); return; }
      const groupHotkey = controlGroupHotkey(event);
      if (groupHotkey) {
        event.preventDefault();
        if (event.repeat) return;
        issueControlGroup(groupHotkey.index, groupHotkey.action, groupHotkey.focus);
        return;
      }
      if (event.repeat && (event.code === "Space" || event.code === "Escape" || event.code === "Home")) return;
      if (event.code === "Space") { event.preventDefault(); toggleMode(); }
      else if (event.code === "Escape") {
        if (battlefieldToolRef.current) { event.preventDefault(); cancelBattlefieldTool(); }
        else if (snapshotRef.current?.terminal) setNotice("Load a save or restart the mission to continue");
        else client.setRunning(!runningRef.current);
      }
      else if (["ArrowLeft", "KeyA", "ArrowRight", "KeyD", "ArrowUp", "KeyW", "ArrowDown", "KeyS"].includes(event.code)) {
        event.preventDefault();
        const distance = event.shiftKey ? 160 : 64;
        panCamera({
          x: event.code === "ArrowLeft" || event.code === "KeyA" ? distance : event.code === "ArrowRight" || event.code === "KeyD" ? -distance : 0,
          y: event.code === "ArrowUp" || event.code === "KeyW" ? distance : event.code === "ArrowDown" || event.code === "KeyS" ? -distance : 0,
        });
      }
      else if (event.code === "KeyQ" || event.code === "KeyE") {
        event.preventDefault();
        if (event.repeat) return;
        if (loadInFlightRef.current) { setNotice("Wait for the load to finish"); return; }
        cancelBattlefieldTool(false);
        const next = event.code === "KeyQ" ? "select" : "order";
        interactionModeRef.current = next;
        contextualHoverDirtyRef.current = true;
        setInteractionMode(next);
        if (canvasRef.current) canvasRef.current.dataset.tool = next;
        setNotice(next === "select" ? "Selection mode active" : "Contextual order mode active");
      }
      else if (event.code === "KeyX") {
        event.preventDefault();
        if (event.repeat) return;
        if (!runningRef.current || loadInFlightRef.current || snapshotRef.current?.terminal) {
          setNotice("Resume the simulation before issuing orders");
          return;
        }
        client.sendCommands([stopSelectedCommand()]);
        setNotice("Stop order sent to selected units");
      }
      else if (event.code === "Equal" || event.code === "NumpadAdd") { event.preventDefault(); zoomCamera(1.2); }
      else if (event.code === "Minus" || event.code === "NumpadSubtract") { event.preventDefault(); zoomCamera(1 / 1.2); }
      else if (event.code === "Home") { event.preventDefault(); resetCamera(); }
    };
    window.addEventListener("keydown", keyboard);

    return () => {
      disposed = true;
      cancelAnimationFrame(frame);
      document.removeEventListener("visibilitychange", visibility);
      window.removeEventListener("pagehide", pageHide);
      window.removeEventListener("keydown", keyboard);
      canvas.removeEventListener("pointerleave", pointerLeave);
      touch.destroy();
      removeSnapshot(); removeError(); removeRunning(); removeProgress(); removeEvent();
      renderer.destroy();
      client.dispose();
      audioPlayer?.destroy();
      if (audioRef.current === audioPlayer) audioRef.current = undefined;
      if (clientRef.current === client) clientRef.current = undefined;
      snapshotRef.current = undefined;
      classicSurfaceAccumulator.reset();
    };
  }, [applyCamera, assignBattlefieldTool, cancelBattlefieldTool, clearContextualHover, focusCameraOnWorld, issueControlGroup, launch, panCamera, persistSave, presentContextualHover, resetCamera, restoreSavePresentation, toggleMode, updateSaveCount, zoomCamera]);

  useEffect(() => {
    if (!running || !supportsSaves || !saveStoreRef.current) return;
    const timer = window.setInterval(() => void persistSave("autosave", "30-second autosave"), 30_000);
    return () => window.clearInterval(timer);
  }, [persistSave, running, supportsSaves]);

  const installComplete = async (packageId: string): Promise<void> => {
    const store = contentStoreRef.current;
    if (!store) return;
    const nextLibrary = await loadRuntimeLibrary(store);
    setLibrary(nextLibrary);
    const pack = nextLibrary.compatible.find((candidate) => candidate.descriptor.id === packageId);
    if (!pack) {
      const incompatible = nextLibrary.incompatible.find((candidate) => candidate.id === packageId);
      throw new Error(incompatible?.reason ?? "Installed package is not compatible with this runtime");
    }
    const mission = pack.catalog.missions[0];
    setSelectedPackId(pack.descriptor.id);
    setSelectedMissionId(mission.id);
    setDiagnostics([]);
    setError(undefined);
    const next = missionLaunch(pack, mission);
    missionGraphicsLockedRef.current = true;
    modeRef.current = "classic";
    setMode("classic");
    setLaunch(next);
  };

  const stopActiveRuntimeForImport = (): Launch | undefined => {
    const previous = activeLaunchRef.current;
    const client = clientRef.current;
    clientRef.current = undefined;
    void audioRef.current?.setPaused(true);
    client?.dispose();
    runningRef.current = false;
    setRunning(false);
    setSupportsSaves(false);
    setMountProgress(undefined);
    assignBattlefieldTool(undefined);
    setLaunch(undefined);
    setLaunching(false);
    return previous;
  };

  const restoreAfterFailedImport = async (previous: Launch | undefined, committed: boolean): Promise<void> => {
    if (committed || !previous) {
      const fallback = demoLaunch(previous?.seed);
      missionGraphicsLockedRef.current = false;
      setLaunch(fallback);
      return;
    }
    let resumeSaveId: string | undefined;
    const store = saveStoreRef.current;
    if (store) {
      try {
        const matches = (await store.list()).filter((save) => saveMatchesLaunch(save, previous));
        resumeSaveId = (await store.readNewestValid(matches)).save?.metadata.id;
      } catch (restoreError) {
        console.warn("Could not select a save while restoring the interrupted launch", restoreError);
      }
    }
    const restored = previous.kind === "mission"
      ? missionLaunch(previous.pack, previous.mission, {
        seed: previous.seed,
        resumeSaveId,
        runId: previous.runId,
        incomingTransition: previous.incomingTransition,
      })
      : demoLaunch(previous.seed, resumeSaveId);
    setLaunch(restored);
  };

  const importPackage = async (file: File | undefined): Promise<void> => {
    const store = contentStoreRef.current;
    if (!file || !storage?.supported || !store) return;
    setImporting(true); setError(undefined);
    const previous = stopActiveRuntimeForImport();
    let committed = false;
    try {
      const installed = await importCncwebPackage(file, store, (done, total) => setNotice(`Validating local content ${done}/${total}`));
      committed = true;
      await installComplete(installed.id);
      setNotice(`${installed.id} installed · launching its first mission`);
    } catch (importError) {
      await restoreAfterFailedImport(previous, committed);
      setError(importError instanceof Error ? importError.message : String(importError));
    }
    finally { setImporting(false); if (importRef.current) importRef.current.value = ""; }
  };

  const choosePackage = (files: FileList | null): void => {
    const file = files?.[0];
    if (!file) return;
    setPendingImport(file);
    if (importRef.current) importRef.current.value = "";
  };

  const confirmPackageImport = (): void => {
    const file = pendingImport;
    setPendingImport(undefined);
    void importPackage(file);
  };

  const importDevelopmentDirectory = async (files: FileList | null): Promise<void> => {
    const store = contentStoreRef.current;
    if (!files?.length || !storage?.supported || !store) return;
    setImporting(true); setError(undefined);
    const previous = stopActiveRuntimeForImport();
    let committed = false;
    try {
      const entries = [...files];
      const manifestFile = entries.find((file) => fileRelativePath(file) === "manifest.json");
      if (!manifestFile) throw new Error("Selected folder does not contain manifest.json at its root");
      const manifest = JSON.parse(await manifestFile.text()) as ContentManifest;
      const sources = new Map<string, ContentFileSource>();
      for (const file of entries) {
        const relative = fileRelativePath(file);
        if (relative !== "manifest.json") sources.set(relative, file);
      }
      await store.install(manifest, sources, (done, total) => setNotice(`Validating local content ${done}/${total}`));
      committed = true;
      await installComplete(manifest.package_id);
      setNotice(`${manifest.package_id} installed from development folder · launching`);
    } catch (importError) {
      await restoreAfterFailedImport(previous, committed);
      setError(importError instanceof Error ? importError.message : String(importError));
    }
    finally { setImporting(false); if (developmentImportRef.current) developmentImportRef.current.value = ""; }
  };

  return (
    <div className="app-shell">
      <header className="topbar" inert={applicationModalOpen} aria-hidden={applicationModalOpen}>
        <div className="brand"><span className="brand-mark" aria-hidden="true" /><div><strong>Theater Runtime</strong><span>browser strategy runtime</span></div></div>
        <div className="runtime-status" aria-label={`Simulation ${running ? "running" : "paused"}, tick ${tick}, ${fps} frames per second`}><span className={`status-dot ${running ? "active" : ""}`} aria-hidden="true" />Tick {tick.toLocaleString()} <span className="status-divider" aria-hidden="true">·</span> {fps} fps</div>
        <nav aria-label="Application">
          <button ref={importButtonRef} className="quiet-button" onClick={() => importRef.current?.click()} disabled={!storage?.supported || bootstrapping || importing || loading || launching || saving}>{importing ? "Importing…" : `Import pack (${contentCount})`}</button>
          <input ref={importRef} className="visually-hidden" type="file" accept=".cncweb,application/zip" onChange={(event) => choosePackage(event.currentTarget.files)} />
          <button ref={aboutButtonRef} className="quiet-button" onClick={() => setAboutOpen(true)}>About &amp; legal</button>
        </nav>
      </header>
      <p className="ea-disclaimer">EA has not endorsed and does not support this product.</p>

      <main className="play-area" inert={applicationModalOpen} aria-hidden={applicationModalOpen} aria-busy={launching || importing || bootstrapping}>
        <div className="viewport-frame">
          <canvas ref={canvasRef} tabIndex={0} aria-label="Real-time strategy battlefield" aria-describedby="battlefield-help" />
          <canvas ref={placementOverlayRef} className="placement-overlay" aria-hidden="true" />
          <p id="battlefield-help" className="visually-hidden">Pointer: tap to select or order, drag to box-select, middle-drag or use two fingers to pan, and pinch or wheel to zoom. Build completed structures from the command console, then tap a green footprint. Repair, sell, and support tools also target the battlefield. Keyboard: arrow or W A S D keys pan, Q selects, E enters contextual order mode, X stops selected units, plus and minus zoom, Home resets the camera, Escape cancels an active tool or pauses, and Space switches graphics when available. Number keys select control groups; press the same selected group again to center it. Control plus a number assigns the current selection, Shift plus a number adds a group to the selection, and Alt plus a number selects and centers it.</p>
          <BattlefieldOnboarding active={launch?.kind === "mission" && !launching && !importing && !bootstrapping} />
          <div className="viewport-chrome top-left">
            <button className="hud-button" onClick={toggleMode} disabled={!launch || importing || launch.kind === "mission"} aria-label={launch?.kind === "mission" ? "Classic graphics required by this mission pack" : "Switch graphics mode"} aria-pressed={mode === "remastered"}><span>Graphics</span><strong>{mode === "classic" ? "Classic" : "Enhanced"}</strong></button>
            <button className="hud-button compact" disabled={!launch || importing || launching || loading || Boolean(gameOver)} onClick={() => clientRef.current?.setRunning(!running)}>{running ? "Pause" : "Resume"}</button>
          </div>
          <div className="viewport-chrome top-right" role="group" aria-label="Camera controls">
            <button className="hud-button compact camera-button" onClick={() => zoomCamera(1 / 1.2)} disabled={!launch || importing || launching} aria-label="Zoom out">−</button>
            <button className="hud-button compact camera-button camera-reset" onClick={resetCamera} disabled={!launch || importing || launching} aria-label={`Reset camera view (${Math.round(cameraZoom * 100)}%)`}>{Math.round(cameraZoom * 100)}%</button>
            <button className="hud-button compact camera-button" onClick={() => zoomCamera(1.2)} disabled={!launch || importing || launching} aria-label="Zoom in">+</button>
          </div>
          {selectionBox && <div className="selection-box" style={selectionBox} aria-hidden="true" />}
          {marker && <span className={`command-marker ${marker.ping ? "ping" : marker.alternate ? "order" : "select"}`} style={{ left: marker.x, top: marker.y }} aria-hidden="true" />}
          {contextualHover && !battlefieldToolLabel && <div
            className={`contextual-order-status ${contextualHover.tone}`}
            aria-hidden="true"
            data-action={String(contextualHover.action)}
            data-source={contextualHover.source}
          >{interactionMode === "select" && contextualHover.label !== "Select" && contextualHover.label !== "Select units first"
              ? <span>Right-click · </span> : null}{contextualHover.label}</div>}
          {battlefieldToolLabel && <div className={`battlefield-tool-status ${battlefieldTool?.kind}`} role="status"><span>{battlefieldToolLabel}</span>{battlefieldTool?.kind === "placement" && battlefieldTool.phase === "active" && <button disabled={loading} onClick={quickPlaceStructure} aria-label={`Quick-place ${describeProductionEntry(battlefieldTool.entry).label} at a legal site`}>Quick place</button>}<button disabled={loading} onClick={() => cancelBattlefieldTool()} aria-label={`Cancel ${battlefieldToolLabel}`}>Cancel</button></div>}
          {(launching || importing || bootstrapping) && <div className="launch-overlay" role="status" aria-live="polite" aria-atomic="true"><span className="launch-spinner" aria-hidden="true" /><strong>{bootstrapping ? "Installing classic freeware" : importing ? "Validating local content" : launch?.kind === "mission" ? `Preparing ${launch.mission.title}` : "Starting demo fallback"}</strong><p>{bootstrapping ? "This one-time, hash-verified download is stored privately for instant and offline launches." : importing ? "The previous simulation is stopped while this package is checked and committed." : progressText(mountProgress)}</p></div>}
          <div className="notice-strip" role="status" aria-live="polite" aria-atomic="true">{notice}</div>
          {error && <div className="error-banner" role="alert"><span>{error}</span><button onClick={() => setError(undefined)} aria-label="Dismiss error">×</button></div>}
          <div className="visually-hidden" role="status" aria-live="polite" aria-atomic="true">{selectionPresentation.label}</div>
          {selectionPresentation.count > 0 && <div className="selection-status" aria-hidden="true">{selectionPresentation.label}</div>}
          <div className="action-bar" role="group" aria-label="Touch commands">
            <button className={!battlefieldTool && interactionMode === "select" ? "active" : ""} aria-label="Select units on next tap" aria-pressed={!battlefieldTool && interactionMode === "select"} disabled={!launch || importing || launching || loading} onClick={() => chooseInteractionMode("select")}><span>◇</span>Select</button>
            <button className={!battlefieldTool && interactionMode === "order" ? "active" : ""} aria-label="Issue a contextual move or attack order on next tap" aria-pressed={!battlefieldTool && interactionMode === "order"} disabled={!launch || importing || launching || loading} onClick={() => chooseInteractionMode("order")}><span>⌖</span>Order</button>
            <button aria-label="Stop selected units" disabled={!launch || importing || loading || !running} onClick={stopSelected}><span>■</span>Stop</button>
            {selectionPresentation.deployment && <button
              aria-label={selectionPresentation.deployment.available ? "Deploy selected unit" : "Selected unit cannot deploy here"}
              disabled={!launch || importing || loading || !running || !selectionPresentation.deployment.available}
              onClick={deploySelected}
            ><span>⬡</span>{selectionPresentation.deployment.available ? "Deploy" : "Blocked"}</button>}
          </div>
        </div>

        <aside className={sidebarOpen ? "sidebar open" : "sidebar"} aria-label="Mission panel">
          <button className="sidebar-toggle" onClick={() => setSidebarOpen((value) => !value)} aria-label={sidebarOpen ? "Collapse mission panel" : "Expand mission panel"} aria-expanded={sidebarOpen} aria-controls="mission-panel-content">{sidebarOpen ? "›" : "‹"}</button>
          <div id="mission-panel-content" className="sidebar-content" inert={!sidebarOpen} aria-hidden={!sidebarOpen}>
            <div className="minimap"><canvas ref={minimapRef} aria-label="Live battlefield minimap" /><span>{launch?.kind === "mission" ? launch.mission.id.toUpperCase() : "LOCAL DEMO"}</span></div>
            {launch?.kind === "mission" && <MissionObjectives mission={launch.mission} stats={activeMissionStats} result={gameOver} />}
            {activeMissionStats && (activeMissionStats.entries.length > 0 || activeMissionStats.repairEnabled || activeMissionStats.sellEnabled) ? <ProductionPanel
              sidebar={activeMissionStats}
              unavailable={!running || importing || launching || loading || Boolean(gameOver)}
              activeTool={battlefieldTool?.kind}
              activeEntryKey={battlefieldTool?.kind === "placement" || battlefieldTool?.kind === "superweapon" ? battlefieldTool.entryKey : undefined}
              entryKey={productionEntryKey}
              presentEntry={presentProductionEntry}
              onPrimary={issueProductionAction}
              onCancelProduction={cancelProduction}
              onRepair={() => toggleStructureTool("repair")}
              onSell={() => toggleStructureTool("sell")}
              onCancelTool={() => cancelBattlefieldTool()}
            /> : null}
            {activeMissionStats && <section><p className="eyebrow">Battle status</p><div className="telemetry-grid"><span>Credits<strong>{(activeMissionStats.credits + activeMissionStats.tiberium).toLocaleString()}</strong></span><span>Power<strong>{activeMissionStats.powerProduced.toLocaleString()} / {activeMissionStats.powerDrained.toLocaleString()}</strong></span><span>Destroyed<strong>{(activeMissionStats.unitsKilled + activeMissionStats.buildingsKilled).toLocaleString()}</strong></span><span>Lost<strong>{(activeMissionStats.unitsLost + activeMissionStats.buildingsLost).toLocaleString()}</strong></span></div></section>}
            {launch && <section className="control-groups" aria-labelledby="control-groups-title">
              <div className="control-groups-heading"><div><p className="eyebrow" id="control-groups-title">Control groups</p><small>{selectionPresentation.assignableCount > 0 ? `${selectionPresentation.assignableCount} mobile selected` : selectionPresentation.count > 0 ? "Selection cannot join a group" : "Select mobile units first"}</small></div><button className={`quiet-button ${assigningControlGroup ? "active" : ""}`} aria-pressed={assigningControlGroup} disabled={!running || loading || selectionPresentation.assignableCount === 0} onClick={() => setAssigningControlGroup((value) => !value)}>{assigningControlGroup ? "Choose group" : "Assign selected"}</button></div>
              <div className="control-group-grid" role="group" aria-label={assigningControlGroup ? "Choose a control group to replace" : "Select a control group"}>
                {selectionPresentation.groups.map((group) => <button
                  key={group.index}
                  className={`quiet-button ${group.active ? "active" : ""}`}
                  aria-pressed={group.active}
                  aria-label={assigningControlGroup ? `Assign ${selectionPresentation.assignableCount} selected mobile units to control group ${group.key}, replacing ${group.count} members` : `Control group ${group.key}, ${group.count} ${group.count === 1 ? "object" : "objects"}${group.active ? ", selected" : ""}`}
                  disabled={!running || loading || (!assigningControlGroup && group.count === 0)}
                  onClick={() => {
                    if (assigningControlGroup) {
                      issueControlGroup(group.index, "create");
                      setAssigningControlGroup(false);
                    } else {
                      issueControlGroup(group.index, "select", group.active);
                    }
                  }}
                ><span>{group.key}</span><small>{group.count || "—"}</small></button>)}
              </div>
              <p className="control-group-hint">Keys 1–0 select · Ctrl assigns · Shift adds · repeat centers</p>
            </section>}
            {library.compatible.length > 0 ? <>
              <section className="mission-picker">
                <p className="eyebrow">Installed content</p>
                <label>Package<select value={selectedPackId} disabled={importing || launching || loading} onChange={(event) => selectPack(event.currentTarget.value)}>{library.compatible.map((pack) => <option key={`${pack.descriptor.id}:${pack.descriptor.revision}`} value={pack.descriptor.id}>{pack.descriptor.id} · {pack.descriptor.revision.slice(0, 8)}</option>)}</select></label>
                <label>Mission<select value={selectedMission?.id ?? ""} disabled={importing || launching || loading} onChange={(event) => setSelectedMissionId(event.currentTarget.value)}>{selectedPack?.catalog.missions.map((mission) => <option key={mission.id} value={mission.id}>{mission.title}</option>)}</select></label>
                <button className="launch-mission" onClick={startSelectedMission} disabled={!selectedMission || importing || launching || loading || saving}>Start new mission</button>
              </section>
              <section><p className="eyebrow">Briefing</p><h2>{selectedMission?.title}</h2><p className="mission-briefing">{selectedMission?.briefing}</p></section>
              {selectedPack?.warnings.length ? <section className="pack-warnings"><p className="eyebrow">Presentation notes</p>{selectedPack.warnings.map((warning) => <p key={warning}>{warning}</p>)}</section> : null}
            </> : <section><p className="eyebrow">Demo fallback</p><h2>Foundation range</h2><p>{libraryReady ? "The hosted classic-freeware campaign is unavailable. You can still import a compatible package from this device." : "Inspecting private browser storage…"}</p></section>}
            {diagnostics.length > 0 && <details className="diagnostics"><summary>Engine diagnostics ({diagnostics.length})</summary>{diagnostics.map((entry, index) => <p className={entry.error ? "diagnostic-error" : ""} key={`${entry.tick}-${entry.id}-${index}`}><strong>{entry.id}</strong>{entry.detail || `status ${entry.status}`}</p>)}</details>}
            {library.incompatible.length > 0 && <details className="diagnostics"><summary>Incompatible packs ({library.incompatible.length})</summary>{library.incompatible.map((pack) => <p key={`${pack.id}:${pack.revision}`}><strong>{pack.id}</strong>{pack.reason}</p>)}</details>}
            <section className="storage-card"><span>Private storage</span><strong>{storage?.supported ? storage.persisted ? "Persistent" : "Browser managed" : "Unavailable"}</strong>{storage?.available !== undefined && <small>{Math.floor(storage.available / 1024 / 1024).toLocaleString()} MiB available</small>}{launch?.kind === "mission" && <small>Revision {launch.pack.descriptor.revision.slice(0, 16)}</small>}</section>
            <div className="save-controls"><button onClick={() => void persistSave("manual", "Manual save")} disabled={!storage?.supported || !supportsSaves || importing || launching || Boolean(gameOver) || saving || loading}>{saving ? "Saving…" : "Save"}</button><button onClick={() => void loadLatest()} disabled={!storage?.supported || !supportsSaves || !saveCount || importing || launching || loading || saving}>{loading ? "Loading…" : `Load (${saveCount})`}</button></div>
            <button className="development-import" onClick={() => developmentImportRef.current?.click()} disabled={!storage?.supported || importing || loading || launching || saving}>Development: import extracted folder</button>
            <input ref={developmentImportRef} className="visually-hidden" type="file" multiple {...({ webkitdirectory: "" } as InputHTMLAttributes<HTMLInputElement>)} onChange={(event) => void importDevelopmentDirectory(event.currentTarget.files)} />
          </div>
        </aside>
      </main>

      {gameOver && <div className="modal-backdrop" role="presentation">
        <section ref={gameOverDialogRef} tabIndex={-1} className={`game-over ${gameOver.won ? "won" : "lost"}`} role="dialog" aria-modal="true" aria-labelledby="game-over-title">
          <p className="eyebrow">{gameOver.won ? "Mission accomplished" : "Mission failed"}</p>
          <h1 id="game-over-title">{campaignComplete ? "Campaign complete" : gameOver.won ? "Victory" : "Defeat"}</h1>
          <div className="score-grid"><span>Score<strong>{gameOver.score.toLocaleString()}</strong></span><span>Leadership<strong>{gameOver.leadership.toLocaleString()}</strong></span><span>Efficiency<strong>{gameOver.efficiency.toLocaleString()}</strong></span><span>Credits<strong>{gameOver.remainingCredits.toLocaleString()}</strong></span></div>
          {(gameOver.movieName || gameOver.afterScoreMovieName) && <small>Movie cues: {[gameOver.movieName, gameOver.afterScoreMovieName].filter(Boolean).join(" · ")}</small>}
          {gameOver.won && campaignChoices.length > 0 && <div className="campaign-choices" aria-label="Next campaign mission">
            <p>{campaignChoices.length === 1 ? "The next operation is ready." : "Choose the next operation."}</p>
            {campaignChoices.map((mission, index) => <button key={mission.id} autoFocus={index === 0} onClick={() => continueCampaign(mission)} disabled={launching || loading || saving}>{mission.title}</button>)}
          </div>}
          {gameOver.won && campaignComplete && <p>The final operation is complete. You can replay this mission or return to the mission panel.</p>}
          {gameOver.won && correlatedCampaignResult && !campaignComplete && campaignChoices.length === 0 && <p role="alert">The next campaign mission is not present in this content pack.</p>}
          {error && <p role="alert">{error}</p>}
          <div className="game-over-actions">
            {launch && <button onClick={() => void loadLatest("manual")} disabled={!manualSaveCount || launching || loading || saving}>{loading ? "Loading…" : "Load latest manual"}</button>}
            <button onClick={restartMission} disabled={loading || saving}>Restart mission</button>
            {gameOver.won && (campaignComplete || !campaignChoices.length) && <button onClick={closeCampaignResult} disabled={loading || saving}>Choose standalone mission</button>}
          </div>
        </section>
      </div>}
      {(updateState.status === "downloading" || updateState.status === "ready" || updateState.status === "activating" || updateState.status === "error") && <div className="update-toast" inert={applicationModalOpen} aria-hidden={applicationModalOpen} aria-live="polite">
        <span>{updateState.status === "downloading"
          ? "A new offline build is downloading…"
          : updateState.status === "ready" ? "An offline update is ready. Your game will be saved first."
            : updateState.status === "activating" ? "Applying offline update…"
              : `Offline update paused · ${updateState.message ?? "check the connection and retry"}`}</span>
        {updateState.status === "ready" && <button disabled={applyingUpdate || launching || saving || loading || importing || Boolean(pendingImport)} onClick={() => void applyOfflineUpdate()}>{applyingUpdate ? "Saving & updating…" : launching || saving || loading || importing || pendingImport ? "Operation active" : "Save & update"}</button>}
        {updateState.status === "error" && <button disabled={applyingUpdate || launching || saving || loading || importing || Boolean(pendingImport)} onClick={() => void updateController?.checkForUpdate()}>Retry update</button>}
      </div>}
      {pendingImport && <div className="modal-backdrop" role="presentation" onPointerDown={(event) => { if (event.target === event.currentTarget) setPendingImport(undefined); }}><section ref={importDialogRef} tabIndex={-1} className="import-dialog" role="dialog" aria-modal="true" aria-labelledby="import-title" aria-describedby="import-description import-retention"><p className="eyebrow">Local content import</p><h1 id="import-title">Install {pendingImport.name}</h1><dl><div><dt>Package archive</dt><dd>{(pendingImport.size / 1024 / 1024).toFixed(1)} MiB</dd></div><div><dt>Private storage available</dt><dd>{storage?.available === undefined ? "Browser-reported after validation" : `${Math.floor(storage.available / 1024 / 1024).toLocaleString()} MiB`}</dd></div></dl><p id="import-description">The package will be validated and stored only in this browser profile. It is never uploaded or added to the offline shell cache.</p><p id="import-retention">Clearing this site’s browser data removes imported content and local saves. Installing ends the current simulation, so save first if you want to return to it. The importer checks expanded size and quota again before committing anything.</p><div className="dialog-actions"><button onClick={() => setPendingImport(undefined)}>Cancel</button><button className="primary" autoFocus onClick={confirmPackageImport} disabled={saving || loading || launching}>{saving || loading || launching ? "Wait for current operation" : "Validate & install"}</button></div></section></div>}
      {aboutOpen && <div className="modal-backdrop" role="presentation" onPointerDown={(event) => { if (event.target === event.currentTarget) setAboutOpen(false); }}><section ref={aboutDialogRef} tabIndex={-1} className="about-dialog" role="dialog" aria-modal="true" aria-labelledby="about-title"><button className="modal-close" autoFocus onClick={() => setAboutOpen(false)} aria-label="Close">×</button><p className="eyebrow">About this build</p><h1 id="about-title">Theater Runtime</h1><p>A neutral-branded, modified browser port. This project was first identified as modified on 10 July 2026.</p><p>Free software under GNU GPL v3 and the project’s additional Section 7 terms, provided without warranty. Full source, build instructions, modification history, and <code>License.txt</code> are in the source checkout.</p><p>A deployment may provide hash-verified classic freeware assets under EA’s modding guidelines. C&amp;C music and movies are excluded. Optional user-imported packages remain in this browser profile.</p><p><strong>EA has not endorsed and does not support this product.</strong> Electronic Arts and related game names and marks belong to their respective owners.</p><div className="about-links"><a href="./legal.html" target="_blank" rel="noreferrer">Full notices</a><a href="https://www.ea.com/games/command-and-conquer/command-and-conquer-remastered/news/modding-faq" target="_blank" rel="noreferrer">EA modding guidelines</a><a href="https://github.com/electronicarts/CnC_Remastered_Collection" target="_blank" rel="noreferrer">Upstream source</a><a href="https://github.com/electronicarts/CnC_Remastered_Collection/blob/master/LICENSE.md" target="_blank" rel="noreferrer">GPL and additional terms</a></div></section></div>}
    </div>
  );
}
