import assert from "node:assert/strict";
import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";

import { inspectBundle } from "./check-bundle-budget.mjs";

const roomyLimits = {
  appJavaScript: { rawBytes: 1024, gzipBytes: 1024 },
  appCss: { rawBytes: 1024, gzipBytes: 1024 },
  simulationWorker: { rawBytes: 1024, gzipBytes: 1024 },
  engineJavaScript: { rawBytes: 1024, gzipBytes: 1024 },
  engineWasm: { rawBytes: 1024, gzipBytes: 1024 },
};

function withBundle(callback) {
  const root = mkdtempSync(join(tmpdir(), "cncweb-bundle-budget-"));
  mkdirSync(join(root, "assets"));
  writeFileSync(join(root, "assets/index-ABC123.js"), "console.log('synthetic');\n");
  writeFileSync(join(root, "assets/index-ABC123.css"), ".synthetic { color: black; }\n");
  writeFileSync(join(root, "assets/simulation.worker-ABC123.js"), "self.synthetic = true;\n");
  try {
    callback(root);
  } finally {
    rmSync(root, { recursive: true, force: true });
  }
}

test("accepts the three deterministic source-only artifact classes", () => {
  withBundle((root) => {
    const result = inspectBundle({
      distDirectory: root,
      budgets: { bundle: roomyLimits },
    });
    assert.equal(result.profile, "source-only");
    assert.equal(result.passed, true);
    assert.deepEqual(
      result.artifacts.filter((artifact) => artifact.status === "omitted").map((artifact) => artifact.id),
      ["engineJavaScript", "engineWasm"],
    );
  });
});

test("requires and measures a complete engine pair for the integrated profile", () => {
  withBundle((root) => {
    mkdirSync(join(root, "engine"));
    writeFileSync(join(root, "engine/tiberiandawn.js"), "export default {};\n");
    writeFileSync(join(root, "engine/tiberiandawn.wasm"), Buffer.from([0, 97, 115, 109]));
    const result = inspectBundle({
      distDirectory: root,
      budgets: { bundle: roomyLimits },
      requireEngine: true,
    });
    assert.equal(result.profile, "integrated");
    assert.equal(result.passed, true);
    assert.equal(result.artifacts.every((artifact) => artifact.status === "within-budget"), true);
  });
});

test("aggregates split application JavaScript and CSS as separate HTTP gzip members", () => {
  withBundle((root) => {
    writeFileSync(join(root, "assets/chunk-SECOND.js"), "export const split = true;\n");
    writeFileSync(join(root, "assets/chunk-SECOND.css"), ".split { display: block; }\n");
    const result = inspectBundle({ distDirectory: root, budgets: { bundle: roomyLimits } });
    const app = result.artifacts.find((artifact) => artifact.id === "appJavaScript");
    const css = result.artifacts.find((artifact) => artifact.id === "appCss");
    assert.deepEqual(app?.paths, ["assets/chunk-SECOND.js", "assets/index-ABC123.js"]);
    assert.deepEqual(css?.paths, ["assets/chunk-SECOND.css", "assets/index-ABC123.css"]);
  });
});

test("rejects missing, incomplete, and duplicate production artifact classes", () => {
  withBundle((root) => {
    assert.throws(
      () => inspectBundle({ distDirectory: root, budgets: { bundle: roomyLimits }, requireEngine: true }),
      /exactly one engine JavaScript/i,
    );
    mkdirSync(join(root, "engine"));
    writeFileSync(join(root, "engine/tiberiandawn.js"), "export default {};\n");
    assert.throws(
      () => inspectBundle({ distDirectory: root, budgets: { bundle: roomyLimits } }),
      /incomplete engine pair/i,
    );
    rmSync(join(root, "engine"), { recursive: true });
    writeFileSync(join(root, "assets/simulation.worker-SECOND.js"), "self.duplicate = true;\n");
    assert.throws(
      () => inspectBundle({ distDirectory: root, budgets: { bundle: roomyLimits } }),
      /exactly one simulation worker/i,
    );
  });
});

test("reports raw and gzip budget violations independently", () => {
  withBundle((root) => {
    const result = inspectBundle({
      distDirectory: root,
      budgets: {
        bundle: {
          ...roomyLimits,
          appJavaScript: { rawBytes: 1, gzipBytes: 1 },
        },
      },
    });
    assert.equal(result.passed, false);
    const app = result.artifacts.find((artifact) => artifact.id === "appJavaScript");
    assert.ok(app);
    assert.deepEqual(app.violations.map((violation) => violation.split(" ")[0]), ["raw", "gzip"]);
  });
});
