import { describe, expect, it } from "vitest";
import { CNC_WEB_MAGIC_MESSAGE, MessageKind, SIMULATION_PROTOCOL_VERSION } from "./protocol";
import { advancedOneTick, assertWasmStatus, isFatalWasmCoreError, parseWasmEvent, resolveEmscriptenModuleUrl, WasmCore } from "./WasmCore";

function eventBuffer(eventType: number, flags: number, args: number[], text1 = "", text2 = ""): ArrayBuffer {
  const encoder = new TextEncoder();
  const first = encoder.encode(text1);
  const second = encoder.encode(text2);
  const buffer = new ArrayBuffer(64 + first.length + second.length);
  const view = new DataView(buffer);
  view.setUint32(0, CNC_WEB_MAGIC_MESSAGE, true);
  view.setUint16(4, SIMULATION_PROTOCOL_VERSION, true);
  view.setUint16(6, MessageKind.Event, true);
  view.setUint32(8, buffer.byteLength, true);
  view.setUint32(12, 1, true);
  view.setUint32(16, 99, true);
  view.setUint16(20, eventType, true);
  view.setUint16(22, flags, true);
  args.forEach((argument, index) => view.setInt32(32 + index * 4, argument, true));
  view.setUint32(56, first.length, true);
  view.setUint32(60, second.length, true);
  new Uint8Array(buffer, 64).set(first);
  new Uint8Array(buffer, 64 + first.length).set(second);
  return buffer;
}

describe("Emscripten engine module URLs", () => {
  it("resolves generated JavaScript loaders and preserves query strings", () => {
    expect(resolveEmscriptenModuleUrl("./engine/tiberiandawn.js?v=42", "https://example.test/play/").href)
      .toBe("https://example.test/play/engine/tiberiandawn.js?v=42");
    expect(resolveEmscriptenModuleUrl("engine/core.mjs", "https://example.test/").href)
      .toBe("https://example.test/engine/core.mjs");
  });

  it("rejects a raw WebAssembly binary before attempting to load it", async () => {
    await expect(WasmCore.create("./engine/tiberiandawn.wasm?build=1"))
      .rejects.toThrow("Raw .wasm engines are not supported");
  });

  it.each(["", "./engine/tiberiandawn", "./engine/tiberiandawn.cjs"])(
    "rejects unsupported module URL %j",
    (url) => {
      expect(() => resolveEmscriptenModuleUrl(url, "https://example.test/")).toThrow(/required|must end in \.js or \.mjs/);
    },
  );

  it("rejects executable URL schemes", () => {
    expect(() => resolveEmscriptenModuleUrl("javascript:payload.js", "https://example.test/"))
      .toThrow("uses unsupported protocol javascript:");
  });
});

describe("terminal advance results", () => {
  it("treats zero advanced ticks as a durable terminal state", () => {
    expect(advancedOneTick(1)).toBe(true);
    expect(advancedOneTick(0)).toBe(false);
    expect(() => advancedOneTick(2)).toThrow("one-tick request");
  });
});

describe("Wasm status failures", () => {
  function statusError(status: number): unknown {
    try { assertWasmStatus(status, "load save"); }
    catch (error) { return error; }
    throw new Error("Expected a failing status");
  }

  it("distinguishes a recoverable rejected load from an unusable legacy state", () => {
    expect(isFatalWasmCoreError(statusError(5))).toBe(false);
    expect(isFatalWasmCoreError(statusError(7))).toBe(true);
    expect(isFatalWasmCoreError(statusError(99))).toBe(true);
    expect(isFatalWasmCoreError(statusError(Number.NaN))).toBe(true);
    expect(() => assertWasmStatus(0, "load save")).not.toThrow();
  });
});

describe("EventV1 translation", () => {
  it("preserves named audio, distinct debug text, and full game-over results", () => {
    expect(parseWasmEvent(eventBuffer(1, 0, [51, 2, 100, 200, 3, 4], "MGUN2"))).toMatchObject({ kind: "sound", name: "MGUN2", variation: 2, x: 100, y: 200, priority: 3, context: 4 });
    expect(parseWasmEvent(eventBuffer(4, 0, [0, 0, 0, 0, 0, 0], "engine trace"))).toEqual({ kind: "debug", tick: 99, text: "engine trace" });
    expect(parseWasmEvent(eventBuffer(3, 6, [1000, 70, 80, 900, -1, 120], "WIN", "SCORE"))).toMatchObject({ kind: "game-over", human: true, won: true, score: 1000, leadership: 70, efficiency: 80, remainingCredits: 900, sabotagedStructure: -1, timerRemaining: 120, movieName: "WIN", afterScoreMovieName: "SCORE" });
  });

  it("decodes bounded campaign continuation state without losing unsigned RNG bits", () => {
    expect(parseWasmEvent(eventBuffer(15, 6, [750, 5, 11, -1, 6, 0], "SCG06EA"))).toEqual({
      kind: "campaign-outcome",
      tick: 99,
      carryOverCredits: 750,
      nukePieces: 5,
      sabotagedStructure: 11,
      randomSeed: 0xffff_ffff,
      scenario: 6,
      house: 0,
      scenarioRoot: "SCG06EA",
    });
    expect(() => parseWasmEvent(eventBuffer(15, 6, [0, 8, -1, 1, 1, 0], "SCG01EA"))).toThrow("invalid campaign outcome");
    expect(() => parseWasmEvent(eventBuffer(15, 6, [0, 0, -1, 1, 1, 0], "../SCG01EA"))).toThrow("invalid campaign outcome");
    expect(() => parseWasmEvent(eventBuffer(15, 1, [0, 0, -1, 1, 1, 0], "SCG01EA"))).toThrow("invalid campaign outcome");
  });

  it("exposes structured runtime start diagnostics", () => {
    expect(parseWasmEvent(eventBuffer(14, 3, [4, 4, 1, 0, 0, 1], "runtime.content.missing", "/engine/td/GENERAL.MIX"))).toMatchObject({ kind: "diagnostic", warning: true, error: true, code: 4, status: 4, id: "runtime.content.missing", detail: "/engine/td/GENERAL.MIX" });
  });

  it("normalizes camera focus and ping events as absolute world pixels", () => {
    expect(parseWasmEvent(eventBuffer(12, 0, [432, 288, 0, 0, 0, 0])))
      .toEqual({ kind: "camera", tick: 99, x: 432, y: 288 });
    expect(parseWasmEvent(eventBuffer(13, 0, [120, 744, 0, 0, 0, 0])))
      .toEqual({ kind: "ping", tick: 99, x: 120, y: 744 });
  });
});
