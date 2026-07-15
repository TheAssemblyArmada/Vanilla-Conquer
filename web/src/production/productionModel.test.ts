import { describe, expect, it } from "vitest";
import {
  ProductionObjectType,
  decodePlacementOffset,
  describeProductionEntry,
  evaluatePlacementCandidate,
  expandPlacementGrid,
  firstLegalPlacementCell,
  initialHostToolState,
  leavesPlacementTool,
  normalizeProductionProgress,
  productionEntryCategory,
  productionEntryKey,
  productionEntryLabel,
  reduceHostTool,
  type ProductionEntryLike,
} from "./productionModel";

function entry(overrides: Partial<ProductionEntryLike> = {}): ProductionEntryLike {
  return {
    assetName: "PYLE",
    buildableType: 27,
    buildableId: 4,
    objectType: ProductionObjectType.Building,
    progress: 0,
    placementOffsets: [0, 1, 128, 129],
    completed: false,
    constructing: false,
    onHold: false,
    busy: false,
    ...overrides,
  };
}

describe("production entry model", () => {
  it("derives stable identities, wire categories, and content-safe labels", () => {
    expect(productionEntryKey(entry())).toBe("production:27:4");
    expect(productionEntryCategory(entry({ objectType: ProductionObjectType.Infantry }))).toBe("infantry");
    expect(productionEntryCategory(entry({ objectType: ProductionObjectType.Unit }))).toBe("vehicle");
    expect(productionEntryCategory(entry({ objectType: ProductionObjectType.Vessel }))).toBe("vehicle");
    expect(productionEntryCategory(entry({ objectType: ProductionObjectType.Aircraft }))).toBe("aircraft");
    expect(productionEntryCategory(entry({ objectType: ProductionObjectType.Building }))).toBe("structure");
    expect(productionEntryCategory(entry({ objectType: ProductionObjectType.Special }))).toBe("special");
    expect(productionEntryCategory(entry({ objectType: -1 }))).toBe("unknown");
    expect(productionEntryLabel(entry())).toBe("Barracks");
    expect(productionEntryLabel(entry({ assetName: "E1" }))).toBe("Minigunner");
    expect(productionEntryLabel(entry({ assetName: "JEEP" }))).toBe("Humvee");
    expect(productionEntryLabel(entry({ assetName: "NUKE" }))).toBe("Power Plant");
    expect(productionEntryLabel(entry({ assetName: "POWER_PLANT\0ignored" }))).toBe("POWER PLANT");
    expect(productionEntryLabel(entry({ assetName: "SW_AirStrike", objectType: ProductionObjectType.Special }))).toBe("Air Strike");
    expect(productionEntryLabel(entry({ assetName: "", objectType: ProductionObjectType.Building, buildableId: 9 }))).toBe("Structure 9");
    expect(() => productionEntryKey(entry({ buildableId: 1.5 }))).toThrow("buildableId");
  });

  it("normalizes progress and maps coherent engine state to one primary action", () => {
    expect(normalizeProductionProgress(Number.NaN)).toBe(0);
    expect(normalizeProductionProgress(-0.2)).toBe(0);
    expect(normalizeProductionProgress(1.2)).toBe(1);
    expect(normalizeProductionProgress(0.2, true)).toBe(1);

    expect(describeProductionEntry(entry())).toMatchObject({ status: "available", progress: 0, primaryAction: "start" });
    expect(describeProductionEntry(entry({ constructing: true, progress: 0.42 }))).toMatchObject({ status: "constructing", progress: 0.42, primaryAction: "hold" });
    expect(describeProductionEntry(entry({ constructing: true, onHold: true, progress: 4 }))).toMatchObject({ status: "on-hold", progress: 1, primaryAction: "resume" });
    expect(describeProductionEntry(entry({ completed: true, progress: 0.1 }))).toMatchObject({ status: "ready", progress: 1, primaryAction: "place" });
    expect(describeProductionEntry(entry({ completed: true, objectType: ProductionObjectType.Unit }))).toMatchObject({ status: "ready", primaryAction: "disabled" });
    expect(describeProductionEntry(entry({ busy: true, progress: 0.7 }))).toMatchObject({ status: "busy", progress: 0, primaryAction: "disabled" });
    expect(describeProductionEntry(entry({ objectType: ProductionObjectType.Special, constructing: true, progress: 0.5 }))).toMatchObject({ status: "constructing", progress: 0.5, primaryAction: "disabled" });
    expect(describeProductionEntry(entry({ objectType: 999 }))).toMatchObject({ status: "unavailable", primaryAction: "disabled" });
  });
});

describe("placement geometry", () => {
  it("finds the first currently legal quick-place cell in row-major order", () => {
    const legal = new Set(["2:1", "0:2"]);
    expect(firstLegalPlacementCell({
      width: 3,
      height: 3,
      canPlace: (_entry, x, y) => legal.has(`${x}:${y}`),
    }, entry({ completed: true }))).toEqual({ x: 2, y: 1 });
    expect(firstLegalPlacementCell({
      width: 3,
      height: 3,
      canPlace: (_entry, x, y) => legal.has(`${x}:${y}`),
    }, entry({ completed: true }), 6)).toEqual({ x: 0, y: 2 });
    expect(firstLegalPlacementCell({
      width: 3,
      height: 3,
      canPlace: (_entry, x, y) => legal.has(`${x}:${y}`),
    }, entry({ completed: true }), 7)).toEqual({ x: 2, y: 1 });
    expect(firstLegalPlacementCell({
      width: 2,
      height: 2,
      canPlace: () => false,
    }, entry({ completed: true }))).toBeUndefined();
  });

  it("mirrors TD's expanded row-major placement grid", () => {
    expect(expandPlacementGrid({ x: 10, y: 20, width: 30, height: 40 })).toEqual({
      x: 9, y: 19, width: 32, height: 42, stride: 128, count: 1344,
    });
    expect(expandPlacementGrid({ x: 0, y: 0, width: 127, height: 127 })).toMatchObject({ x: 0, y: 0, width: 128, height: 128, count: 16384 });
    expect(expandPlacementGrid({ x: 1, y: 1, width: 127, height: 127 })).toMatchObject({ x: 0, y: 0, width: 128, height: 128, count: 16384 });
    expect(() => expandPlacementGrid({ x: 120, y: 0, width: 9, height: 1 })).toThrow("outside");
    expect(() => expandPlacementGrid({ x: 0, y: 0, width: 0, height: 1 })).toThrow("outside");
  });

  it("decodes signed 128-stride footprint offsets without row-wrap ambiguity", () => {
    expect(decodePlacementOffset(0)).toEqual({ x: 0, y: 0 });
    expect(decodePlacementOffset(1)).toEqual({ x: 1, y: 0 });
    expect(decodePlacementOffset(128)).toEqual({ x: 0, y: 1 });
    expect(decodePlacementOffset(129)).toEqual({ x: 1, y: 1 });
    expect(decodePlacementOffset(-1)).toEqual({ x: -1, y: 0 });
    expect(decodePlacementOffset(-127)).toEqual({ x: 1, y: -1 });
    expect(decodePlacementOffset(127)).toEqual({ x: -1, y: 1 });
    expect(() => decodePlacementOffset(0x8000)).toThrow("16-bit");
  });

  it("requires anchor proximity and every unique footprint cell to be clear", () => {
    const grid = expandPlacementGrid({ x: 1, y: 1, width: 3, height: 3 });
    const flags = new Uint8Array(grid.count).fill(3);
    const ready = entry({ completed: true });
    const candidate = evaluatePlacementCandidate(grid, flags, ready, { x: 1, y: 1 });
    expect(candidate).toMatchObject({ legal: true, proximity: true, commandCell: { x: 1, y: 1 } });
    expect(candidate.footprint.map(({ x, y }) => [x, y])).toEqual([[1, 1], [2, 1], [1, 2], [2, 2]]);

    const noProximity = flags.slice();
    noProximity[6] = 2;
    expect(evaluatePlacementCandidate(grid, noProximity, ready, { x: 1, y: 1 }).rejection).toBe("no-proximity");

    const obstructed = flags.slice();
    obstructed[12] = 1;
    const blocked = evaluatePlacementCandidate(grid, obstructed, ready, { x: 1, y: 1 });
    expect(blocked.rejection).toBe("obstructed");
    expect(blocked.footprint.find((cell) => cell.index === 12)?.clear).toBe(false);

    expect(evaluatePlacementCandidate(grid, flags, ready, { x: 4, y: 4 }).rejection).toBe("footprint-outside");
    expect(evaluatePlacementCandidate(grid, flags, ready, { x: 8, y: 8 }).rejection).toBe("outside-grid");
    expect(evaluatePlacementCandidate(grid, flags, entry({ completed: true, placementOffsets: [] }), { x: 1, y: 1 }).rejection).toBe("missing-footprint");
    expect(evaluatePlacementCandidate(grid, flags, entry({ completed: true, objectType: ProductionObjectType.Unit }), { x: 1, y: 1 }).rejection).toBe("not-placeable");
    expect(evaluatePlacementCandidate(grid, flags, entry({ completed: true, placementOffsets: [0, 0] }), { x: 1, y: 1 }).footprint).toHaveLength(1);
    expect(() => evaluatePlacementCandidate(grid, flags.subarray(1), ready, { x: 1, y: 1 })).toThrow("flags");
  });
});

describe("host production tools", () => {
  it("toggles ordinary tools and reconciles disabled repair or sell modes", () => {
    const initial = initialHostToolState();
    const order = reduceHostTool(initial, { type: "activate", mode: "order" });
    expect(order).toEqual({ mode: "order" });
    expect(reduceHostTool(order, { type: "activate", mode: "order" })).toEqual({ mode: "select" });

    const repair = reduceHostTool(initial, { type: "activate", mode: "repair" });
    expect(reduceHostTool(repair, { type: "reconcile", entries: [], placementActive: false, repairEnabled: false, sellEnabled: true })).toEqual({ mode: "select" });
    const sell = reduceHostTool(initial, { type: "activate", mode: "sell" });
    expect(reduceHostTool(sell, { type: "reconcile", entries: [], placementActive: false, repairEnabled: true, sellEnabled: true })).toBe(sell);
  });

  it("keeps a placement request pending and reconciles its engine lifecycle", () => {
    const building = entry({ completed: true });
    const requesting = reduceHostTool(initialHostToolState(), { type: "begin-placement", entry: building });
    expect(requesting).toMatchObject({
      mode: "placement", entryKey: "production:27:4", buildableType: 27, buildableId: 4, phase: "requesting",
    });
    const stillRequesting = reduceHostTool(requesting, {
      type: "reconcile", entries: [building], placementActive: false, repairEnabled: true, sellEnabled: true,
    });
    expect(stillRequesting).toEqual(requesting);
    const active = reduceHostTool(stillRequesting, {
      type: "reconcile", entries: [building], placementActive: true, repairEnabled: true, sellEnabled: true,
    });
    expect(active).toMatchObject({ mode: "placement", phase: "active" });
    expect(leavesPlacementTool(requesting, active)).toBe(false);
    const finished = reduceHostTool(active, {
      type: "reconcile", entries: [building], placementActive: false, repairEnabled: true, sellEnabled: true,
    });
    expect(finished).toEqual({ mode: "select" });
    expect(leavesPlacementTool(active, finished)).toBe(true);
  });

  it("cancels stale placement and ignores non-placeable entries", () => {
    const unit = entry({ completed: true, objectType: ProductionObjectType.Unit });
    expect(reduceHostTool({ mode: "order" }, { type: "begin-placement", entry: unit })).toEqual({ mode: "order" });
    const building = entry({ completed: true });
    const placement = reduceHostTool(initialHostToolState(), { type: "begin-placement", entry: building });
    expect(reduceHostTool(placement, { type: "cancel" })).toEqual({ mode: "select" });
    expect(reduceHostTool(placement, {
      type: "reconcile", entries: [{ ...building, completed: false }], placementActive: true, repairEnabled: true, sellEnabled: true,
    })).toEqual({ mode: "select" });
    expect(reduceHostTool(placement, {
      type: "reconcile", entries: [], placementActive: true, repairEnabled: true, sellEnabled: true,
    })).toEqual({ mode: "select" });
  });
});
