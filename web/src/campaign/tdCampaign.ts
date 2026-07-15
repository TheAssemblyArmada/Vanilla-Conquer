import type { RuntimeCatalogV1, RuntimeMissionV1 } from "../simulation/runtimeCatalog";

export const TD_AIRSTRIP_STRUCTURE = 11;

export interface TdCampaignCarryState {
  carryOverCredits: number;
  nukePieces: number;
  sabotagedStructure: number;
}

const GDI_NEXT_ROOTS: Readonly<Record<string, readonly string[]>> = {
  "1:0": ["SCG02EA"],
  "2:0": ["SCG03EA"],
  "3:0": ["SCG04WA", "SCG04WB", "SCG04EA"],
  "4:0": ["SCG05EA"],
  "4:1": ["SCG05WA", "SCG05WB"],
  "5:0": ["SCG06EA"],
  "5:1": ["SCG06EA"],
  "6:0": ["SCG07EA"],
  "7:0": ["SCG08EA", "SCG08EB"],
  "8:0": ["SCG09EA"],
  "9:0": ["SCG10EA", "SCG10EB"],
  "10:0": ["SCG11EA"],
  "11:0": ["SCG12EA", "SCG12EB"],
  "12:0": ["SCG13EA", "SCG13EB"],
  "13:0": ["SCG14EA"],
  "14:0": ["SCG15EA", "SCG15EB", "SCG15EC"],
};

const NOD_NEXT_ROOTS: Readonly<Record<string, readonly string[]>> = {
  "1:0": ["SCB02EA", "SCB02EB"],
  "2:0": ["SCB03EA", "SCB03EB"],
  "3:0": ["SCB04EA", "SCB04EB"],
  "4:0": ["SCB05EA"],
  "5:0": ["SCB06EA", "SCB06EB", "SCB06EC"],
  "6:0": ["SCB07EA", "SCB07EB", "SCB07EC"],
  "7:0": ["SCB08EA", "SCB08EB"],
  "8:0": ["SCB09EA"],
  "9:0": ["SCB10EA", "SCB10EB"],
  "10:0": ["SCB11EA", "SCB11EB"],
  "11:0": ["SCB12EA"],
  "12:0": ["SCB13EA", "SCB13EB", "SCB13EC"],
};

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

/**
 * Resolves the original Tiberian Dawn CountryArray branch graph against the
 * selected immutable catalog. Missing or non-canonical targets are omitted;
 * catalog data can never invent a campaign edge.
 */
export function nextTdCampaignMissions(
  catalog: RuntimeCatalogV1,
  current: RuntimeMissionV1,
  sabotagedStructure: number,
): RuntimeMissionV1[] {
  if (!catalog.missions.some((mission) => sameMission(mission, current))) return [];
  let roots: readonly string[] | undefined;
  if (current.faction === "gdi") {
    // Classic Do_Win skips mission 7 when its airstrip was sabotaged, then
    // uses mission 7's country choices to select the mission 8 variant.
    roots = current.scenario === 6 && sabotagedStructure === TD_AIRSTRIP_STRUCTURE
      ? ["SCG08EA", "SCG08EB"]
      : GDI_NEXT_ROOTS[`${current.scenario}:${current.direction}`];
  } else {
    roots = NOD_NEXT_ROOTS[`${current.scenario}:${current.direction}`];
  }
  if (!roots) return [];
  const byRoot = new Map(catalog.missions.map((mission) => [mission.scenarioRoot, mission]));
  return roots.flatMap((root) => {
    const mission = byRoot.get(root);
    return mission && mission.faction === current.faction ? [mission] : [];
  });
}

export function isTdCampaignFinalMission(mission: RuntimeMissionV1): boolean {
  return (mission.faction === "gdi" && mission.scenario === 15)
    || (mission.faction === "nod" && mission.scenario === 13);
}

export function tdCampaignCarryState(
  current: RuntimeMissionV1,
  next: RuntimeMissionV1,
  outcome: Pick<TdCampaignCarryState, "carryOverCredits" | "nukePieces" | "sabotagedStructure">,
): TdCampaignCarryState {
  const carriesSabotage = current.faction === "gdi"
    && current.scenario === 6
    && next.faction === "gdi"
    && next.scenario === 7
    && outcome.sabotagedStructure !== TD_AIRSTRIP_STRUCTURE;
  return {
    carryOverCredits: outcome.carryOverCredits,
    nukePieces: outcome.nukePieces,
    sabotagedStructure: carriesSabotage ? outcome.sabotagedStructure : -1,
  };
}
