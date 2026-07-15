import { validateId } from "./helpers";

export const SESSION_STORAGE_KEY = "theater.runtime-session.v1";

export type PersistedSessionV1 =
  | { version: 1; mode: "demo"; seed: number; resumeSaveId?: string }
  | { version: 1; mode: "mission"; packageId: string; revision: string; missionId: string; seed: number; resumeSaveId?: string };

export interface PersistedCampaignTransitionV2 {
  carryOverCredits: number;
  nukePieces: number;
  sabotagedStructure: number;
}

export interface PersistedCampaignOutcomeV2 {
  tick: number;
  carryOverCredits: number;
  nukePieces: number;
  sabotagedStructure: number;
  randomSeed: number;
  scenario: number;
  house: 0 | 1;
  scenarioRoot: string;
}

export interface PersistedVictoryResultV2 {
  tick: number;
  score: number;
  leadership: number;
  efficiency: number;
  remainingCredits: number;
  sabotagedStructure: number;
  movieName: string;
  afterScoreMovieName: string;
}

export interface PersistedPendingVictoryV2 {
  gameOver: PersistedVictoryResultV2;
  outcome: PersistedCampaignOutcomeV2;
}

export type PersistedSessionV2 =
  | { version: 2; mode: "demo"; seed: number; runId: string; resumeSaveId?: string }
  | {
    version: 2;
    mode: "mission";
    packageId: string;
    revision: string;
    missionId: string;
    seed: number;
    runId: string;
    resumeSaveId?: string;
    legacyResume?: true;
    incomingTransition?: PersistedCampaignTransitionV2;
    pendingVictory?: PersistedPendingVictoryV2;
  };

export type PersistedSession = PersistedSessionV1 | PersistedSessionV2;

function exactKeys(value: object, required: readonly string[], optional: readonly string[] = []): void {
  const keys = Object.keys(value);
  if (required.some((key) => !Object.hasOwn(value, key)) || keys.some((key) => !required.includes(key) && !optional.includes(key))) {
    throw new Error("Saved session contains missing or unknown fields");
  }
}

function boundedInteger(value: unknown, label: string, minimum: number, maximum: number): number {
  if (!Number.isInteger(value) || (value as number) < minimum || (value as number) > maximum) throw new Error(`${label} is invalid`);
  return value as number;
}

function validateSeed(seed: unknown, allowZero = false): number {
  return boundedInteger(seed, "Saved session seed", allowZero ? 0 : 1, 0xffff_ffff);
}

function optionalSaveId(value: unknown): string | undefined {
  if (value === undefined) return undefined;
  if (typeof value !== "string") throw new Error("Resume save ID is invalid");
  validateId(value, "Resume save ID");
  return value;
}

function validateRunId(value: unknown): string {
  if (typeof value !== "string") throw new Error("Campaign run ID is invalid");
  validateId(value, "Campaign run ID");
  return value;
}

function validateMissionIdentity(session: Record<string, unknown>): { packageId: string; revision: string; missionId: string } {
  if (typeof session.packageId !== "string") throw new Error("Session package ID is invalid");
  validateId(session.packageId, "Session package ID");
  if (typeof session.revision !== "string" || !/^[a-f0-9]{64}$/.test(session.revision)) throw new Error("Saved session content revision is invalid");
  if (typeof session.missionId !== "string") throw new Error("Session mission ID is invalid");
  validateId(session.missionId, "Session mission ID");
  return { packageId: session.packageId, revision: session.revision, missionId: session.missionId };
}

function validateTransition(value: unknown): PersistedCampaignTransitionV2 {
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error("Campaign transition is invalid");
  exactKeys(value, ["carryOverCredits", "nukePieces", "sabotagedStructure"]);
  const transition = value as Record<string, unknown>;
  return {
    carryOverCredits: boundedInteger(transition.carryOverCredits, "Campaign carry-over credits", -0x8000_0000, 0x7fff_ffff),
    nukePieces: boundedInteger(transition.nukePieces, "Campaign nuke pieces", 0, 7),
    sabotagedStructure: boundedInteger(transition.sabotagedStructure, "Campaign sabotaged structure", -1, 255),
  };
}

function validateOutcome(value: unknown): PersistedCampaignOutcomeV2 {
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error("Campaign outcome is invalid");
  exactKeys(value, ["tick", "carryOverCredits", "nukePieces", "sabotagedStructure", "randomSeed", "scenario", "house", "scenarioRoot"]);
  const outcome = value as Record<string, unknown>;
  if (typeof outcome.scenarioRoot !== "string" || !/^SC[GB][0-9]{2,3}[EW][A-DL]$/.test(outcome.scenarioRoot)) throw new Error("Campaign outcome scenario root is invalid");
  return {
    tick: boundedInteger(outcome.tick, "Campaign outcome tick", 0, 0xffff_ffff),
    carryOverCredits: boundedInteger(outcome.carryOverCredits, "Campaign carry-over credits", -0x8000_0000, 0x7fff_ffff),
    nukePieces: boundedInteger(outcome.nukePieces, "Campaign nuke pieces", 0, 7),
    sabotagedStructure: boundedInteger(outcome.sabotagedStructure, "Campaign sabotaged structure", -1, 255),
    randomSeed: boundedInteger(outcome.randomSeed, "Campaign random seed", 0, 0xffff_ffff),
    scenario: boundedInteger(outcome.scenario, "Campaign outcome scenario", 1, 999),
    house: boundedInteger(outcome.house, "Campaign outcome house", 0, 1) as 0 | 1,
    scenarioRoot: outcome.scenarioRoot,
  };
}

function validateVictory(value: unknown): PersistedVictoryResultV2 {
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error("Campaign victory result is invalid");
  exactKeys(value, ["tick", "score", "leadership", "efficiency", "remainingCredits", "sabotagedStructure", "movieName", "afterScoreMovieName"]);
  const victory = value as Record<string, unknown>;
  if (typeof victory.movieName !== "string" || typeof victory.afterScoreMovieName !== "string"
    || victory.movieName.length > 256 || victory.afterScoreMovieName.length > 256) throw new Error("Campaign victory movie cue is invalid");
  return {
    tick: boundedInteger(victory.tick, "Campaign victory tick", 0, 0xffff_ffff),
    score: boundedInteger(victory.score, "Campaign victory score", -0x8000_0000, 0x7fff_ffff),
    leadership: boundedInteger(victory.leadership, "Campaign victory leadership", -0x8000_0000, 0x7fff_ffff),
    efficiency: boundedInteger(victory.efficiency, "Campaign victory efficiency", -0x8000_0000, 0x7fff_ffff),
    remainingCredits: boundedInteger(victory.remainingCredits, "Campaign victory credits", -0x8000_0000, 0x7fff_ffff),
    sabotagedStructure: boundedInteger(victory.sabotagedStructure, "Campaign victory sabotaged structure", -1, 255),
    movieName: victory.movieName,
    afterScoreMovieName: victory.afterScoreMovieName,
  };
}

function validatePendingVictory(value: unknown): PersistedPendingVictoryV2 {
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error("Pending campaign victory is invalid");
  exactKeys(value, ["gameOver", "outcome"]);
  const pending = value as Record<string, unknown>;
  const gameOver = validateVictory(pending.gameOver);
  const outcome = validateOutcome(pending.outcome);
  if (gameOver.tick !== outcome.tick || gameOver.sabotagedStructure !== outcome.sabotagedStructure) throw new Error("Pending campaign victory events do not correlate");
  return { gameOver, outcome };
}

export function validatePersistedSession(value: unknown): PersistedSession {
  if (!value || typeof value !== "object" || Array.isArray(value)) throw new Error("Saved session is not an object");
  const session = value as Record<string, unknown>;
  if ((session.version !== 1 && session.version !== 2) || (session.mode !== "demo" && session.mode !== "mission")) throw new Error("Saved session format is unsupported");

  if (session.version === 1) {
    const required = session.mode === "demo" ? ["version", "mode", "seed"] : ["version", "mode", "packageId", "revision", "missionId", "seed"];
    exactKeys(session, required, ["resumeSaveId"]);
    const seed = validateSeed(session.seed);
    const resumeSaveId = optionalSaveId(session.resumeSaveId);
    if (session.mode === "demo") return { version: 1, mode: "demo", seed, ...(resumeSaveId ? { resumeSaveId } : {}) };
    return { version: 1, mode: "mission", ...validateMissionIdentity(session), seed, ...(resumeSaveId ? { resumeSaveId } : {}) };
  }

  if (session.mode === "demo") {
    exactKeys(session, ["version", "mode", "seed", "runId"], ["resumeSaveId"]);
    const resumeSaveId = optionalSaveId(session.resumeSaveId);
    return { version: 2, mode: "demo", seed: validateSeed(session.seed), runId: validateRunId(session.runId), ...(resumeSaveId ? { resumeSaveId } : {}) };
  }

  exactKeys(session, ["version", "mode", "packageId", "revision", "missionId", "seed", "runId"], ["resumeSaveId", "legacyResume", "incomingTransition", "pendingVictory"]);
  const resumeSaveId = optionalSaveId(session.resumeSaveId);
  if (session.legacyResume !== undefined && session.legacyResume !== true) throw new Error("Legacy resume marker is invalid");
  if (session.legacyResume && !resumeSaveId) throw new Error("Legacy resume marker requires a save ID");
  const incomingTransition = session.incomingTransition === undefined ? undefined : validateTransition(session.incomingTransition);
  const pendingVictory = session.pendingVictory === undefined ? undefined : validatePendingVictory(session.pendingVictory);
  if (resumeSaveId && pendingVictory) throw new Error("A pending victory cannot also resume a save");
  return {
    version: 2,
    mode: "mission",
    ...validateMissionIdentity(session),
    seed: validateSeed(session.seed, Boolean(incomingTransition)),
    runId: validateRunId(session.runId),
    ...(resumeSaveId ? { resumeSaveId } : {}),
    ...(session.legacyResume ? { legacyResume: true as const } : {}),
    ...(incomingTransition ? { incomingTransition } : {}),
    ...(pendingVictory ? { pendingVictory } : {}),
  };
}

export function loadPersistedSession(storage: Pick<Storage, "getItem"> = localStorage): PersistedSession | undefined {
  try {
    const encoded = storage.getItem(SESSION_STORAGE_KEY);
    if (!encoded) return undefined;
    return validatePersistedSession(JSON.parse(encoded));
  } catch {
    return undefined;
  }
}

export function savePersistedSession(session: PersistedSession, storage: Pick<Storage, "setItem"> = localStorage): void {
  storage.setItem(SESSION_STORAGE_KEY, JSON.stringify(validatePersistedSession(session)));
}
