import type { DecodedCommandBatch, SimulationEvent, StartConfiguration } from "./protocol";

export interface SimulationCore {
  readonly supportsSaves: boolean;
  start(configuration: StartConfiguration): void | Promise<void>;
  submitCommands(batch: DecodedCommandBatch): void;
  currentTick(): number;
  /** Returns false when the core is in a durable terminal state. */
  advance(): boolean;
  snapshotSize(): number;
  writeSnapshot(target: ArrayBuffer): number;
  save(): Uint8Array;
  load(data: Uint8Array): void;
  /** Deliberately absent from normal cores; used by the loopback release gate. */
  acceptanceForceVictory?(): void;
  drainEvents(): SimulationEvent[];
  destroy(): void;
}
