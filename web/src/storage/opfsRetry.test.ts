// @vitest-environment node
import { afterEach, describe, expect, it, vi } from "vitest";
import { retryTransientOpfsNotFound } from "./opfsRetry";

afterEach(() => vi.useRealTimers());

describe("transient OPFS reads", () => {
  it("retries bounded NotFoundError failures and preserves the eventual value", async () => {
    vi.useFakeTimers();
    let attempts = 0;
    const result = retryTransientOpfsNotFound(async () => {
      attempts += 1;
      if (attempts < 3) throw new DOMException("temporarily unavailable", "NotFoundError");
      return "ready";
    });

    await vi.runAllTimersAsync();
    await expect(result).resolves.toBe("ready");
    expect(attempts).toBe(3);
  });

  it("does not retry unrelated failures", async () => {
    const operation = vi.fn(async () => { throw new DOMException("denied", "NotAllowedError"); });
    await expect(retryTransientOpfsNotFound(operation)).rejects.toMatchObject({ name: "NotAllowedError" });
    expect(operation).toHaveBeenCalledOnce();
  });

  it("returns the original NotFoundError after the retry budget is exhausted", async () => {
    vi.useFakeTimers();
    const missing = new DOMException("still missing", "NotFoundError");
    const operation = vi.fn(async () => { throw missing; });
    const result = retryTransientOpfsNotFound(operation);
    const rejected = expect(result).rejects.toBe(missing);

    await vi.runAllTimersAsync();
    await rejected;
    expect(operation).toHaveBeenCalledTimes(4);
  });
});
