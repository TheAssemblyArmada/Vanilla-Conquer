import { describe, expect, it } from "vitest";
import type { RuntimeCatalogV1, RuntimeMissionV1 } from "../simulation/runtimeCatalog";
import { isTdCampaignFinalMission, nextTdCampaignMissions, tdCampaignCarryState, TD_AIRSTRIP_STRUCTURE } from "./tdCampaign";

function mission(root: string): RuntimeMissionV1 {
  const match = /^SC([GB])(\d{2})([EW])([A-C])$/.exec(root);
  if (!match) throw new Error(`Bad test root ${root}`);
  const faction = match[1] === "G" ? "gdi" : "nod";
  const scenario = Number(match[2]);
  const direction = match[3] === "E" ? 0 : 1;
  const variation = match[4].charCodeAt(0) - "A".charCodeAt(0);
  return {
    id: `${faction}-${String(scenario).padStart(2, "0")}-${direction ? "west" : "east"}-${match[4].toLowerCase()}`,
    scenarioRoot: root,
    scenario,
    variation,
    direction,
    buildLevel: scenario,
    sabotagedStructure: -1,
    faction,
    title: root,
    briefing: "Synthetic briefing.",
    theater: "temperate",
  };
}

function catalog(roots: readonly string[]): RuntimeCatalogV1 {
  return {
    format: "cncweb-runtime",
    version: 1,
    engine: "tiberian-dawn",
    engineRoot: "engine/td",
    missions: roots.map(mission),
  };
}

const GDI_ROOTS = [
  "SCG01EA", "SCG02EA", "SCG03EA", "SCG04WA", "SCG04WB", "SCG04EA",
  "SCG05EA", "SCG05WA", "SCG05WB", "SCG06EA", "SCG07EA", "SCG08EA",
  "SCG08EB", "SCG09EA", "SCG10EA", "SCG10EB", "SCG11EA", "SCG12EA",
  "SCG12EB", "SCG13EA", "SCG13EB", "SCG14EA", "SCG15EA", "SCG15EB", "SCG15EC",
] as const;

const NOD_ROOTS = [
  "SCB01EA", "SCB02EA", "SCB02EB", "SCB03EA", "SCB03EB", "SCB04EA",
  "SCB04EB", "SCB05EA", "SCB06EA", "SCB06EB", "SCB06EC", "SCB07EA",
  "SCB07EB", "SCB07EC", "SCB08EA", "SCB08EB", "SCB09EA", "SCB10EA",
  "SCB10EB", "SCB11EA", "SCB11EB", "SCB12EA", "SCB13EA", "SCB13EB", "SCB13EC",
] as const;

describe("Tiberian Dawn campaign graph", () => {
  it("reproduces every deduplicated GDI country-selection edge", () => {
    const value = catalog(GDI_ROOTS);
    const expected: Record<string, string[]> = {
      SCG01EA: ["SCG02EA"], SCG02EA: ["SCG03EA"], SCG03EA: ["SCG04WA", "SCG04WB", "SCG04EA"],
      SCG04EA: ["SCG05EA"], SCG04WA: ["SCG05WA", "SCG05WB"], SCG04WB: ["SCG05WA", "SCG05WB"],
      SCG05EA: ["SCG06EA"], SCG05WA: ["SCG06EA"], SCG05WB: ["SCG06EA"], SCG06EA: ["SCG07EA"],
      SCG07EA: ["SCG08EA", "SCG08EB"], SCG08EA: ["SCG09EA"], SCG08EB: ["SCG09EA"],
      SCG09EA: ["SCG10EA", "SCG10EB"], SCG10EA: ["SCG11EA"], SCG10EB: ["SCG11EA"],
      SCG11EA: ["SCG12EA", "SCG12EB"], SCG12EA: ["SCG13EA", "SCG13EB"], SCG12EB: ["SCG13EA", "SCG13EB"],
      SCG13EA: ["SCG14EA"], SCG13EB: ["SCG14EA"], SCG14EA: ["SCG15EA", "SCG15EB", "SCG15EC"],
      SCG15EA: [], SCG15EB: [], SCG15EC: [],
    };
    for (const current of value.missions) {
      expect(nextTdCampaignMissions(value, current, -1).map(({ scenarioRoot }) => scenarioRoot), current.scenarioRoot)
        .toEqual(expected[current.scenarioRoot]);
    }
  });

  it("reproduces every deduplicated Nod country-selection edge", () => {
    const value = catalog(NOD_ROOTS);
    const choiceCounts = [2, 2, 2, 1, 3, 3, 2, 1, 2, 2, 1, 3];
    for (let scenario = 1; scenario <= 12; scenario += 1) {
      for (const current of value.missions.filter((entry) => entry.scenario === scenario)) {
        const next = nextTdCampaignMissions(value, current, -1);
        expect(next).toHaveLength(choiceCounts[scenario - 1]);
        expect(next.every((entry) => entry.scenario === scenario + 1 && entry.faction === "nod")).toBe(true);
      }
    }
    expect(value.missions.filter((entry) => entry.scenario === 13).every((entry) => nextTdCampaignMissions(value, entry, -1).length === 0)).toBe(true);
  });

  it("applies the GDI mission-six airstrip skip and fails closed on unavailable targets", () => {
    const full = catalog(GDI_ROOTS);
    const current = full.missions.find(({ scenarioRoot }) => scenarioRoot === "SCG06EA")!;
    expect(nextTdCampaignMissions(full, current, TD_AIRSTRIP_STRUCTURE).map(({ scenarioRoot }) => scenarioRoot))
      .toEqual(["SCG08EA", "SCG08EB"]);
    const incomplete = catalog(["SCG06EA", "SCG08EB"]);
    expect(nextTdCampaignMissions(incomplete, incomplete.missions[0], TD_AIRSTRIP_STRUCTURE).map(({ scenarioRoot }) => scenarioRoot))
      .toEqual(["SCG08EB"]);
    expect(nextTdCampaignMissions(full, { ...current, title: "forged" }, -1)).toEqual([]);
  });

  it("recognizes both campaign endings", () => {
    expect(isTdCampaignFinalMission(mission("SCG15EC"))).toBe(true);
    expect(isTdCampaignFinalMission(mission("SCB13EA"))).toBe(true);
    expect(isTdCampaignFinalMission(mission("SCG14EA"))).toBe(false);
  });

  it("carries raw cash and Nod pieces while scoping GDI sabotage to mission 7", () => {
    const outcome = { carryOverCredits: 321, nukePieces: 5, sabotagedStructure: 4 };
    expect(tdCampaignCarryState(mission("SCG06EA"), mission("SCG07EA"), outcome)).toEqual(outcome);
    expect(tdCampaignCarryState(mission("SCG07EA"), mission("SCG08EA"), outcome)).toEqual({ ...outcome, sabotagedStructure: -1 });
    expect(tdCampaignCarryState(mission("SCG06EA"), mission("SCG08EA"), { ...outcome, sabotagedStructure: TD_AIRSTRIP_STRUCTURE }))
      .toEqual({ ...outcome, sabotagedStructure: -1 });
    expect(tdCampaignCarryState(mission("SCB12EA"), mission("SCB13EC"), outcome)).toEqual({ ...outcome, sabotagedStructure: -1 });
  });
});
