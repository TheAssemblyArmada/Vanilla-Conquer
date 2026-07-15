import type { RuntimeTheaterV1 } from "./runtimeCatalog";

export const MAX_RUNTIME_SCENARIO_INI_BYTES = 1024 * 1024;

const THEATERS: Readonly<Record<string, RuntimeTheaterV1>> = {
  TEMPERATE: "temperate",
  DESERT: "desert",
  WINTER: "winter",
};

/** Mirrors the native fixed-name [Map] Theater parser without deriving paths. */
export function parseRuntimeScenarioTheater(data: Uint8Array): RuntimeTheaterV1 {
  if (data.byteLength === 0 || data.byteLength > MAX_RUNTIME_SCENARIO_INI_BYTES) {
    throw new Error(`Runtime scenario INI must contain 1 to ${MAX_RUNTIME_SCENARIO_INI_BYTES} bytes`);
  }
  // Released classic INIs can contain legacy single-byte text outside the
  // ASCII structural fields. Replacement decoding preserves the fixed
  // [Map]/Theater tokens and never turns non-ASCII bytes into path input.
  let contents = new TextDecoder("utf-8").decode(data);
  if (contents.charCodeAt(0) === 0xfeff) contents = contents.slice(1);
  let inMap = false;
  const declarations: string[] = [];
  for (const rawLine of contents.split("\n")) {
    const comment = rawLine.search(/[;#]/);
    const line = (comment < 0 ? rawLine : rawLine.slice(0, comment)).trim();
    if (line.startsWith("[")) {
      inMap = false;
      if (line.length >= 2 && line.endsWith("]")) inMap = line.slice(1, -1).trim().toUpperCase() === "MAP";
      continue;
    }
    if (!inMap || !line) continue;
    const separator = line.indexOf("=");
    if (separator >= 0 && line.slice(0, separator).trim().toUpperCase() === "THEATER") {
      declarations.push(line.slice(separator + 1).trim());
    }
  }
  if (declarations.length !== 1) throw new Error("Runtime scenario INI [Map] section must declare Theater exactly once");
  const theater = THEATERS[declarations[0].toUpperCase()];
  if (!theater) throw new Error("Runtime scenario INI [Map] Theater must be TEMPERATE, DESERT, or WINTER");
  return theater;
}
