# X68000 Runtime Notes

The gem exposes X68000 helpers under the `X68k` module.

The implementation is currently pragmatic rather than complete. It wraps a mix
of IOCS calls and direct hardware register access where that was the most
reliable route during testing.

## Implemented Areas

- `Kernel#puts`, `print`, `p`, `open`
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
