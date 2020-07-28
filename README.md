# Vanilla Conquer
No extras. No bloat. No major changes. Just the games on your favourite platform.

This project aims to be a clean portable upstream for anyone wanting to mod the Remastered Collection or add features to the original standalone games.

Current project goals are tracked as [GitHub issues with the goal label](https://github.com/Vanilla-Conquer/Vanilla-Conquer/issues?q=is%3Aissue+is%3Aopen+label%3Agoal).

All contributions towards project goals are welcome.
Please use GitHub issues in advance to make sure your work will be accepted.
If you are suggesting a new feature please bear in mind that we do not plan on adding gameplay changes.
Work that do not fit the project will be recommended to be developed as a downstream project.

Developers hang around [The Assembly Armada Discord server](https://discord.gg/UnWK2Tw) if you feel like chatting.

## Building

### Windows

#### Requirements
For Remastered DLLs you need [Visual Studio Build Tools 2019](https://visualstudio.microsoft.com/visual-cpp-build-tools/).
The following components are needed:

 - MSVC v142 C++ x86/x64 build tools
 - Windows 10 SDK
 - CMake

#### Building

From an x86 VS command line window:

```sh
mkdir build
cd build
cmake .. -A win32
cmake --build .
```

This will build all available targets to the build directory.

### Linux (MinGW cross-compiling)

#### Requirements

Recent enough MinGW-w64 packages, CMake and [JWasm](https://www.japheth.de/JWasm.html). On Ubuntu you can install the first two with:

```
sudo apt install mingw-w64 cmake
```

JWasm needs to be compiled from sources and added to `$PATH`.

#### Building

```sh
mkdir build
cd build
cmake ..
make -j
```

This will build all available targets to the build directory.

## Running

### Remastered

The build process will produce `TiberianDawn.dll` and `RedAlert.dll` in your build directory.
These work as mods for the Remastered Collection.

To manually create a local Remastered mod after launching both games once, head to _My Documents/CnCRemastered/CnCRemastered/Mods_.
You should see _Tiberian\_Dawn_ and _Red\_Alert_ directories.

Create a mod directory within either game, we'll call it _Vanilla_. Create a directory inside it called _Data_.

#### Tiberian Dawn

Copy _TiberianDawn.dll_ to the _Data_ directory. Next create a JSON file (a text file) `ccmod.json` in the mod directory and add the following content:

```json
{
    "name": "Vanilla",
    "description": "",
    "author": "",
    "load_order": 1,
    "version_high": 1,
    "version_low": 0,
    "game_type": "TD"
}
```

The directory structure should look like this:

    My Documents/CnCRemastered/CnCRemastered/Mods/Tiberian_Dawn/Vanilla/Data/TiberianDawn.dll
    My Documents/CnCRemastered/CnCRemastered/Mods/Tiberian_Dawn/Vanilla/ccmod.json

You should now see the new mod in the mods list of Tiberian Dawn Remastered.

#### Red Alert

Copy _RedAlert.dll_ to the _Data_ directory. Next create a JSON file (a text file) `ccmod.json` in the mod directory and add the following content:

```json
{
    "name": "Vanilla",
    "description": "",
    "author": "",
    "load_order": 1,
    "version_high": 1,
    "version_low": 0,
    "game_type": "RA"
}
```

The directory structure should look like this:

    My Documents/CnCRemastered/CnCRemastered/Mods/Red_Alert/Vanilla/Data/RedAlert.dll
    My Documents/CnCRemastered/CnCRemastered/Mods/Red_Alert/Vanilla/ccmod.json

You should now see the new mod in the mods list of Tiberian Dawn Remastered.

### VanillaTD and VanillaRA

Copy the Vanilla executable (`vanillatd.exe` or `vanillara.exe`) to your legacy game directory and run.

For Tiberian Dawn the final freeware Gold CD release ([GDI](https://www.fileplanet.com/archive/p-63497/Command-Conquer-Gold), [NOD](https://www.fileplanet.com/archive/p-8778/Command-Conquer-Gold)) works fine.

For Red Alert on top of the freeware [CD release](https://web.archive.org/web/20080901183216/http://www.ea.com/redalert/news-detail.jsp?id=62) you need the unreleased [3.03 patch](https://www.moddb.com/games/cc-red-alert/downloads/red-alert-303-beta-english-patch).
