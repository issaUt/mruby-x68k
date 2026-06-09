# Third Party Notices

This repository is intended to contain only the X68000-specific mruby helper
gem, samples, and documentation. It does not bundle mruby, mruby/c, xdev68k,
elf2x68k, GCC, newlib, or emulator binaries.

## mruby

mruby is licensed under the MIT License.

- Project: https://github.com/mruby/mruby
- License: https://github.com/mruby/mruby/blob/master/LICENSE

If distributing a binary built from mruby, include mruby's copyright notice and
license text with the binary distribution.

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
