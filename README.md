Vanilla Conquer
===============
No extras. No bloat. No major changes. Just the games and libraries.

- executable targets for standalone builds
- portability to any SDL2 capable system (Windows, Linux, BSD, macOS, Android, web browser etc.)
- ability to develop for remastered collection (if possible to keep compatibility)
- merge as much of TD and RA codebases to the extent it makes sense
- replace all x86 assembly with plain C
- general code cleanup, refactors, a lot of this
- clean and consistent git history for easy bisecting from the start, no merge commits
- possible to track any bug all the way back to CMake port if it can be reproduced as a remastered dll (if possible to keep compatibility)

Goals in order
--------------
0. [X] Clean upstream fork to preserve history, README
1. [X] Lowercase TIBERIANDAWN and REDALERT directories including contents (portability)
2. [X] Add proper .clang-format, reformat all files
3. [X] Change build system from VS to CMake VS (portability)
4. [ ] Replace x86 assembly with plain C (portability)
5. [ ] MinGW GCC compatibility for cross compiling (portability)
6. [ ] Add executable targets (DirectDraw enabled, windowed mode)
7. [ ] Common SDL2 renderer port (portability)
8. [ ] Reimplement sound support with SDL2

This list is updated or reordered when seen fit.

Linux support?
--------------
Yes, that is pretty much the whole point here to be honest.

Will we get there?
------------------
No. This project will be dead in the water.
