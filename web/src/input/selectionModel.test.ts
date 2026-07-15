import { describe, expect, it } from "vitest";
import { SnapshotContextualAction } from "../simulation/snapshot";
import { battlefieldSelectionPresentation, type SelectionObjectLike } from "./selectionModel";

function object(overrides: Partial<SelectionObjectLike> = {}): SelectionObjectLike {
  return {
    id: 1,
    assetName: "E1",
    typeName: "Infantry",
    type: 1,
    x: 10,
    y: 20,
    width: 8,
    height: 10,
    drawFlags: 0,
    maxStrength: 100,
    strength: 75,
    owner: 2,
    selectedMask: 1 << 2,
    actionWithSelected: Array(32).fill(SnapshotContextualAction.None),
    controlGroup: 0,
    root: true,
    ...overrides,
  };
}

describe("battlefield selection presentation", () => {
  it("uses only the active player's root records for selection and groups", () => {
    const presentation = battlefieldSelectionPresentation([
      object(),
      object({ id: 2, root: false }),
      object({ id: 3, owner: 1, selectedMask: 1 << 2 }),
      object({ id: 4, selectedMask: 0, controlGroup: 1 }),
    ], 2);
    expect(presentation.count).toBe(1);
    expect(presentation.assignableCount).toBe(1);
    expect(presentation.label).toBe("Minigunner selected · 75% health");
    expect(presentation.groups[0]).toMatchObject({ key: "1", count: 1, active: true, center: { x: 14, y: 25 } });
    expect(presentation.groups[1]).toMatchObject({ key: "2", count: 1, active: false });
    expect(presentation.groups[9].key).toBe("0");
  });

  it("summarizes mixed selections without exposing more than three type names", () => {
    const presentation = battlefieldSelectionPresentation([
      object({ id: 1, assetName: "E1", controlGroup: undefined }),
      object({ id: 2, assetName: "E1", controlGroup: undefined }),
      object({ id: 3, assetName: "JEEP", controlGroup: undefined }),
      object({ id: 4, assetName: "MTNK", controlGroup: undefined }),
      object({ id: 5, assetName: "ORCA", controlGroup: undefined }),
    ], 2);
    expect(presentation.label).toBe("5 objects selected · 2 Minigunner, 1 Humvee, 1 Medium Tank, plus 1 more type");
    expect(presentation.groups.every((group) => group.count === 0 && !group.active)).toBe(true);
  });

  it("returns a safe empty presentation when player identity is absent", () => {
    const presentation = battlefieldSelectionPresentation([object()], undefined);
    expect(presentation.count).toBe(0);
    expect(presentation.assignableCount).toBe(0);
    expect(presentation.label).toBe("No friendly objects selected");
  });

  it("does not advertise selected structures as control-group assignable", () => {
    const presentation = battlefieldSelectionPresentation([object({ type: 4, assetName: "PYLE" })], 2);
    expect(presentation).toMatchObject({ count: 1, assignableCount: 0, label: "PYLE selected · 75% health" });
  });

  it("surfaces the engine-authored self action as deployable or blocked", () => {
    const actions = Array(32).fill(SnapshotContextualAction.None);
    actions[2] = SnapshotContextualAction.Self;
    expect(battlefieldSelectionPresentation([
      object({ assetName: "MCV", actionWithSelected: actions }),
    ], 2)).toMatchObject({
      label: "Mobile Construction Vehicle selected · 75% health · Deploy available",
      deployment: { available: true, target: { x: 14, y: 25 } },
    });

    actions[2] = SnapshotContextualAction.CannotDeploy;
    expect(battlefieldSelectionPresentation([
      object({ assetName: "MCV", actionWithSelected: actions }),
    ], 2)).toMatchObject({
      label: "Mobile Construction Vehicle selected · 75% health · Deployment blocked at current location",
      deployment: { available: false },
    });
  });
});
