# Vanilla Conquer
Vanilla Conquer is a fully portable version of the first generation C&C engine and is capable of running both Tiberian Dawn and Red Alert on multiple platforms. It can also be used for mod development for the Remastered Collection.

The main focus of Vanilla Conquer is to keep the default out-of-box experience faithful to what the games were back when they were released and work as a drop-in replacement for the original executables while also providing bug fixes, compatiblity and quality of life improvements.

Current project goals are tracked as [GitHub issues with the goal label](https://github.com/Vanilla-Conquer/Vanilla-Conquer/issues?q=is%3Aissue+is%3Aopen+label%3Agoal).

Developers hang around [The Assembly Armada Discord server](https://discord.gg/UnWK2Tw) if you feel like chatting.

## Building

We support wide variety of compilers and platforms to target. Vanilla Conquer is known to compile with recent enough gcc, MSVC, mingw-w64 or clang and known to run on Windows, Linux, macOS and BSDs.

### Windows

#### Requirements

The following components are needed to build Vanilla Conquer executables:

 - [MSVC v142 C++ x86/x64 build tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)
 - Windows 10 SDK
 - CMake (installable from MSVC build tools)
 - [SDL2 development libraries, Visual C++](https://libsdl.org/download-2.0.php)
 - [OpenAL Core SDK](https://www.openal.org/downloads/)

Extract SDL2 and OpenAL somewhere you know. If you are building only Remastered dlls you can skip installing SDL2 and OpenAL.

#### Building

In a VS command line window:

```sh
mkdir build
cd build
cmake .. -DSDL2_ROOT_DIR=C:\path\to\SDL2 -DOPENAL_ROOT=C:\path\to\OpenAL
cmake --build .
```

This will build Vanilla Conquer executables in the build directory. If you are building Remastered dlls you need to configure cmake with `-A win32` and ensure your VS command line is x86.

### Linux / macOS / BSD

#### Requirements

- GNU C++ Compiler (g++) or Clang
- CMake
- SDL2
- OpenAL

On Debian/Ubuntu you can install the build requirements as follows:

```
sudo apt-get update
sudo apt-get install g++ cmake libsdl2-dev libopenal-dev
```

#### Building

```sh
mkdir build
cd build
cmake ..
make -j8
```

This will build Vanilla Conquer executables in the build directory.

## Running

### VanillaTD and VanillaRA

Copy the Vanilla executable (`vanillatd.exe` or `vanillara.exe`) to your legacy game directory, on Windows also copy `SDL2.dll` and `OpenAL32.dll`.

For Tiberian Dawn the final freeware Gold CD release ([GDI](https://www.fileplanet.com/archive/p-63497/Command-Conquer-Gold), [NOD](https://www.fileplanet.com/archive/p-8778/Command-Conquer-Gold)) works fine.

For Red Alert the freeware [CD release](https://web.archive.org/web/20080901183216/http://www.ea.com/redalert/news-detail.jsp?id=62) works fine as well.
The official [Red Alert demo](https://www.moddb.com/games/cc-red-alert/downloads/command-conquer-red-alert-demo) is also fully playable.
The demo supports custom skirmish maps (except interior) and includes one campaign mission for both Allied and Soviet from the retail game.

While it is possible to use the game data from the Remastered Collection, The Ultimate Collection or The First Decade they are currently _not_ supported.
Any repackaged version that you may already have from any unofficial source is _not_ supported.
If you encounter a bug that may be data related like invisible things or crashing when using a certain unit please retest with the retail data first before submitting a bug report.

### Remastered

The build process will produce `TiberianDawn.dll` and `RedAlert.dll` in your build directory if you enable them with `-DBUILD_REMASTERTD=ON` and `-DBUILD_REMASTERRA=ON`.
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
