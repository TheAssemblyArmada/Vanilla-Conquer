const NOT_FOUND_RETRY_DELAYS_MS = [25, 100, 250] as const;

export function isOpfsNotFoundError(error: unknown): boolean {
  return error instanceof DOMException && error.name === "NotFoundError";
}

/**
 * Chromium can briefly return NotFoundError while a fresh OPFS handle is
 * traversed immediately after a long-lived reader is released. Reads are
 * side-effect free, so retry only that narrow failure with a short bound.
 */
export async function retryTransientOpfsNotFound<T>(operation: () => Promise<T>): Promise<T> {
  for (let attempt = 0; ; attempt += 1) {
    try {
      return await operation();
    } catch (error) {
      if (!isOpfsNotFoundError(error) || attempt >= NOT_FOUND_RETRY_DELAYS_MS.length) throw error;
      await new Promise<void>((resolve) => setTimeout(resolve, NOT_FOUND_RETRY_DELAYS_MS[attempt]));
    }
  }
}
