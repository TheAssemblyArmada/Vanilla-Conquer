import { SnapshotObjectType, type SnapshotSidebar, type SnapshotSidebarEntry } from "../simulation/snapshot";
import { productionEntryLabel } from "./productionModel";
import "./production.css";

export type ProductionEntryView = SnapshotSidebarEntry;
export type ProductionSidebarView = SnapshotSidebar;

export type ProductionPrimaryAction = "start" | "hold" | "resume" | "place" | "target";

export interface ProductionEntryPresentation {
  action?: ProductionPrimaryAction;
  actionLabel: string;
  disabled: boolean;
  status: string;
}

export interface ProductionPanelProps {
  sidebar: ProductionSidebarView;
  unavailable?: boolean;
  activeTool?: "placement" | "repair" | "sell" | "superweapon";
  activeEntryKey?: string;
  entryKey: (entry: ProductionEntryView) => string;
  presentEntry: (entry: ProductionEntryView) => ProductionEntryPresentation;
  onPrimary: (entry: ProductionEntryView, action: ProductionPrimaryAction) => void;
  onCancelProduction: (entry: ProductionEntryView) => void;
  onRepair: () => void;
  onSell: () => void;
  onCancelTool: () => void;
}

function displayName(entry: Pick<SnapshotSidebarEntry, "assetName" | "buildableId" | "objectType">): string {
  return productionEntryLabel(entry);
}

function category(objectType: number): string {
  switch (objectType) {
    case SnapshotObjectType.BuildingType: return "Structures";
    case SnapshotObjectType.InfantryType: return "Infantry";
    case SnapshotObjectType.UnitType: return "Vehicles";
    case SnapshotObjectType.AircraftType: return "Aircraft";
    case SnapshotObjectType.Special: return "Support";
    default: return "Production";
  }
}

function EntryCard({
  entry,
  entryKey,
  presentation,
  unavailable,
  active,
  onPrimary,
  onCancelProduction,
}: {
  entry: ProductionEntryView;
  entryKey: string;
  presentation: ProductionEntryPresentation;
  unavailable: boolean;
  active: boolean;
  onPrimary: ProductionPanelProps["onPrimary"];
  onCancelProduction: ProductionPanelProps["onCancelProduction"];
}) {
  const progress = entry.completed ? 100 : Math.round(Math.max(0, Math.min(1, entry.progress)) * 100);
  const canCancel = entry.constructing || entry.onHold || entry.completed;
  return <article className={`production-entry${active ? " active" : ""}`} data-entry-key={entryKey}>
    <div className="production-entry-heading"><strong>{displayName(entry)}</strong><span>{Math.max(0, entry.cost).toLocaleString()}</span></div>
    <div className="production-progress" role="progressbar" aria-label={`${displayName(entry)} production`} aria-valuemin={0} aria-valuemax={100} aria-valuenow={progress}>
      <span style={{ width: `${progress}%` }} />
    </div>
    <div className="production-entry-meta"><span>{presentation.status}</span>{entry.powerDelta !== 0 && <span className={entry.powerDelta < 0 ? "power-negative" : ""}>{entry.powerDelta > 0 ? "+" : ""}{entry.powerDelta} power</span>}</div>
    <div className="production-entry-actions">
      <button
        className="production-primary"
        aria-label={`${presentation.actionLabel} ${displayName(entry)}`}
        disabled={unavailable || active || presentation.disabled || !presentation.action}
        aria-pressed={active || undefined}
        onClick={() => presentation.action && onPrimary(entry, presentation.action)}
      >{presentation.actionLabel}</button>
      {canCancel && <button className="production-cancel" disabled={unavailable} aria-label={`Cancel ${displayName(entry)} production`} onClick={() => onCancelProduction(entry)}>×</button>}
    </div>
  </article>;
}

export function ProductionPanel(props: ProductionPanelProps) {
  const availableMoney = props.sidebar.credits + props.sidebar.tiberium;
  const columns = ([0, 1] as const).map((column) => props.sidebar.entries.filter((entry) => entry.column === column));
  return <section className="production-panel" aria-label="Construction and production">
    <div className="production-heading"><div><p className="eyebrow">Command console</p><strong>{availableMoney.toLocaleString()} credits</strong></div><span>{props.sidebar.entries.length} available</span></div>
    <div className="structure-tools" role="group" aria-label="Structure tools">
      <button aria-pressed={props.activeTool === "repair"} className={props.activeTool === "repair" ? "active" : ""} disabled={props.unavailable || !props.sidebar.repairEnabled} onClick={props.onRepair}>Repair</button>
      <button aria-pressed={props.activeTool === "sell"} className={props.activeTool === "sell" ? "active danger" : ""} disabled={props.unavailable || !props.sidebar.sellEnabled} onClick={props.onSell}>Sell</button>
      {props.activeTool && <button className="cancel-tool" onClick={props.onCancelTool}>Cancel tool</button>}
    </div>
    <div className="production-columns">
      {columns.map((entries, column) => <div className="production-column" key={column} aria-label={`${column === 0 ? "Left" : "Right"} production column`}>
        {entries.map((entry, index) => {
          const key = props.entryKey(entry);
          const prior = entries[index - 1];
          const showCategory = !prior || category(prior.objectType) !== category(entry.objectType);
          return <div className="production-entry-group" key={key}>
            {showCategory && <h3>{category(entry.objectType)}</h3>}
            <EntryCard entry={entry} entryKey={key} presentation={props.presentEntry(entry)} unavailable={Boolean(props.unavailable)} active={props.activeEntryKey === key} onPrimary={props.onPrimary} onCancelProduction={props.onCancelProduction} />
          </div>;
        })}
      </div>)}
    </div>
  </section>;
}
