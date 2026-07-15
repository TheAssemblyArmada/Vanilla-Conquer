import { describe, expect, it } from "vitest";
import { MAX_RUNTIME_SCENARIO_INI_BYTES, parseRuntimeScenarioTheater } from "./runtimeTheater";

const encode = (value: string) => new TextEncoder().encode(value);

describe("runtime scenario theater", () => {
  it.each([
    ["[Map]\nTheater=TEMPERATE\n", "temperate"],
    ["\ufeff [ map ]\r\n theater = desert ; owned comment\r\n", "desert"],
    ["# comment\n[Basic]\nName=X\n[MAP] # comment\nTHEATER = winter\n", "winter"],
  ] as const)("parses fixed supported theater metadata", (ini, expected) => {
    expect(parseRuntimeScenarioTheater(encode(ini))).toBe(expected);
  });

  it("rejects ambiguous, unknown, malformed, and oversized metadata", () => {
    expect(() => parseRuntimeScenarioTheater(encode("[Map]\nTheater=DESERT\nTheater=WINTER\n"))).toThrow("exactly once");
    expect(() => parseRuntimeScenarioTheater(encode("[Map]\nTheater=../DESERT\n"))).toThrow("must be");
    expect(() => parseRuntimeScenarioTheater(encode("[Map\nTheater=DESERT\n"))).toThrow("exactly once");
    expect(() => parseRuntimeScenarioTheater(new Uint8Array())).toThrow("1 to");
    expect(() => parseRuntimeScenarioTheater(new Uint8Array(MAX_RUNTIME_SCENARIO_INI_BYTES + 1))).toThrow("1 to");
  });

  it("ignores legacy non-UTF-8 prose while parsing ASCII theater metadata", () => {
    const prefix = encode("[Basic]\nName=");
    const suffix = encode("\n[Map]\nTheater=WINTER\n");
    const ini = new Uint8Array(prefix.byteLength + 1 + suffix.byteLength);
    ini.set(prefix);
    ini[prefix.byteLength] = 0x96;
    ini.set(suffix, prefix.byteLength + 1);
    expect(parseRuntimeScenarioTheater(ini)).toBe("winter");
  });
});
