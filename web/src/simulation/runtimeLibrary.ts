import { type ContentRevisionDescriptor, ContentStore } from "../storage/ContentStore";
import { assertFirstPlayableAudio, MAX_RUNTIME_AUDIO_INDEX_BYTES, parseRuntimeAudioIndex, RUNTIME_AUDIO_PATH } from "../audio/RuntimeAudio";
import { MAX_RUNTIME_CATALOG_BYTES, parseRuntimeCatalog, RUNTIME_CATALOG_PATH, type RuntimeCatalogV1, type RuntimeMissionV1, type RuntimeTheaterV1 } from "./runtimeCatalog";
import { parseRuntimeScenarioTheater } from "./runtimeTheater";

export const REQUIRED_TD_ENGINE_FILES = [
  "CCLOCAL.MIX",
  "CONQUER.MIX",
  "GENERAL.MIX",
  "SOUNDS.MIX",
  "SPEECH.MIX",
  "TRANSIT.MIX",
  "UPDATEC.MIX",
] as const;

export const TD_THEATER_FILES = {
  temperate: { archives: ["TEMPERAT.MIX", "TEMPICNH.MIX"], palette: "TEMPERAT.PAL" },
  desert: { archives: ["DESERT.MIX", "DESEICNH.MIX"], palette: "DESERT.PAL" },
  winter: { archives: ["WINTER.MIX", "WINTICNH.MIX"], palette: "WINTER.PAL" },
} as const satisfies Record<RuntimeTheaterV1, { archives: readonly [string, string]; palette: string }>;

export interface CompatibleRuntimePack {
  descriptor: ContentRevisionDescriptor;
  catalog: RuntimeCatalogV1;
  warnings: string[];
}

export interface IncompatibleRuntimePack {
  id: string;
  revision: string;
  reason: string;
}

export interface RuntimeLibrary {
  compatible: CompatibleRuntimePack[];
  incompatible: IncompatibleRuntimePack[];
}

const MAX_RUNTIME_ENGINE_FILE_BYTES = 64 * 1024 * 1024;
const MAX_SCENARIO_INI_BYTES = 1024 * 1024;
const EXPECTED_SCENARIO_BIN_BYTES = 8 * 1024;

function scenarioFiles(mission: RuntimeMissionV1): readonly [string, string] {
  return [`${mission.scenarioRoot}.INI`, `${mission.scenarioRoot}.BIN`];
}

function usedTheaters(catalog: RuntimeCatalogV1): RuntimeTheaterV1[] {
  return [...new Set(catalog.missions.map((mission) => mission.theater))];
}

export function requiredRuntimeEngineFiles(catalog: RuntimeCatalogV1): string[] {
  const required = new Set<string>(REQUIRED_TD_ENGINE_FILES);
  for (const mission of catalog.missions) {
    for (const name of scenarioFiles(mission)) required.add(name);
  }
  for (const theater of usedTheaters(catalog)) {
    for (const name of TD_THEATER_FILES[theater].archives) required.add(name);
  }
  return [...required];
}

export function assertRuntimeLayout(descriptor: ContentRevisionDescriptor, catalog: RuntimeCatalogV1): string[] {
  if (!descriptor.manifest.content.games.includes("tiberian-dawn")) throw new Error("Manifest does not declare Tiberian Dawn content");
  const files = new Map<string, ContentRevisionDescriptor["manifest"]["files"][number]>();
  for (const file of descriptor.manifest.files) {
    if (files.has(file.path)) throw new Error(`Runtime manifest contains a duplicate file: ${file.path}`);
    files.set(file.path, file);
  }
  const requiredEngineFiles = requiredRuntimeEngineFiles(catalog);
  const required = [
    RUNTIME_AUDIO_PATH,
    ...requiredEngineFiles.map((name) => `${catalog.engineRoot}/${name}`),
  ];
  const missing = required.filter((path) => !files.has(path));
  if (missing.length) throw new Error(`Runtime pack is missing required file${missing.length === 1 ? "" : "s"}: ${missing.join(", ")}`);
  const scenarioFileNames = new Set(catalog.missions.flatMap((mission) => scenarioFiles(mission)));
  for (const name of requiredEngineFiles) {
    const path = `${catalog.engineRoot}/${name}`;
    const file = files.get(path)!;
    if (name.endsWith(".INI") && scenarioFileNames.has(name)) {
      if (file.role !== "engine-data" || file.size <= 0 || file.size > MAX_SCENARIO_INI_BYTES) throw new Error(`Runtime scenario INI does not match the manifest profile: ${path}`);
    } else if (name.endsWith(".BIN") && scenarioFileNames.has(name)) {
      if (file.role !== "engine-data" || file.size !== EXPECTED_SCENARIO_BIN_BYTES) throw new Error(`Runtime scenario map does not match the manifest profile: ${path}`);
    } else if (file.role !== "engine-data" || file.size < 6 || file.size > MAX_RUNTIME_ENGINE_FILE_BYTES) {
      throw new Error(`Runtime engine file does not match the manifest profile: ${path}`);
    }
  }
  const moviesPath = `${catalog.engineRoot}/MOVIES.MIX`;
  const movies = files.get(moviesPath);
  if (movies && (movies.role !== "engine-data" || movies.size < 6 || movies.size > MAX_RUNTIME_ENGINE_FILE_BYTES)) throw new Error(`Optional movie archive does not match the manifest profile: ${moviesPath}`);
  const warnings: string[] = [];
  if (!movies) warnings.push("MOVIES.MIX is unavailable; movie requests will be reported without playback");
  for (const theater of usedTheaters(catalog)) {
    const paletteName = TD_THEATER_FILES[theater].palette;
    const palettePath = `${catalog.engineRoot}/${paletteName}`;
    const palette = files.get(palettePath);
    if (palette && (palette.role !== "engine-data" || palette.size !== 768)) throw new Error(`Optional runtime palette does not match the manifest profile: ${palettePath}`);
    if (!palette) warnings.push(`Loose ${paletteName} is unavailable; the ${theater} palette will be loaded from MIX content`);
  }
  return warnings;
}

// Retain the milestone-era export for callers while applying the generalized
// catalog-wide layout contract.
export const assertFirstPlayableRuntimeLayout = assertRuntimeLayout;

export async function inspectRuntimePack(store: ContentStore, id: string): Promise<CompatibleRuntimePack> {
  const descriptor = await store.getRevisionDescriptor(id);
  const catalogFile = descriptor.manifest.files.find((file) => file.path === RUNTIME_CATALOG_PATH);
  if (!catalogFile) throw new Error(`Runtime catalog is missing at ${RUNTIME_CATALOG_PATH}`);
  if (catalogFile.role !== "configuration" || catalogFile.size <= 0 || catalogFile.size > MAX_RUNTIME_CATALOG_BYTES) throw new Error(`Runtime catalog does not match the manifest: ${RUNTIME_CATALOG_PATH}`);
  const catalog = parseRuntimeCatalog(await store.readRevisionFile(descriptor, RUNTIME_CATALOG_PATH));
  const warnings = assertRuntimeLayout(descriptor, catalog);
  for (const mission of catalog.missions) {
    const path = `${catalog.engineRoot}/${mission.scenarioRoot}.INI`;
    let theater: RuntimeTheaterV1;
    try {
      theater = parseRuntimeScenarioTheater(await store.readRevisionFile(descriptor, path));
    } catch (cause) {
      throw new Error(`Runtime scenario theater metadata is invalid: ${path}${cause instanceof Error ? `: ${cause.message}` : ""}`, { cause });
    }
    if (theater !== mission.theater) throw new Error(`Runtime catalog theater does not match scenario INI: ${path}`);
  }
  const audioIndexFile = descriptor.manifest.files.find((file) => file.path === RUNTIME_AUDIO_PATH);
  if (!audioIndexFile || audioIndexFile.role !== "configuration" || audioIndexFile.size <= 0 || audioIndexFile.size > MAX_RUNTIME_AUDIO_INDEX_BYTES) throw new Error(`Runtime audio index does not match the manifest: ${RUNTIME_AUDIO_PATH}`);
  const audio = parseRuntimeAudioIndex(await store.readRevisionFile(descriptor, RUNTIME_AUDIO_PATH));
  assertFirstPlayableAudio(audio, descriptor.manifest.source.product === "tiberian-dawn-freeware");
  const manifestFiles = new Map(descriptor.manifest.files.map((file) => [file.path, file]));
  for (const asset of audio.assets) {
    const file = manifestFiles.get(asset.path);
    if (!file || file.role !== "audio" || file.sha256 !== asset.sha256 || file.size < 44) throw new Error(`Runtime audio asset does not match the manifest: ${asset.path}`);
  }
  if (audio.diagnostics.missingCandidates) warnings.push(`${audio.diagnostics.missingCandidates} engine audio cue${audio.diagnostics.missingCandidates === 1 ? " was" : "s were"} absent from the source archives`);
  if (audio.diagnostics.decodeFailures.length) warnings.push(`${audio.diagnostics.decodeFailures.length} source audio cue${audio.diagnostics.decodeFailures.length === 1 ? " could" : "s could"} not be converted to browser PCM`);
  return { descriptor, catalog, warnings };
}

export async function loadRuntimeLibrary(store: ContentStore): Promise<RuntimeLibrary> {
  const listing = await store.listWithIssues();
  const installed = listing.installed;
  type Inspection = { kind: "compatible"; value: CompatibleRuntimePack } | { kind: "incompatible"; value: IncompatibleRuntimePack };
  const results: Inspection[] = await Promise.all(installed.map(async ({ id, revision }): Promise<Inspection> => {
    try {
      return { kind: "compatible", value: await inspectRuntimePack(store, id) };
    } catch (error) {
      return { kind: "incompatible", value: { id, revision, reason: error instanceof Error ? error.message : String(error) } };
    }
  }));
  const compatible: CompatibleRuntimePack[] = [];
  const incompatible: IncompatibleRuntimePack[] = listing.issues.map((issue) => ({
    id: issue.id ?? issue.indexPath,
    revision: issue.revision ?? "unknown",
    reason: `Stored package index ${issue.indexPath} could not be loaded: ${issue.reason}`,
  }));
  for (const result of results) {
    if (result.kind === "compatible") compatible.push(result.value);
    else incompatible.push(result.value);
  }
  return { compatible, incompatible };
}
