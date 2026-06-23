# Graph color / horizontal drawing check
#   mruby graph_color_chk.rb

def x68k_color(r, g, b, i = 1)
  ((g & 31) << 11) | ((r & 31) << 6) | ((b & 31) << 1) | (i & 1)
end

def hline_pset(x1, x2, y, color)
  x = x1
  while x <= x2
    X68k::Graph.pset(x, y, color)
    x += 1
  end
end

BLACK = x68k_color(0, 0, 0, 0)
WHITE = x68k_color(31, 31, 31)
GREEN = x68k_color(0, 24, 0)
DARK = x68k_color(0, 8, 0)
CYAN = x68k_color(0, 31, 31)
YELLOW = x68k_color(31, 31, 0)

X68k::Screen.set_mode(12)
X68k::Screen.clear
X68k::Text.cursor_off
X68k::Graph.fill(0, 0, 511, 511, BLACK)

X68k::Graph.symbol(8, 8, 'GREEN=$%04X  mode 12 / 65536 colors' % GREEN, WHITE)
X68k::Graph.symbol(8, 28, 'ESC: quit', CYAN)

X68k::Graph.symbol(24, 52, 'Graph.line even y', WHITE)
y = 72
while y < 164
  X68k::Graph.line(24, y, 232, y, GREEN)
  y += 4
end

X68k::Graph.symbol(280, 52, 'Graph.line odd y', WHITE)
y = 73
while y < 165
  X68k::Graph.line(280, y, 488, y, GREEN)
  y += 4
end

X68k::Graph.symbol(24, 184, 'Graph.pset loop even y', WHITE)
y = 204
while y < 276
  hline_pset(24, 232, y, GREEN)
  y += 8
end

X68k::Graph.symbol(280, 184, 'Graph.pset loop odd y', WHITE)
y = 205
while y < 277
  hline_pset(280, 488, y, GREEN)
  y += 8
end

X68k::Graph.symbol(24, 296, 'Graph.fill 1px strips even/odd', WHITE)
y = 316
while y < 396
  X68k::Graph.fill(24, y, 232, y, GREEN)
  X68k::Graph.fill(280, y + 1, 488, y + 1, GREEN)
  y += 8
end
X68k::Graph.symbol(24, 400, 'left even y', WHITE)
X68k::Graph.symbol(280, 400, 'right odd y', WHITE)

y = 440
while y < 492
  X68k::Graph.line(64, y, 448, y, DARK)
  X68k::Graph.line(64, y + 1, 448, y + 1, GREEN)
  y += 10
end
X68k::Graph.symbol(64, 496, 'DARK line then GREEN line pairs', YELLOW)

loop do
  if X68k::Key.sense != 0
    key = X68k::Key.input & 0xff
    break if key == 27
  end
  X68k::Crtc.wait_vblank
end
X68k::Text.cursor_on
X68k::Screen.clear