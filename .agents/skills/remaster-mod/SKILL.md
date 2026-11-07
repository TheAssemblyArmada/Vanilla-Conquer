---
name: remaster-mod
description: >-
  Build the 32-bit Tiberian Dawn and Red Alert remaster DLLs and copy the
  komkommer_TD / komkommer_RA mod folders into the CnC Remastered mods directory.
  Use when the user asks to deploy, install, or debug a remaster mod in game,
  build TiberianDawn.dll or RedAlert.dll, or copy a mod into CnCRemastered.
---

# Remaster mod deploy

Build the remaster DLL and copy the mod folder into the installed Command & Conquer Remastered Collection so the change can be tried in game.

The deploy script is `scripts/deploy.ps1` beside this file. From the repo root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .agents/skills/remaster-mod/scripts/deploy.ps1 -Game TD
```

## Which game

- Tiberian Dawn, TD, or edits under `tiberiandawn/` → `-Game TD`
- Red Alert, RA, or edits under `redalert/` → `-Game RA`
- Both, shared `common/` code, or the user did not say → `-Game Both`

Add `-Config Debug` only when the user wants an unoptimized DLL. The default `RelWithDebInfo` build includes PDBs, and the script copies each PDB next to its DLL.

## After the script

Report the deployed DLL path and timestamp from the script output. The Remastered game loads 32-bit DLLs only; the script refuses to copy anything else.

The game locks the DLL while it is running. If the copy fails, ask the user to exit the game and deploy again. They enable **Komkommer TD** or **Komkommer RA** once in the in-game mods list. A new DLL is picked up the next time that game starts.

Do not launch Steam or the game unless the user asks.
