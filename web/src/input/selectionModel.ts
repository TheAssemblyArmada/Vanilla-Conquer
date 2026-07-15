import type { ScreenPoint } from "./TouchController";
import { SnapshotContextualAction, SnapshotObjectType } from "../simulation/snapshot";

export interface SelectionObjectLike {
  id: number;
  assetName: string;
  typeName: string;
  type: number;
  x: number;
  y: number;
  width: number;
  height: number;
  drawFlags: number;
  maxStrength: number;
  strength: number;
  owner: number;
  selectedMask: number;
  actionWithSelected: readonly SnapshotContextualAction[];
  controlGroup: number | undefined;
  root: boolean;
}

export interface ControlGroupPresentation {
  /** Zero-based engine group index. */
  index: number;
  /** Physical keyboard label: groups 1-9 use 1-9 and group 10 uses 0. */
  key: string;
  count: number;
  active: boolean;
  center: ScreenPoint | undefined;
}

export interface BattlefieldSelectionPresentation {
  count: number;
  assignableCount: number;
  label: string;
  deployment?: { available: boolean; target: ScreenPoint };
  groups: readonly ControlGroupPresentation[];
}

function objectLabel(object: SelectionObjectLike): string {
  const raw = object.assetName.trim() || object.typeName.trim() || "Object";
  const normalized = raw.replace(/[_-]+/g, " ").replace(/\s+/g, " ");
  const key = normalized.toUpperCase().replace(/\s+/g, "");
  const labels: Readonly<Record<string, string>> = {
    E1: "Minigunner",
    JEEP: "Humvee",
    MTNK: "Medium Tank",
    ORCA: "Orca",
    MCV: "Mobile Construction Vehicle",
    FACT: "Construction Yard",
    FACTMAKE: "Construction Yard",
  };
  return labels[key] ?? normalized;
}

function objectCenter(object: SelectionObjectLike): ScreenPoint {
  if (object.drawFlags & 0x20) {
    return { x: object.x, y: object.y };
  }
  if (object.drawFlags & 0x40) {
    return { x: object.x, y: object.y - object.height / 2 };
  }
  return { x: object.x + object.width / 2, y: object.y + object.height / 2 };
}

function selectionLabel(selected: readonly SelectionObjectLike[]): string {
  if (selected.length === 0) return "No friendly objects selected";
  if (selected.length === 1) {
    const object = selected[0];
    const health = object.maxStrength > 0
      ? ` · ${Math.round(Math.max(0, Math.min(1, object.strength / object.maxStrength)) * 100)}% health`
      : "";
    return `${objectLabel(object)} selected${health}`;
  }
  const counts = new Map<string, number>();
  for (const object of selected) {
    const label = objectLabel(object);
    counts.set(label, (counts.get(label) ?? 0) + 1);
  }
  const composition = [...counts]
    .sort((left, right) => right[1] - left[1] || left[0].localeCompare(right[0]))
    .slice(0, 3)
    .map(([label, count]) => `${count} ${label}`)
    .join(", ");
  const remainder = counts.size > 3 ? `, plus ${counts.size - 3} more ${counts.size - 3 === 1 ? "type" : "types"}` : "";
  return `${selected.length} objects selected · ${composition}${remainder}`;
}

/**
 * Derives stable, DOM-friendly selection and group data from one engine snapshot.
 * Child render records and enemy selection bits are deliberately excluded.
 */
export function battlefieldSelectionPresentation(
  objects: readonly SelectionObjectLike[],
  playerHouse: number | undefined,
): BattlefieldSelectionPresentation {
  const friendly = playerHouse === undefined || !Number.isInteger(playerHouse) || playerHouse < 0 || playerHouse > 31
    ? []
    : objects.filter((object) => object.root && object.owner === playerHouse);
  const playerMask = playerHouse === undefined ? 0 : (1 << playerHouse);
  const selected = friendly.filter((object) => (object.selectedMask & playerMask) !== 0);
  const assignableCount = selected.filter((object) => object.type === SnapshotObjectType.Infantry
    || object.type === SnapshotObjectType.Unit || object.type === SnapshotObjectType.Aircraft).length;
  const deploymentAction = selected.length === 1 && playerHouse !== undefined
    ? selected[0].actionWithSelected[playerHouse]
    : SnapshotContextualAction.None;
  const deployment = deploymentAction === SnapshotContextualAction.Self
    || deploymentAction === SnapshotContextualAction.CannotDeploy
    ? { available: deploymentAction === SnapshotContextualAction.Self, target: objectCenter(selected[0]) }
    : undefined;
  const memberLists = Array.from({ length: 10 }, () => [] as SelectionObjectLike[]);
  for (const object of friendly) {
    if (object.controlGroup !== undefined && object.controlGroup >= 0 && object.controlGroup < memberLists.length) {
      memberLists[object.controlGroup].push(object);
    }
  }
  const selectedObjects = new Set(selected);
  const groups = memberLists.map((members, index): ControlGroupPresentation => {
    const centers = members.map(objectCenter);
    const center = centers.length > 0 ? {
      x: centers.reduce((sum, point) => sum + point.x, 0) / centers.length,
      y: centers.reduce((sum, point) => sum + point.y, 0) / centers.length,
    } : undefined;
    return {
      index,
      key: index === 9 ? "0" : String(index + 1),
      count: members.length,
      active: members.length > 0 && selected.length === members.length && members.every((member) => selectedObjects.has(member)),
      center,
    };
  });
  const deploymentStatus = deployment
    ? deployment.available ? " · Deploy available" : " · Deployment blocked at current location"
    : "";
  return {
    count: selected.length,
    assignableCount,
    label: `${selectionLabel(selected)}${deploymentStatus}`,
    deployment,
    groups,
  };
}
