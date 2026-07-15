import { describe, expect, it } from "vitest";
import { SnapshotCloakState, SnapshotContextualAction, SnapshotObjectType, SnapshotSectionKind, SnapshotView, snapshotByteLength, writeSnapshot } from "./snapshot";

interface ClassicWireLayout {
  width: number;
  height: number;
  pitch: number;
  format: number;
  rectX?: number;
  rectY?: number;
  rectWidth?: number;
  rectHeight?: number;
  pixels?: Uint8Array;
  count?: number;
  headerBytes?: number;
}

function classicOnlySnapshot(layout: ClassicWireLayout): ArrayBuffer {
  const headerBytes = layout.headerBytes ?? (layout.format === 2 ? 32 : 16);
  const pixels = layout.pixels ?? new Uint8Array(0);
  const payloadLength = headerBytes + pixels.byteLength;
  const buffer = new ArrayBuffer(40 + 16 + payloadLength);
  const view = new DataView(buffer);
  view.setUint32(0, 0x57434e43, true);
  view.setUint16(4, 1, true);
  view.setUint16(6, 3, true);
  view.setUint32(8, buffer.byteLength, true);
  view.setUint32(12, 1, true);
  view.setUint32(32, 1, true);
  view.setUint16(40, 9, true);
  view.setUint32(44, payloadLength, true);
  view.setUint32(48, layout.count ?? pixels.byteLength, true);
  const payload = 56;
  view.setUint32(payload, layout.width, true);
  view.setUint32(payload + 4, layout.height, true);
  view.setUint32(payload + 8, layout.pitch, true);
  view.setUint32(payload + 12, layout.format, true);
  if (headerBytes >= 32) {
    view.setUint32(payload + 16, layout.rectX ?? 0, true);
    view.setUint32(payload + 20, layout.rectY ?? 0, true);
    view.setUint32(payload + 24, layout.rectWidth ?? 0, true);
    view.setUint32(payload + 28, layout.rectHeight ?? 0, true);
  }
  new Uint8Array(buffer, payload + headerBytes).set(pixels);
  return buffer;
}

interface WireSection {
  kind: SnapshotSectionKind;
  count: number;
  payload: Uint8Array;
  flags?: number;
}

function sectionSnapshot(sections: readonly WireSection[], tick = 23, baseTick = tick): ArrayBuffer {
  const length = 40 + sections.reduce((total, section) => total + 16 + section.payload.byteLength, 0);
  const buffer = new ArrayBuffer(length);
  const view = new DataView(buffer);
  view.setUint32(0, 0x57434e43, true);
  view.setUint16(4, 1, true);
  view.setUint16(6, 3, true);
  view.setUint32(8, length, true);
  view.setUint32(12, sections.length, true);
  view.setUint32(16, tick, true);
  view.setUint32(20, baseTick, true);
  view.setUint32(32, sections.length, true);
  let offset = 40;
  for (const section of sections) {
    view.setUint16(offset, section.kind, true);
    view.setUint16(offset + 2, section.flags ?? 0, true);
    view.setUint32(offset + 4, section.payload.byteLength, true);
    view.setUint32(offset + 8, section.count, true);
    new Uint8Array(buffer, offset + 16, section.payload.byteLength).set(section.payload);
    offset += 16 + section.payload.byteLength;
  }
  return buffer;
}

interface StaticMapWireBounds {
  cellX: number;
  cellY: number;
  originalCellX: number;
  originalCellY: number;
  originalWidth: number;
  originalHeight: number;
}

function staticMapPayload(
  width: number,
  height: number,
  retained: boolean,
  bounds: Partial<StaticMapWireBounds> = {},
): Uint8Array {
  const count = width * height;
  const payload = new Uint8Array(304 + (retained ? 0 : count * 36));
  const view = new DataView(payload.buffer);
  view.setInt32(0, bounds.cellX ?? 0, true);
  view.setInt32(4, bounds.cellY ?? 0, true);
  view.setInt32(8, width, true);
  view.setInt32(12, height, true);
  view.setInt32(16, bounds.originalCellX ?? bounds.cellX ?? 0, true);
  view.setInt32(20, bounds.originalCellY ?? bounds.cellY ?? 0, true);
  view.setInt32(24, bounds.originalWidth ?? width, true);
  view.setInt32(28, bounds.originalHeight ?? height, true);
  view.setInt32(32, 1, true);
  putText(payload, 36, 264, "SCG01EA");
  view.setUint32(300, count, true);
  return payload;
}

function putText(bytes: Uint8Array, offset: number, length: number, text: string): void {
  bytes.set(new TextEncoder().encode(text).subarray(0, length), offset);
}

function occupierPayload(cells: readonly (readonly [SnapshotObjectType, number][])[]): Uint8Array {
  const payload = new Uint8Array(cells.reduce((length, entries) => length + 4 + entries.length * 8, 0));
  const view = new DataView(payload.buffer);
  let offset = 0;
  for (const entries of cells) {
    view.setUint32(offset, entries.length, true);
    offset += 4;
    for (const [type, id] of entries) {
      view.setInt32(offset, type, true);
      view.setInt32(offset + 4, id, true);
      offset += 8;
    }
  }
  return payload;
}

describe("SnapshotV1", () => {
  it("writes and validates classic pixels, palette, and sprites", () => {
    const palette = new Uint8Array(1024).fill(255);
    const pixels = new Uint8Array([1, 2, 3, 4, 5, 6]);
    const buffer = new ArrayBuffer(snapshotByteLength(3, 2, 1));
    const written = writeSnapshot(buffer, {
      tick: 99,
      worldWidth: 128,
      worldHeight: 64,
      classicWidth: 3,
      classicHeight: 2,
      classicPixels: pixels,
      palette,
      cameraX: 4,
      cameraY: 8,
      zoom: 1.5,
      sprites: [{ x: 1, y: 2, width: 3, height: 4, u0: 0, v0: 0.1, u1: 0.9, v1: 1, atlasPage: 2, flags: 1, sortKey: -7, tint: 0x11223344, teamColor: 0xaabbccdd }],
    });
    const snapshot = new SnapshotView(buffer);
    expect(snapshot.byteLength).toBe(written);
    expect(snapshot.tick).toBe(99);
    expect(snapshot.terminal).toBe(false);
    expect([...snapshot.classicPixels!]).toEqual([...pixels]);
    expect(snapshot.sprite(0)).toMatchObject({ x: 1, y: 2, width: 3, height: 4, atlasPage: 0, flags: 1, sortKey: -7 });
  });

  it("removes full-frame wire padding while preserving baseline metadata", () => {
    const snapshot = new SnapshotView(classicOnlySnapshot({
      width: 2,
      height: 2,
      pitch: 3,
      format: 1,
      pixels: new Uint8Array([1, 2, 99, 3, 4, 99]),
    }));
    expect(snapshot.classicSurface).toMatchObject({
      format: 1,
      width: 2,
      height: 2,
      rectX: 0,
      rectY: 0,
      rectWidth: 2,
      rectHeight: 2,
    });
    expect([...snapshot.classicPixels!]).toEqual([1, 2, 3, 4]);
  });

  it("validates and exposes a tightly packed dirty rectangle", () => {
    const snapshot = new SnapshotView(classicOnlySnapshot({
      width: 4,
      height: 3,
      pitch: 3,
      format: 2,
      rectX: 1,
      rectY: 1,
      rectWidth: 2,
      rectHeight: 2,
      pixels: new Uint8Array([7, 8, 99, 9, 10, 99]),
    }));
    expect(snapshot.classicFormat).toBe(2);
    expect(snapshot.classicSurface).toMatchObject({
      format: 2,
      width: 4,
      height: 3,
      rectX: 1,
      rectY: 1,
      rectWidth: 2,
      rectHeight: 2,
    });
    expect([...snapshot.classicPixels!]).toEqual([7, 8, 9, 10]);
  });

  it("accepts an empty dirty rectangle as an unchanged frame", () => {
    const snapshot = new SnapshotView(classicOnlySnapshot({
      width: 4,
      height: 3,
      pitch: 0,
      format: 2,
      rectX: 4,
      rectY: 3,
      rectWidth: 0,
      rectHeight: 0,
    }));
    expect(snapshot.classicSurface).toMatchObject({ format: 2, rectX: 4, rectY: 3, rectWidth: 0, rectHeight: 0 });
    expect(snapshot.classicPixels).toHaveLength(0);
  });

  it.each([
    ["truncated dirty header", { width: 4, height: 3, pitch: 0, format: 2, headerBytes: 16 }],
    ["unsupported format", { width: 4, height: 3, pitch: 4, format: 3, pixels: new Uint8Array(12) }],
    ["oversized dimensions", { width: 3073, height: 1, pitch: 3073, format: 1, pixels: new Uint8Array(3073) }],
    ["short dirty pitch", { width: 4, height: 3, pitch: 1, format: 2, rectX: 1, rectY: 1, rectWidth: 2, rectHeight: 1, pixels: new Uint8Array(1) }],
    ["out-of-bounds dirty rectangle", { width: 4, height: 3, pitch: 2, format: 2, rectX: 3, rectY: 1, rectWidth: 2, rectHeight: 1, pixels: new Uint8Array(2) }],
    ["half-empty dirty rectangle", { width: 4, height: 3, pitch: 0, format: 2, rectX: 1, rectY: 1, rectWidth: 0, rectHeight: 1 }],
    ["nonempty unchanged payload", { width: 4, height: 3, pitch: 1, format: 2, rectWidth: 0, rectHeight: 0, pixels: new Uint8Array(1) }],
    ["mismatched dirty byte count", { width: 4, height: 3, pitch: 2, format: 2, rectX: 1, rectY: 1, rectWidth: 2, rectHeight: 1, pixels: new Uint8Array(2), count: 1 }],
  ] satisfies [string, ClassicWireLayout][]) ("rejects %s", (_label, layout) => {
    expect(() => new SnapshotView(classicOnlySnapshot(layout))).toThrow(/classic/i);
  });

  it("exposes the durable terminal snapshot flag", () => {
    const buffer = new ArrayBuffer(snapshotByteLength(1, 1, 0));
    writeSnapshot(buffer, { tick: 1, worldWidth: 1, worldHeight: 1, classicWidth: 1, classicHeight: 1, classicPixels: new Uint8Array(1), palette: new Uint8Array(1024), cameraX: 0, cameraY: 0, zoom: 1, sprites: [] });
    new DataView(buffer).setUint32(36, 1, true);
    expect(new SnapshotView(buffer).terminal).toBe(true);
  });

  it("accepts full and retained static-map layouts while exposing retention", () => {
    const full = new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 4, payload: staticMapPayload(2, 2, false) },
    ], 30, 30));
    expect(full.staticMap).toMatchObject({ width: 2, height: 2, cellCount: 4, retained: false });
    expect(full.requiresBaseline).toBe(false);

    const retained = new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 4, payload: staticMapPayload(2, 2, true) },
    ], 31, 30));
    expect(retained.staticMap).toMatchObject({ width: 2, height: 2, cellCount: 4, retained: true });
    expect(retained.requiresBaseline).toBe(true);
  });

  it("rejects malformed or flagged retained static-map layouts", () => {
    const truncatedFull = staticMapPayload(2, 2, false).subarray(0, 304 + 3 * 36);
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 4, payload: truncatedFull },
    ]))).toThrow(/static map section layout/i);
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 4, payload: staticMapPayload(2, 2, true), flags: 1 },
    ]))).toThrow(/static map section layout/i);
  });

  it("exposes copied contextual actions on original bounds and shroud on expanded bounds", () => {
    const staticMap = staticMapPayload(4, 3, false, {
      cellX: 4,
      cellY: 7,
      originalCellX: 5,
      originalCellY: 8,
      originalWidth: 2,
      originalHeight: 1,
    });
    const player = new Uint8Array(504 + 2);
    new DataView(player.buffer).setUint32(116, 2, true);
    player[504] = SnapshotContextualAction.Move;
    player[505] = SnapshotContextualAction.Capture;

    const shroud = new Uint8Array(4 * 3 * 2);
    shroud[0] = 0xff;
    shroud[1] = 2;
    const center = (1 * 4 + 1) * 2;
    shroud[center] = 0xf9;
    shroud[center + 1] = 7;
    const bottomRight = (2 * 4 + 3) * 2;
    shroud[bottomRight] = 12;
    shroud[bottomRight + 1] = 1;

    const buffer = sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 12, payload: staticMap },
      { kind: SnapshotSectionKind.Player, count: 1, payload: player },
      { kind: SnapshotSectionKind.Shroud, count: 12, payload: shroud },
    ]);
    const snapshot = new SnapshotView(buffer);

    expect(snapshot.player?.actions).toMatchObject({ originCellX: 5, originCellY: 8, width: 2, height: 1, count: 2 });
    expect(snapshot.player?.actions.atMapCell(5, 8)).toBe(SnapshotContextualAction.Move);
    expect(snapshot.player?.actions.atMapCell(6, 8)).toBe(SnapshotContextualAction.Capture);
    expect(snapshot.player?.actions.atMapCell(4, 8)).toBeUndefined();
    expect(snapshot.player?.actions.atWorldPoint(6 * 24 + 23.5, 8 * 24 + 12)).toBe(SnapshotContextualAction.Capture);
    expect(snapshot.player?.actions.atWorldPoint(Number.NaN, 0)).toBeUndefined();

    expect(snapshot.shroud).toMatchObject({ originCellX: 4, originCellY: 7, width: 4, height: 3, count: 12 });
    expect(snapshot.shroud?.cellAtMapCell(5, 8)).toEqual({
      mapCellX: 5,
      mapCellY: 8,
      shadow: -7,
      visible: true,
      mapped: true,
      jammed: true,
    });
    expect(snapshot.shroud?.cellAtWorldPoint(7 * 24 + 23.5, 9 * 24 + 12)).toEqual({
      mapCellX: 7,
      mapCellY: 9,
      shadow: 12,
      visible: true,
      mapped: false,
      jammed: false,
    });
    expect(snapshot.shroud?.cellAtMapCell(8, 9)).toBeUndefined();
    expect(snapshot.shroud?.cellAtWorldPoint(-1, -1)).toBeUndefined();
    expect(snapshot.shroud?.isVisibleAtMapCell(5, 8)).toBe(true);
    expect(snapshot.shroud?.isVisibleAtMapCell(8, 9)).toBe(false);
    expect(snapshot.shroud?.isVisibleAtWorldPoint(Number.POSITIVE_INFINITY, 0)).toBe(false);

    new Uint8Array(buffer).fill(0);
    expect(snapshot.player?.actions.atMapCell(5, 8)).toBe(SnapshotContextualAction.Move);
    expect(snapshot.shroud?.cellAtMapCell(5, 8)?.shadow).toBe(-7);
  });

  it("rejects malformed contextual-action and shroud layouts", () => {
    const staticMap = staticMapPayload(4, 3, false, {
      cellX: 4,
      cellY: 7,
      originalCellX: 5,
      originalCellY: 8,
      originalWidth: 2,
      originalHeight: 1,
    });
    const shortActions = new Uint8Array(505);
    new DataView(shortActions.buffer).setUint32(116, 1, true);
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 12, payload: staticMap },
      { kind: SnapshotSectionKind.Player, count: 1, payload: shortActions },
    ]))).toThrow(/player action grid dimensions/i);

    const invalidActions = new Uint8Array(506);
    new DataView(invalidActions.buffer).setUint32(116, 2, true);
    invalidActions[505] = SnapshotContextualAction.CannotRepair + 1;
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 12, payload: staticMap },
      { kind: SnapshotSectionKind.Player, count: 1, payload: invalidActions },
    ]))).toThrow(/invalid contextual action/i);

    const shroud = new Uint8Array(24);
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 12, payload: staticMap },
      { kind: SnapshotSectionKind.Shroud, count: 11, payload: shroud.subarray(0, 22) },
    ]))).toThrow(/shroud grid dimensions/i);
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 12, payload: staticMap },
      { kind: SnapshotSectionKind.Shroud, count: 12, payload: shroud.subarray(0, 23) },
    ]))).toThrow(/shroud section layout/i);
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 12, payload: staticMap },
      { kind: SnapshotSectionKind.Shroud, count: 12, payload: shroud, flags: 1 },
    ]))).toThrow(/shroud section layout/i);

    shroud[1] = 8;
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 12, payload: staticMap },
      { kind: SnapshotSectionKind.Shroud, count: 12, payload: shroud },
    ]))).toThrow(/shroud cell flags/i);
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.Shroud, count: 12, payload: new Uint8Array(24) },
    ]))).toThrow(/static map/i);
  });

  it("rejects out-of-range sections before exposing views", () => {
    const buffer = new ArrayBuffer(64);
    const view = new DataView(buffer);
    view.setUint32(0, 0x57434e43, true);
    view.setUint16(4, 1, true);
    view.setUint16(6, 3, true);
    view.setUint32(8, 64, true);
    view.setUint32(12, 1, true);
    view.setUint32(32, 1, true);
    view.setUint16(40, 9, true);
    view.setUint32(44, 128, true);
    expect(() => new SnapshotView(buffer)).toThrow(/outside/i);
  });

  it("keeps canonical world coordinates when the camera is nonzero", () => {
    const palette = new Uint8Array(1024).fill(255);
    const buffer = new ArrayBuffer(snapshotByteLength(2, 2, 1));
    writeSnapshot(buffer, {
      tick: 7,
      worldWidth: 320,
      worldHeight: 200,
      classicWidth: 2,
      classicHeight: 2,
      classicPixels: new Uint8Array(4),
      palette,
      cameraX: 100,
      cameraY: 60,
      zoom: 1,
      sprites: [{ x: 140, y: 90, width: 20, height: 10, u0: 0, v0: 0, u1: 1, v1: 1, atlasPage: 0, flags: 0, sortKey: 1, tint: 0xffffffff, teamColor: 0xffffffff }],
    });
    // Preserve the legacy coordinate flag in the synthetic wire record. The
    // engine adapter has already normalized PositionX/Y to world pixels.
    const objectRecord = 40 + 16;
    new DataView(buffer).setInt32(objectRecord + 156, 0x10, true);
    const snapshot = new SnapshotView(buffer);
    expect(snapshot.cameraX).toBe(100);
    expect(snapshot.cameraY).toBe(60);
    expect(snapshot.sprite(0)).toMatchObject({ x: 140, y: 90 });
  });

  it("uses owner color when the signed remap color is absent", () => {
    const buffer = new ArrayBuffer(snapshotByteLength(1, 1, 1));
    writeSnapshot(buffer, {
      tick: 1,
      worldWidth: 320,
      worldHeight: 200,
      classicWidth: 1,
      classicHeight: 1,
      classicPixels: new Uint8Array(1),
      palette: new Uint8Array(1024),
      cameraX: 0,
      cameraY: 0,
      zoom: 1,
      sprites: [{ x: 1, y: 2, width: 3, height: 4, u0: 0, v0: 0, u1: 1, v1: 1, atlasPage: 0, flags: 0, sortKey: 0, tint: 0xffffffff, teamColor: 0xffffffff }],
    });
    const objectRecord = 40 + 16;
    const view = new DataView(buffer);
    view.setUint8(objectRecord + 182, 2);
    view.setUint8(objectRecord + 183, 0xff);
    expect(new SnapshotView(buffer).sprite(0).tint).toBe(0xff7aa7ca);
  });

  it("exposes validated live sidebar telemetry", () => {
    const buffer = new ArrayBuffer(40 + 16 + 60 + 5 * 128);
    const view = new DataView(buffer);
    view.setUint32(0, 0x57434e43, true);
    view.setUint16(4, 1, true);
    view.setUint16(6, 3, true);
    view.setUint32(8, buffer.byteLength, true);
    view.setUint32(12, 1, true);
    view.setUint32(16, 15, true);
    view.setUint32(20, 15, true);
    view.setUint32(32, 1, true);
    view.setUint16(40, 4, true);
    view.setUint32(44, 60 + 5 * 128, true);
    view.setUint32(48, 5, true);
    const payload = 56;
    view.setInt32(payload, 2, true);
    view.setInt32(payload + 4, 3, true);
    view.setInt32(payload + 8, 1500, true);
    view.setInt32(payload + 24, 200, true);
    view.setInt32(payload + 28, 75, true);
    view.setUint32(payload + 36, 7, true);
    view.setUint32(payload + 40, 2, true);
    view.setUint32(payload + 44, 1, true);
    view.setUint32(payload + 56, 5, true);
    expect(new SnapshotView(buffer).sidebar).toMatchObject({
      leftEntries: 2,
      rightEntries: 3,
      credits: 1500,
      powerProduced: 200,
      powerDrained: 75,
      unitsKilled: 7,
      buildingsKilled: 2,
      unitsLost: 1,
      repairEnabled: true,
      radarActive: true,
    });
  });

  it("parses production, placement, ownership, and safe repair/sell targets", () => {
    const staticMap = new Uint8Array(304 + 16 * 36);
    const staticView = new DataView(staticMap.buffer);
    staticView.setInt32(0, 0, true);
    staticView.setInt32(4, 0, true);
    staticView.setInt32(8, 4, true);
    staticView.setInt32(12, 4, true);
    staticView.setInt32(16, 1, true);
    staticView.setInt32(20, 1, true);
    staticView.setInt32(24, 2, true);
    staticView.setInt32(28, 2, true);
    staticView.setInt32(32, 1, true);
    putText(staticMap, 36, 264, "SCG01EA");
    staticView.setUint32(300, 16, true);

    const sidebar = new Uint8Array(60 + 128);
    const sidebarView = new DataView(sidebar.buffer);
    sidebarView.setInt32(0, 1, true);
    sidebarView.setInt32(4, 0, true);
    sidebarView.setInt32(8, 1250, true);
    sidebarView.setInt32(24, 200, true);
    sidebarView.setInt32(28, 75, true);
    sidebarView.setUint32(56, 3, true);
    const production = 60;
    putText(sidebar, production, 16, "PYLE");
    sidebarView.setInt32(production + 16, 123, true);
    sidebarView.setInt32(production + 20, 4, true);
    sidebarView.setInt32(production + 24, SnapshotObjectType.BuildingType, true);
    sidebarView.setInt32(production + 32, 500, true);
    sidebarView.setInt32(production + 36, -20, true);
    sidebarView.setInt32(production + 40, 450, true);
    sidebarView.setFloat32(production + 44, 0.75, true);
    sidebarView.setUint32(production + 48, 2, true);
    sidebarView.setUint32(production + 52, 1 | 8, true);
    sidebarView.setInt16(production + 56, 0, true);
    sidebarView.setInt16(production + 58, 1, true);

    const placement = new Uint8Array(16).fill(2);
    placement[5] = 3;

    const player = new Uint8Array(504);
    const playerView = new DataView(player.buffer);
    putText(player, 0, 64, "Commander");
    player[64] = 2;
    player[65] = 3;
    player[66] = 4;
    playerView.setInt32(68, 5, true);
    playerView.setBigUint64(72, 99n, true);
    playerView.setInt32(80, 1, true);
    playerView.setInt32(84, 7, true);
    playerView.setUint32(92, (1 << 1) | (1 << 2), true);
    playerView.setUint32(116, 0, true);

    const objects = new Uint8Array(472);
    const objectView = new DataView(objects.buffer);
    putText(objects, 0, 16, "PYLE");
    putText(objects, 16, 16, "PYLE");
    objectView.setInt32(112, SnapshotObjectType.Building, true);
    objectView.setInt32(116, 7, true);
    objectView.setInt32(124, SnapshotObjectType.Unknown, true);
    objectView.setInt32(128, 48, true);
    objectView.setInt32(132, 48, true);
    objectView.setInt32(136, 48, true);
    objectView.setInt32(140, 48, true);
    objectView.setInt32(144, 256, true);
    objectView.setInt32(148, 9, true);
    objectView.setInt32(156, 0x20, true);
    objectView.setInt16(160, 400, true);
    objectView.setInt16(162, 200, true);
    objectView.setUint16(166, 1, true);
    objectView.setUint16(168, 1, true);
    objectView.setUint16(170, 384, true);
    objectView.setUint16(172, 512, true);
    objects[182] = 2;
    objects[185] = SnapshotCloakState.Cloaking;
    objects[186] = 4;
    objectView.setUint32(188, 1 << 2, true);
    objectView.setUint32(204, 1 | (1 << 4) | (1 << 5) | (1 << 9) | (1 << 22), true);
    objectView.setUint16(216, 2, true);
    objectView.setInt16(224, 0, true);
    objectView.setInt16(226, 1, true);
    objects[442] = SnapshotContextualAction.Self;

    const dynamicMap = new Uint8Array(20 + 48);
    const dynamicView = new DataView(dynamicMap.buffer);
    putText(dynamicMap, 20, 16, "SANDBAG");
    dynamicView.setInt32(36, 108, true);
    dynamicView.setInt32(40, 36, true);
    dynamicView.setInt32(44, 24, true);
    dynamicView.setInt32(48, 24, true);
    dynamicView.setInt32(52, 0x20, true);
    dynamicView.setInt16(56, 1, true);
    dynamicMap[58] = 2;
    dynamicMap[60] = 4;
    dynamicMap[61] = 1;
    dynamicView.setUint16(62, 2 | 8, true);

    const snapshot = new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 16, payload: staticMap },
      { kind: SnapshotSectionKind.DynamicMap, count: 1, payload: dynamicMap },
      { kind: SnapshotSectionKind.Objects, count: 1, payload: objects },
      { kind: SnapshotSectionKind.Sidebar, count: 1, payload: sidebar },
      { kind: SnapshotSectionKind.Placement, count: 16, payload: placement },
      { kind: SnapshotSectionKind.Player, count: 1, payload: player },
    ]));

    expect(snapshot.staticMap).toMatchObject({ cellX: 0, cellY: 0, width: 4, height: 4, originalCellX: 1, originalCellY: 1, originalWidth: 2, originalHeight: 2, scenarioName: "SCG01EA" });
    expect(snapshot.player).toMatchObject({ name: "Commander", house: 2, playerId: 99n, allyFlags: (1 << 1) | (1 << 2) });
    const entry = snapshot.sidebarEntry(123, 4)!;
    expect(entry).toMatchObject({ assetName: "PYLE", objectType: SnapshotObjectType.BuildingType, cost: 500, powerDelta: -20, buildTime: 450, progress: 0.75, completed: true, busy: true });
    expect(entry.placementOffsets).toEqual([0, 1]);
    expect(snapshot.sidebar?.leftColumn).toHaveLength(1);
    expect(snapshot.placement).toMatchObject({ originCellX: 0, originCellY: 0, width: 4, height: 4, count: 16 });
    expect(snapshot.placement?.cell(1, 1)).toMatchObject({ mapCellX: 1, mapCellY: 1, passesProximityCheck: true, generallyClear: true });
    expect(snapshot.canPlaceSidebarEntry(entry, 1, 1)).toBe(true);
    expect(snapshot.object(0)).toMatchObject({ id: 7, owner: 2, altitude: 256, fixedWing: true, centerCoordX: 384, centerCoordY: 512, cloak: SnapshotCloakState.Cloaking, controlGroup: 4, selectedMask: 1 << 2, root: true, canRepair: true, canDemolish: true, factory: true });
    expect(snapshot.object(0).actionWithSelected[2]).toBe(SnapshotContextualAction.Self);
    expect(snapshot.findBuildingAtWorldPoint(48, 48, { capability: "repair" })?.id).toBe(7);
    expect(snapshot.findBuildingAtWorldPoint(48, 48, { owner: 1, capability: "repair" })).toBeUndefined();
    expect(snapshot.findSellTargetAtWorldPoint(108, 36)).toMatchObject({ kind: "wall", wall: { assetName: "SANDBAG", owner: 2, sellable: true } });
  });

  it("validates and exposes ordered occupier identities without per-cell storage", () => {
    const staticMap = staticMapPayload(2, 2, true, { cellX: 4, cellY: 5 });
    const occupiers = occupierPayload([
      [],
      [[SnapshotObjectType.Unit, 7], [SnapshotObjectType.Unit, 7], [SnapshotObjectType.Infantry, 3]],
      [[SnapshotObjectType.Building, 9]],
      [],
    ]);
    const snapshot = new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.StaticMap, count: 4, payload: staticMap },
      { kind: SnapshotSectionKind.Occupiers, count: 4, payload: occupiers },
    ]));
    expect(snapshot.occupiers).toMatchObject({ originCellX: 4, originCellY: 5, width: 2, height: 2, count: 4 });
    expect(snapshot.occupiers?.atMapCell(5, 5)).toEqual([
      { type: SnapshotObjectType.Unit, id: 7 },
      { type: SnapshotObjectType.Unit, id: 7 },
      { type: SnapshotObjectType.Infantry, id: 3 },
    ]);
    expect(snapshot.occupiers?.atMapCell(4, 6)).toEqual([{ type: SnapshotObjectType.Building, id: 9 }]);
    expect(snapshot.occupiers?.atMapCell(3, 5)).toEqual([]);
  });

  it("rejects malformed occupier grids", () => {
    const staticMap = staticMapPayload(1, 1, true);
    const validStatic = { kind: SnapshotSectionKind.StaticMap, count: 1, payload: staticMap } as const;
    expect(() => new SnapshotView(sectionSnapshot([
      { kind: SnapshotSectionKind.Occupiers, count: 1, payload: occupierPayload([[]]) },
    ]))).toThrow(/occupier/i);
    expect(() => new SnapshotView(sectionSnapshot([
      validStatic,
      { kind: SnapshotSectionKind.Occupiers, count: 2, payload: occupierPayload([[], []]) },
    ]))).toThrow(/occupier/i);

    const truncated = new Uint8Array(4);
    new DataView(truncated.buffer).setUint32(0, 1, true);
    expect(() => new SnapshotView(sectionSnapshot([
      validStatic,
      { kind: SnapshotSectionKind.Occupiers, count: 1, payload: truncated },
    ]))).toThrow(/occupier/i);

    const excessive = new Uint8Array(4);
    new DataView(excessive.buffer).setUint32(0, 4097, true);
    expect(() => new SnapshotView(sectionSnapshot([
      validStatic,
      { kind: SnapshotSectionKind.Occupiers, count: 1, payload: excessive },
    ]))).toThrow(/occupier/i);

    const invalidType = occupierPayload([[[SnapshotObjectType.VesselType + 1, 1]]]);
    expect(() => new SnapshotView(sectionSnapshot([
      validStatic,
      { kind: SnapshotSectionKind.Occupiers, count: 1, payload: invalidType },
    ]))).toThrow(/occupier/i);

    const trailing = new Uint8Array(5);
    expect(() => new SnapshotView(sectionSnapshot([
      validStatic,
      { kind: SnapshotSectionKind.Occupiers, count: 1, payload: trailing },
    ]))).toThrow(/occupier/i);
  });

  it("rejects inconsistent production ownership and placement layouts", () => {
    const sidebar = new Uint8Array(60);
    new DataView(sidebar.buffer).setInt32(0, 1, true);
    expect(() => new SnapshotView(sectionSnapshot([{ kind: SnapshotSectionKind.Sidebar, count: 0, payload: sidebar }]))).toThrow(/sidebar/i);

    const player = new Uint8Array(504);
    new DataView(player.buffer).setUint32(116, 1, true);
    expect(() => new SnapshotView(sectionSnapshot([{ kind: SnapshotSectionKind.Player, count: 1, payload: player }]))).toThrow(/player/i);

    expect(() => new SnapshotView(sectionSnapshot([{ kind: SnapshotSectionKind.Placement, count: 1, payload: new Uint8Array([3]) }]))).toThrow(/static map/i);

    const object = new Uint8Array(472);
    const objectView = new DataView(object.buffer);
    objectView.setInt32(112, SnapshotObjectType.Unknown, true);
    objectView.setInt32(124, SnapshotObjectType.Unknown, true);
    object[186] = 10;
    expect(() => new SnapshotView(sectionSnapshot([{ kind: SnapshotSectionKind.Objects, count: 1, payload: object }]))).toThrow(/object record/i);

    object[186] = 0xff;
    object[440] = SnapshotContextualAction.CannotRepair + 1;
    expect(() => new SnapshotView(sectionSnapshot([{ kind: SnapshotSectionKind.Objects, count: 1, payload: object }]))).toThrow(/object record/i);

    object[440] = SnapshotContextualAction.None;
    object[185] = SnapshotCloakState.Uncloaking + 1;
    expect(() => new SnapshotView(sectionSnapshot([{ kind: SnapshotSectionKind.Objects, count: 1, payload: object }]))).toThrow(/object record/i);
  });
});
