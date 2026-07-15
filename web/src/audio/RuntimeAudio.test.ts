import { afterEach, describe, expect, it, vi } from "vitest";
import type { SimulationEvent } from "../simulation/protocol";
import type { ContentRevisionDescriptor, ContentStore } from "../storage/ContentStore";
import { assertFirstPlayableAudio, MAX_RUNTIME_AUDIO_INDEX_BYTES, parseRuntimeAudioIndex, RUNTIME_AUDIO_PATH, RuntimeAudio, runtimeAudioKey } from "./RuntimeAudio";

function indexValue(): Record<string, unknown> {
  return {
    format: "cncweb-audio",
    version: 1,
    encoding: "wav-pcm",
    assets: [
      {
        kind: "sound",
        eventName: "ACKNO.V00",
        eventIds: [19],
        path: "audio/sfx/ackno.v00.wav",
        sourceArchive: "SOUNDS.MIX",
        sourceName: "ACKNO.V00",
        sourceCompression: 1,
        sampleRate: 22050,
        channels: 1,
        bitsPerSample: 8,
        frames: 2205,
        sha256: "11".repeat(32),
      },
      {
        kind: "speech",
        eventName: "ACCOM1",
        eventIds: [0],
        path: "audio/speech/accom1.wav",
        sourceArchive: "SPEECH.MIX",
        sourceName: "ACCOM1.AUD",
        sourceCompression: 99,
        sampleRate: 22050,
        channels: 1,
        bitsPerSample: 16,
        frames: 4410,
        sha256: "22".repeat(32),
      },
    ],
    diagnostics: { candidateCount: 4, missingCandidates: 1, decodeFailures: [
      { kind: "speech", eventName: "FAIL1", sourceArchive: "SPEECH.MIX", sourceName: "FAIL1.AUD", reason: "synthetic unsupported sample" },
    ] },
  };
}

function encode(value: unknown): Uint8Array {
  return new TextEncoder().encode(JSON.stringify(value));
}

function coreIndexValue(): Record<string, unknown> {
  const asset = (kind: "sound" | "speech", eventName: string, eventId: number) => ({
    kind,
    eventName,
    eventIds: [eventId],
    path: `audio/${kind === "sound" ? "sfx" : "speech"}/${eventName.toLowerCase()}.wav`,
    sourceArchive: kind === "sound" ? "SOUNDS.MIX" : "SPEECH.MIX",
    sourceName: kind === "sound" && eventName.includes(".") ? eventName : `${eventName}.AUD`,
    sourceCompression: 1,
    sampleRate: 22050,
    channels: 1,
    bitsPerSample: 8,
    frames: 2205,
    sha256: eventId.toString(16).padStart(2, "0").repeat(32),
  });
  const assets = [
    asset("sound", "ACKNO.V00", 1),
    asset("sound", "BUTTON", 2),
    asset("sound", "MGUN2", 3),
    asset("sound", "XPLOS", 4),
    asset("speech", "ACCOM1", 5),
    asset("speech", "FAIL1", 6),
    asset("speech", "REINFOR1", 7),
  ];
  return { format: "cncweb-audio", version: 1, encoding: "wav-pcm", assets, diagnostics: { candidateCount: assets.length, missingCandidates: 0, decodeFailures: [] } };
}

class FakeSource {
  buffer?: AudioBuffer;
  onended: (() => void) | null = null;
  private active = false;

  constructor(private readonly context: FakeAudioContext) {}

  connect<T>(target: T): T { return target; }
  start(): void { this.active = true; this.context.starts += 1; this.context.active += 1; }
  stop(): void {
    if (!this.active) return;
    this.active = false;
    this.context.active -= 1;
    this.onended?.();
  }
}

class FakeAudioContext {
  state: AudioContextState = "running";
  readonly destination = {} as AudioDestinationNode;
  starts = 0;
  active = 0;
  panners = 0;
  readonly panValues: Array<{ value: number }> = [];

  createBufferSource(): AudioBufferSourceNode { return new FakeSource(this) as unknown as AudioBufferSourceNode; }
  createStereoPanner(): StereoPannerNode {
    this.panners += 1;
    const pan = { value: 0 };
    this.panValues.push(pan);
    return { pan, connect: <T>(target: T) => target } as unknown as StereoPannerNode;
  }
  async decodeAudioData(): Promise<AudioBuffer> { return { duration: 1, numberOfChannels: 1 } as AudioBuffer; }
  async resume(): Promise<void> { this.state = "running"; }
  async suspend(): Promise<void> { this.state = "suspended"; }
  async close(): Promise<void> { this.state = "closed"; }
}

class DelayedSuspendAudioContext extends FakeAudioContext {
  private releaseSuspend?: () => void;

  override suspend(): Promise<void> {
    return new Promise((resolve) => {
      this.releaseSuspend = () => {
        this.state = "suspended";
        resolve();
      };
    });
  }

  finishSuspend(): void { this.releaseSuspend?.(); }
}

function runtimeIndex(): { bytes: Uint8Array; sha256: string } {
  const sha256 = "11".repeat(32);
  const speechSha256 = "22".repeat(32);
  const value = {
    format: "cncweb-audio",
    version: 1,
    encoding: "wav-pcm",
    assets: [{
      kind: "sound",
      eventName: "MGUN2",
      eventIds: [0],
      path: "audio/sfx/mgun2.wav",
      sourceArchive: "SOUNDS.MIX",
      sourceName: "MGUN2.AUD",
      sourceCompression: 1,
      sampleRate: 8000,
      channels: 1,
      bitsPerSample: 8,
      frames: 8000,
      sha256,
    }, {
      kind: "speech",
      eventName: "REINFOR1",
      eventIds: [1],
      path: "audio/speech/reinfor1.wav",
      sourceArchive: "SPEECH.MIX",
      sourceName: "REINFOR1.AUD",
      sourceCompression: 1,
      sampleRate: 8000,
      channels: 1,
      bitsPerSample: 8,
      frames: 8000,
      sha256: speechSha256,
    }],
    diagnostics: { candidateCount: 2, missingCandidates: 0, decodeFailures: [] },
  };
  return { bytes: encode(value), sha256 };
}

async function runtimeAudio(): Promise<RuntimeAudio> {
  const index = runtimeIndex();
  const revision = {
    id: "td-test",
    revision: "22".repeat(32),
    storageKey: "22".repeat(32),
    installedAt: new Date(0).toISOString(),
    manifest: {
      files: [
        { path: RUNTIME_AUDIO_PATH, size: index.bytes.byteLength, sha256: "33".repeat(32), role: "configuration" },
        { path: "audio/sfx/mgun2.wav", size: 44, sha256: index.sha256, role: "audio" },
        { path: "audio/speech/reinfor1.wav", size: 44, sha256: "22".repeat(32), role: "audio" },
      ],
    },
  } as unknown as ContentRevisionDescriptor;
  const store = {
    readRevisionFile: async (_revision: ContentRevisionDescriptor, path: string) => path === RUNTIME_AUDIO_PATH ? index.bytes : new Uint8Array(44),
  } as unknown as ContentStore;
  const audio = await RuntimeAudio.load(store, revision);
  if (!audio) throw new Error("Synthetic runtime audio did not load");
  return audio;
}

function sound(tick: number, x = -1, y = -1, priority = 1): SimulationEvent {
  return { kind: "sound", tick, assetId: 0, name: "MGUN2", variation: 0, x, y, priority, context: 0 };
}

function speech(tick: number): SimulationEvent {
  return { kind: "speech", tick, assetId: 1, name: "REINFOR1" };
}

async function flushAudio(): Promise<void> {
  await Promise.resolve();
  await new Promise<void>((resolve) => setTimeout(resolve, 0));
}

afterEach(() => vi.unstubAllGlobals());

describe("runtime audio index", () => {
  it("parses exact callback names and browser PCM metadata", () => {
    const parsed = parseRuntimeAudioIndex(encode(indexValue()));
    expect(parsed.assets.map((asset) => runtimeAudioKey(asset.kind, asset.eventName))).toEqual([
      "sound:ACKNO.V00",
      "speech:ACCOM1",
    ]);
  });

  it("independently gates the seven core GDI Mission 1 audio groups", () => {
    expect(() => assertFirstPlayableAudio(parseRuntimeAudioIndex(encode(coreIndexValue())))).not.toThrow();
    expect(() => assertFirstPlayableAudio(parseRuntimeAudioIndex(encode(indexValue())))).toThrow(/weapon.*interface-feedback.*explosion.*mission-failed.*gameplay-speech/);

    const freeware = coreIndexValue();
    freeware.assets = (freeware.assets as Record<string, unknown>[])
      .filter((asset) => asset.eventName !== "ACCOM1" && asset.eventName !== "FAIL1");
    freeware.diagnostics = { candidateCount: 7, missingCandidates: 2, decodeFailures: [] };
    const parsed = parseRuntimeAudioIndex(encode(freeware));
    expect(() => assertFirstPlayableAudio(parsed)).toThrow(/mission-accomplished.*mission-failed/);
    expect(() => assertFirstPlayableAudio(parsed, true)).not.toThrow();
  });

  it("rejects unknown fields and unaccounted candidates", () => {
    const unknown = indexValue();
    (unknown.assets as Record<string, unknown>[])[0].unexpected = true;
    expect(() => parseRuntimeAudioIndex(encode(unknown))).toThrow("unknown fields");

    const unaccounted = indexValue();
    (unaccounted.diagnostics as Record<string, unknown>).candidateCount = 9;
    expect(() => parseRuntimeAudioIndex(encode(unaccounted))).toThrow("do not account");
  });

  it("rejects case-folded event identities and unsafe paths", () => {
    const lower = indexValue();
    (lower.assets as Record<string, unknown>[])[0].eventName = "ackno.v00";
    expect(() => parseRuntimeAudioIndex(encode(lower))).toThrow("exact engine audio event name");

    const traversal = indexValue();
    (traversal.assets as Record<string, unknown>[])[0].path = "audio/sfx/../probe.wav";
    expect(() => parseRuntimeAudioIndex(encode(traversal))).toThrow("Invalid content path");
  });

  it("requires canonical event paths and source identities", () => {
    const aliasedPath = indexValue();
    (aliasedPath.assets as Record<string, unknown>[])[0].path = "audio/sfx/other.v00.wav";
    expect(() => parseRuntimeAudioIndex(encode(aliasedPath))).toThrow("canonical unique event path");

    const aliasedSource = indexValue();
    (aliasedSource.assets as Record<string, unknown>[])[1].sourceName = "OTHER.AUD";
    expect(() => parseRuntimeAudioIndex(encode(aliasedSource))).toThrow("sourceName does not match its event");
  });

  it("requires sorted decode failures and a bounded index", () => {
    const unsorted = indexValue();
    (unsorted.diagnostics as Record<string, unknown>).candidateCount = 5;
    (unsorted.diagnostics as Record<string, unknown>).decodeFailures = [
      { kind: "speech", eventName: "FAIL2", sourceArchive: "SPEECH.MIX", sourceName: "FAIL2.AUD", reason: "synthetic failure two" },
      { kind: "speech", eventName: "FAIL1", sourceArchive: "SPEECH.MIX", sourceName: "FAIL1.AUD", reason: "synthetic failure one" },
    ];
    expect(() => parseRuntimeAudioIndex(encode(unsorted))).toThrow("strictly sorted");
    expect(() => parseRuntimeAudioIndex(new Uint8Array(MAX_RUNTIME_AUDIO_INDEX_BYTES + 1))).toThrow("size is invalid");
  });

  it("retains the newest queued cues, expires stale sounds, and caps active voices", async () => {
    const context = new FakeAudioContext();
    vi.stubGlobal("AudioContext", class { constructor() { return context; } });
    const audio = await runtimeAudio();
    for (let tick = 1; tick <= 33; tick += 1) audio.handle(sound(tick));
    const pending = (audio as unknown as { pending: SimulationEvent[] }).pending;
    expect(pending).toHaveLength(32);
    expect(pending[0].tick).toBe(2);
    await audio.unlock();
    await flushAudio();
    expect(context.starts).toBe(24);
    expect(context.active).toBe(24);
    expect(context.panners).toBe(0);
    audio.destroy();
    expect(context.active).toBe(0);
  });

  it("spatializes only cues with world coordinates", async () => {
    const context = new FakeAudioContext();
    vi.stubGlobal("AudioContext", class { constructor() { return context; } });
    const audio = await runtimeAudio();
    await audio.unlock();
    audio.setView(0, 200);
    audio.handle(sound(1, 150, 100));
    await flushAudio();
    expect(context.panners).toBe(1);
    expect(context.panValues[0].value).toBe(0.5);
    audio.destroy();
  });

  it("stops voices and drops cues while simulation playback is paused", async () => {
    const context = new FakeAudioContext();
    vi.stubGlobal("AudioContext", class { constructor() { return context; } });
    const audio = await runtimeAudio();
    await audio.unlock();
    audio.handle(sound(1));
    audio.handle(speech(1));
    await flushAudio();
    expect(context.active).toBe(2);
    await audio.setPaused(true);
    expect(context.state).toBe("suspended");
    expect(context.active).toBe(0);
    audio.handle(sound(2));
    audio.handle(speech(2));
    await audio.setPaused(false);
    await flushAudio();
    expect(context.starts).toBe(2);
    audio.handle(sound(3));
    await flushAudio();
    expect(context.starts).toBe(3);
    audio.destroy();
  });

  it("ends rapid pause and resume transitions with a running context", async () => {
    const context = new DelayedSuspendAudioContext();
    vi.stubGlobal("AudioContext", class { constructor() { return context; } });
    const audio = await runtimeAudio();
    await audio.unlock();

    const pausing = audio.setPaused(true);
    await Promise.resolve();
    const resuming = audio.setPaused(false);
    context.finishSuspend();
    await Promise.all([pausing, resuming]);

    expect(context.state).toBe("running");
    audio.handle(sound(1));
    await flushAudio();
    expect(context.starts).toBe(1);
    audio.destroy();
  });
});
