# BG API

BG support is currently focused on a stable single-plane API.

## Stable Main BG

The stable helpers use BG text page 1 and scroll register 0:

```ruby
X68k::Bg.main_setup
X68k::Bg.main_clear
X68k::Bg.main_put(x, y, code, color = 1)
X68k::Bg.main_fill(x, y, w, h, code, color = 1)
X68k::Bg.main_scroll(x, y)
```

This route matches what was observed in the `gameB` sample code: BG text data
written at `0xEBE000`, with the first scroll register pair controlling visible
scroll.

## Tile Maps

Tile map helpers:

```ruby
X68k::Bg.map_draw(map, map_width, map_x, map_y, screen_x, screen_y, width, height)
X68k::Bg.map_draw_full(map, map_width)
```

`map_draw` uses the C-side bulk transfer function:

```ruby
X68k::Bg.map_blit(map, map_width, map_x, map_y, screen_x, screen_y, width, height, page = 1, default_code = 8, default_color = 1)
```

Map entries can be either:

```ruby
code
[code, color]
[code, color, h_flip, v_flip]
```

For a pattern defined with:

```ruby
X68k::Sprite.def16(n, data)
```

the BG code is typically:

```ruby
n * 4
```

## Current Limitation

Independent two-plane BG0/BG1 scrolling is not finalized yet.

Observed during testing:

- page 1 is visible and scrolls through register 0
- `BGCTRLST`'s first argument should not be treated as a simple BG plane number
- second-plane behavior needs more hardware/register investigation

For now, code should use the `main_*` helpers for BG work.
