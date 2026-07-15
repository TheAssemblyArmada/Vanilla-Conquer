import type { SimulationCore } from "./core";
import { mountPreparedContent, type EmscriptenModuleWithFs, type PreparedContentMount } from "./contentMount";
import type { DecodedCommandBatch, SimulationEvent, StartConfiguration } from "./protocol";
import { CNC_WEB_ABI_VERSION, CNC_WEB_MAGIC_MESSAGE, encodeCommandBatch, encodeStartConfiguration, MessageKind, SIMULATION_PROTOCOL_VERSION } from "./protocol";
import { WasmScratchBuffers } from "./WasmScratchBuffers";

interface CncWasmExports {
  memoryBuffer(): ArrayBuffer;
  malloc(size: number): number;
  free(pointer: number): void;
  cnc_web_abi_version(): number;
  cnc_web_create(version: number, outHandle: number): number;
  cnc_web_destroy(handle: number): number;
  cnc_web_set_campaign_transition?(handle: number, carryOverCredits: number, nukePieces: number): number;
  cnc_web_start(handle: number, data: number, length: number): number;
  cnc_web_submit_commands(handle: number, data: number, length: number): number;
  cnc_web_advance(handle: number, tickCount: number, outAdvanced: number): number;
  cnc_web_snapshot_size(handle: number, outSize: number): number;
  cnc_web_write_snapshot(handle: number, data: number, capacity: number, outWritten: number): number;
  cnc_web_save_size?(handle: number, outSize: number): number;
  cnc_web_write_save?(handle: number, data: number, capacity: number, outWritten: number): number;
  cnc_web_load_save?(handle: number, data: number, length: number): number;
  cnc_web_event_size?(handle: number, outSize: number): number;
  cnc_web_poll_event?(handle: number, data: number, capacity: number, outWritten: number): number;
  CNC_Web_Acceptance_Force_Victory?(): number;
}

const STATUS_NAMES = ["OK", "NEED_BUFFER", "INVALID_ARGUMENT", "INVALID_STATE", "CONTENT_MISMATCH", "IO_ERROR", "OUT_OF_MEMORY", "FATAL"];
const UTF8_DECODER = new TextDecoder("utf-8", { fatal: true });

class WasmCoreStatusError extends Error {
  constructor(readonly status: number, operation: string) {
    super(`${operation} failed with ${STATUS_NAMES[status] ?? `status ${status}`}`);
    this.name = "WasmCoreStatusError";
  }
}

export function assertWasmStatus(status: number, operation: string): void {
  if (status !== 0) throw new WasmCoreStatusError(status, operation);
}

/** A FATAL ABI result means the legacy state can no longer be used safely. */
export function isFatalWasmCoreError(error: unknown): boolean {
  return error instanceof WasmCoreStatusError
    && (!Number.isInteger(error.status) || error.status === 7 || error.status < 0 || error.status >= STATUS_NAMES.length);
}

export function advancedOneTick(count: number): boolean {
  if (count !== 0 && count !== 1) throw new Error(`advance reported ${count} ticks for a one-tick request`);
  return count === 1;
}

export function parseWasmEvent(buffer: ArrayBuffer): SimulationEvent {
  const view = new DataView(buffer);
  if (buffer.byteLength < 64 || view.getUint32(0, true) !== CNC_WEB_MAGIC_MESSAGE || view.getUint16(4, true) !== SIMULATION_PROTOCOL_VERSION || view.getUint16(6, true) !== MessageKind.Event || view.getUint32(8, true) !== buffer.byteLength || view.getUint32(12, true) !== 1) throw new Error("Wasm core returned an invalid EventV1");
  const tick = view.getUint32(16, true);
  const eventType = view.getUint16(20, true);
  const flags = view.getUint16(22, true);
  const playerId = view.getBigUint64(24, true);
  const args = Array.from({ length: 6 }, (_, index) => view.getInt32(32 + index * 4, true));
  const text1Length = view.getUint32(56, true);
  const text2Length = view.getUint32(60, true);
  if (64 + text1Length + text2Length !== buffer.byteLength) throw new Error("EventV1 string lengths are invalid");
  const text1 = UTF8_DECODER.decode(new Uint8Array(buffer, 64, text1Length));
  const text2 = UTF8_DECODER.decode(new Uint8Array(buffer, 64 + text1Length, text2Length));
  if (eventType === 1) return { kind: "sound", tick, assetId: args[0], name: text1, variation: args[1], x: args[2], y: args[3], priority: args[4], context: args[5] };
  if (eventType === 2) return { kind: "speech", tick, assetId: args[0], name: text1 };
  if (eventType === 3) return {
    kind: "game-over",
    tick,
    multiplayer: Boolean(flags & 1),
    human: Boolean(flags & 2),
    won: Boolean(flags & 4),
    score: args[0],
    leadership: args[1],
    efficiency: args[2],
    remainingCredits: args[3],
    sabotagedStructure: args[4],
    timerRemaining: args[5],
    movieName: text1,
    afterScoreMovieName: text2,
  };
  if (eventType === 4) return { kind: "debug", tick, text: text1 };
  if (eventType === 5) return { kind: "movie", tick, name: text1, immediate: Boolean(flags & 1), theme: args[0] };
  if (eventType === 12) return { kind: "camera", tick, x: args[0], y: args[1] };
  if (eventType === 13) return { kind: "ping", tick, x: args[0], y: args[1] };
  if (eventType === 15) {
    if ((flags & ~7) !== 0 || (flags & 3) !== 2 || text2.length !== 0 || !Number.isInteger(args[1]) || args[1] < 0 || args[1] > 7
      || !Number.isInteger(args[2]) || args[2] < -1 || args[2] > 255
      || !Number.isInteger(args[4]) || args[4] < 1 || args[4] > 999
      || (args[5] !== 0 && args[5] !== 1) || !/^SC[GB][0-9]{2,3}[EW][A-DL]$/.test(text1)) {
      throw new Error("Wasm core returned an invalid campaign outcome event");
    }
    return {
      kind: "campaign-outcome",
      tick,
      carryOverCredits: args[0],
      nukePieces: args[1],
      sabotagedStructure: args[2],
      randomSeed: args[3] >>> 0,
      scenario: args[4],
      house: args[5],
      scenarioRoot: text1,
    };
  }
  if (eventType === 6) {
    const bits = new ArrayBuffer(4);
    new DataView(bits).setInt32(0, args[0], true);
    const parameter = BigInt.asIntN(64, BigInt.asUintN(32, BigInt(args[2])) | (BigInt.asUintN(32, BigInt(args[3])) << 32n));
    return { kind: "message", tick, text: text1, timeoutSeconds: new DataView(bits).getFloat32(0, true), messageType: args[1], parameter };
  }
  if (eventType === 14) return {
    kind: "diagnostic",
    tick,
    warning: Boolean(flags & 1),
    error: Boolean(flags & 2),
    code: args[0],
    status: args[1],
    scenario: args[2],
    variation: args[3],
    direction: args[4],
    buildLevel: args[5],
    id: text1,
    detail: text2,
  };
  return { kind: "engine", tick, eventType, flags, playerId, args, text1, text2 };
}

export function resolveEmscriptenModuleUrl(value: string, baseUrl = globalThis.location?.href ?? import.meta.url): URL {
  if (!value.trim()) throw new Error("An Emscripten engine module URL is required");
  let url: URL;
  try {
    url = new URL(value, baseUrl);
  } catch (cause) {
    throw new Error("The Emscripten engine module URL is invalid", { cause });
  }
  if (url.protocol !== "http:" && url.protocol !== "https:" && url.protocol !== "file:") {
    throw new Error(`The Emscripten engine module URL uses unsupported protocol ${url.protocol}`);
  }
  if (/\.wasm$/i.test(url.pathname)) {
    throw new Error("Raw .wasm engines are not supported; pass the generated Emscripten ES module URL (for example, ./engine/tiberiandawn.js)");
  }
  if (!/\.m?js$/i.test(url.pathname)) {
    throw new Error("The Emscripten engine module URL must end in .js or .mjs");
  }
  return url;
}

export class WasmCore implements SimulationCore {
  readonly supportsSaves: boolean;
  private readonly exports: CncWasmExports;
  private readonly handle: number;
  private readonly scratch: WasmScratchBuffers;
  private tick = 0;
  private destroyed = false;

  private constructor(exports: CncWasmExports) {
    this.exports = exports;
    this.scratch = new WasmScratchBuffers(exports);
    if (exports.cnc_web_abi_version() !== CNC_WEB_ABI_VERSION) throw new Error("The Wasm core ABI is incompatible with this browser runtime");
    const outHandle = exports.malloc(4);
    if (!outHandle) throw new Error("Wasm core could not allocate its handle output");
    try {
      assertWasmStatus(exports.cnc_web_create(CNC_WEB_ABI_VERSION, outHandle), "create");
      this.handle = new DataView(exports.memoryBuffer()).getUint32(outHandle, true);
    } finally {
      exports.free(outHandle);
    }
    if (!this.handle) throw new Error("The Wasm core returned an invalid simulation handle");
    this.supportsSaves = Boolean(exports.cnc_web_save_size && exports.cnc_web_write_save && exports.cnc_web_load_save);
  }

  static async create(emscriptenModuleUrl: string, preparedMount?: PreparedContentMount): Promise<WasmCore> {
    const moduleUrl = resolveEmscriptenModuleUrl(emscriptenModuleUrl);
    let namespace: { default?: (options?: Record<string, unknown>) => Promise<Record<string, unknown>> };
    try {
      namespace = await import(/* @vite-ignore */ moduleUrl.href) as typeof namespace;
    } catch (cause) {
      throw new Error(`Could not load the Emscripten engine module at ${moduleUrl.href}${cause instanceof Error ? `: ${cause.message}` : ""}`, { cause });
    }
    if (typeof namespace.default !== "function") throw new Error("Emscripten engine module has no default factory export");
    let module: Record<string, unknown>;
    try {
      const options: EmscriptenModuleWithFs = {
        locateFile: (path: string) => new URL(path, new URL(".", moduleUrl)).href,
      };
      if (preparedMount) {
        options.preRun = [(runtimeModule: EmscriptenModuleWithFs) => mountPreparedContent(runtimeModule, preparedMount)];
      }
      module = await namespace.default(options);
    } catch (cause) {
      throw new Error(`The Emscripten engine module at ${moduleUrl.href} failed to initialize${cause instanceof Error ? `: ${cause.message}` : ""}`, { cause });
    }
    const heap = module.HEAPU8;
    if (!(heap instanceof Uint8Array)) throw new Error("Emscripten engine module does not expose HEAPU8");
    return WasmCore.fromRuntime(module, () => (module.HEAPU8 as Uint8Array).buffer as ArrayBuffer);
  }

  private static fromRuntime(raw: Record<string, unknown>, memoryBuffer: () => ArrayBuffer): WasmCore {
    const pick = <T>(name: string): T | undefined => (raw[name] ?? raw[`_${name}`]) as T | undefined;
    const exports = {
      memoryBuffer,
      malloc: pick<(size: number) => number>("malloc"),
      free: pick<(pointer: number) => void>("free"),
      cnc_web_abi_version: pick<() => number>("cnc_web_abi_version"),
      cnc_web_create: pick<(version: number, outHandle: number) => number>("cnc_web_create"),
      cnc_web_destroy: pick<(handle: number) => number>("cnc_web_destroy"),
      cnc_web_set_campaign_transition: pick<(handle: number, carryOverCredits: number, nukePieces: number) => number>("cnc_web_set_campaign_transition"),
      cnc_web_start: pick<(handle: number, data: number, length: number) => number>("cnc_web_start"),
      cnc_web_submit_commands: pick<(handle: number, data: number, length: number) => number>("cnc_web_submit_commands"),
      cnc_web_advance: pick<(handle: number, tickCount: number, outAdvanced: number) => number>("cnc_web_advance"),
      cnc_web_snapshot_size: pick<(handle: number, outSize: number) => number>("cnc_web_snapshot_size"),
      cnc_web_write_snapshot: pick<(handle: number, data: number, capacity: number, outWritten: number) => number>("cnc_web_write_snapshot"),
      cnc_web_save_size: pick<(handle: number, outSize: number) => number>("cnc_web_save_size"),
      cnc_web_write_save: pick<(handle: number, data: number, capacity: number, outWritten: number) => number>("cnc_web_write_save"),
      cnc_web_load_save: pick<(handle: number, data: number, length: number) => number>("cnc_web_load_save"),
      cnc_web_event_size: pick<(handle: number, outSize: number) => number>("cnc_web_event_size"),
      cnc_web_poll_event: pick<(handle: number, data: number, capacity: number, outWritten: number) => number>("cnc_web_poll_event"),
      CNC_Web_Acceptance_Force_Victory: pick<() => number>("CNC_Web_Acceptance_Force_Victory"),
    };
    const required = ["memoryBuffer", "malloc", "free", "cnc_web_abi_version", "cnc_web_create", "cnc_web_destroy", "cnc_web_start", "cnc_web_submit_commands", "cnc_web_advance", "cnc_web_snapshot_size", "cnc_web_write_snapshot"] as const;
    for (const name of required) if (!exports[name]) throw new Error(`Wasm core is missing required export ${name}`);
    return new WasmCore(exports as CncWasmExports);
  }

  start(configuration: StartConfiguration): void {
    if (configuration.campaignTransition) {
      const { carryOverCredits, nukePieces } = configuration.campaignTransition;
      if (!Number.isInteger(carryOverCredits) || carryOverCredits < -0x8000_0000 || carryOverCredits > 0x7fff_ffff
        || !Number.isInteger(nukePieces) || nukePieces < 0 || nukePieces > 7) {
        throw new RangeError("Campaign transition state is out of range");
      }
      if (!this.exports.cnc_web_set_campaign_transition) throw new Error("Wasm core does not support campaign transitions");
      assertWasmStatus(this.exports.cnc_web_set_campaign_transition(this.handle, carryOverCredits, nukePieces), "set campaign transition");
    }
    this.withBytes(new Uint8Array(encodeStartConfiguration(configuration)), (pointer, length) => {
      assertWasmStatus(this.exports.cnc_web_start(this.handle, pointer, length), "start");
    });
    this.tick = 0;
  }

  submitCommands(batch: DecodedCommandBatch): void {
    const bytes = new Uint8Array(encodeCommandBatch(batch.targetTick, batch.commands, batch.playerId));
    this.withBytes(bytes, (pointer, length) => assertWasmStatus(this.exports.cnc_web_submit_commands(this.handle, pointer, length), "submit commands"));
  }

  currentTick(): number { return this.tick; }

  advance(): boolean {
    const advanced = this.withOutputU32((output) => this.exports.cnc_web_advance(this.handle, 1, output), "advance");
    const didAdvance = advancedOneTick(advanced);
    if (didAdvance) this.tick = (this.tick + 1) >>> 0;
    return didAdvance;
  }

  snapshotSize(): number {
    const size = this.withOutputU32((output) => this.exports.cnc_web_snapshot_size(this.handle, output), "snapshot size");
    if (size < 40) throw new Error("Wasm core returned an invalid snapshot size");
    return size;
  }

  writeSnapshot(target: ArrayBuffer): number {
    return this.copyOutput(target, (data, capacity, outWritten) => this.exports.cnc_web_write_snapshot(this.handle, data, capacity, outWritten), "write snapshot");
  }

  save(): Uint8Array {
    if (!this.exports.cnc_web_save_size || !this.exports.cnc_web_write_save) throw new Error("Wasm core does not support saves");
    const size = this.withOutputU32((output) => this.exports.cnc_web_save_size!(this.handle, output), "save size");
    const target = new ArrayBuffer(size);
    const written = this.copyOutput(target, (data, capacity, outWritten) => this.exports.cnc_web_write_save!(this.handle, data, capacity, outWritten), "write save");
    return new Uint8Array(target, 0, written);
  }

  load(data: Uint8Array): void {
    if (!this.exports.cnc_web_load_save) throw new Error("Wasm core does not support saves");
    if (data.byteLength < 20) throw new Error("Wasm save is truncated");
    const savedTick = new DataView(data.buffer, data.byteOffset, data.byteLength).getUint32(16, true);
    this.withBytes(data, (pointer, length) => assertWasmStatus(this.exports.cnc_web_load_save!(this.handle, pointer, length), "load save"));
    this.tick = savedTick;
  }

  acceptanceForceVictory(): void {
    if (!this.exports.CNC_Web_Acceptance_Force_Victory) throw new Error("Wasm core does not provide the release-acceptance victory hook");
    if (this.exports.CNC_Web_Acceptance_Force_Victory() !== 1) throw new Error("Wasm core rejected the release-acceptance victory request");
  }

  drainEvents(): SimulationEvent[] {
    if (!this.exports.cnc_web_event_size || !this.exports.cnc_web_poll_event) return [];
    const events: SimulationEvent[] = [];
    for (let index = 0; index < 1024; index += 1) {
      const size = this.withOutputU32((output) => this.exports.cnc_web_event_size!(this.handle, output), "event size");
      if (size === 0) break;
      if (size < 64 || size > 1024 * 1024) throw new Error("Wasm event size is invalid");
      const target = new ArrayBuffer(size);
      const written = this.copyOutput(target, (data, capacity, outWritten) => this.exports.cnc_web_poll_event!(this.handle, data, capacity, outWritten), "poll event");
      if (written !== size) throw new Error("Wasm event size changed while polling");
      events.push(parseWasmEvent(target));
    }
    return events;
  }

  destroy(): void {
    if (this.destroyed) return;
    this.destroyed = true;
    try {
      assertWasmStatus(this.exports.cnc_web_destroy(this.handle), "destroy");
    } finally {
      this.scratch.release();
    }
  }

  private withOutputU32(callback: (output: number) => number, operation: string): number {
    const output = this.scratch.outputU32(operation);
    assertWasmStatus(callback(output), operation);
    return new DataView(this.exports.memoryBuffer()).getUint32(output, true);
  }

  private copyOutput(target: ArrayBuffer, callback: (data: number, capacity: number, outWritten: number) => number, operation: string): number {
    const data = this.scratch.bytes(target.byteLength, operation);
    const outWritten = this.scratch.outputU32(operation);
    assertWasmStatus(callback(data, target.byteLength, outWritten), operation);
    const written = new DataView(this.exports.memoryBuffer()).getUint32(outWritten, true);
    if (written > target.byteLength) throw new Error(`${operation} exceeded its output buffer`);
    new Uint8Array(target, 0, written).set(new Uint8Array(this.exports.memoryBuffer(), data, written));
    return written;
  }

  private withBytes(bytes: Uint8Array, callback: (pointer: number, length: number) => void): void {
    const pointer = this.scratch.bytes(bytes.byteLength, "copying input");
    new Uint8Array(this.exports.memoryBuffer(), pointer, bytes.byteLength).set(bytes);
    callback(pointer, bytes.byteLength);
  }
}
