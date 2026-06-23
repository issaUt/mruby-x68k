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

## Build Variants

mruby-x68k currently uses two local gems.

```text
mruby-x68k-os        Human68k file/dir/env/command helpers
mruby-x68k-hardware  Graph/Input/Interrupt/Z-MUSIC and other X68000 helpers
```

The X68000 build configs are split by two axes:

- `.rb` direct execution or `.mrb` precompiled execution
- full hardware APIs or OS-only APIs

```text
build_config/x68k.rb         Full build, runs .rb directly
build_config/x68k-os.rb      OS-only build, runs .rb directly
build_config/x68k-mrb.rb     Full build, runs precompiled .mrb only
build_config/x68k-os-mrb.rb  OS-only build, runs precompiled .mrb only
build_config/x68k-all.rb     Builds all of the above
```

## Recommended Build

For normal development, build the full `.rb` runner:

```sh
cd /home/utsu/work/elf2x68k/mruby-host
./minirake MRUBY_CONFIG=build_config/x68k.rb
```

Outputs:

```text
build/x68k/bin/mruby.x
build/x68k/bin/mrbc.x
```

`mruby.x` can run `.rb` files directly. `mrbc.x` compiles `.rb` files to RiteBinary `.mrb` files on X68000.

## Precompiled VM Builds

For distribution, use the compiler-free `.mrb` VM builds.

```sh
./minirake MRUBY_CONFIG=build_config/x68k-mrb.rb
./minirake MRUBY_CONFIG=build_config/x68k-os-mrb.rb
```

Outputs:

```text
build/x68k-mrb/bin/mrb.x
build/x68k-os-mrb/bin/mrb.x
```

`mrb.x` only runs precompiled `.mrb` files. It does not include `mruby-compiler`, so it is much smaller than `mruby.x`.

## About mruby/c

The `mrb.x` and `mrb-os.x` binaries are not mruby/c.
They are built from the normal mruby source tree and use mruby's RiteBinary `.mrb` execution path.
The only difference from `mruby.x` is that they do not include the mruby compiler and cannot parse `.rb` source code by themselves.

Use this terminology in release notes and documentation:

```text
mruby.x     full mruby runner, can execute .rb directly
mrbc.x      mruby compiler, converts .rb to .mrb
mrb.x       mruby RiteBinary VM, runs .mrb only
mrb-os.x    OS-only mruby RiteBinary VM, runs .mrb only
mruby/c     separate project; not used by these binaries
```

`.mrb` compatibility should be treated as compatibility with the same mruby version and a compatible gem/API set.
For example, an `.mrb` using `X68k::Graph` or `X68k::ZMusic` should be run with `mrb.x`, not `mrb-os.x`.

Typical workflow:

```sh
# compile
build/x68k/bin/mrbc.x -o program.mrb program.rb

# run
build/x68k-mrb/bin/mrb.x program.mrb
```

The `x68k-mrb` VM includes both OS and hardware APIs. The `x68k-os-mrb` VM includes only OS APIs and is useful for smaller command-line style tools.

## Build Everything

To build all four target variants in one run:

```sh
./minirake MRUBY_CONFIG=build_config/x68k-all.rb
```

Expected outputs:

```text
build/x68k/bin/mruby.x
build/x68k/bin/mrbc.x
build/x68k-os/bin/mruby.x
build/x68k-os/bin/mrbc.x
build/x68k-mrb/bin/mrb.x
build/x68k-os-mrb/bin/mrb.x
```

The OS-only `.rb` runner is mainly for checking OS-only scripts before using `x68k-os-mrb`. The normal interactive/development binary is the full `mruby.x`.

## Local Test Copy

For XM6 TypeG testing on Windows, the binary and samples are copied to:

```text
D:\home\emulator\X68000\xm6TypeG_Windrv\elf2x68k
```

The local helper scripts may rename the VM binaries during copy, for example:

```text
mrb.x      Full .mrb VM
mrb-os.x   OS-only .mrb VM
```

## Notes

The `mrblib/*.rb` files are compiled into each binary. They do not need to be loaded with `require` at runtime when using the built binary.

The `.mrb` VM can only execute precompiled RiteBinary files. It cannot parse or compile `.rb` source code by itself.
