import { validateContentPath, type ContentRevisionDescriptor, type ContentStore } from "../storage/ContentStore";
import type { SimulationEvent } from "../simulation/protocol";

export const RUNTIME_AUDIO_PATH = "runtime/audio-v1.json";
export const MAX_RUNTIME_AUDIO_INDEX_BYTES = 4 * 1024 * 1024;

const MAX_PENDING_CUES = 32;
const MAX_PENDING_SPEECH = 2;
const MAX_PENDING_SOUND_AGE_TICKS = 30;
const MAX_ACTIVE_SOUND_VOICES = 24;

export interface RuntimeAudioAsset {
  kind: "sound" | "speech";
  eventName: string;
  eventIds: number[];
  path: string;
  sourceArchive: "SPEECH.MIX" | "SOUNDS.MIX";
  sourceName: string;
  sourceCompression: 0 | 1 | 99;
  sampleRate: number;
  channels: 1 | 2;
  bitsPerSample: 8 | 16;
  frames: number;
  sha256: string;
}

interface RuntimeAudioFailure {
  kind: "sound" | "speech";
  eventName: string;
  sourceArchive: "SPEECH.MIX" | "SOUNDS.MIX";
  sourceName: string;
  reason: string;
}

export interface RuntimeAudioIndexV1 {
  format: "cncweb-audio";
  version: 1;
  encoding: "wav-pcm";
  assets: RuntimeAudioAsset[];
  diagnostics: { candidateCount: number; missingCandidates: number; decodeFailures: RuntimeAudioFailure[] };
}

function exactKeys(value: object, expected: readonly string[], label: string): void {
  const actual = Object.keys(value).sort();
  const wanted = [...expected].sort();
  if (actual.length !== wanted.length || actual.some((key, index) => key !== wanted[index])) throw new Error(`${label} contains missing or unknown fields`);
}

function boundedInteger(value: unknown, minimum: number, maximum: number, label: string): asserts value is number {
  if (!Number.isSafeInteger(value) || (value as number) < minimum || (value as number) > maximum) throw new Error(`${label} is invalid`);
}

function validateEventName(value: unknown, label: string): asserts value is string {
  if (typeof value !== "string" || !/^[A-Z0-9]+(?:\.V0[0-3])?$/.test(value) || value.length > 16) throw new Error(`${label} must be an exact engine audio event name`);
}

function expectedSourceName(kind: "sound" | "speech", eventName: string): string {
  return kind === "sound" && eventName.includes(".") ? eventName : `${eventName}.AUD`;
}

function expectedAudioPath(kind: "sound" | "speech", eventName: string): string {
  return `audio/${kind === "sound" ? "sfx" : "speech"}/${eventName.toLowerCase()}.wav`;
}

export function runtimeAudioKey(kind: "sound" | "speech", eventName: string): string { return `${kind}:${eventName}`; }

export function assertFirstPlayableAudio(index: RuntimeAudioIndexV1, allowMissingOutcomeSpeech = false): void {
  const available = new Set(index.assets.map((asset) => runtimeAudioKey(asset.kind, asset.eventName)));
  const hasExact = (kind: "sound" | "speech", names: readonly string[]) => names.some((name) => available.has(runtimeAudioKey(kind, name)));
  const hasVariant = (kind: "sound" | "speech", prefixes: readonly string[]) => index.assets.some((asset) => asset.kind === kind
    && prefixes.some((prefix) => asset.eventName === prefix || new RegExp(`^${prefix}\\.V0[0-3]$`).test(asset.eventName)));
  const gates: [string, boolean][] = [
    ["weapon", hasExact("sound", ["MGUN2", "GUN18", "BAZOOK1"])],
    ["interface-feedback", hasExact("sound", ["BUTTON", "SCOLD2", "BLEEP2"])],
    ["explosion", hasExact("sound", ["XPLOS", "XPLODE", "XPLOSML2", "XPLOBIG4"])],
    ["unit-response", hasVariant("sound", ["ACKNO", "AFFIRM1", "MOVOUT1", "REPORT1", "UNIT1", "YESSIR1"])],
    ["mission-accomplished", hasExact("speech", ["ACCOM1"])],
    ["mission-failed", hasExact("speech", ["FAIL1"])],
    ["gameplay-speech", hasExact("speech", ["REINFOR1", "UNITREDY", "NEWOPT1", "BASEATK1"])],
  ];
  const missing = gates
    .filter(([name, present]) => !present && !(allowMissingOutcomeSpeech && (name === "mission-accomplished" || name === "mission-failed")))
    .map(([name]) => name);
  if (missing.length) throw new Error(`Runtime audio is insufficient for GDI Mission 1; missing core groups: ${missing.join(", ")}`);
}

function validateFailure(value: RuntimeAudioFailure, index: number): void {
  if (!value || typeof value !== "object") throw new Error(`Audio decode failure ${index} is invalid`);
  exactKeys(value, ["kind", "eventName", "sourceArchive", "sourceName", "reason"], `Audio decode failure ${index}`);
  if (value.kind !== "sound" && value.kind !== "speech") throw new Error(`Audio decode failure ${index} kind is invalid`);
  validateEventName(value.eventName, `Audio decode failure ${index} eventName`);
  if (value.sourceArchive !== "SPEECH.MIX" && value.sourceArchive !== "SOUNDS.MIX") throw new Error(`Audio decode failure ${index} archive is invalid`);
  if (typeof value.sourceName !== "string" || !/^[A-Z0-9]+(?:\.AUD|\.V0[0-3])$/.test(value.sourceName) || value.sourceName.length > 16) throw new Error(`Audio decode failure ${index} sourceName is invalid`);
  if (typeof value.reason !== "string" || !value.reason || value.reason.length > 1024) throw new Error(`Audio decode failure ${index} reason is invalid`);
}

export function parseRuntimeAudioIndex(data: Uint8Array): RuntimeAudioIndexV1 {
  if (data.byteLength === 0 || data.byteLength > MAX_RUNTIME_AUDIO_INDEX_BYTES) throw new Error("Runtime audio index size is invalid");
  let value: unknown;
  try { value = JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(data)); }
  catch (cause) { throw new Error("Runtime audio index is not valid UTF-8 JSON", { cause }); }
  if (!value || typeof value !== "object") throw new Error("Runtime audio index is not an object");
  const index = value as RuntimeAudioIndexV1;
  exactKeys(index, ["format", "version", "encoding", "assets", "diagnostics"], "Runtime audio index");
  if (index.format !== "cncweb-audio" || index.version !== 1 || index.encoding !== "wav-pcm") throw new Error("Runtime audio index format is unsupported");
  if (!Array.isArray(index.assets) || index.assets.length < 1 || index.assets.length > 1000) throw new Error("Runtime audio asset count is invalid");
  let previous = "";
  const paths = new Set<string>();
  for (const [assetIndex, asset] of index.assets.entries()) {
    if (!asset || typeof asset !== "object") throw new Error(`Runtime audio asset ${assetIndex} is invalid`);
    exactKeys(asset, ["kind", "eventName", "eventIds", "path", "sourceArchive", "sourceName", "sourceCompression", "sampleRate", "channels", "bitsPerSample", "frames", "sha256"], `Runtime audio asset ${assetIndex}`);
    if (asset.kind !== "sound" && asset.kind !== "speech") throw new Error(`Runtime audio asset ${assetIndex} kind is invalid`);
    validateEventName(asset.eventName, `Runtime audio asset ${assetIndex} eventName`);
    const order = `${asset.kind}\0${asset.eventName}`;
    if (previous && previous >= order) throw new Error("Runtime audio assets must be strictly sorted by kind and eventName");
    previous = order;
    if (!Array.isArray(asset.eventIds) || asset.eventIds.length < 1 || asset.eventIds.length > 4) throw new Error(`Runtime audio asset ${assetIndex} eventIds are invalid`);
    let previousId = -1;
    for (const id of asset.eventIds) { boundedInteger(id, 0, 65535, `Runtime audio asset ${assetIndex} event ID`); if (id <= previousId) throw new Error(`Runtime audio asset ${assetIndex} eventIds must be strictly increasing`); previousId = id; }
    validateContentPath(asset.path);
    if (!/^audio\/(?:sfx|speech)\/[a-z0-9]+(?:\.v0[0-3])?\.wav$/.test(asset.path)) throw new Error(`Runtime audio asset ${assetIndex} path is invalid`);
    if ((asset.kind === "sound") !== asset.path.startsWith("audio/sfx/")) throw new Error(`Runtime audio asset ${assetIndex} path does not match its kind`);
    if (asset.path !== expectedAudioPath(asset.kind, asset.eventName) || paths.has(asset.path)) throw new Error(`Runtime audio asset ${assetIndex} path is not the canonical unique event path`);
    paths.add(asset.path);
    if (asset.sourceArchive !== "SPEECH.MIX" && asset.sourceArchive !== "SOUNDS.MIX") throw new Error(`Runtime audio asset ${assetIndex} archive is invalid`);
    if ((asset.kind === "sound") !== (asset.sourceArchive === "SOUNDS.MIX")) throw new Error(`Runtime audio asset ${assetIndex} archive does not match its kind`);
    if (typeof asset.sourceName !== "string" || !/^[A-Z0-9]+(?:\.AUD|\.V0[0-3])$/.test(asset.sourceName) || asset.sourceName.length > 16) throw new Error(`Runtime audio asset ${assetIndex} sourceName is invalid`);
    if (asset.sourceName !== expectedSourceName(asset.kind, asset.eventName)) throw new Error(`Runtime audio asset ${assetIndex} sourceName does not match its event`);
    if (asset.sourceCompression !== 0 && asset.sourceCompression !== 1 && asset.sourceCompression !== 99) throw new Error(`Runtime audio asset ${assetIndex} compression is invalid`);
    boundedInteger(asset.sampleRate, 4000, 192000, `Runtime audio asset ${assetIndex} sampleRate`);
    if (asset.channels !== 1 && asset.channels !== 2) throw new Error(`Runtime audio asset ${assetIndex} channels are invalid`);
    if (asset.bitsPerSample !== 8 && asset.bitsPerSample !== 16) throw new Error(`Runtime audio asset ${assetIndex} sample width is invalid`);
    boundedInteger(asset.frames, 1, Number.MAX_SAFE_INTEGER, `Runtime audio asset ${assetIndex} frames`);
    if (!/^[a-f0-9]{64}$/.test(asset.sha256)) throw new Error(`Runtime audio asset ${assetIndex} checksum is invalid`);
  }
  if (!index.diagnostics || typeof index.diagnostics !== "object") throw new Error("Runtime audio diagnostics are invalid");
  exactKeys(index.diagnostics, ["candidateCount", "missingCandidates", "decodeFailures"], "Runtime audio diagnostics");
  boundedInteger(index.diagnostics.candidateCount, 1, 1000, "Runtime audio candidateCount");
  boundedInteger(index.diagnostics.missingCandidates, 0, 1000, "Runtime audio missingCandidates");
  if (!Array.isArray(index.diagnostics.decodeFailures) || index.diagnostics.decodeFailures.length > 1000) throw new Error("Runtime audio decodeFailures are invalid");
  let previousFailure = "";
  index.diagnostics.decodeFailures.forEach((failure, failureIndex) => {
    validateFailure(failure, failureIndex);
    const order = `${failure.kind}\0${failure.eventName}`;
    if (previousFailure && previousFailure >= order) throw new Error("Runtime audio decode failures must be strictly sorted");
    previousFailure = order;
    if ((failure.kind === "sound") !== (failure.sourceArchive === "SOUNDS.MIX")) throw new Error(`Audio decode failure ${failureIndex} archive does not match its kind`);
    if (failure.sourceName !== expectedSourceName(failure.kind, failure.eventName)) throw new Error(`Audio decode failure ${failureIndex} sourceName does not match its event`);
  });
  if (index.diagnostics.candidateCount !== index.assets.length + index.diagnostics.missingCandidates + index.diagnostics.decodeFailures.length) throw new Error("Runtime audio diagnostics do not account for every candidate");
  return index;
}

type AudioCue = Extract<SimulationEvent, { kind: "sound" | "speech" }>;

export class RuntimeAudio {
  private context?: AudioContext;
  private readonly assets = new Map<string, RuntimeAudioAsset>();
  private readonly decoded = new Map<string, Promise<AudioBuffer>>();
  private readonly failed = new Set<string>();
  private readonly pending: AudioCue[] = [];
  private readonly activeSounds = new Map<AudioBufferSourceNode, number>();
  private readonly activeSpeech = new Set<AudioBufferSourceNode>();
  private speechTail = Promise.resolve();
  private cameraX = 0;
  private viewWidth = 1;
  private latestTick = 0;
  private paused = false;
  private pauseTransition = Promise.resolve();
  private disposed = false;

  private constructor(private readonly store: ContentStore, private readonly revision: ContentRevisionDescriptor, index: RuntimeAudioIndexV1) {
    const manifestFiles = new Map(revision.manifest.files.map((file) => [file.path, file]));
    for (const asset of index.assets) {
      const descriptor = manifestFiles.get(asset.path);
      if (!descriptor || descriptor.role !== "audio" || descriptor.sha256 !== asset.sha256 || descriptor.size < 44) throw new Error(`Runtime audio asset does not match the installed manifest: ${asset.path}`);
      this.assets.set(runtimeAudioKey(asset.kind, asset.eventName), asset);
    }
  }

  static async load(store: ContentStore, revision: ContentRevisionDescriptor): Promise<RuntimeAudio | undefined> {
    const descriptor = revision.manifest.files.find((file) => file.path === RUNTIME_AUDIO_PATH);
    if (!descriptor) return undefined;
    if (descriptor.role !== "configuration" || descriptor.size <= 0 || descriptor.size > MAX_RUNTIME_AUDIO_INDEX_BYTES) throw new Error("Runtime audio index does not match the installed manifest");
    const index = parseRuntimeAudioIndex(await store.readRevisionFile(revision, RUNTIME_AUDIO_PATH));
    return new RuntimeAudio(store, revision, index);
  }

  handle(event: SimulationEvent): void {
    if ((event.kind !== "sound" && event.kind !== "speech") || this.disposed || this.paused || !this.assets.has(runtimeAudioKey(event.kind, event.name))) return;
    this.latestTick = Math.max(this.latestTick, event.tick);
    if (!this.context || this.context.state !== "running") {
      if (event.kind === "speech") {
        while (this.pending.filter((cue) => cue.kind === "speech").length >= MAX_PENDING_SPEECH) {
          const oldestSpeech = this.pending.findIndex((cue) => cue.kind === "speech");
          if (oldestSpeech < 0) break;
          this.pending.splice(oldestSpeech, 1);
        }
      }
      if (this.pending.length >= MAX_PENDING_CUES) this.pending.shift();
      this.pending.push(event);
      return;
    }
    this.schedule(event);
  }

  setView(cameraX: number, viewWidth: number): void {
    if (Number.isFinite(cameraX)) this.cameraX = cameraX;
    if (Number.isFinite(viewWidth) && viewWidth > 0) this.viewWidth = viewWidth;
  }

  async unlock(): Promise<void> {
    if (this.disposed || this.paused) return;
    try {
      this.context ??= new AudioContext({ latencyHint: "interactive" });
      if (this.context.state === "suspended") await this.context.resume();
      if (this.context.state !== "running") return;
      const pending = this.pending.splice(0);
      for (const cue of pending) {
        if (cue.kind === "speech" || this.latestTick - cue.tick <= MAX_PENDING_SOUND_AGE_TICKS) this.schedule(cue);
      }
    } catch (error) {
      console.warn("Could not enable runtime audio", error);
    }
  }

  setPaused(paused: boolean): Promise<void> {
    if (this.disposed) return Promise.resolve();
    if (this.paused === paused) return this.pauseTransition;
    this.paused = paused;
    if (paused) {
      this.pending.length = 0;
      for (const source of [...this.activeSounds.keys()]) {
        try { source.stop(); } catch { /* The source may already have ended. */ }
      }
      for (const source of [...this.activeSpeech]) {
        try { source.stop(); } catch { /* The source may already have ended. */ }
      }
      this.activeSounds.clear();
      this.activeSpeech.clear();
    }
    const applyLatestState = async (): Promise<void> => {
      const context = this.context;
      if (!context || this.disposed) return;
      if (this.paused && context.state === "running") await context.suspend().catch(() => undefined);
      else if (!this.paused && context.state === "suspended") await context.resume().catch(() => undefined);
    };
    // AudioContext state changes are asynchronous. Serializing them ensures a
    // rapid pause/resume cannot leave a resumed simulation with a context whose
    // earlier suspend() promise completed last.
    this.pauseTransition = this.pauseTransition.then(applyLatestState, applyLatestState);
    return this.pauseTransition;
  }

  destroy(): void {
    this.disposed = true;
    this.pending.length = 0;
    for (const source of this.activeSounds.keys()) {
      try { source.stop(); } catch { /* The source may already have ended. */ }
    }
    for (const source of this.activeSpeech) {
      try { source.stop(); } catch { /* The source may already have ended. */ }
    }
    this.activeSounds.clear();
    this.activeSpeech.clear();
    void this.context?.close().catch(() => undefined);
  }

  private schedule(event: AudioCue): void {
    if (event.kind === "speech") this.speechTail = this.speechTail.then(() => this.playNow(event));
    else void this.playNow(event);
  }

  private async playNow(event: AudioCue): Promise<void> {
    const context = this.context;
    const asset = this.assets.get(runtimeAudioKey(event.kind, event.name));
    if (!context || context.state !== "running" || !asset || this.disposed || this.paused || this.failed.has(asset.path)) return;
    let decoded = this.decoded.get(asset.path);
    if (!decoded) {
      decoded = this.store.readRevisionFile(this.revision, asset.path)
        .then((bytes) => context.decodeAudioData(bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength) as ArrayBuffer))
        .then((buffer) => {
          const expectedDuration = asset.frames / asset.sampleRate;
          const durationTolerance = Math.max(0.05, expectedDuration * 0.03);
          if (buffer.numberOfChannels !== asset.channels || Math.abs(buffer.duration - expectedDuration) > durationTolerance) throw new Error("Decoded WAV metadata does not match its audio index");
          return buffer;
        });
      this.decoded.set(asset.path, decoded);
    }
    try {
      const buffer = await decoded;
      if (this.disposed || this.paused || context.state !== "running") return;
      if (event.kind === "sound" && this.activeSounds.size >= MAX_ACTIVE_SOUND_VOICES) {
        let quietest: [AudioBufferSourceNode, number] | undefined;
        for (const voice of this.activeSounds) if (!quietest || voice[1] < quietest[1]) quietest = voice;
        if (quietest && quietest[1] >= event.priority) return;
        if (quietest) this.activeSounds.delete(quietest[0]);
        try { quietest?.[0].stop(); } catch { /* The source may have ended between selection and stop. */ }
      }
      const source = context.createBufferSource();
      source.buffer = buffer;
      if (event.kind === "sound" && event.x >= 0 && event.y >= 0 && typeof context.createStereoPanner === "function") {
        const panner = context.createStereoPanner();
        const center = this.cameraX + this.viewWidth / 2;
        panner.pan.value = Math.max(-1, Math.min(1, (event.x - center) / Math.max(1, this.viewWidth / 2)));
        source.connect(panner).connect(context.destination);
      } else source.connect(context.destination);
      if (event.kind === "sound") this.activeSounds.set(source, event.priority);
      else this.activeSpeech.add(source);
      const ended = new Promise<void>((resolve) => {
        source.onended = () => {
          this.activeSounds.delete(source);
          this.activeSpeech.delete(source);
          resolve();
        };
      });
      source.start();
      if (event.kind === "speech") await ended;
    } catch (error) {
      this.decoded.delete(asset.path);
      this.failed.add(asset.path);
      console.warn(`Could not play runtime audio ${asset.eventName}`, error);
    }
  }
}
