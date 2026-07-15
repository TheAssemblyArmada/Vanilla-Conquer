import { act } from "react";
import { createRoot, type Root } from "react-dom/client";
import { afterEach, describe, expect, it } from "vitest";
import type { RuntimeMissionV1 } from "../simulation/runtimeCatalog";
import type { SnapshotSidebar } from "../simulation/snapshot";
import { MissionObjectives, missionObjectivePresentation } from "./MissionObjectives";

const mission: RuntimeMissionV1 = {
  id: "gdi-01-east-a",
  scenarioRoot: "SCG01EA",
  scenario: 1,
  variation: 0,
  direction: 0,
  buildLevel: 1,
  sabotagedStructure: -1,
  faction: "gdi",
  title: "GDI Mission 1",
  briefing: "Briefing",
  theater: "temperate",
};

const missionTwo: RuntimeMissionV1 = {
  ...mission,
  id: "gdi-02-east-a",
  scenarioRoot: "SCG02EA",
  scenario: 2,
  buildLevel: 2,
  title: "GDI Mission 2 (East A)",
};

const missionThree: RuntimeMissionV1 = {
  ...mission,
  id: "gdi-03-east-a",
  scenarioRoot: "SCG03EA",
  scenario: 3,
  buildLevel: 3,
  title: "GDI Mission 3 (East A)",
};

const missionFourWestA: RuntimeMissionV1 = {
  ...mission,
  id: "gdi-04-west-a",
  scenarioRoot: "SCG04WA",
  scenario: 4,
  variation: 0,
  direction: 1,
  buildLevel: 4,
  title: "GDI Mission 4 (West A)",
};

const missionFourWestB: RuntimeMissionV1 = {
  ...missionFourWestA,
  id: "gdi-04-west-b",
  scenarioRoot: "SCG04WB",
  variation: 1,
  title: "GDI Mission 4 (West B)",
};

const missionFourEastA: RuntimeMissionV1 = {
  ...missionFourWestA,
  id: "gdi-04-east-a",
  scenarioRoot: "SCG04EA",
  direction: 0,
  title: "GDI Mission 4 (East A)",
};

const missionFiveEastA: RuntimeMissionV1 = {
  ...mission,
  id: "gdi-05-east-a",
  scenarioRoot: "SCG05EA",
  scenario: 5,
  variation: 0,
  direction: 0,
  buildLevel: 5,
  title: "GDI Mission 5 (East A)",
};

const missionFiveWestA: RuntimeMissionV1 = {
  ...missionFiveEastA,
  id: "gdi-05-west-a",
  scenarioRoot: "SCG05WA",
  direction: 1,
  title: "GDI Mission 5 (West A)",
};

const missionFiveWestB: RuntimeMissionV1 = {
  ...missionFiveWestA,
  id: "gdi-05-west-b",
  scenarioRoot: "SCG05WB",
  variation: 1,
  title: "GDI Mission 5 (West B)",
};

const stats = {
  unitsKilled: 3,
  buildingsKilled: 1,
  unitsLost: 2,
  buildingsLost: 0,
} as SnapshotSidebar;

let container: HTMLDivElement | undefined;
let root: Root | undefined;

afterEach(() => {
  if (root) act(() => root?.unmount());
  container?.remove();
  root = undefined;
  container = undefined;
});

function render(result?: { won: boolean }): HTMLElement {
  container = document.createElement("div");
  document.body.append(container);
  root = createRoot(container);
  act(() => root?.render(<MissionObjectives mission={mission} stats={stats} result={result} />));
  return container;
}

describe("Mission 1 objectives", () => {
  it("presents the exact active win and loss rules without claiming snapshot-derived completion", () => {
    const presentation = missionObjectivePresentation(mission, stats, undefined);
    expect(presentation).toMatchObject({ status: "active", title: "Operation orders" });
    expect(presentation?.items[0]).toMatchObject({
      label: "Eliminate the Nod force",
      progress: "3 units and 1 structure destroyed",
      status: "active",
    });
    expect(presentation?.items[1]).toMatchObject({
      label: "Keep a GDI ground force operational",
      progress: "2 losses recorded",
      status: "active",
    });
  });

  it("uses only the authoritative engine result for completion or failure", () => {
    expect(missionObjectivePresentation(mission, stats, { won: true })?.items).toEqual(expect.arrayContaining([
      expect.objectContaining({ status: "complete", progress: "Engine-confirmed objective complete" }),
    ]));
    expect(missionObjectivePresentation(mission, stats, { won: false })?.items).toEqual(expect.arrayContaining([
      expect.objectContaining({ status: "failed" }),
    ]));
  });

  it("presents the exact reviewed Mission 2 elimination and survival rules", () => {
    const presentation = missionObjectivePresentation(missionTwo, stats, undefined);
    expect(presentation?.items).toEqual([
      expect.objectContaining({ label: "Eliminate the Nod occupation", progress: "3 units and 1 structure destroyed", status: "active" }),
      expect.objectContaining({ label: "Keep a GDI force operational", progress: "2 losses recorded", status: "active" }),
    ]);
  });

  it("presents the exact reviewed Mission 3 elimination and survival rules", () => {
    const presentation = missionObjectivePresentation(missionThree, stats, undefined);
    expect(presentation?.items).toEqual([
      expect.objectContaining({
        label: "Eliminate the Nod force",
        description: "Destroy every counted Nod unit and structure in the operation area. Nod production, rebuilt structures, and attack teams can add targets.",
        progress: "3 units and 1 structure destroyed",
        status: "active",
      }),
      expect.objectContaining({
        label: "Keep GDI operational",
        description: "The operation fails if no counted GDI structure, infantry, or ground vehicle remains.",
        progress: "2 losses recorded",
        status: "active",
      }),
    ]);
  });

  it("keeps Mission 3 terminal status engine-authoritative", () => {
    expect(missionObjectivePresentation(missionThree, stats, { won: true })?.items).toEqual(expect.arrayContaining([
      expect.objectContaining({ status: "complete", progress: "Engine-confirmed objective complete" }),
    ]));
    expect(missionObjectivePresentation(missionThree, stats, { won: false })?.items).toEqual(expect.arrayContaining([
      expect.objectContaining({ status: "failed", progress: "All counted GDI ground forces were lost" }),
    ]));
  });

  it.each([
    ["West A", missionFourWestA],
    ["East A", missionFourEastA],
  ])("presents the exact reviewed Mission 4 %s crate-recovery rules", (_name, reviewedMission) => {
    expect(missionObjectivePresentation(reviewedMission, stats, undefined)?.items).toEqual([
      {
        id: "recover-crate",
        label: "Recover the GDI crate",
        description: "Reach the marked recovery area. The operation completes when a GDI unit enters the crate cell; destroying Nod is not required.",
        progress: "Crate recovery objective active",
        status: "active",
      },
      {
        id: "preserve-gdi",
        label: "Keep the recovery force operational",
        description: "The operation fails if every counted GDI infantry unit and ground vehicle is destroyed. A transport aircraft alone does not prevent defeat.",
        progress: "Recovery force condition active",
        status: "active",
      },
    ]);
  });

  it("presents West B's exact elimination, village, and GDI survival rules", () => {
    const unrelatedGdiLosses = { ...stats, unitsLost: 7, buildingsLost: 4 } as SnapshotSidebar;
    const presentation = missionObjectivePresentation(missionFourWestB, unrelatedGdiLosses, undefined);
    expect(presentation?.items).toEqual([
      {
        id: "eliminate-nod",
        label: "Eliminate the Nod force",
        description: "Destroy every counted Nod unit in the operation area. Triggered Nod assault groups become additional targets.",
        progress: "Nod elimination objective active",
        status: "active",
      },
      {
        id: "preserve-village",
        label: "Preserve the protected village",
        description: "The operation fails if all four protected village structures are destroyed.",
        progress: "Village protection condition active",
        status: "active",
      },
      {
        id: "preserve-gdi",
        label: "Keep GDI operational",
        description: "The operation fails if every counted GDI infantry unit and ground vehicle is destroyed.",
        progress: "GDI survival condition active",
        status: "active",
      },
    ]);
    expect(presentation?.items[1]?.progress).not.toContain("loss");
  });

  it("keeps every Mission 4 terminal presentation engine-authoritative and cause-neutral", () => {
    for (const reviewedMission of [missionFourWestA, missionFourWestB, missionFourEastA]) {
      const won = missionObjectivePresentation(reviewedMission, stats, { won: true });
      expect(won?.status).toBe("complete");
      expect(won?.items.every(({ status }) => status === "complete")).toBe(true);

      const lost = missionObjectivePresentation(reviewedMission, stats, { won: false });
      expect(lost?.status).toBe("failed");
      expect(lost?.items.every(({ status, progress }) => status === "failed" && progress === "Engine-confirmed operation failed"))
        .toBe(true);
    }
  });

  it.each([
    ["East A", missionFiveEastA],
    ["West A", missionFiveWestA],
    ["West B", missionFiveWestB],
  ])("presents the exact reviewed Mission 5 %s siege rules", (_name, reviewedMission) => {
    expect(missionObjectivePresentation(reviewedMission, stats, undefined)?.items).toEqual([
      {
        id: "eliminate-nod",
        label: "Eliminate the Nod force",
        description: "Destroy every counted Nod unit and structure in the operation area. Nod production, rebuilt structures, patrols, and timed attack teams can add targets.",
        progress: "3 units and 1 structure destroyed",
        status: "active",
      },
      {
        id: "relieve-base",
        label: "Relieve the separated GDI base",
        description: "Move GDI units through both authored relief zones. Until each zone is crossed, losing the last member of its protected starting group—field force or base structures—immediately fails the operation.",
        progress: "Base-relief conditions active",
        status: "active",
      },
      {
        id: "preserve-gdi",
        label: "Keep GDI operational",
        description: "The operation also fails if every counted GDI unit and structure is destroyed.",
        progress: "GDI survival condition active",
        status: "active",
      },
    ]);
  });

  it("keeps Mission 5 completion and failure engine-authoritative", () => {
    for (const reviewedMission of [missionFiveEastA, missionFiveWestA, missionFiveWestB]) {
      const won = missionObjectivePresentation(reviewedMission, stats, { won: true });
      expect(won?.items.every(({ status }) => status === "complete")).toBe(true);
      expect(won?.items[1]?.progress).toBe("Engine-confirmed relief conditions satisfied");

      const lost = missionObjectivePresentation(reviewedMission, stats, { won: false });
      expect(lost?.items.every(({ status }) => status === "failed")).toBe(true);
      expect(lost?.items.slice(1).every(({ progress }) => progress === "Engine-confirmed operation failed")).toBe(true);
    }
  });

  it("fails closed for missions and variants without a reviewed rule set", () => {
    expect(missionObjectivePresentation({ ...missionTwo, variation: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionTwo, direction: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionThree, variation: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionThree, direction: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourWestA, direction: 0 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourWestA, variation: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourWestB, direction: 0 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourWestB, variation: 0 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourEastA, direction: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourEastA, variation: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourEastA, faction: "nod" }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourEastA, scenario: 5 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourEastA, id: "forged-mission" }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourEastA, buildLevel: 5 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFourEastA, scenarioRoot: "SCG05EA", scenario: 5, buildLevel: 5 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFiveEastA, direction: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFiveWestA, variation: 1 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFiveWestB, variation: 0 }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFiveWestB, faction: "nod" }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFiveWestB, id: "forged-mission" }, stats, undefined)).toBeUndefined();
    expect(missionObjectivePresentation({ ...missionFiveWestB, buildLevel: 4 }, stats, undefined)).toBeUndefined();
  });

  it("renders a semantic objective list and visible state", () => {
    const element = render();
    expect(element.querySelector("section")?.getAttribute("aria-labelledby")).toBe("mission-objectives-title");
    expect(element.querySelectorAll("li")).toHaveLength(2);
    expect(element.textContent).toContain("In progress");
    expect(element.textContent).toContain("Reinforcements may enter the battlefield");
  });
});
