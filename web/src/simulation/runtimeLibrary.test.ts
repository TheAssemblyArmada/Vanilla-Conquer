// @vitest-environment node
import { describe, expect, it, vi } from "vitest";
import { RUNTIME_AUDIO_PATH } from "../audio/RuntimeAudio";
import type { ContentFileDescriptor, ContentRevisionDescriptor, ContentStore } from "../storage/ContentStore";
import { parseRuntimeCatalog, RUNTIME_CATALOG_PATH, type RuntimeCatalogV1 } from "./runtimeCatalog";
import {
  assertFirstPlayableRuntimeLayout,
  assertRuntimeLayout,
  inspectRuntimePack,
  loadRuntimeLibrary,
  requiredRuntimeEngineFiles,
  REQUIRED_TD_ENGINE_FILES,
  TD_THEATER_FILES,
} from "./runtimeLibrary";

const catalogValue = {
  format: "cncweb-runtime",
  version: 1,
  engine: "tiberian-dawn",
  engineRoot: "engine/td",
  missions: [
    {
      id: "gdi-01-east-a",
      scenarioRoot: "SCG01EA",
      scenario: 1,
      variation: 0,
      direction: 0,
      buildLevel: 1,
      sabotagedStructure: -1,
      faction: "gdi",
      title: "GDI Mission 1",
      briefing: "Briefing.",
      theater: "temperate",
    },
    {
      id: "gdi-02-west-b",
      scenarioRoot: "SCG02WB",
      scenario: 2,
      variation: 1,
      direction: 1,
      buildLevel: 2,
      sabotagedStructure: 7,
      faction: "gdi",
      title: "GDI Mission 2",
      briefing: "Second briefing.",
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
      briefing: "Nod briefing.",
      theater: "desert",
    },
  ],
};

const legacyCatalogValue = {
  ...catalogValue,
  missions: [{
    id: "gdi-01-east-a",
    scenario: 1,
    variation: 0,
    direction: 0,
    buildLevel: 1,
    sabotagedStructure: -1,
    faction: "gdi",
    title: "GDI Mission 1",
    briefing: "Briefing.",
  }],
};

const encode = (value: unknown) => new TextEncoder().encode(JSON.stringify(value));
const encodeText = (value: string) => new TextEncoder().encode(value);

function catalog(value: unknown = catalogValue): RuntimeCatalogV1 {
  return parseRuntimeCatalog(encode(value));
}

function descriptor(runtimeCatalog = catalog()): ContentRevisionDescriptor {
  const engineFiles = requiredRuntimeEngineFiles(runtimeCatalog).map((name): ContentFileDescriptor => ({
    path: `engine/td/${name}`,
    size: name.endsWith(".BIN") ? 8192 : 1024,
    sha256: "03".repeat(32),
    role: "engine-data",
  }));
  const files: ContentFileDescriptor[] = [
    { path: RUNTIME_AUDIO_PATH, size: 128, sha256: "01".repeat(32), role: "configuration" },
    { path: RUNTIME_CATALOG_PATH, size: 256, sha256: "02".repeat(32), role: "configuration" },
    ...engineFiles,
  ];
  files.sort((left, right) => left.path.localeCompare(right.path));
  return {
    id: "owned-pack",
    revision: "05".repeat(32),
    storageKey: `${"05".repeat(32)}-test`,
    installedAt: new Date(0).toISOString(),
    manifest: {
      format: "cncweb-content",
      version: 1,
      package_id: "owned-pack",
      created_at_unix_ms: 0,
      source: { product: "cnc-remastered-collection", provider: "copied-installation", install_fingerprint_sha256: "06".repeat(32) },
      content: { games: ["tiberian-dawn"], locales: ["en-US"] },
      content_sha256: "07".repeat(32),
      files,
    },
  };
}

describe("runtime library catalog coverage", () => {
  it("derives every mission pair and the union of temperate/desert theater archives", () => {
    const runtimeCatalog = catalog();
    expect(REQUIRED_TD_ENGINE_FILES).toEqual([
      "CCLOCAL.MIX",
      "CONQUER.MIX",
      "GENERAL.MIX",
      "SOUNDS.MIX",
      "SPEECH.MIX",
      "TRANSIT.MIX",
      "UPDATEC.MIX",
    ]);
    expect(requiredRuntimeEngineFiles(runtimeCatalog)).toEqual([
      ...REQUIRED_TD_ENGINE_FILES,
      "SCG01EA.INI",
      "SCG01EA.BIN",
      "SCG02WB.INI",
      "SCG02WB.BIN",
      "SCB01WL.INI",
      "SCB01WL.BIN",
      ...TD_THEATER_FILES.temperate.archives,
      ...TD_THEATER_FILES.desert.archives,
    ]);
    expect(assertRuntimeLayout(descriptor(runtimeCatalog), runtimeCatalog)).toEqual([
      "MOVIES.MIX is unavailable; movie requests will be reported without playback",
      "Loose TEMPERAT.PAL is unavailable; the temperate palette will be loaded from MIX content",
      "Loose DESERT.PAL is unavailable; the desert palette will be loaded from MIX content",
    ]);
  });

  it("retains legacy GDI1 layout compatibility after catalog normalization", () => {
    const legacy = catalog(legacyCatalogValue);
    expect(requiredRuntimeEngineFiles(legacy)).toEqual([
      ...REQUIRED_TD_ENGINE_FILES,
      "SCG01EA.INI",
      "SCG01EA.BIN",
      ...TD_THEATER_FILES.temperate.archives,
    ]);
    expect(assertFirstPlayableRuntimeLayout(descriptor(legacy), legacy)).toHaveLength(2);
  });

  it("requires coverage for every mission and each theater exactly once", () => {
    const runtimeCatalog = catalog();
    const missingSecondMission = descriptor(runtimeCatalog);
    missingSecondMission.manifest.files = missingSecondMission.manifest.files.filter((file) => file.path !== "engine/td/SCG02WB.INI");
    expect(() => assertRuntimeLayout(missingSecondMission, runtimeCatalog)).toThrow("engine/td/SCG02WB.INI");

    const missingDesertArchive = descriptor(runtimeCatalog);
    missingDesertArchive.manifest.files = missingDesertArchive.manifest.files.filter((file) => file.path !== "engine/td/DESEICNH.MIX");
    expect(() => assertRuntimeLayout(missingDesertArchive, runtimeCatalog)).toThrow("engine/td/DESEICNH.MIX");

    const files = requiredRuntimeEngineFiles(runtimeCatalog);
    expect(files.filter((name) => name === "DESERT.MIX")).toHaveLength(1);
    expect(files.filter((name) => name === "DESEICNH.MIX")).toHaveLength(1);
  });

  it("validates base/theater roles, every scenario size, and optional palettes", () => {
    const runtimeCatalog = catalog();
    const wrongRole = descriptor(runtimeCatalog);
    wrongRole.manifest.files.find((file) => file.path === "engine/td/CONQUER.MIX")!.role = "other";
    expect(() => assertRuntimeLayout(wrongRole, runtimeCatalog)).toThrow("engine file");

    const wrongMapSize = descriptor(runtimeCatalog);
    wrongMapSize.manifest.files.find((file) => file.path === "engine/td/SCB01WL.BIN")!.size = 4096;
    expect(() => assertRuntimeLayout(wrongMapSize, runtimeCatalog)).toThrow("scenario map");

    const emptyIni = descriptor(runtimeCatalog);
    emptyIni.manifest.files.find((file) => file.path === "engine/td/SCG02WB.INI")!.size = 0;
    expect(() => assertRuntimeLayout(emptyIni, runtimeCatalog)).toThrow("scenario INI");

    const withPalettes = descriptor(runtimeCatalog);
    withPalettes.manifest.files.push(
      { path: "engine/td/TEMPERAT.PAL", size: 768, sha256: "08".repeat(32), role: "engine-data" },
      { path: "engine/td/DESERT.PAL", size: 768, sha256: "09".repeat(32), role: "engine-data" },
    );
    expect(assertRuntimeLayout(withPalettes, runtimeCatalog)).toEqual([
      "MOVIES.MIX is unavailable; movie requests will be reported without playback",
    ]);
    withPalettes.manifest.files.find((file) => file.path === "engine/td/DESERT.PAL")!.size = 767;
    expect(() => assertRuntimeLayout(withPalettes, runtimeCatalog)).toThrow("Optional runtime palette");
  });

  it("maps the supported winter theater to its archive pair", () => {
    const winter = catalog({
      ...catalogValue,
      missions: [{
        ...catalogValue.missions[0],
        id: "gdi-03-east-c",
        scenarioRoot: "SCG03EC",
        scenario: 3,
        variation: 2,
        buildLevel: 3,
        theater: "winter",
      }],
    });
    expect(requiredRuntimeEngineFiles(winter)).toEqual([
      ...REQUIRED_TD_ENGINE_FILES,
      "SCG03EC.INI",
      "SCG03EC.BIN",
      ...TD_THEATER_FILES.winter.archives,
    ]);
    expect(assertRuntimeLayout(descriptor(winter), winter)).toContain("Loose WINTER.PAL is unavailable; the winter palette will be loaded from MIX content");
  });

  it("rejects a catalog whose manifest descriptor is not bounded configuration data", async () => {
    const invalid = descriptor();
    invalid.manifest.files.find((file) => file.path === RUNTIME_CATALOG_PATH)!.role = "other";
    const readRevisionFile = vi.fn();
    const store = { getRevisionDescriptor: vi.fn(async () => invalid), readRevisionFile } as unknown as ContentStore;
    await expect(inspectRuntimePack(store, invalid.id)).rejects.toThrow("catalog does not match the manifest");
    expect(readRevisionFile).not.toHaveBeenCalled();
  });

  it("reads catalog metadata through the selected immutable revision descriptor", async () => {
    const selected = descriptor();
    const readRevisionFile = vi.fn(async (_descriptor: ContentRevisionDescriptor, path: string) => path === RUNTIME_CATALOG_PATH
      ? encode(catalogValue)
      : encodeText(`[Map]\nTheater=${path.includes("SCG01") ? "TEMPERATE" : "DESERT"}\n`));
    const store = { getRevisionDescriptor: vi.fn(async () => selected), readRevisionFile } as unknown as ContentStore;
    await expect(inspectRuntimePack(store, selected.id)).rejects.toThrow("Runtime audio index");
    expect(readRevisionFile).toHaveBeenCalledWith(selected, RUNTIME_CATALOG_PATH);
  });

  it("cross-checks catalog theater metadata against every immutable scenario INI", async () => {
    const selected = descriptor();
    const readRevisionFile = vi.fn(async (_descriptor: ContentRevisionDescriptor, path: string) => path === RUNTIME_CATALOG_PATH
      ? encode(catalogValue)
      : path.endsWith("SCG01EA.INI") ? encodeText("[Map]\nTheater=DESERT\n") : encodeText("[Map]\nTheater=DESERT\n"));
    const store = { getRevisionDescriptor: vi.fn(async () => selected), readRevisionFile } as unknown as ContentStore;
    await expect(inspectRuntimePack(store, selected.id)).rejects.toThrow("catalog theater does not match scenario INI");
    expect(readRevisionFile).toHaveBeenCalledWith(selected, "engine/td/SCG01EA.INI");
  });

  it("reports an unreadable package index without failing library startup", async () => {
    const store = {
      listWithIssues: vi.fn(async () => ({
        installed: [],
        issues: [{ indexPath: "content-index/damaged-pack.json", id: "damaged-pack", revision: "09".repeat(32), reason: "synthetic torn index" }],
      })),
    } as unknown as ContentStore;

    await expect(loadRuntimeLibrary(store)).resolves.toEqual({
      compatible: [],
      incompatible: [{ id: "damaged-pack", revision: "09".repeat(32), reason: "Stored package index content-index/damaged-pack.json could not be loaded: synthetic torn index" }],
    });
  });
});
