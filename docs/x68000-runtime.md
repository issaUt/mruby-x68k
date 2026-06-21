# X68000 Runtime Notes

The gem exposes X68000 helpers under the `X68k` module.

The implementation is currently pragmatic rather than complete. It wraps a mix
of IOCS calls and direct hardware register access where that was the most
reliable route during testing.

## Implemented Areas

- `Kernel#puts`, `print`, `p`, `open`, backquote
- `File.exist?`, `File.open`, `File.read`, `File.write`, `File.size`
- `X68k::Screen`
- `X68k::Graph`
- `X68k::Text`
- `X68k::Sprite`
- `X68k::Key`
- `X68k::Joy`
- `X68k::Bg`

## Screen Modes

The samples mainly use:

```ruby
X68k::Screen.set_mode(10) # 256x256 / 256 colors
```

Other modes have been tested in small samples, including 512x512 and 768x512
graphics modes, but the game-style sprite/BG samples currently assume mode 10.

## Graphics Page Switching

`X68k::Screen.apage(page)` sets the active graphics page used by IOCS drawing calls.
`X68k::Screen.vpage(mask)` sets the visible graphics page mask.

For double buffering, draw to the hidden page, wait for VBlank, then switch the visible page.

```ruby
draw_page = 1
X68k::Screen.apage(draw_page)
# draw with X68k::Graph.*
X68k::Crtc.wait_vblank
X68k::Screen.vpage(1 << draw_page)
```

`cyber_flight.rb` uses this pattern to reduce visible redraw while keeping status text on the text plane.

## Mode 10 Color Notes

In 256-color graphics mode 10, the current `X68k::Graph` wrappers pass the color code directly to IOCS.
For the wireframe samples, the following codes were confirmed useful during testing.

```ruby
BLACK  = 0
DARK   = 66
GREEN  = 130
CYAN   = 199
YELLOW = 248
WHITE  = 255
```

Do not assume that a 16-bit RGB value such as `0xffff` can be used as a drawing color in mode 10.
For lines and fills in this environment, use the palette/color number expected by the active graphics mode.

## Backquote

The X68000 build currently includes provisional Ruby backquote support.

```ruby
out = `dir`
puts out
```

This is a provisional, small Human68k-oriented implementation: the command is executed with stdout redirected to a temporary file, then mruby reads that file back as a `String`.
Only standard output is captured. Error messages from the command may still appear on the console.
The temporary file is created in the current directory using a name like `MRBBQ000.TMP` and removed after reading. The API surface is intended to match Ruby backquote usage, but the implementation details and limitations may change after more Human68k testing.

## Sprite Pattern Codes and BG Codes

`X68k::Sprite.def16(n, data)` defines a 16x16 pattern. For BG usage, the
corresponding BG character code is normally `n * 4`.

Example:

```ruby
X68k::Sprite.def16(2, pattern_data)
X68k::Bg.main_put(0, 0, 8, 1) # 2 * 4
```

## Keyboard and Joystick

Keyboard helpers currently include convenience predicates for the tested keys:

- numeric keypad 2/4/6/8
- Space
- Z
- X

Joystick values are active-low and include helpers for direction and buttons.

The joystick observations used during testing:

```text
Left:   00FB
Right:  00F7
Up:     00FE
Down:   00FD
Select: 00F3
Start:  00FC
A:      00DF
B:      00BF
X:      009F
```
