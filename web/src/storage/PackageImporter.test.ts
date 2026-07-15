// @vitest-environment node
import { webcrypto } from "node:crypto";
import { readFile } from "node:fs/promises";
import { BlobWriter, TextReader, ZipWriter } from "@zip.js/zip.js";
import { afterAll, beforeAll, describe, expect, it, vi } from "vitest";
import { MemoryBinaryStore } from "./BinaryStore";
import { ContentStore, type ContentManifest } from "./ContentStore";
import { importCncwebPackage, packageInstallQuotaBytes } from "./PackageImporter";

const originalCrypto = Object.getOwnPropertyDescriptor(globalThis, "crypto");
const originalNavigator = Object.getOwnPropertyDescriptor(globalThis, "navigator");

beforeAll(() => {
  Object.defineProperty(globalThis, "crypto", { value: webcrypto, configurable: true });
});

afterAll(() => {
  if (originalCrypto) Object.defineProperty(globalThis, "crypto", originalCrypto);
  else Reflect.deleteProperty(globalThis, "crypto");
  if (originalNavigator) Object.defineProperty(globalThis, "navigator", originalNavigator);
  else Reflect.deleteProperty(globalThis, "navigator");
});

describe("browser package importer", () => {
  it("installs a ZIP carrying the canonical Rust manifest contract with one-candidate quota", async () => {
    const fixtureUrl = new URL("../../../tools/content-packer/fixtures/manifest-v1.example.json", import.meta.url);
    const manifestText = await readFile(fixtureUrl, "utf8");
    const manifest = JSON.parse(manifestText) as ContentManifest;
    const content = "{}\n";
    const contentBytes = new TextEncoder().encode(content);
    const manifestBytes = new TextEncoder().encode(manifestText);
    const output = new BlobWriter("application/zip");
    const zip = new ZipWriter(output, { level: 6, useWebWorkers: false });
    await zip.add("manifest.json", new TextReader(manifestText));
    await zip.add("config/example.json", new TextReader(content));
    const archive = await zip.close();

    const estimate = vi.fn(async () => ({
      usage: 0,
      quota: packageInstallQuotaBytes(contentBytes.byteLength, manifestBytes.byteLength),
    }));
    Object.defineProperty(globalThis, "navigator", {
      value: { storage: { getDirectory: vi.fn(), estimate, persist: vi.fn(async () => false) } },
      configurable: true,
    });

    const binary = new MemoryBinaryStore();
    const progress = vi.fn();
    const installed = await importCncwebPackage(archive, new ContentStore(binary), progress);

    expect(installed.id).toBe(manifest.package_id);
    expect(await new ContentStore(binary).readFile(installed.id, "config/example.json")).toEqual(contentBytes);
    expect(progress).toHaveBeenLastCalledWith(1, 1);
    expect(estimate).toHaveBeenCalledOnce();
    expect(await binary.list("content-staging")).toEqual([]);
    expect(await binary.list("content-index")).toEqual([`content-index/${manifest.package_id}.json`]);
  });

  it("rejects bootstrap identity constraints before committing content", async () => {
    const fixtureUrl = new URL("../../../tools/content-packer/fixtures/manifest-v1.example.json", import.meta.url);
    const manifestText = await readFile(fixtureUrl, "utf8");
    const manifest = JSON.parse(manifestText) as ContentManifest;
    const output = new BlobWriter("application/zip");
    const zip = new ZipWriter(output, { level: 6, useWebWorkers: false });
    await zip.add("manifest.json", new TextReader(manifestText));
    await zip.add("config/example.json", new TextReader("{}\n"));
    const archive = await zip.close();
    Object.defineProperty(globalThis, "navigator", {
      value: {
        storage: {
          getDirectory: vi.fn(),
          estimate: vi.fn(async () => ({ usage: 0, quota: 1024 * 1024 })),
          persist: vi.fn(async () => false),
        },
      },
      configurable: true,
    });
    const binary = new MemoryBinaryStore();

    await expect(importCncwebPackage(
      archive,
      new ContentStore(binary),
      undefined,
      undefined,
      {
        packageId: "classic-freeware-gdi",
        contentSha256: manifest.content_sha256,
        sourceProduct: "tiberian-dawn-freeware",
        sourceProvider: "ea-freeware",
      },
    )).rejects.toThrow(/ID does not match/i);
    expect(await binary.list("content-index")).toEqual([]);
    expect(await binary.list("content")).toEqual([]);
  });
});
