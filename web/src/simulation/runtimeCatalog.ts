import { Faction, GameMode, type StartConfiguration } from "./protocol";
import { validateContentPath, type ContentRevisionDescriptor } from "../storage/ContentStore";

export const RUNTIME_CATALOG_PATH = "runtime/catalog-v1.json";
export const MAX_RUNTIME_CATALOG_BYTES = 64 * 1024;

export type RuntimeTheaterV1 = "temperate" | "desert" | "winter";

export interface RuntimeMissionV1 {
  id: string;
  scenarioRoot: string;
  scenario: number;
  variation: number;
  direction: number;
  buildLevel: number;
  sabotagedStructure: number;
  faction: "gdi" | "nod";
  title: string;
  briefing: string;
  theater: RuntimeTheaterV1;
}

export interface RuntimeCatalogV1 {
  format: "cncweb-runtime";
  version: 1;
  engine: "tiberian-dawn";
  engineRoot: string;
  missions: RuntimeMissionV1[];
}

function exactKeys(value: object, expected: readonly string[], label: string): void {
  const actual = Object.keys(value).sort();
  const wanted = [...expected].sort();
  if (actual.length !== wanted.length || actual.some((key, index) => key !== wanted[index])) throw new Error(`${label} contains missing or unknown fields`);
}

function hasExactKeys(value: object, expected: readonly string[]): boolean {
  const actual = Object.keys(value).sort();
  const wanted = [...expected].sort();
  return actual.length === wanted.length && actual.every((key, index) => key === wanted[index]);
}

function boundedInteger(value: number, label: string, minimum: number, maximum: number): void {
  if (!Number.isInteger(value) || value < minimum || value > maximum) throw new Error(`${label} must be an integer between ${minimum} and ${maximum}`);
}

function expectedScenarioRoot(mission: Pick<RuntimeMissionV1, "faction" | "scenario" | "direction" | "variation">): string {
  const faction = mission.faction === "gdi" ? "G" : "B";
  const direction = mission.direction === 0 ? "E" : "W";
  const variation = mission.variation === 5 ? "L" : String.fromCharCode("A".charCodeAt(0) + mission.variation);
  return `SC${faction}${String(mission.scenario).padStart(2, "0")}${direction}${variation}`;
}

function sameMission(left: RuntimeMissionV1, right: RuntimeMissionV1): boolean {
  return left.id === right.id
    && left.scenarioRoot === right.scenarioRoot
    && left.scenario === right.scenario
    && left.variation === right.variation
    && left.direction === right.direction
    && left.buildLevel === right.buildLevel
    && left.sabotagedStructure === right.sabotagedStructure
    && left.faction === right.faction
    && left.title === right.title
    && left.briefing === right.briefing
    && left.theater === right.theater;
}

export function parseRuntimeCatalog(data: Uint8Array): RuntimeCatalogV1 {
  if (data.byteLength === 0 || data.byteLength > MAX_RUNTIME_CATALOG_BYTES) throw new Error(`Runtime catalog must contain 1 to ${MAX_RUNTIME_CATALOG_BYTES} bytes`);
  let value: unknown;
  try {
    value = JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(data));
  } catch (cause) {
    throw new Error("Runtime catalog is not valid UTF-8 JSON", { cause });
  }
  if (!value || typeof value !== "object") throw new Error("Runtime catalog is not an object");
  const catalog = value as RuntimeCatalogV1;
  exactKeys(catalog, ["format", "version", "engine", "engineRoot", "missions"], "Runtime catalog");
  if (catalog.format !== "cncweb-runtime" || catalog.version !== 1) throw new Error("Runtime catalog format is unsupported");
  if (catalog.engine !== "tiberian-dawn") throw new Error(`Runtime engine is unsupported: ${String(catalog.engine)}`);
  validateContentPath(catalog.engineRoot);
  if (catalog.engineRoot !== "engine/td") throw new Error("Runtime engineRoot must be engine/td for this browser profile");
  if (!Array.isArray(catalog.missions) || catalog.missions.length === 0 || catalog.missions.length > 256) throw new Error("Runtime catalog must contain 1 to 256 missions");
  const ids = new Set<string>();
  const scenarioRoots = new Set<string>();
  for (const mission of catalog.missions) {
    if (!mission || typeof mission !== "object") throw new Error("Runtime mission is not an object");
    const legacyKeys = ["id", "scenario", "variation", "direction", "buildLevel", "sabotagedStructure", "faction", "title", "briefing"] as const;
    const missionKeys = [...legacyKeys, "scenarioRoot", "theater"] as const;
    if (hasExactKeys(mission, legacyKeys)) {
      const canonicalLegacyMission = catalog.missions.length === 1
        && mission.id === "gdi-01-east-a"
        && mission.scenario === 1
        && mission.variation === 0
        && mission.direction === 0
        && mission.buildLevel === 1
        && mission.sabotagedStructure === -1
        && mission.faction === "gdi"
        && mission.title === "GDI Mission 1";
      if (!canonicalLegacyMission) throw new Error("Legacy runtime mission fields are only supported for canonical GDI Mission 1 East A");
      mission.scenarioRoot = "SCG01EA";
      mission.theater = "temperate";
    } else {
      exactKeys(mission, missionKeys, "Runtime mission");
    }
    if (typeof mission.id !== "string" || !/^[a-z0-9][a-z0-9._-]{0,127}$/.test(mission.id) || ids.has(mission.id)) throw new Error(`Runtime mission ID is invalid or duplicated: ${String(mission.id)}`);
    ids.add(mission.id);
    if (typeof mission.scenarioRoot !== "string" || !/^SC[GB][0-9]{2,3}[EW][A-DL]$/.test(mission.scenarioRoot)) throw new Error(`${mission.id}.scenarioRoot is invalid`);
    if (scenarioRoots.has(mission.scenarioRoot)) throw new Error(`Runtime mission scenarioRoot is duplicated: ${mission.scenarioRoot}`);
    scenarioRoots.add(mission.scenarioRoot);
    boundedInteger(mission.scenario, `${mission.id}.scenario`, 1, 999);
    if (![0, 1, 2, 3, 5].includes(mission.variation)) throw new Error(`${mission.id}.variation must be 0, 1, 2, 3, or 5`);
    boundedInteger(mission.direction, `${mission.id}.direction`, 0, 1);
    boundedInteger(mission.buildLevel, `${mission.id}.buildLevel`, 0, 255);
    boundedInteger(mission.sabotagedStructure, `${mission.id}.sabotagedStructure`, -1, 255);
    if (!(["gdi", "nod"] as const).includes(mission.faction)) throw new Error(`${mission.id}.faction is unsupported`);
    if (mission.scenarioRoot !== expectedScenarioRoot(mission)) throw new Error(`${mission.id}.scenarioRoot does not match its launch fields`);
    if (typeof mission.title !== "string" || !mission.title.trim() || new TextEncoder().encode(mission.title).byteLength > 128) throw new Error(`${mission.id}.title is invalid`);
    if (typeof mission.briefing !== "string"
      || !mission.briefing.trim()
      || new TextEncoder().encode(mission.briefing).byteLength > 4096
      || /[\r@\0]/.test(mission.briefing)) throw new Error(`${mission.id}.briefing is invalid`);
    if (!(["temperate", "desert", "winter"] as const).includes(mission.theater)) throw new Error(`${mission.id}.theater is unsupported`);
    validateContentPath(`${catalog.engineRoot}/${mission.scenarioRoot}.INI`);
    validateContentPath(`${catalog.engineRoot}/${mission.scenarioRoot}.BIN`);
  }
  return catalog;
}

export function contentIdHashFromRevision(revision: string): bigint {
  if (!/^[a-f0-9]{64}$/.test(revision)) throw new Error("Content revision must be a lowercase SHA-256 digest");
  const value = BigInt(`0x${revision.slice(0, 16)}`);
  return value === 0n ? 1n : value;
}

export function contentMountRoot(revision: string): string {
  contentIdHashFromRevision(revision);
  return `/cnc-content/${revision.slice(0, 16)}`;
}

export function missionStartConfiguration(content: ContentRevisionDescriptor, catalog: RuntimeCatalogV1, mission: RuntimeMissionV1, seed: number): StartConfiguration {
  if (!catalog.missions.some((candidate) => sameMission(candidate, mission))) throw new Error("Mission is not part of the selected runtime catalog");
  const faction = mission.faction === "gdi" ? Faction.Gdi : Faction.Nod;
  return {
    game: "tiberian-dawn",
    seed: seed >>> 0 || 1,
    scenario: mission.scenario,
    variation: mission.variation,
    direction: mission.direction,
    buildLevel: mission.buildLevel,
    sabotagedStructure: mission.sabotagedStructure,
    faction,
    gameMode: GameMode.Campaign,
    playerId: 0n,
    contentDirectory: `${contentMountRoot(content.revision)}/${catalog.engineRoot}`,
    overrideMapName: "",
    contentIdHash: contentIdHashFromRevision(content.revision),
  };
}
