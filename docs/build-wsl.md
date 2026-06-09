# Build on WSL

This project was developed with a WSL Ubuntu cross-build environment.

The current local paths were:

```text
/home/utsu/work/elf2x68k
/home/utsu/work/elf2x68k/mruby-host
```

## Expected Tools

- X68000 cross compiler environment based on elf2x68k / xdev68k
- mruby source tree
- Ruby and rake on the host side

Reference projects:

- https://github.com/yunkya2/elf2x68k
- https://github.com/yosshin4004/xdev68k

## Build

Copy this repository's gem into the mruby source tree:

```text
mrbgems/mruby-x68k-stdio
```

Enable the gem from the X68000 build configuration, then build:

```sh
cd /home/utsu/work/elf2x68k/mruby-host
MRUBY_CONFIG=x68k rake -j2
```

The resulting binary is:

```text
build/x68k/bin/mruby.x
```

In the local test environment it was copied to:

```sh
cp build/x68k/bin/mruby.x /home/utsu/work/elf2x68k/mruby.x
```

For XM6 TypeG testing on Windows, the binary and samples were copied to:

```text
D:\home\emulator\X68000\xm6TypeG_Windrv\elf2x68k
```

## Notes

The `mrblib/*.rb` files are compiled into `mruby.x`. They do not need to be
loaded with `require` at runtime when using the built binary.
