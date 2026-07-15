import { act } from "react";
import { createRoot, type Root } from "react-dom/client";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { SnapshotObjectType, type SnapshotSidebar, type SnapshotSidebarEntry } from "../simulation/snapshot";
import {
  ProductionPanel,
  type ProductionEntryPresentation,
  type ProductionPanelProps,
  type ProductionPrimaryAction,
} from "./ProductionPanel";

(globalThis as typeof globalThis & { IS_REACT_ACT_ENVIRONMENT: boolean }).IS_REACT_ACT_ENVIRONMENT = true;

function makeEntry(overrides: Partial<SnapshotSidebarEntry> = {}): SnapshotSidebarEntry {
  return {
    column: 0,
    columnIndex: 0,
    assetName: "PYLE",
    buildableType: 27,
    buildableId: 4,
    objectType: SnapshotObjectType.BuildingType,
    superweaponType: 0,
    cost: 300,
    powerDelta: -10,
    buildTime: 150,
    progress: 0,
    placementOffsets: [],
    completed: false,
    constructing: false,
    onHold: false,
    busy: false,
    viaCapture: false,
    fake: false,
    ...overrides,
  };
}

function makeSidebar(entries: readonly SnapshotSidebarEntry[], overrides: Partial<SnapshotSidebar> = {}): SnapshotSidebar {
  const leftColumn = entries.filter((entry) => entry.column === 0);
  const rightColumn = entries.filter((entry) => entry.column === 1);
  return {
    leftEntries: leftColumn.length,
    rightEntries: rightColumn.length,
    credits: 1_200,
    creditsCounter: 1_200,
    tiberium: 300,
    maxTiberium: 1_000,
    powerProduced: 100,
    powerDrained: 40,
    missionTimer: -1,
    unitsKilled: 0,
    buildingsKilled: 0,
    unitsLost: 0,
    buildingsLost: 0,
    harvestedCredits: 0,
    repairEnabled: true,
    sellEnabled: true,
    radarActive: false,
    entries,
    leftColumn,
    rightColumn,
    ...overrides,
  };
}

const entryKey = (entry: SnapshotSidebarEntry) => `${entry.buildableType}:${entry.buildableId}`;

function defaultPresentation(entry: SnapshotSidebarEntry): ProductionEntryPresentation {
  return {
    action: entry.busy ? undefined : "start",
    actionLabel: entry.busy ? "Busy" : "Build",
    disabled: entry.busy,
    status: entry.busy ? "Factory busy" : "Available",
  };
}

describe("ProductionPanel", () => {
  let container: HTMLDivElement;
  let root: Root;

  beforeEach(() => {
    container = document.createElement("div");
    document.body.append(container);
    root = createRoot(container);
  });

  afterEach(() => {
    act(() => root.unmount());
    container.remove();
  });

  function render(overrides: Partial<ProductionPanelProps> = {}, entries: readonly SnapshotSidebarEntry[] = [makeEntry()]): ProductionPanelProps {
    const props: ProductionPanelProps = {
      sidebar: makeSidebar(entries),
      entryKey,
      presentEntry: defaultPresentation,
      onPrimary: vi.fn(),
      onCancelProduction: vi.fn(),
      onRepair: vi.fn(),
      onSell: vi.fn(),
      onCancelTool: vi.fn(),
      ...overrides,
    };
    act(() => root.render(<ProductionPanel {...props} />));
    return props;
  }

  function button(text: string): HTMLButtonElement {
    const match = [...container.querySelectorAll("button")].find((candidate) => candidate.textContent?.trim() === text);
    if (!(match instanceof HTMLButtonElement)) throw new Error(`Button not found: ${text}`);
    return match;
  }

  it("renders category groups in their engine columns and reports spendable credits", () => {
    const entries = [
      makeEntry({ assetName: "POWER_PLANT", objectType: SnapshotObjectType.BuildingType }),
      makeEntry({ columnIndex: 1, assetName: "E1", buildableId: 1, objectType: SnapshotObjectType.InfantryType }),
      makeEntry({ column: 1, assetName: "JEEP", buildableId: 2, objectType: SnapshotObjectType.UnitType }),
      makeEntry({ column: 1, columnIndex: 1, assetName: "ORCA", buildableId: 3, objectType: SnapshotObjectType.AircraftType }),
      makeEntry({ column: 1, columnIndex: 2, assetName: "SW_ION", buildableId: 4, objectType: SnapshotObjectType.Special }),
    ] as const;
    render({ sidebar: makeSidebar(entries, { credits: 1_250, tiberium: 375 }) }, entries);

    const panel = container.querySelector('[aria-label="Construction and production"]');
    expect(panel?.textContent).toContain("1,625 credits");
    expect(panel?.textContent).toContain("5 available");
    const left = container.querySelector('[aria-label="Left production column"]');
    const right = container.querySelector('[aria-label="Right production column"]');
    expect(left?.textContent).toContain("Structures");
    expect(left?.textContent).toContain("POWER PLANT");
    expect(left?.textContent).toContain("Infantry");
    expect(left?.textContent).toContain("Minigunner");
    expect(right?.textContent).toContain("Vehicles");
    expect(right?.textContent).toContain("Humvee");
    expect(right?.textContent).toContain("Aircraft");
    expect(right?.textContent).toContain("Support");
    expect(right?.textContent).toContain("Ion Cannon");
  });

  it("renders normalized progress and forwards every primary action with its exact entry", () => {
    const actions: ProductionPrimaryAction[] = ["start", "hold", "resume", "place", "target"];
    const entries = actions.map((action, index) => makeEntry({
      column: index < 3 ? 0 : 1,
      columnIndex: index < 3 ? index : index - 3,
      assetName: action.toUpperCase(),
      buildableId: index,
      progress: index === 1 ? 0.374 : index === 2 ? 2 : 0,
      constructing: action === "hold",
      onHold: action === "resume",
      completed: action === "place" || action === "target",
      objectType: action === "target" ? SnapshotObjectType.Special : SnapshotObjectType.BuildingType,
    }));
    const presentations = new Map(entries.map((entry, index) => [entryKey(entry), {
      action: actions[index],
      actionLabel: actions[index].toUpperCase(),
      disabled: false,
      status: actions[index],
    } satisfies ProductionEntryPresentation]));
    const onPrimary = vi.fn();
    const onCancelProduction = vi.fn();
    const props = render({
      presentEntry: (entry) => presentations.get(entryKey(entry))!,
      onPrimary,
      onCancelProduction,
    }, entries);

    const progress = container.querySelector('[aria-label="HOLD production"]');
    expect(progress?.getAttribute("role")).toBe("progressbar");
    expect(progress?.getAttribute("aria-valuenow")).toBe("37");
    expect((progress?.firstElementChild as HTMLElement).style.width).toBe("37%");
    expect(container.querySelector('[aria-label="RESUME production"]')?.getAttribute("aria-valuenow")).toBe("100");
    const primaryFor = (entry: SnapshotSidebarEntry): HTMLButtonElement => {
      const primary = container.querySelector(`article[data-entry-key="${entryKey(entry)}"] button.production-primary`);
      if (!(primary instanceof HTMLButtonElement)) throw new Error(`Primary action not found for ${entry.assetName}`);
      return primary;
    };
    entries.forEach((entry) => act(() => primaryFor(entry).click()));
    expect(onPrimary.mock.calls).toEqual(actions.map((action, index) => [entries[index], action]));

    act(() => root.render(<ProductionPanel {...props} activeEntryKey={entryKey(entries[3])} />));
    expect(primaryFor(entries[3]).getAttribute("aria-pressed")).toBe("true");
    expect(primaryFor(entries[3]).disabled).toBe(true);

    const cancel = container.querySelector('[aria-label="Cancel HOLD production"]');
    expect(cancel).toBeInstanceOf(HTMLButtonElement);
    act(() => (cancel as HTMLButtonElement).click());
    expect(onCancelProduction).toHaveBeenCalledWith(entries[1]);
  });

  it("disables busy or globally unavailable production while retaining semantic button targets", () => {
    const available = makeEntry({ assetName: "E1", objectType: SnapshotObjectType.InfantryType });
    const busy = makeEntry({ columnIndex: 1, assetName: "JEEP", buildableId: 8, objectType: SnapshotObjectType.UnitType, busy: true });
    const props = render({
      unavailable: true,
      activeTool: "placement",
      sidebar: makeSidebar([available, busy], { repairEnabled: true, sellEnabled: false }),
    }, [available, busy]);

    expect(button("Build").disabled).toBe(true);
    expect(button("Busy").disabled).toBe(true);
    expect(button("Repair").disabled).toBe(true);
    expect(button("Sell").disabled).toBe(true);
    expect(button("Cancel tool").disabled).toBe(false);
    expect(button("Build").tagName).toBe("BUTTON");
    expect(button("Repair").tagName).toBe("BUTTON");
    expect(button("Cancel tool").tagName).toBe("BUTTON");
    expect(button("Build").classList.contains("production-primary")).toBe(true);
    expect(button("Repair").closest('[role="group"]')?.getAttribute("aria-label")).toBe("Structure tools");

    act(() => button("Cancel tool").click());
    expect(props.onCancelTool).toHaveBeenCalledOnce();
    expect(props.onPrimary).not.toHaveBeenCalled();
  });

  it("reflects repair/sell pressed state and forwards structure-tool callbacks", () => {
    const onRepair = vi.fn();
    const onSell = vi.fn();
    const onCancelTool = vi.fn();
    const props = render({ activeTool: "repair", onRepair, onSell, onCancelTool });

    expect(button("Repair").getAttribute("aria-pressed")).toBe("true");
    expect(button("Repair").classList.contains("active")).toBe(true);
    expect(button("Sell").getAttribute("aria-pressed")).toBe("false");
    act(() => {
      button("Repair").click();
      button("Sell").click();
      button("Cancel tool").click();
    });
    expect(onRepair).toHaveBeenCalledOnce();
    expect(onSell).toHaveBeenCalledOnce();
    expect(onCancelTool).toHaveBeenCalledOnce();

    act(() => root.render(<ProductionPanel {...props} activeTool="sell" />));
    expect(button("Repair").getAttribute("aria-pressed")).toBe("false");
    expect(button("Sell").getAttribute("aria-pressed")).toBe("true");
    expect(button("Sell").className).toContain("danger");
  });
});
