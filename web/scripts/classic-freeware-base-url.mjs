export const CLASSIC_FREEWARE_BASE_URL_ENV = "CNCWEB_CLASSIC_FREEWARE_BASE_URL";

function isLoopbackHostname(hostname) {
  if (hostname === "localhost" || hostname === "[::1]") return true;
  return /^127(?:\.\d{1,3}){3}$/.test(hostname);
}

/**
 * Resolves the optional origin used by the classic-freeware staging matrix.
 * An absent value keeps Playwright's local preview configuration unchanged.
 */
export function resolveClassicFreewareBaseURL(value) {
  if (value === undefined) return undefined;
  if (typeof value !== "string" || value.length === 0 || value !== value.trim()) {
    throw new Error(`${CLASSIC_FREEWARE_BASE_URL_ENV} must be a non-empty absolute URL without surrounding whitespace`);
  }

  let url;
  try {
    url = new URL(value);
  } catch {
    throw new Error(`${CLASSIC_FREEWARE_BASE_URL_ENV} must be a valid absolute URL`);
  }

  if (url.protocol !== "https:" && url.protocol !== "http:") {
    throw new Error(`${CLASSIC_FREEWARE_BASE_URL_ENV} must use HTTPS, except for loopback HTTP`);
  }

  const authority = /^[a-z][a-z\d+.-]*:\/\/([^/?#]*)/i.exec(value)?.[1] ?? "";
  if (authority.includes("@") || url.username || url.password) {
    throw new Error(`${CLASSIC_FREEWARE_BASE_URL_ENV} must not contain credentials`);
  }
  // URL.search/hash are empty for a bare trailing '?' or '#', so reject the
  // delimiters in the input as well as populated components.
  if (value.includes("?") || value.includes("#") || url.search || url.hash) {
    throw new Error(`${CLASSIC_FREEWARE_BASE_URL_ENV} must not contain a query or fragment`);
  }
  if (url.protocol === "http:" && !isLoopbackHostname(url.hostname)) {
    throw new Error(`${CLASSIC_FREEWARE_BASE_URL_ENV} must use HTTPS for non-loopback hosts`);
  }

  if (!url.pathname.endsWith("/")) url.pathname += "/";
  return url.href;
}

export function classicFreewareBaseURLFromEnvironment(environment = process.env) {
  return resolveClassicFreewareBaseURL(environment[CLASSIC_FREEWARE_BASE_URL_ENV]);
}

/** Returns the inherited local config or a remote target with no web server. */
export function classicFreewarePlaywrightTarget(baseConfig, environment = process.env) {
  const remoteBaseURL = classicFreewareBaseURLFromEnvironment(environment);
  if (remoteBaseURL === undefined) return baseConfig;
  const remoteConfig = { ...baseConfig };
  delete remoteConfig.webServer;
  return {
    ...remoteConfig,
    use: { ...baseConfig.use, baseURL: remoteBaseURL },
  };
}
