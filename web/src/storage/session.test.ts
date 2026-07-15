import { describe, expect, it, vi } from "vitest";
import { loadPersistedSession, savePersistedSession, SESSION_STORAGE_KEY, validatePersistedSession } from "./session";

describe("persisted runtime sessions", () => {
  it("round-trips exact package revision, mission, seed, and save identity", () => {
    const values = new Map<string, string>();
    const storage = { getItem: (key: string) => values.get(key) ?? null, setItem: (key: string, value: string) => { values.set(key, value); } };
    const session = { version: 1 as const, mode: "mission" as const, packageId: "owned-pack", revision: "12".repeat(32), missionId: "gdi-01-east-a", seed: 42, resumeSaveId: "mission-autosave" };
    savePersistedSession(session, storage);
    expect(loadPersistedSession(storage)).toEqual(session);
    expect(values.has(SESSION_STORAGE_KEY)).toBe(true);
  });

  it("rejects stale shapes and treats inaccessible storage as no session", () => {
    expect(() => validatePersistedSession({ version: 1, mode: "demo", seed: 1, physicalStorageKey: "secret" })).toThrow("unknown fields");
    expect(loadPersistedSession({ getItem: vi.fn(() => { throw new DOMException("blocked", "SecurityError"); }) })).toBeUndefined();
  });

  it("round-trips a correlated pending campaign victory and incoming carry state", () => {
    const values = new Map<string, string>();
    const storage = { getItem: (key: string) => values.get(key) ?? null, setItem: (key: string, value: string) => { values.set(key, value); } };
    const session = {
      version: 2 as const,
      mode: "mission" as const,
      packageId: "owned-pack",
      revision: "34".repeat(32),
      missionId: "gdi-06-east-a",
      seed: 1,
      runId: "campaign-run-1",
      incomingTransition: { carryOverCredits: 400, nukePieces: 7, sabotagedStructure: -1 },
      pendingVictory: {
        gameOver: { tick: 90, score: 1000, leadership: 70, efficiency: 80, remainingCredits: 900, sabotagedStructure: 11, movieName: "WIN", afterScoreMovieName: "" },
        outcome: { tick: 90, carryOverCredits: 500, nukePieces: 7, sabotagedStructure: 11, randomSeed: 0, scenario: 6, house: 0 as const, scenarioRoot: "SCG06EA" },
      },
    };
    savePersistedSession(session, storage);
    expect(loadPersistedSession(storage)).toEqual(session);
  });

  it("retains only an explicit v2 marker for a run-less legacy resume", () => {
    const session = {
      version: 2 as const, mode: "mission" as const, packageId: "owned-pack", revision: "78".repeat(32), missionId: "gdi-01-east-a",
      seed: 4, runId: "run-2", resumeSaveId: "old-autosave", legacyResume: true as const,
    };
    expect(validatePersistedSession(session)).toEqual(session);
    expect(() => validatePersistedSession({ ...session, resumeSaveId: undefined })).toThrow("requires a save ID");
    expect(() => validatePersistedSession({ ...session, legacyResume: false })).toThrow("marker is invalid");
  });

  it("rejects uncorrelated or unsafe v2 campaign state", () => {
    const base = {
      version: 2, mode: "mission", packageId: "owned-pack", revision: "56".repeat(32), missionId: "gdi-01-east-a", seed: 1, runId: "run-1",
    };
    const gameOver = { tick: 9, score: 1, leadership: 2, efficiency: 3, remainingCredits: 4, sabotagedStructure: -1, movieName: "", afterScoreMovieName: "" };
    const outcome = { tick: 10, carryOverCredits: 4, nukePieces: 7, sabotagedStructure: -1, randomSeed: 5, scenario: 1, house: 0, scenarioRoot: "SCG01EA" };
    expect(() => validatePersistedSession({ ...base, pendingVictory: { gameOver, outcome } })).toThrow("do not correlate");
    expect(() => validatePersistedSession({ ...base, incomingTransition: { carryOverCredits: 0, nukePieces: 8, sabotagedStructure: -1 } })).toThrow("nuke pieces");
    expect(() => validatePersistedSession({ ...base, runId: "../run" })).toThrow("run ID");
  });
});
