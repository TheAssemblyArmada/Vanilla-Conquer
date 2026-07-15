#!/usr/bin/env node

import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";

import {
  CLASSIC_FREEWARE_BASE_URL_ENV,
  resolveClassicFreewareBaseURL,
} from "./classic-freeware-base-url.mjs";

const repositoryRoot = fileURLToPath(new URL("../../", import.meta.url));
const verificationScript = fileURLToPath(new URL("./verify-classic-freeware-deployment.mjs", import.meta.url));
const matrixScript = fileURLToPath(new URL("./run-classic-freeware-matrix.mjs", import.meta.url));

let activeChild;
let receivedSignal;

function stopForwardingSignals() {
  process.off("SIGINT", forwardSignal);
  process.off("SIGTERM", forwardSignal);
}

function terminateWithSignal(signal) {
  stopForwardingSignals();
  process.kill(process.pid, signal);
}

function forwardSignal(signal) {
  receivedSignal ??= signal;
  if (activeChild) activeChild.kill(signal);
  else terminateWithSignal(signal);
}

process.on("SIGINT", forwardSignal);
process.on("SIGTERM", forwardSignal);

function run(script, arguments_, environment) {
  return new Promise((resolve, reject) => {
    const child = spawn(process.execPath, [script, ...arguments_], {
      cwd: repositoryRoot,
      env: environment,
      shell: false,
      stdio: "inherit",
    });
    activeChild = child;
    child.once("error", (error) => {
      if (activeChild === child) activeChild = undefined;
      reject(error);
    });
    child.once("exit", (code, signal) => {
      if (activeChild === child) activeChild = undefined;
      if (receivedSignal) {
        terminateWithSignal(receivedSignal);
        return;
      }
      if (signal) {
        terminateWithSignal(signal);
        return;
      }
      resolve(code ?? 1);
    });
  });
}

async function main() {
  const configuredURL = process.env[CLASSIC_FREEWARE_BASE_URL_ENV];
  if (configuredURL === undefined) {
    throw new Error(`${CLASSIC_FREEWARE_BASE_URL_ENV} is required for staging acceptance`);
  }
  const baseURL = resolveClassicFreewareBaseURL(configuredURL);
  if (!baseURL) throw new Error(`${CLASSIC_FREEWARE_BASE_URL_ENV} is required for staging acceptance`);
  const environment = { ...process.env, [CLASSIC_FREEWARE_BASE_URL_ENV]: baseURL };

  const verificationStatus = await run(
    verificationScript,
    [baseURL, "--dist", "web/dist"],
    environment,
  );
  if (verificationStatus !== 0) {
    process.exitCode = verificationStatus;
    return;
  }

  process.exitCode = await run(matrixScript, process.argv.slice(2), environment);
}

main()
  .then(() => stopForwardingSignals())
  .catch((error) => {
    stopForwardingSignals();
    console.error(error instanceof Error ? error.message : String(error));
    process.exitCode = 1;
  });
