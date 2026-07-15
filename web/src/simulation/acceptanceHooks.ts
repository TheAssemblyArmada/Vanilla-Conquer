const ACCEPTANCE_SESSION = /^[a-f0-9]{32,64}$/;
const LOOPBACK_HOSTS = new Set(["localhost", "127.0.0.1", "[::1]", "::1"]);

export function localAcceptanceSession(location: Pick<Location, "href"> = window.location): string | undefined {
  let url: URL;
  try {
    url = new URL(location.href);
  } catch {
    return undefined;
  }
  if ((url.protocol !== "http:" && url.protocol !== "https:")
    || !LOOPBACK_HOSTS.has(url.hostname)
    || url.pathname !== "/"
    || url.hash
    || url.searchParams.size !== 1) return undefined;
  const session = url.searchParams.get("acceptance");
  return session && ACCEPTANCE_SESSION.test(session) ? session : undefined;
}

export interface CncwebAcceptanceApi {
  forceVictory(): Promise<void>;
}

declare global {
  interface Window {
    __cncwebAcceptance?: Readonly<CncwebAcceptanceApi>;
  }
}
