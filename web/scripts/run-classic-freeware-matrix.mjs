#!/usr/bin/env node

import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";

const playwrightCli = fileURLToPath(import.meta.resolve("@playwright/test/cli"));
const playwrightArguments = [
  playwrightCli,
  "test",
  "--config",
  "playwright.classic.config.ts",
  ...process.argv.slice(2),
];
const command = process.platform === "linux" ? "xvfb-run" : process.execPath;
const arguments_ = process.platform === "linux"
  ? ["-a", process.execPath, ...playwrightArguments]
  : playwrightArguments;

const child = spawn(command, arguments_, {
  cwd: fileURLToPath(new URL("../", import.meta.url)),
  env: { ...process.env, CNCWEB_CLASSIC_FREEWARE_CROSS_BROWSER: "1" },
  stdio: "inherit",
});

for (const signal of ["SIGINT", "SIGTERM"]) {
  process.on(signal, () => child.kill(signal));
}

child.once("error", (error) => {
  console.error(`Could not start the classic-freeware browser matrix: ${error.message}`);
  process.exitCode = 1;
});
child.once("exit", (code, signal) => {
  if (signal) process.kill(process.pid, signal);
  else process.exitCode = code ?? 1;
});
