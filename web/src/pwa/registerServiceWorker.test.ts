import { describe, expect, it } from "vitest";
import { deployedBuildDiffers, parseBuildDescriptor, readBuildDescriptorResponse } from "./registerServiceWorker";

function buildResponse(
  url: string,
  options: { contentType?: string; redirected?: boolean; status?: number } = {},
): Response {
  const response = new Response(JSON.stringify({
    format: "cncweb-build",
    version: 1,
    id: "0123456789abcdef",
  }), {
    status: options.status ?? 200,
    headers: { "Content-Type": options.contentType ?? "application/json; charset=utf-8" },
  });
  Object.defineProperties(response, {
    url: { configurable: true, value: url },
    redirected: { configurable: true, value: options.redirected ?? false },
  });
  return response;
}

describe("service-worker build update policy", () => {
  it("accepts only the signed-off build descriptor shape", () => {
    expect(parseBuildDescriptor({
      format: "cncweb-build",
      version: 1,
      id: "0123456789abcdef",
    })).toEqual({ format: "cncweb-build", version: 1, id: "0123456789abcdef" });

    for (const value of [
      null,
      { format: "cncweb-build", version: 2, id: "0123456789abcdef" },
      { format: "other", version: 1, id: "0123456789abcdef" },
      { format: "cncweb-build", version: 1, id: "../../old-cache" },
      { format: "cncweb-build", version: 1, id: "ABCDEF0123456789" },
    ]) expect(() => parseBuildDescriptor(value)).toThrow(/Update metadata is (?:invalid|not an object)/);
  });

  it("distinguishes current, stale, and legacy controlling workers", () => {
    expect(deployedBuildDiffers("0123456789abcdef", "0123456789abcdef")).toBe(false);
    expect(deployedBuildDiffers("0123456789abcdef", "fedcba9876543210")).toBe(true);
    expect(deployedBuildDiffers(undefined, "fedcba9876543210")).toBeUndefined();
  });

  it("accepts only a non-redirected JSON response from the requested deployment path", async () => {
    const request = new URL("https://game.example/releases/current/build-v1.json?update-check=unique");
    await expect(readBuildDescriptorResponse(buildResponse(request.href), request)).resolves.toMatchObject({
      id: "0123456789abcdef",
    });
    await expect(readBuildDescriptorResponse(buildResponse(request.href, { contentType: "text/html" }), request))
      .rejects.toThrow("application/json");
    await expect(readBuildDescriptorResponse(buildResponse(request.href, { redirected: true }), request))
      .rejects.toThrow("must not redirect");
    await expect(readBuildDescriptorResponse(buildResponse("https://mirror.example/releases/current/build-v1.json"), request))
      .rejects.toThrow("same-origin deployment path");
    await expect(readBuildDescriptorResponse(buildResponse("https://game.example/other/build-v1.json"), request))
      .rejects.toThrow("same-origin deployment path");
  });
});
