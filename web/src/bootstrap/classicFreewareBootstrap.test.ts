// @vitest-environment node
import { createHash, webcrypto } from "node:crypto";
import { beforeAll, describe, expect, it, vi } from "vitest";
import {
  bootstrapClassicFreeware,
  CLASSIC_FREEWARE_DESCRIPTOR_FILENAME,
  fetchClassicFreewareDescriptor,
  parseClassicFreewareDescriptor,
  type ClassicFreewareDescriptorV1,
  type ClassicFreewareFetch,
  type ClassicFreewareInstalledContent,
} from "./classicFreewareBootstrap";

const applicationUrl = "https://play.example.test/theater/";
const descriptorUrl = new URL(CLASSIC_FREEWARE_DESCRIPTOR_FILENAME, applicationUrl).href;
const archiveUrl = new URL("classic-freeware-td-v1.cncweb", descriptorUrl).href;
const archiveBytes = new TextEncoder().encode("synthetic classic freeware package");
const archiveSha256 = createHash("sha256").update(archiveBytes).digest("hex");
const contentSha256 = "ab".repeat(32);

beforeAll(() => {
  Object.defineProperty(globalThis, "crypto", { value: webcrypto, configurable: true });
});

function descriptor(overrides: Record<string, unknown> = {}) {
  return {
    format: "cncweb-classic-freeware",
    version: 1,
    package: {
      id: "classic-freeware-td-v1",
      contentSha256,
      source: { product: "tiberian-dawn-freeware", provider: "ea-freeware" },
      archive: {
        url: "classic-freeware-td-v1.cncweb",
        bytes: archiveBytes.byteLength,
        sha256: archiveSha256,
      },
    },
    ...overrides,
  };
}

function installed(overrides: Partial<ClassicFreewareInstalledContent["manifest"]> = {}): ClassicFreewareInstalledContent {
  return {
    id: "classic-freeware-td-v1",
    manifest: {
      package_id: "classic-freeware-td-v1",
      content_sha256: contentSha256,
      source: { product: "tiberian-dawn-freeware", provider: "ea-freeware" },
      ...overrides,
    },
  };
}

function responses(value = descriptor(), archive = archiveBytes): ClassicFreewareFetch {
  return vi.fn(async (input) => {
    const url = input instanceof URL ? input.href : input instanceof Request ? input.url : String(input);
    if (url === descriptorUrl) {
      return new Response(JSON.stringify(value), { headers: { "Content-Type": "application/json" } });
    }
    if (url === archiveUrl) return new Response(archive, { headers: { "Content-Type": "application/zip" } });
    return new Response(null, { status: 404 });
  });
}

describe("classic freeware descriptor", () => {
  it("strictly parses one same-origin package and resolves its archive URL", () => {
    const parsed = parseClassicFreewareDescriptor(descriptor(), { applicationUrl, descriptorUrl });
    expect(parsed).toMatchObject({
      format: "cncweb-classic-freeware",
      version: 1,
      package: {
        id: "classic-freeware-td-v1",
        contentSha256,
        source: { product: "tiberian-dawn-freeware", provider: "ea-freeware" },
        archive: { bytes: archiveBytes.byteLength, sha256: archiveSha256 },
      },
    });
    expect(parsed.package.archive.url.href).toBe(archiveUrl);
  });

  it("rejects unversioned descriptors, unknown fields, foreign archives, and wrong provenance", () => {
    expect(() => parseClassicFreewareDescriptor(descriptor(), {
      applicationUrl,
      descriptorUrl: new URL("classic-freeware.json", applicationUrl),
    })).toThrow(`must end with ${CLASSIC_FREEWARE_DESCRIPTOR_FILENAME}`);
    expect(() => parseClassicFreewareDescriptor({ ...descriptor(), extra: true }, { applicationUrl, descriptorUrl })).toThrow("unknown fields");
    expect(() => parseClassicFreewareDescriptor(descriptor({
      package: { ...descriptor().package, archive: { ...descriptor().package.archive, url: "https://cdn.example.test/pack.cncweb" } },
    }), { applicationUrl, descriptorUrl })).toThrow("same-origin");
    expect(() => parseClassicFreewareDescriptor(descriptor({
      package: { ...descriptor().package, source: { product: "cnc-remastered-collection", provider: "unknown" } },
    }), { applicationUrl, descriptorUrl })).toThrow("EA Tiberian Dawn freeware release");
  });

  it("fetches the fixed JSON descriptor without cache or redirect ambiguity", async () => {
    const fetcher = responses();
    const parsed = await fetchClassicFreewareDescriptor({ applicationUrl: `${applicationUrl}?acceptance=local#session`, fetcher });
    expect(parsed.package.id).toBe("classic-freeware-td-v1");
    expect(fetcher).toHaveBeenCalledWith(new URL(descriptorUrl), {
      cache: "no-store",
      credentials: "same-origin",
      headers: { Accept: "application/json" },
      redirect: "error",
    });
  });
});

describe("classic freeware bootstrap", () => {
  it("skips the archive download and importer for the exact installed package/content digest", async () => {
    const fetcher = responses();
    const current = installed();
    const importPackage = vi.fn();
    const result = await bootstrapClassicFreeware({
      applicationUrl,
      fetcher,
      listInstalled: async () => [current],
      importPackage,
    });
    expect(result.status).toBe("already-installed");
    expect(result.installed).toBe(current);
    expect(fetcher).toHaveBeenCalledTimes(1);
    expect(importPackage).not.toHaveBeenCalled();
  });

  it("verifies archive bytes and SHA-256 before invoking the injected importer", async () => {
    const fetcher = responses();
    const imported = installed();
    const importPackage = vi.fn(async (archive: Blob, expected: ClassicFreewareDescriptorV1["package"]) => {
      expect(archive.size).toBe(archiveBytes.byteLength);
      expect(new Uint8Array(await archive.arrayBuffer())).toEqual(archiveBytes);
      expect(expected.id).toBe(imported.id);
      return imported;
    });
    const result = await bootstrapClassicFreeware({
      applicationUrl,
      fetcher,
      listInstalled: async () => [installed({ content_sha256: "cd".repeat(32) })],
      importPackage,
    });
    expect(result.status).toBe("installed");
    expect(importPackage).toHaveBeenCalledOnce();
    expect(fetcher).toHaveBeenCalledTimes(2);
  });

  it("does not invoke the importer for an archive byte-length or digest mismatch", async () => {
    const importPackage = vi.fn();
    await expect(bootstrapClassicFreeware({
      applicationUrl,
      fetcher: responses(descriptor(), new Uint8Array([1])),
      listInstalled: async () => [],
      importPackage,
    })).rejects.toThrow("byte length mismatch");
    expect(importPackage).not.toHaveBeenCalled();

    const wrongDigest = descriptor({
      package: { ...descriptor().package, archive: { ...descriptor().package.archive, sha256: "00".repeat(32) } },
    });
    await expect(bootstrapClassicFreeware({
      applicationUrl,
      fetcher: responses(wrongDigest),
      listInstalled: async () => [],
      importPackage,
    })).rejects.toThrow("SHA-256 mismatch");
    expect(importPackage).not.toHaveBeenCalled();
  });

  it("rejects an importer result that does not satisfy the descriptor manifest contract", async () => {
    await expect(bootstrapClassicFreeware({
      applicationUrl,
      fetcher: responses(),
      listInstalled: async () => [],
      importPackage: async () => installed({
        source: { product: "cnc-remastered-collection", provider: "unknown" },
      }),
    })).rejects.toThrow("does not match the bootstrap descriptor");
  });
});
