# Graph mode parity/color check
#   mruby graph_mode_chk.rb [mode]
# Useful modes:
#   10: 256x256 / 256 colors
#   12: 512x512 / 65536 colors
#   14: 256x256 / 65536 colors

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

mode = ARGV.length > 0 ? ARGV[0].to_i : 14
indexed = (mode >= 8 && mode <= 11)

if indexed
  BLACK = 0
  WHITE = 7
  GREEN = 2
  CYAN = 3
  YELLOW = 6
else
  BLACK = x68k_color(0, 0, 0, 0)
  WHITE = x68k_color(31, 31, 31)
  GREEN = x68k_color(0, 24, 0)
  CYAN = x68k_color(0, 31, 31)
  YELLOW = x68k_color(31, 31, 0)
end

X68k::Screen.set_mode(mode)
X68k::Screen.clear
X68k::Text.cursor_off

if indexed
  X68k::Graph.gpalet(BLACK, x68k_color(0, 0, 0, 0))
  X68k::Graph.gpalet(WHITE, x68k_color(31, 31, 31))
  X68k::Graph.gpalet(GREEN, x68k_color(0, 24, 0))
  X68k::Graph.gpalet(CYAN, x68k_color(0, 31, 31))
  X68k::Graph.gpalet(YELLOW, x68k_color(31, 31, 0))
end

max_x = (mode == 12 || mode == 13) ? 511 : 255
max_y = (mode == 12 || mode == 13) ? 511 : 255
X68k::Graph.fill(0, 0, max_x, max_y, BLACK)

X68k::Graph.symbol(8, 8, 'mode %d  %s  ESC: quit' % [mode, indexed ? 'indexed' : '65536'], WHITE)
X68k::Graph.symbol(8, 28, 'green code=$%04X' % GREEN, CYAN)

x1 = 24
x2 = max_x - 24
mid = max_x / 2
right1 = mid + 16
right2 = max_x - 16

X68k::Graph.symbol(x1, 52, 'line even y', WHITE)
y = 72
while y < 116
  X68k::Graph.line(x1, y, mid - 16, y, GREEN)
  y += 4
end

X68k::Graph.symbol(right1, 52, 'line odd y', WHITE)
y = 73
while y < 117
  X68k::Graph.line(right1, y, right2, y, GREEN)
  y += 4
end

X68k::Graph.symbol(x1, 130, 'pset even y', WHITE)
y = 150
while y < 194 && y <= max_y
  hline_pset(x1, mid - 16, y, GREEN)
  y += 8
end

X68k::Graph.symbol(right1, 130, 'pset odd y', WHITE)
y = 151
while y < 195 && y <= max_y
  hline_pset(right1, right2, y, GREEN)
  y += 8
end

if max_y > 260
  X68k::Graph.symbol(x1, 220, 'fill strips even/odd', WHITE)
  y = 240
  while y < 320
    X68k::Graph.fill(x1, y, mid - 16, y, GREEN)
    X68k::Graph.fill(right1, y + 1, right2, y + 1, GREEN)
    y += 8
  end
end

loop do
  if X68k::Key.sense != 0
    key = X68k::Key.input & 0xff
    break if key == 27
  end
  X68k::Crtc.wait_vblank
end
X68k::Text.cursor_on
X68k::Screen.clear