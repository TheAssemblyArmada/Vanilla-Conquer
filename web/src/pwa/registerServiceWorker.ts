import { setRuntimeMetricsBuildId } from "../performance/runtimeMetrics";

const BUILD_ID_PATTERN = /^[a-f0-9]{16}$/;
const UPDATE_CHECK_INTERVAL_MS = 15 * 60 * 1_000;
const UPDATE_ACTIVATION_TIMEOUT_MS = 10_000;

export type ServiceWorkerUpdateStatus =
  | "unsupported"
  | "checking"
  | "current"
  | "downloading"
  | "ready"
  | "activating"
  | "error";

export interface ServiceWorkerUpdateState {
  readonly status: ServiceWorkerUpdateStatus;
  /** Build serving the currently loaded application. */
  readonly currentBuildId?: string;
  /** Cache-busted build advertised by the deployment, when known. */
  readonly availableBuildId?: string;
  readonly message?: string;
}

export interface ServiceWorkerRegistrationController {
  readonly supported: boolean;
  readonly registration?: ServiceWorkerRegistration;
  readonly state: ServiceWorkerUpdateState;
  checkForUpdate(): Promise<void>;
  /** Returns false when there is no installed update to activate or reload into. */
  applyUpdate(): Promise<boolean>;
  dispose(): void;
}

export interface BuildDescriptor {
  format: "cncweb-build";
  version: 1;
  id: string;
}

export function parseBuildDescriptor(value: unknown): BuildDescriptor {
  if (!value || typeof value !== "object") throw new Error("Update metadata is not an object");
  const candidate = value as Partial<BuildDescriptor>;
  if (candidate.format !== "cncweb-build" || candidate.version !== 1 || typeof candidate.id !== "string" || !BUILD_ID_PATTERN.test(candidate.id)) {
    throw new Error("Update metadata is invalid");
  }
  return { format: candidate.format, version: candidate.version, id: candidate.id };
}

export function deployedBuildDiffers(currentBuildId: string | undefined, deployedBuildId: string): boolean | undefined {
  if (!BUILD_ID_PATTERN.test(deployedBuildId)) throw new Error("Deployed build ID is invalid");
  if (currentBuildId === undefined) return undefined;
  if (!BUILD_ID_PATTERN.test(currentBuildId)) throw new Error("Current build ID is invalid");
  return currentBuildId !== deployedBuildId;
}

export async function readBuildDescriptorResponse(response: Response, requestedUrl: URL): Promise<BuildDescriptor> {
  if (!response.ok) throw new Error(`Update metadata returned ${response.status}`);
  if (response.redirected) throw new Error("Update metadata must not redirect");
  let responseUrl: URL;
  try {
    responseUrl = new URL(response.url);
  } catch {
    throw new Error("Update metadata response URL is invalid");
  }
  if (responseUrl.origin !== requestedUrl.origin || responseUrl.pathname !== requestedUrl.pathname) {
    throw new Error("Update metadata response escaped its same-origin deployment path");
  }
  const contentType = response.headers.get("content-type")?.split(";", 1)[0].trim().toLowerCase();
  if (contentType !== "application/json") throw new Error("Update metadata must use application/json");
  return parseBuildDescriptor(await response.json());
}

async function readBuildId(worker: ServiceWorker): Promise<string | undefined> {
  const channel = new MessageChannel();
  return new Promise((resolve) => {
    let settled = false;
    const finish = (buildId?: string): void => {
      if (settled) return;
      settled = true;
      window.clearTimeout(timeout);
      channel.port1.close();
      channel.port2.close();
      resolve(buildId);
    };
    const timeout = window.setTimeout(() => finish(), 2_000);
    channel.port1.onmessage = (event: MessageEvent<unknown>) => {
      const value = event.data as { type?: unknown; buildId?: unknown } | null;
      finish(value?.type === "BUILD_ID" && typeof value.buildId === "string" && BUILD_ID_PATTERN.test(value.buildId)
        ? value.buildId
        : undefined);
    };
    try {
      worker.postMessage({ type: "GET_BUILD_ID" }, [channel.port2]);
    } catch {
      finish();
    }
  });
}

async function fetchDeployedBuildId(): Promise<string> {
  const url = new URL(`${import.meta.env.BASE_URL}build-v1.json`, window.location.href);
  // A unique query keeps this probe outside the service worker's exact-match
  // static-shell route, so an old controller cannot answer with its cached ID.
  url.searchParams.set("update-check", `${Date.now().toString(36)}-${Math.random().toString(36).slice(2)}`);
  const response = await fetch(url, {
    cache: "no-store",
    credentials: "same-origin",
    headers: { Accept: "application/json" },
    redirect: "error",
  });
  return (await readBuildDescriptorResponse(response, url)).id;
}

function statesEqual(left: ServiceWorkerUpdateState, right: ServiceWorkerUpdateState): boolean {
  return left.status === right.status
    && left.currentBuildId === right.currentBuildId
    && left.availableBuildId === right.availableBuildId
    && left.message === right.message;
}

export async function registerServiceWorker(
  onStateChange?: (state: ServiceWorkerUpdateState) => void,
): Promise<ServiceWorkerRegistrationController> {
  if (!("serviceWorker" in navigator) || !import.meta.env.PROD) {
    const state: ServiceWorkerUpdateState = { status: "unsupported" };
    onStateChange?.(state);
    return {
      supported: false,
      state,
      checkForUpdate: async () => undefined,
      applyUpdate: async () => false,
      dispose: () => undefined,
    };
  }

  const registration = await navigator.serviceWorker.register(`${import.meta.env.BASE_URL}sw.js`, {
    scope: import.meta.env.BASE_URL,
    updateViaCache: "none",
  });
  let state: ServiceWorkerUpdateState = { status: "checking" };
  let disposed = false;
  let reloadOnControllerChange = false;
  let reloadRequired = false;
  let currentBuildId: string | undefined;
  let availableBuildId: string | undefined;
  let checkInFlight: Promise<void> | undefined;
  const observedWorkers = new WeakSet<ServiceWorker>();
  // This page's JavaScript and unversioned engine URLs belong to the worker
  // that controlled the navigation. Never let a later controller race rewrite
  // that identity before the page reloads.
  const loadedWorker = navigator.serviceWorker.controller ?? registration.active;
  let activationAttempt: {
    promise: Promise<boolean>;
    resolve: (activated: boolean) => void;
    timeout: number;
  } | undefined;

  const emit = (next: ServiceWorkerUpdateState): void => {
    if (disposed || statesEqual(state, next)) return;
    state = next;
    onStateChange?.(state);
  };

  const stateWithBuilds = (
    status: ServiceWorkerUpdateStatus,
    extra: Pick<ServiceWorkerUpdateState, "message"> = {},
  ): ServiceWorkerUpdateState => ({ status, currentBuildId, availableBuildId, ...extra });

  const captureCurrentBuildId = async (): Promise<string | undefined> => {
    if (currentBuildId) return currentBuildId;
    if (!loadedWorker) return undefined;
    const buildId = await readBuildId(loadedWorker);
    if (!disposed && buildId && (navigator.serviceWorker.controller ?? registration.active) === loadedWorker) {
      currentBuildId = buildId;
      setRuntimeMetricsBuildId(buildId);
    }
    return buildId;
  };

  const finishActivation = (activated: boolean, message?: string): void => {
    const attempt = activationAttempt;
    if (!attempt) return;
    activationAttempt = undefined;
    reloadOnControllerChange = false;
    window.clearTimeout(attempt.timeout);
    if (!activated) emit(stateWithBuilds("error", {
      message: message ?? "The update did not activate. Your game remains paused safely; retry when ready.",
    }));
    attempt.resolve(activated);
  };

  const markWaitingUpdate = async (worker: ServiceWorker): Promise<void> => {
    const buildId = await readBuildId(worker);
    if (buildId) availableBuildId = buildId;
    emit(stateWithBuilds("ready"));
  };

  const observeInstalling = (worker?: ServiceWorker | null): void => {
    if (!worker || observedWorkers.has(worker)) return;
    observedWorkers.add(worker);
    emit(stateWithBuilds("downloading"));
    const stateChange = (): void => {
      if (worker.state === "installed") {
        if (navigator.serviceWorker.controller || registration.active) void markWaitingUpdate(worker);
      } else if (worker.state === "redundant") {
        worker.removeEventListener("statechange", stateChange);
        finishActivation(false, "The update became unavailable before activation. Retry the update.");
        emit(stateWithBuilds("error", { message: "The update could not be installed. Retry when the connection is stable." }));
      } else if (worker.state === "activated") {
        worker.removeEventListener("statechange", stateChange);
      }
    };
    worker.addEventListener("statechange", stateChange);
    stateChange();
  };

  const inspectInstalling = (): void => {
    if (registration.waiting) {
      observeInstalling(registration.waiting);
      void markWaitingUpdate(registration.waiting);
    }
    observeInstalling(registration.installing);
  };

  const updateFound = (): void => inspectInstalling();
  registration.addEventListener("updatefound", updateFound);
  inspectInstalling();

  const controllerChange = (): void => {
    if (reloadOnControllerChange) {
      finishActivation(true);
      window.location.reload();
      return;
    }
    void (async () => {
      const nextControllerBuildId = await readBuildId(navigator.serviceWorker.controller as ServiceWorker);
      if (disposed) return;
      if (loadedWorker && navigator.serviceWorker.controller !== loadedWorker) {
        availableBuildId = nextControllerBuildId;
        reloadRequired = true;
        emit(stateWithBuilds("ready"));
      } else {
        if (!loadedWorker && nextControllerBuildId) {
          currentBuildId = nextControllerBuildId;
          setRuntimeMetricsBuildId(nextControllerBuildId);
        } else {
          await captureCurrentBuildId();
        }
        emit(stateWithBuilds("current"));
      }
    })();
  };
  navigator.serviceWorker.addEventListener("controllerchange", controllerChange);

  const performUpdateCheck = async (): Promise<void> => {
    if (disposed || state.status === "activating") return;
    if (registration.waiting) {
      await markWaitingUpdate(registration.waiting);
      return;
    }
    await captureCurrentBuildId();
    if (reloadRequired) {
      emit(stateWithBuilds("ready"));
      return;
    }
    if (state.status !== "downloading") emit(stateWithBuilds("checking"));
    try {
      const deployedBuildId = await fetchDeployedBuildId();
      if (disposed) return;
      if (reloadRequired) {
        emit(stateWithBuilds("ready"));
        return;
      }
      const differs = deployedBuildDiffers(currentBuildId, deployedBuildId);
      if (differs === true) {
        availableBuildId = deployedBuildId;
        emit(stateWithBuilds("downloading"));
        await registration.update();
        inspectInstalling();
        if (registration.waiting) await markWaitingUpdate(registration.waiting);
        else if (!registration.installing) emit(stateWithBuilds("error", {
          message: "The deployment changed, but its service worker was not updated. Retry after the release finishes publishing.",
        }));
        return;
      }
      if (differs === undefined) {
        // Older compatible workers might not implement GET_BUILD_ID. Asking the
        // browser to compare sw.js remains safe and preserves offline startup.
        await registration.update();
        inspectInstalling();
      }
      if (!registration.waiting && !registration.installing) emit(stateWithBuilds("current"));
    } catch (error) {
      if (disposed) return;
      // Losing the network while using a valid cached build is expected. Only
      // surface an error after deployment drift was already established.
      if (availableBuildId && availableBuildId !== currentBuildId) {
        emit(stateWithBuilds("error", {
          message: error instanceof Error ? error.message : "The update check failed",
        }));
      } else {
        emit(stateWithBuilds("current"));
      }
    }
  };

  const checkForUpdate = (): Promise<void> => {
    if (checkInFlight) return checkInFlight;
    checkInFlight = performUpdateCheck().finally(() => { checkInFlight = undefined; });
    return checkInFlight;
  };

  const checkWhenVisible = (): void => {
    if (document.visibilityState === "visible") void checkForUpdate();
  };
  const checkWhenOnline = (): void => void checkForUpdate();
  document.addEventListener("visibilitychange", checkWhenVisible);
  window.addEventListener("online", checkWhenOnline);
  const interval = window.setInterval(checkWhenVisible, UPDATE_CHECK_INTERVAL_MS);
  void navigator.serviceWorker.ready.then(() => checkForUpdate());

  const controller: ServiceWorkerRegistrationController = {
    supported: true,
    registration,
    get state() { return state; },
    checkForUpdate,
    applyUpdate: async () => {
      if (state.status !== "ready") return false;
      emit(stateWithBuilds("activating"));
      if (registration.waiting) {
        if (activationAttempt) return activationAttempt.promise;
        reloadOnControllerChange = true;
        let resolveActivation!: (activated: boolean) => void;
        const promise = new Promise<boolean>((resolve) => { resolveActivation = resolve; });
        const timeout = window.setTimeout(() => {
          finishActivation(false, "The update did not take control within 10 seconds. Your game was kept and can resume.");
        }, UPDATE_ACTIVATION_TIMEOUT_MS);
        activationAttempt = { promise, resolve: resolveActivation, timeout };
        try {
          registration.waiting.postMessage({ type: "SKIP_WAITING" });
        } catch {
          finishActivation(false, "The update could not be activated. Retry when the connection is stable.");
        }
        return promise;
      }
      if (reloadRequired) {
        window.location.reload();
        return true;
      }
      emit(stateWithBuilds("error", { message: "The installed update is no longer available. Check again." }));
      return false;
    },
    dispose: () => {
      disposed = true;
      finishActivation(false, "The update check was closed before activation completed.");
      window.clearInterval(interval);
      registration.removeEventListener("updatefound", updateFound);
      navigator.serviceWorker.removeEventListener("controllerchange", controllerChange);
      document.removeEventListener("visibilitychange", checkWhenVisible);
      window.removeEventListener("online", checkWhenOnline);
    },
  };
  onStateChange?.(state);
  return controller;
}
