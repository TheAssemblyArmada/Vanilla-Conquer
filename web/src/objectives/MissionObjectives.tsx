import type { RuntimeMissionV1 } from "../simulation/runtimeCatalog";
import type { SnapshotSidebar } from "../simulation/snapshot";
import "./missionObjectives.css";

export type MissionObjectiveStatus = "active" | "complete" | "failed";

export interface MissionObjectiveResult {
  won: boolean;
}

export interface MissionObjectiveItem {
  id: string;
  label: string;
  description: string;
  progress: string;
  status: MissionObjectiveStatus;
}

export interface MissionObjectivePresentation {
  title: string;
  status: MissionObjectiveStatus;
  items: readonly MissionObjectiveItem[];
}

type MissionFour = "4-west-a" | "4-west-b" | "4-east-a";
type MissionFive = "5-east-a" | "5-west-a" | "5-west-b";
type ReviewedGdiMission = 1 | 2 | 3 | MissionFour | MissionFive;

function reviewedGdiMission(mission: RuntimeMissionV1): ReviewedGdiMission | undefined {
  if (mission.faction !== "gdi") return undefined;
  if (mission.direction === 0 && mission.variation === 0) {
    if (mission.scenarioRoot === "SCG01EA" && mission.scenario === 1) return 1;
    if (mission.scenarioRoot === "SCG02EA" && mission.scenario === 2) return 2;
    if (mission.scenarioRoot === "SCG03EA" && mission.scenario === 3) return 3;
  }
  if (
    mission.id === "gdi-04-west-a"
    && mission.scenarioRoot === "SCG04WA"
    && mission.scenario === 4
    && mission.variation === 0
    && mission.direction === 1
    && mission.buildLevel === 4
  ) return "4-west-a";
  if (
    mission.id === "gdi-04-west-b"
    && mission.scenarioRoot === "SCG04WB"
    && mission.scenario === 4
    && mission.variation === 1
    && mission.direction === 1
    && mission.buildLevel === 4
  ) return "4-west-b";
  if (
    mission.id === "gdi-04-east-a"
    && mission.scenarioRoot === "SCG04EA"
    && mission.scenario === 4
    && mission.variation === 0
    && mission.direction === 0
    && mission.buildLevel === 4
  ) return "4-east-a";
  if (
    mission.id === "gdi-05-east-a"
    && mission.scenarioRoot === "SCG05EA"
    && mission.scenario === 5
    && mission.variation === 0
    && mission.direction === 0
    && mission.buildLevel === 5
  ) return "5-east-a";
  if (
    mission.id === "gdi-05-west-a"
    && mission.scenarioRoot === "SCG05WA"
    && mission.scenario === 5
    && mission.variation === 0
    && mission.direction === 1
    && mission.buildLevel === 5
  ) return "5-west-a";
  if (
    mission.id === "gdi-05-west-b"
    && mission.scenarioRoot === "SCG05WB"
    && mission.scenario === 5
    && mission.variation === 1
    && mission.direction === 1
    && mission.buildLevel === 5
  ) return "5-west-b";
  return undefined;
}

function resultStatus(result: MissionObjectiveResult | undefined): MissionObjectiveStatus {
  if (!result) return "active";
  return result.won ? "complete" : "failed";
}

function destroyedProgress(stats: SnapshotSidebar | undefined, result: MissionObjectiveResult | undefined): string {
  if (result?.won) return "Engine-confirmed objective complete";
  const units = stats?.unitsKilled ?? 0;
  const structures = stats?.buildingsKilled ?? 0;
  return `${units.toLocaleString()} ${units === 1 ? "unit" : "units"} and ${structures.toLocaleString()} ${structures === 1 ? "structure" : "structures"} destroyed`;
}

function survivalProgress(stats: SnapshotSidebar | undefined, result: MissionObjectiveResult | undefined): string {
  if (result?.won) return "GDI force survived";
  if (result && !result.won) return "All counted GDI ground forces were lost";
  const losses = (stats?.unitsLost ?? 0) + (stats?.buildingsLost ?? 0);
  return `${losses.toLocaleString()} ${losses === 1 ? "loss" : "losses"} recorded`;
}

function engineRuleProgress(
  result: MissionObjectiveResult | undefined,
  active: string,
  complete = "Engine-confirmed objective complete",
): string {
  if (!result) return active;
  return result.won ? complete : "Engine-confirmed operation failed";
}

function missionFourPresentation(
  mission: MissionFour,
  result: MissionObjectiveResult | undefined,
): MissionObjectivePresentation {
  const status = resultStatus(result);
  if (mission === "4-west-b") {
    return {
      title: "Operation orders",
      status,
      items: [
        {
          id: "eliminate-nod",
          label: "Eliminate the Nod force",
          description: "Destroy every counted Nod unit in the operation area. Triggered Nod assault groups become additional targets.",
          progress: engineRuleProgress(result, "Nod elimination objective active"),
          status,
        },
        {
          id: "preserve-village",
          label: "Preserve the protected village",
          description: "The operation fails if all four protected village structures are destroyed.",
          progress: engineRuleProgress(result, "Village protection condition active", "Engine-confirmed protection condition satisfied"),
          status,
        },
        {
          id: "preserve-gdi",
          label: "Keep GDI operational",
          description: "The operation fails if every counted GDI infantry unit and ground vehicle is destroyed.",
          progress: engineRuleProgress(result, "GDI survival condition active", "GDI force survived"),
          status,
        },
      ],
    };
  }
  return {
    title: "Operation orders",
    status,
    items: [
      {
        id: "recover-crate",
        label: "Recover the GDI crate",
        description: "Reach the marked recovery area. The operation completes when a GDI unit enters the crate cell; destroying Nod is not required.",
        progress: engineRuleProgress(result, "Crate recovery objective active"),
        status,
      },
      {
        id: "preserve-gdi",
        label: "Keep the recovery force operational",
        description: "The operation fails if every counted GDI infantry unit and ground vehicle is destroyed. A transport aircraft alone does not prevent defeat.",
        progress: engineRuleProgress(result, "Recovery force condition active", "GDI recovery force survived"),
        status,
      },
    ],
  };
}

function missionFivePresentation(
  result: MissionObjectiveResult | undefined,
  stats: SnapshotSidebar | undefined,
): MissionObjectivePresentation {
  const status = resultStatus(result);
  return {
    title: "Operation orders",
    status,
    items: [
      {
        id: "eliminate-nod",
        label: "Eliminate the Nod force",
        description: "Destroy every counted Nod unit and structure in the operation area. Nod production, rebuilt structures, patrols, and timed attack teams can add targets.",
        progress: destroyedProgress(stats, result),
        status,
      },
      {
        id: "relieve-base",
        label: "Relieve the separated GDI base",
        description: "Move GDI units through both authored relief zones. Until each zone is crossed, losing the last member of its protected starting group—field force or base structures—immediately fails the operation.",
        progress: engineRuleProgress(result, "Base-relief conditions active", "Engine-confirmed relief conditions satisfied"),
        status,
      },
      {
        id: "preserve-gdi",
        label: "Keep GDI operational",
        description: "The operation also fails if every counted GDI unit and structure is destroyed.",
        progress: engineRuleProgress(result, "GDI survival condition active", "GDI force survived"),
        status,
      },
    ],
  };
}

/**
 * Returns mission rules only when the browser has an exact, reviewed rule set.
 * Final status always comes from the engine result; snapshot statistics are
 * progress context and are never treated as proof of victory.
 */
export function missionObjectivePresentation(
  mission: RuntimeMissionV1,
  stats: SnapshotSidebar | undefined,
  result: MissionObjectiveResult | undefined,
): MissionObjectivePresentation | undefined {
  const reviewedMission = reviewedGdiMission(mission);
  if (!reviewedMission) return undefined;
  if (typeof reviewedMission === "string") {
    return reviewedMission.startsWith("4-")
      ? missionFourPresentation(reviewedMission as MissionFour, result)
      : missionFivePresentation(result, stats);
  }
  const status = resultStatus(result);
  const missionTwo = reviewedMission === 2;
  const missionThree = reviewedMission === 3;
  return {
    title: "Operation orders",
    status,
    items: [
      {
        id: "eliminate-nod",
        label: missionTwo ? "Eliminate the Nod occupation" : "Eliminate the Nod force",
        description: missionThree
          ? "Destroy every counted Nod unit and structure in the operation area. Nod production, rebuilt structures, and attack teams can add targets."
          : missionTwo
            ? "Destroy every Nod unit and structure in the occupied region. Attack teams and field reinforcements may change the force count."
            : "Destroy the Nod units and structures assigned to this operation. Reinforcements may enter the battlefield.",
        progress: destroyedProgress(stats, result),
        status,
      },
      {
        id: "preserve-gdi",
        label: missionThree
          ? "Keep GDI operational"
          : missionTwo
            ? "Keep a GDI force operational"
            : "Keep a GDI ground force operational",
        description: missionThree
          ? "The operation fails if no counted GDI structure, infantry, or ground vehicle remains."
          : missionTwo
            ? "The operation fails if every counted GDI unit and structure is destroyed."
            : "The operation fails if every counted GDI ground force is destroyed.",
        progress: survivalProgress(stats, result),
        status,
      },
    ],
  };
}

function statusLabel(status: MissionObjectiveStatus): string {
  if (status === "complete") return "Complete";
  if (status === "failed") return "Failed";
  return "In progress";
}

export interface MissionObjectivesProps {
  mission: RuntimeMissionV1;
  stats?: SnapshotSidebar;
  result?: MissionObjectiveResult;
}

export function MissionObjectives({ mission, stats, result }: MissionObjectivesProps) {
  const presentation = missionObjectivePresentation(mission, stats, result);
  if (!presentation) return null;
  return <section className="mission-objectives" aria-labelledby="mission-objectives-title">
    <div className="mission-objectives-heading">
      <p className="eyebrow">Mission objectives</p>
      <span className={`mission-objectives-state ${presentation.status}`}>{statusLabel(presentation.status)}</span>
    </div>
    <h2 id="mission-objectives-title">{presentation.title}</h2>
    <ul>
      {presentation.items.map((item) => <li key={item.id} className={item.status}>
        <span className="mission-objective-marker" aria-hidden="true" />
        <div>
          <strong>{item.label}</strong>
          <p>{item.description}</p>
          <small>{item.progress}</small>
        </div>
      </li>)}
    </ul>
  </section>;
}
