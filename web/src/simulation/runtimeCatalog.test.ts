// @vitest-environment node
import { readFile } from "node:fs/promises";
import { describe, expect, it } from "vitest";
import type { ContentRevisionDescriptor } from "../storage/ContentStore";
import { Faction } from "./protocol";
import {
  contentIdHashFromRevision,
  contentMountRoot,
  MAX_RUNTIME_CATALOG_BYTES,
  missionStartConfiguration,
  parseRuntimeCatalog,
  type RuntimeCatalogV1,
  type RuntimeMissionV1,
} from "./runtimeCatalog";

const legacyGdi1 = {
  format: "cncweb-runtime",
  version: 1,
  engine: "tiberian-dawn",
  engineRoot: "engine/td",
  missions: [{
    id: "gdi-01-east-a",
    scenario: 1,
    variation: 0,
    direction: 0,
    buildLevel: 1,
    sabotagedStructure: -1,
    faction: "gdi",
    title: "GDI Mission 1",
    briefing: "Synthetic briefing.",
  }],
};

const generalizedMissions = [
  {
    ...legacyGdi1.missions[0],
    scenarioRoot: "SCG01EA",
    theater: "temperate",
  },
  {
    id: "gdi-02-west-b",
    scenarioRoot: "SCG02WB",
    scenario: 2,
    variation: 1,
    direction: 1,
    buildLevel: 2,
    sabotagedStructure: 12,
    faction: "gdi",
    title: "GDI Mission 2",
    briefing: "A second synthetic briefing.",
    theater: "desert",
  },
  {
    id: "nod-01-west-l",
    scenarioRoot: "SCB01WL",
    scenario: 1,
    variation: 5,
    direction: 1,
    buildLevel: 1,
    sabotagedStructure: -1,
    faction: "nod",
    title: "Nod Mission 1",
    briefing: "A synthetic Nod briefing.",
    theater: "desert",
  },
] as const;

const generalized = {
  ...legacyGdi1,
  missions: generalizedMissions,
};

const encode = (value: unknown) => new TextEncoder().encode(JSON.stringify(value));

function descriptor(revision = "1234567890abcdef" + "00".repeat(24)): ContentRevisionDescriptor {
  return {
    id: "owned-pack",
    revision,
    storageKey: `${revision}-00000000-0000-4000-8000-000000000000`,
  } as ContentRevisionDescriptor;
}

function replaceMission(mission: Record<string, unknown>): unknown {
  return { ...generalized, missions: [mission] };
}

describe("runtime mission catalog", () => {
  it("accepts the content-packer catalog fixture through the web consumer", async () => {
    const fixture = await readFile(new URL("../../../tools/content-packer/fixtures/runtime-catalog-v1.example.json", import.meta.url));
    expect(parseRuntimeCatalog(fixture).missions[0]).toMatchObject({
      id: "gdi-01-east-a",
      scenarioRoot: "SCG01EA",
      theater: "temperate",
    });
  });

  it("accepts and normalizes the installed legacy GDI1 v1 fixture shape", () => {
    const catalog = parseRuntimeCatalog(encode(legacyGdi1));
    expect(catalog.missions).toEqual([{
      ...legacyGdi1.missions[0],
      scenarioRoot: "SCG01EA",
      theater: "temperate",
    }]);
  });

  it("accepts ordered GDI and Nod missions across temperate and desert theaters", () => {
    const catalog = parseRuntimeCatalog(encode(generalized));
    expect(catalog.missions.map(({ id, scenarioRoot, theater }) => ({ id, scenarioRoot, theater }))).toEqual([
      { id: "gdi-01-east-a", scenarioRoot: "SCG01EA", theater: "temperate" },
      { id: "gdi-02-west-b", scenarioRoot: "SCG02WB", theater: "desert" },
      { id: "nod-01-west-l", scenarioRoot: "SCB01WL", theater: "desert" },
    ]);

    const start = missionStartConfiguration(descriptor(), catalog, { ...catalog.missions[2] }, 7);
    expect(start).toMatchObject({
      seed: 7,
      scenario: 1,
      variation: 5,
      direction: 1,
      buildLevel: 1,
      sabotagedStructure: -1,
      faction: Faction.Nod,
      contentDirectory: "/cnc-content/1234567890abcdef/engine/td",
      contentIdHash: 0x1234567890abcdefn,
    });
  });

  it("enforces catalog cardinality, unique IDs, and unique scenario roots", () => {
    expect(() => parseRuntimeCatalog(encode({ ...generalized, missions: [] }))).toThrow("1 to 256 missions");
    const maximum = Array.from({ length: 256 }, (_, index) => {
      const scenario = index + 1;
      return {
        id: `mission-${scenario}`,
        scenarioRoot: `SCG${String(scenario).padStart(2, "0")}EA`,
        scenario,
        variation: 0,
        direction: 0,
        buildLevel: Math.min(255, scenario),
        sabotagedStructure: -1,
        faction: "gdi",
        title: "M",
        briefing: "B",
        theater: "temperate",
      };
    });
    const maximumData = encode({ ...generalized, missions: maximum });
    expect(maximumData.byteLength).toBeLessThanOrEqual(MAX_RUNTIME_CATALOG_BYTES);
    expect(parseRuntimeCatalog(maximumData).missions).toHaveLength(256);
    expect(() => parseRuntimeCatalog(encode({ ...generalized, missions: [...maximum, { ...maximum[0], id: "overflow", scenario: 257, scenarioRoot: "SCG257EA" }] }))).toThrow("1 to 256 missions");
    expect(() => parseRuntimeCatalog(encode({ ...generalized, missions: [generalizedMissions[0], { ...generalizedMissions[1], id: generalizedMissions[0].id }] }))).toThrow("ID is invalid or duplicated");
    expect(() => parseRuntimeCatalog(encode({ ...generalized, missions: [generalizedMissions[0], { ...generalizedMissions[0], id: "duplicate-root" }] }))).toThrow("scenarioRoot is duplicated");
    expect(() => parseRuntimeCatalog(encode({ ...generalized, missions: [{ ...generalizedMissions[0], id: 1 }] }))).toThrow("ID is invalid or duplicated");
    expect(() => parseRuntimeCatalog(encode({ ...generalized, missions: [{ ...generalizedMissions[0], id: null }] }))).toThrow("ID is invalid or duplicated");
  });

  it("requires canonical scenario roots and bounded engine launch fields", () => {
    const base = generalizedMissions[0];
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, scenarioRoot: "../SCG01EA" })))).toThrow("scenarioRoot is invalid");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, scenarioRoot: "SCB01EA" })))).toThrow("does not match its launch fields");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, scenario: 0 })))).toThrow("between 1 and 999");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, scenario: 1_000, scenarioRoot: "SCG1000EA" })))).toThrow("scenarioRoot is invalid");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, variation: 4 })))).toThrow("variation must be");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, direction: 2 })))).toThrow("between 0 and 1");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, buildLevel: 256 })))).toThrow("between 0 and 255");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, sabotagedStructure: -2 })))).toThrow("between -1 and 255");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, faction: "jurassic" })))).toThrow("faction is unsupported");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, theater: "jungle" })))).toThrow("theater is unsupported");
  });

  it("rejects missing, extra, unsafe, and oversized v1 data", () => {
    const base = generalizedMissions[0];
    const { theater: _theater, ...missingTheater } = base;
    expect(() => parseRuntimeCatalog(encode(replaceMission(missingTheater)))).toThrow("missing or unknown fields");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, unexpected: true })))).toThrow("missing or unknown fields");
    expect(() => parseRuntimeCatalog(encode({ ...generalized, unexpected: true }))).toThrow("missing or unknown fields");
    expect(() => parseRuntimeCatalog(encode({ ...generalized, engineRoot: "../engine/td" }))).toThrow("Invalid content path");
    expect(() => parseRuntimeCatalog(encode({ ...generalized, engineRoot: "engine/other" }))).toThrow("must be engine/td");
    expect(() => parseRuntimeCatalog(new Uint8Array())).toThrow("1 to 65536 bytes");
    expect(() => parseRuntimeCatalog(new Uint8Array(MAX_RUNTIME_CATALOG_BYTES + 1))).toThrow("1 to 65536 bytes");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, briefing: "x".repeat(4097) })))).toThrow("briefing");
    expect(() => parseRuntimeCatalog(encode(replaceMission({ ...base, briefing: "owned@briefing" })))).toThrow("briefing");
  });

  it("allows the legacy field set only for the canonical single GDI1 launch", () => {
    expect(() => parseRuntimeCatalog(encode({ ...legacyGdi1, missions: [{ ...legacyGdi1.missions[0], id: "gdi-01-renamed" }] }))).toThrow("only supported for canonical GDI Mission 1");
    expect(() => parseRuntimeCatalog(encode({ ...legacyGdi1, missions: [{ ...legacyGdi1.missions[0], title: "Renamed" }] }))).toThrow("only supported for canonical GDI Mission 1");
    expect(() => parseRuntimeCatalog(encode({ ...legacyGdi1, missions: [...legacyGdi1.missions, legacyGdi1.missions[0]] }))).toThrow("only supported for canonical GDI Mission 1");
  });

  it("binds launch identity to the selected manifest revision and exact catalog mission", () => {
    const catalog = parseRuntimeCatalog(encode(generalized));
    expect(() => missionStartConfiguration(descriptor(), catalog, { ...catalog.missions[0], buildLevel: 99 }, 1)).toThrow("not part of the selected runtime catalog");
    expect(() => missionStartConfiguration(descriptor("ABCDEF".repeat(10) + "ABCD"), catalog, catalog.missions[0], 1)).toThrow("lowercase SHA-256");
    expect(missionStartConfiguration(descriptor(), catalog, catalog.missions[0], 0)).toMatchObject({ seed: 1, faction: Faction.Gdi });
  });

  it("derives a stable nonzero mount identity from the manifest revision", () => {
    const zeroPrefix = "0".repeat(16) + "ab".repeat(24);
    expect(contentIdHashFromRevision(zeroPrefix)).toBe(1n);
    expect(contentMountRoot(zeroPrefix)).toBe("/cnc-content/0000000000000000");
  });
});
