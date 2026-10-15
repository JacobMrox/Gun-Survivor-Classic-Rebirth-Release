# Survivor Fix

Optional add-on for Classic Rebirth that brings back zombie behaviours of the
PlayStation version of Resident Evil Survivor, ported from the original PS1 code
(SLUS-01087). It does **not** modify `gun_bio.exe` or any game file: it hooks the
game in memory and every fix can be switched off in `survivor_fix.ini`.

Only the Taiwanese/Chinese PC executable supported by Classic Rebirth is handled
(SHA-256 `2e5b7a3d4d53a68499b36be5ec45fffa30ccfaaca24e66650cccdda8651be3f9`). On any
other executable the fixes stay off and the reason is written to the log.

## What it changes

| Setting | Default | What you see |
|---|---|---|
| `Ps1ZombieWalk` | on | Zombies keep one walk cycle, like on PS1. The PC port re-rolled the walk every few frames (it checks the end of the animation on a track that loops every 4 frames in two of the walks), which made zombies jerk and "reset". |
| `Ps1ZombieNavigation` | on | Zombies chase like on PS1: they steer while walking and go around walls using the room wall data that ships with the game (`Resource\ROOMENV.DAT`, never read by the PC port) instead of walking into them. Zombies only, for now. |
| `Ps1ZombieStumble` | on | Zombies can stumble forward and get back up while chasing you (about 1 in 20 approaches, as on PS1). |

## Install

1. Install Classic Rebirth as usual.
2. Copy `dsound.dll` and `survivor_fix.ini` into the game folder, next to `gun_bio.exe`.
3. Start the game as usual (`gun_bio.exe`).

`dsound.dll` forwards every DirectSound call to the system `dsound.dll`, so sound keeps working;
Classic Rebirth does not use it. Settings can be changed while the game runs: the file is re-read
within a second.

## Uninstall

Delete `dsound.dll`, `survivor_fix.ini` and `survivor_fix.log`.

## Reporting problems

Attach `survivor_fix.log` (created in the game folder) and describe what you did.

## Source and credits

Source code, PS1 references (MIPS addresses for every fix) and tests are in `source/`;
the research notes for each fix are in `source/docs/` (in Italian). To build `dsound.dll`
(Windows, Visual Studio with C++ and CMake; MinHook is downloaded by CMake):

```
cmake -S source -B build -A Win32
cmake --build build --config Release --target survivor_fix_dsound
```
Built with CMake and Visual Studio (Win32 Release). Uses [MinHook](https://github.com/TsudaKageyu/minhook)
(BSD 2-Clause, see `THIRD-PARTY-NOTICES.txt`). Survivor Fix is released under the MIT License (`LICENSE`).
