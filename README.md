# PPSS5

PPSS5 is a custom PlayStation 5 emulator built on [KytyPS5](https://github.com/KytyPS5/KytyPS5)
(itself based on [Kyty](https://github.com/InoriRus/Kyty)). It is licensed under GPL-2.0, like
upstream. The upstream README, with full build instructions for Windows, Linux, macOS, NixOS and
Fedora, is kept as [UPSTREAM_README.md](UPSTREAM_README.md).

PPSS5 is not affiliated with Sony Interactive Entertainment. It does not include games or Sony
system software. Use only game dumps you own.

## Firmware

**KytyPS5 does not use any PS5 firmware.** It is a high-level emulator: it re-implements the PS5
system libraries (`libkernel`, `libSceAgc`, `libSceVideoOut`, and so on) in C++. It never loads a
`PS5UPDATE.PUP` or any Sony system module, and it never tells a game which firmware it is running.

PPSS5 adds a **virtual firmware**: a system software version the emulator reports to games.

- Default: **10.00** (`0x10000000`).
- Games read it through `sceKernelGetSystemSwVersion`, which PPSS5 implements.
- At boot, PPSS5 reads `requiredSystemSoftwareVersion` from the game's `sce_sys/param.json`. If the
  game needs a newer firmware than the one installed, PPSS5 logs a warning with the command that
  fixes it.
- The installed version is saved in `ppss5_firmware.ini` in the working directory.

### Updating the firmware

Because no Sony files are involved, an update just raises the reported version:

```bash
kyty_emulator --firmware-info               # show the installed version
kyty_emulator --firmware-update 11.20       # install a higher version
kyty_emulator --game <dir> --firmware 12.00 # use a version for one boot only, nothing saved
```

- `--firmware-update` refuses to go to a lower version. Add `--firmware-allow-downgrade` to
  force it.
- `--firmware-profile <file>` uses a different profile file.
- Versions are written `X.YY` or `X.YY.ZZ`, with each part from 0 to 99.

You can also edit `ppss5_firmware.ini` directly:

```ini
version=11.20
```

## Building

Linux, without the Qt launcher:

```bash
git submodule update --init --recursive
cmake -S . -B _Build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DKYTY_BUILD_LAUNCHER=OFF
cmake --build _Build/linux --target kyty_emulator kyty_tests --parallel
ctest --test-dir _Build/linux -R firmware
```

See [UPSTREAM_README.md](UPSTREAM_README.md) for the required packages and for other platforms.

## Changes from KytyPS5

- `src/common/firmware.{h,cpp}`: virtual firmware versions, profile file, update and downgrade
  rules.
- `src/libs/libKernel.cpp`: `sceKernelGetSystemSwVersion` (NID `Mv1zUObHvXI`) reports the virtual
  firmware.
- `src/loader/systemContent.cpp`: reads `requiredSystemSoftwareVersion` from `param.json`.
- `src/emulator.cpp`: logs the active firmware and warns when a game needs a newer one.
- `src/main.cpp`: the `--firmware*` command-line options.
- `tests/FirmwareTests.cpp`: unit tests (`firmware_tests`).
- The launcher's update checker looks for releases in this repository, not in KytyPS5.
