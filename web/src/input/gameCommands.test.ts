import { describe, expect, it } from "vitest";
import {
  CommandType,
  ControlGroupRequest,
  DllObjectType,
  GameRequest,
  InputRequest,
  SidebarRequest,
  StructureRequest,
  SuperweaponRequest,
  UnitRequest,
} from "../simulation/protocol";
import {
  boxSelectCommand,
  cancelPlacementCommand,
  cancelProductionCommand,
  cancelStructureActionCommand,
  createControlGroupCommand,
  holdProductionCommand,
  movieDoneCommand,
  placeProductionCommand,
  pointCommand,
  repairStructureCommand,
  selectControlGroupCommand,
  sellStructureCommand,
  sellWallAtWorldCommand,
  startPlacementCommand,
  startProductionCommand,
  startRepairCommand,
  startSellCommand,
  stopSelectedCommand,
  targetSuperweaponCommand,
} from "./gameCommands";

describe("touch action commands", () => {
  it("distinguishes selection from contextual orders", () => {
    expect(pointCommand("select", { x: 10.4, y: 20.6 })).toEqual({ type: CommandType.Input, args: [InputRequest.SelectAtPosition, 10, 21, 0, 0, 0, 0] });
    expect(pointCommand("order", { x: 10, y: 20 })).toEqual({ type: CommandType.Input, args: [InputRequest.CommandAtPosition, 10, 20, 0, 0, 0, 0] });
    expect(pointCommand("select", { x: 10, y: 20 }, true)).toEqual({ type: CommandType.Input, args: [InputRequest.CommandAtPosition, 10, 20, 0, 0, 0, 0] });
  });

  it("encodes box selection and immediate stop through supported engine requests", () => {
    expect(boxSelectCommand({ x: 1, y: 2 }, { x: 30, y: 40 })).toEqual({ type: CommandType.Input, args: [InputRequest.MouseArea, 1, 2, 30, 40, 0, 0] });
    expect(stopSelectedCommand()).toEqual({ type: CommandType.Unit, args: [UnitRequest.Stop, 0, 0, 0, 0, 0, 0] });
  });

  it("encodes all native control-group operations with zero-based indices", () => {
    expect(createControlGroupCommand(0)).toEqual({
      type: CommandType.ControlGroup,
      args: [ControlGroupRequest.Create, 0, 0, 0, 0, 0, 0],
    });
    expect(selectControlGroupCommand(9)).toEqual({
      type: CommandType.ControlGroup,
      args: [ControlGroupRequest.Toggle, 9, 0, 0, 0, 0, 0],
    });
    expect(selectControlGroupCommand(4, true)).toEqual({
      type: CommandType.ControlGroup,
      args: [ControlGroupRequest.AdditiveSelection, 4, 0, 0, 0, 0, 0],
    });
    expect(() => createControlGroupCommand(-1)).toThrow(/control group index/i);
    expect(() => selectControlGroupCommand(10)).toThrow(/control group index/i);
    expect(() => selectControlGroupCommand(1.5)).toThrow(/control group index/i);
  });

  it("acknowledges omitted movie presentation through the game command contract", () => {
    expect(movieDoneCommand()).toEqual({ type: CommandType.Game, args: [GameRequest.MovieDone, 0, 0, 0, 0, 0, 0] });
  });

  it("mirrors the legacy production and object enum values without changing protocol v1", () => {
    expect(StructureRequest).toMatchObject({ RepairStart: 1, Repair: 2, SellStart: 3, Sell: 4, Cancel: 5 });
    expect(SidebarRequest).toMatchObject({
      StartConstruction: 0,
      HoldConstruction: 1,
      CancelConstruction: 2,
      StartPlacement: 3,
      Place: 4,
      CancelPlacement: 5,
    });
    expect(SuperweaponRequest.Place).toBe(0);
    expect(DllObjectType).toMatchObject({ Building: 4, Special: 11, InfantryType: 12, UnitType: 13, AircraftType: 14, BuildingType: 15 });
  });

  it("encodes construction lifecycle actions using the sidebar entry wire identity", () => {
    const identity = { buildableType: 17, buildableId: 23 };
    expect(startProductionCommand(identity)).toEqual({ type: CommandType.Sidebar, args: [SidebarRequest.StartConstruction, 17, 23, 0, 0, 0, 0] });
    expect(holdProductionCommand(identity)).toEqual({ type: CommandType.Sidebar, args: [SidebarRequest.HoldConstruction, 17, 23, 0, 0, 0, 0] });
    expect(cancelProductionCommand(identity)).toEqual({ type: CommandType.Sidebar, args: [SidebarRequest.CancelConstruction, 17, 23, 0, 0, 0, 0] });
    expect(startPlacementCommand(identity)).toEqual({ type: CommandType.Sidebar, args: [SidebarRequest.StartPlacement, 17, 23, 0, 0, 0, 0] });
    expect(cancelPlacementCommand(identity)).toEqual({ type: CommandType.Sidebar, args: [SidebarRequest.CancelPlacement, 17, 23, 0, 0, 0, 0] });
  });

  it("rounds and bounds map-relative placement cells to the adapter's int16 contract", () => {
    const identity = { buildableType: 17, buildableId: 23 };
    expect(placeProductionCommand(identity, { x: 12.4, y: -3.6 })).toEqual({
      type: CommandType.Sidebar,
      args: [SidebarRequest.Place, 17, 23, 12, -4, 0, 0],
    });
    expect(placeProductionCommand(identity, { x: -0.2, y: 32_767 })).toEqual({
      type: CommandType.Sidebar,
      args: [SidebarRequest.Place, 17, 23, 0, 32_767, 0, 0],
    });
    expect(() => placeProductionCommand(identity, { x: 32_767.5, y: 0 })).toThrow(/placement cell x/i);
    expect(() => placeProductionCommand(identity, { x: 0, y: -32_768.6 })).toThrow(/placement cell y/i);
    expect(() => placeProductionCommand(identity, { x: Number.NaN, y: 0 })).toThrow(/finite/i);
  });

  it("encodes repair and sell actions against exact engine object IDs", () => {
    expect(startRepairCommand()).toEqual({ type: CommandType.Structure, args: [StructureRequest.RepairStart, 0, 0, 0, 0, 0, 0] });
    expect(repairStructureCommand(91)).toEqual({ type: CommandType.Structure, args: [StructureRequest.Repair, 91, 0, 0, 0, 0, 0] });
    expect(startSellCommand()).toEqual({ type: CommandType.Structure, args: [StructureRequest.SellStart, 0, 0, 0, 0, 0, 0] });
    expect(sellStructureCommand(91)).toEqual({ type: CommandType.Structure, args: [StructureRequest.Sell, 91, 0, 0, 0, 0, 0] });
    expect(cancelStructureActionCommand()).toEqual({ type: CommandType.Structure, args: [StructureRequest.Cancel, 0, 0, 0, 0, 0, 0] });
    expect(() => repairStructureCommand(1.5)).toThrow(/signed 32-bit integer/i);
    expect(() => sellStructureCommand(0x8000_0000)).toThrow(/signed 32-bit integer/i);
  });

  it("encodes wall selling and superweapon targeting in absolute world pixels", () => {
    expect(sellWallAtWorldCommand({ x: 100.49, y: -20.5 })).toEqual({
      type: CommandType.Input,
      args: [InputRequest.SellAtPosition, 100, -20, 0, 0, 0, 0],
    });
    expect(targetSuperweaponCommand({ buildableType: 20, buildableId: 2 }, { x: 456.6, y: 789.2 })).toEqual({
      type: CommandType.Superweapon,
      args: [SuperweaponRequest.Place, 20, 2, 457, 789, 0, 0],
    });
  });

  it("rejects non-finite or adapter-unsafe world positions consistently", () => {
    expect(pointCommand("select", { x: 0x3fff_ffff, y: -0x3fff_ffff })).toMatchObject({
      args: [InputRequest.SelectAtPosition, 0x3fff_ffff, -0x3fff_ffff, 0, 0, 0, 0],
    });
    expect(() => pointCommand("order", { x: Number.POSITIVE_INFINITY, y: 0 })).toThrow(/finite/i);
    expect(() => boxSelectCommand({ x: 0, y: 0 }, { x: 0x4000_0000, y: 0 })).toThrow(/selection end x/i);
    expect(() => sellWallAtWorldCommand({ x: Number.NaN, y: 0 })).toThrow(/finite/i);
    expect(() => targetSuperweaponCommand({ buildableType: 1, buildableId: 2 }, { x: 0, y: -0x4000_0000 })).toThrow(/superweapon world point y/i);
  });

  it("rejects invalid sidebar wire identities before the command reaches DataView coercion", () => {
    expect(() => startProductionCommand({ buildableType: 1.2, buildableId: 2 })).toThrow(/buildable type/i);
    expect(() => targetSuperweaponCommand({ buildableType: 1, buildableId: Number.NaN }, { x: 0, y: 0 })).toThrow(/buildable ID/i);
  });
});
