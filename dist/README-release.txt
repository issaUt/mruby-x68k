mruby-x68k v0.3.1

This archive contains mruby.x and runnable samples for Sharp X68000 / Human68k.

Contents:
- mruby.x: mruby interpreter binary for X68000
- samples/: Ruby samples and sample assets
- README-release.txt
- release-notes-v0.3.1.md
- LICENSE
- THIRD_PARTY_NOTICES.md
- third_party/libzm2/COPYING
- third_party/libzm2/COPYING.RUNTIME

Example:
  mruby --version
  mruby maze_chase.rb
  mruby maze_chase.rb joy
  mruby maze_chase.rb wait=2
  mruby zm_game_audio.rb
  mruby ajoy_chk.rb
  mruby cyber_flight.rb
  mruby joy_sega6b_test.rb
  mruby backquote_chk.rb

Version output:
  mruby-x68k 0.3.1
  target: X68000 / Human68k
  mruby 4.0.0 (2026-04-20)

Notes:
- Z-MUSIC samples require resident Z-MUSIC.X. Sound font/tone setup depends on your Z-MUSIC environment.
- Z-MUSIC function calls use libzm2. See THIRD_PARTY_NOTICES.md for details.
- CyberStick / analog joystick samples require resident AJOY.X.
- SEGA 3B/6B samples require an adapter where TH/Select is wired to joystick pin 8.
- Backquote support is provisional. It captures stdout through temporary-file redirection.
- maze_chase.rb runs without Z-MUSIC when noaudio is specified.
- This binary was built for the experimental mruby-x68k runtime.
- The source repository does not bundle mruby, xdev68k, elf2x68k, GCC, newlib, emulator binaries, Z-MUSIC.X, or AJOY.X.
- mruby is licensed under the MIT License. If redistributing this binary, include mruby's license notice as appropriate.

Repository:
  https://github.com/issaUt/mruby-x68k
