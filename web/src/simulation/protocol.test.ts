import { describe, expect, it } from "vitest";
import { CommandType, Faction, GameMode, InputRequest, decodeCommandBatch, encodeCommandBatch, encodeStartConfiguration, resolveImmediateCommandTick } from "./protocol";

describe("command batches", () => {
  it("round-trips normalized commands in little-endian form", () => {
    const encoded = encodeCommandBatch(42, [
      { type: CommandType.Input, flags: 3, args: [InputRequest.CommandAtPosition, -2, 11, 0, 1, 2, 3] },
      { type: CommandType.ClearSelection, args: [0, 0, 0, 0, 0, 0, 0] },
    ], 17n);
    const decoded = decodeCommandBatch(encoded);
    expect(decoded.targetTick).toBe(42);
    expect(decoded.playerId).toBe(17n);
    expect(decoded.commands).toEqual([
      { type: CommandType.Input, flags: 3, args: [InputRequest.CommandAtPosition, -2, 11, 0, 1, 2, 3] },
      { type: CommandType.ClearSelection, flags: 0, args: [0, 0, 0, 0, 0, 0, 0] },
    ]);
  });

  it("rejects corrupted and truncated batches", () => {
    const valid = encodeCommandBatch(1, []);
    new DataView(valid).setUint32(0, 0, true);
    expect(() => decodeCommandBatch(valid)).toThrow(/magic/i);
    expect(() => decodeCommandBatch(new ArrayBuffer(3))).toThrow(/truncated/i);
  });

  it("bounds the tick and command count", () => {
    expect(() => encodeCommandBatch(-1, [])).toThrow(RangeError);
    expect(() => encodeCommandBatch(0x1_0000_0000, [])).toThrow(RangeError);
  });

  it("resolves interactive tick zero against the worker's current core tick", () => {
    const immediate = decodeCommandBatch(encodeCommandBatch(0, []));
    expect(resolveImmediateCommandTick(immediate, 41).targetTick).toBe(42);
    const explicit = decodeCommandBatch(encodeCommandBatch(100, []));
    expect(resolveImmediateCommandTick(explicit, 41)).toBe(explicit);
  });

  it("encodes the engine StartV1 fixed fields and content revision hash", () => {
    const buffer = encodeStartConfiguration({
      game: "tiberian-dawn", seed: 7, scenario: 1, variation: 0, direction: -1, buildLevel: 2,
      sabotagedStructure: -1, faction: Faction.Gdi, gameMode: GameMode.Campaign, playerId: 42n,
      contentDirectory: "/content", overrideMapName: "map", contentIdHash: 0x1122334455667788n,
    });
    const view = new DataView(buffer);
    expect(view.getUint32(8, true)).toBe(72 + 8 + 3);
    expect(view.getUint32(56, true)).toBe(8);
    expect(view.getUint32(60, true)).toBe(3);
    expect(view.getBigUint64(64, true)).toBe(0x1122334455667788n);
  });
});
