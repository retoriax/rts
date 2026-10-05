# RTS Aufgabe 1 - Blinky on the Nucleo-F439ZI

Zephyr app for the ST Nucleo-F439ZI. LD1 (green) blinks, and pressing the user
button B1 switches between slow (500 ms) and fast (100 ms) blinking. The code is
in `src/main.c`.

Zephyr, west and the SDK all run in a VS Code dev container. Only OpenOCD runs
on the host, since it needs USB access to the board.

## How flashing works

Docker Desktop on macOS/Windows can't pass USB devices through to a container.
Because of that `west flash` in the container doesn't call the real OpenOCD but
`.devcontainer/openocd-remote`, a small Python script that connects to OpenOCD
on the host over GDB (port 3333) and loads the firmware that way:

```
container:  west flash -> openocd-remote --TCP--> host.docker.internal:3333
host:       OpenOCD -> USB -> ST-Link -> board
```

The host doesn't need access to the build directory. Linux uses the same setup,
so it works the same on all three systems.

## Setup

You need:

- VS Code with the Dev Containers extension
- Docker (Docker Desktop, on Linux Docker Engine is enough) with at least 4 CPUs
  and 8 GB RAM. Zephyr + SDK take about 10 GB of disk space.
- OpenOCD on the host

### macOS

```bash
brew install open-ocd
```

No USB driver needed.

### Windows

- Docker Desktop with the WSL 2 backend
- ST-Link driver: [STSW-LINK009](https://www.st.com/en/development-tools/stsw-link009.html)
- OpenOCD from [xPack](https://github.com/xpack-dev-tools/openocd-xpack/releases):
  unzip it somewhere (e.g. `C:\Tools\openocd`) and add the `bin` folder to `PATH`

The shell scripts need LF line endings, `.gitattributes` takes care of that. If
you still get `bad interpreter` or `$'\r': command not found`, run
`git config --global core.autocrlf input` and clone the repo again.

### Linux

```bash
sudo usermod -aG docker $USER   # then log out and back in
sudo apt install openocd
```

For USB access without root, OpenOCD's udev rules have to be in
`/etc/udev/rules.d/`. If they're missing, copy
`/usr/share/openocd/contrib/60-openocd.rules` there, run
`sudo udevadm control --reload-rules && sudo udevadm trigger` and replug the
board.

## Dev container

Open the folder in VS Code and run "Dev Containers: Reopen in Container". On the
first start `.devcontainer/post-create.sh` downloads Zephyr, the modules and the
SDK into the Docker volume `zephyr-workdir`, which takes 10-20 minutes. After
that, rebuilding the container is fast because the volume is kept.

Zephyr version and SDK toolchains are set in `.devcontainer/devcontainer.json`
(`ZEPHYR_REVISION`, currently `v4.4.0`, and `SDK_TOOLCHAINS`, currently
`arm-zephyr-eabi`).

## Build

In the container:

```bash
west build -p always -b nucleo_f439zi .
```

IntelliSense reads the include paths from `build/compile_commands.json`, so the
`#include <zephyr/...>` errors only go away after the first build (maybe
"Developer: Reload Window" afterwards).

## Flash

Connect the board via USB (CN1, the ST-Link side) and start OpenOCD on the host,
not in the container:

```bash
openocd -f board/st_nucleo_f4.cfg
```

On Linux add `-c "bindto 0.0.0.0"`, because the container reaches the host
through the Docker bridge and not through localhost. Same on Windows if the
container can't connect. OpenOCD is then reachable from the network, so don't
do this on an untrusted network.

Once OpenOCD shows `Listening on port 3333 for gdb connections`, flash from the
container:

```bash
west flash
```

`west debug` works the same way as long as OpenOCD is running.

Alternative without OpenOCD: the ST-Link also shows up as a USB drive
(`NOD_F439ZI`), copying `build/zephyr/zephyr.bin` onto it flashes the board.

The ST-Link also has a virtual COM port (115200 baud) where the Zephyr boot
banner shows up, e.g. `screen /dev/tty.usbmodem* 115200` on macOS or
`picocom -b 115200 /dev/ttyACM0` on Linux. On Windows use PuTTY with the COM
port from the Device Manager.

## Known problems

- `Kein OpenOCD auf dem Host erreichbar`: OpenOCD isn't running on the host or
  the board isn't connected.
- `west flash` complains about `... are zephyr build directories`: the last
  build was interrupted (e.g. container rebuild), just build again.
- `Error finishing flash operation` happens sometimes, running `west flash`
  again usually works. Otherwise restart OpenOCD.
- `Unable to match requested speed ...` can be ignored.
- `open failed` / `LIBUSB_ERROR_ACCESS` from OpenOCD: driver (Windows) or udev
  rules (Linux) missing. Charge-only USB cables also cause this.
- "Host requirements not met" when opening the container: Docker has less RAM
  than set in `hostRequirements`. Increase it in Docker or just click Continue.
