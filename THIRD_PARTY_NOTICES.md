# Third Party Notices

This repository contains the X68000-specific mruby helper gem, samples,
documentation, and bundled third-party headers used by the helper gem. It
does not bundle mruby, mruby/c, xdev68k, elf2x68k, GCC, newlib, or emulator
binaries.

## mruby

mruby is licensed under the MIT License.

- Project: https://github.com/mruby/mruby
- License: https://github.com/mruby/mruby/blob/master/LICENSE

If distributing a binary built from mruby, include mruby's copyright notice and
license text with the binary distribution.

## libzm2

The Z-MUSIC wrapper code uses libzm2, a header-only C library for Z-MUSIC v2
function calls.

- Project: https://github.com/kg68k/libzm2
- Author: TcbnErik / kg68k
- Bundled files: `third_party/libzm2/include/`
- License: GNU GPLv3 or later, with the GCC Runtime Library Exception 3.1
- License texts: `third_party/libzm2/COPYING`,
  `third_party/libzm2/COPYING.RUNTIME`

The upstream README states that executable files created using this library
are not subject to distribution restrictions from libzm2. The libzm2 author
also confirmed that mruby-x68k may use libzm2 and distribute binaries built
with it.


## mattn/mruby-require

The `load` / `require` implementation in `mruby-x68k-os` is adapted from
`mattn/mruby-require`. Dynamic shared-library loading from the original
implementation is not used on Human68k; mruby-x68k supports Ruby source
loading in compiler-enabled builds and `.mrb` loading in VM builds.

- Project: https://github.com/mattn/mruby-require
- Author: mattn
- License: MIT

## elf2x68k

elf2x68k is developed separately by yunkya2.

- Project: https://github.com/yunkya2/elf2x68k

Follow the license terms from the upstream repository when using or
redistributing elf2x68k or its files.

## xdev68k

xdev68k is developed separately by yosshin4004.

- Project: https://github.com/yosshin4004/xdev68k

Follow the license terms from the upstream repository when using or
redistributing xdev68k or its files.

## Toolchain Components

GCC, binutils, newlib, and related runtime components have their own licenses.
This repository does not redistribute them. If you publish a binary package that
includes toolchain output or runtime objects, review the corresponding
`COPYING` files from the toolchain distribution.

## mruby/c

mruby/c is not included in this repository at this stage.

## Sample Z-MUSIC Assets

The files under `samples/ZMD/` are small sample assets prepared for this
repository's examples. They are included so the Z-MUSIC samples and
`maze_chase.rb` can run with sound immediately after checkout.
