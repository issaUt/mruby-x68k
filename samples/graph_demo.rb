WHITE   = 0xffff
CYAN    = 0x07ff
MAGENTA = 0xf81f
GREEN   = 0x07e0
YELLOW  = 0xffe0
DARK_GREEN = 0x4000
RED     = 0xf800
X68k::Screen.set_mode(12)
X68k::Screen.clear
X68k::Graph.gpalet(0, DARK_GREEN)
X68k::Graph.fill(0, 0, 511, 511, DARK_GREEN)
# Palette entries are useful in indexed-color modes; direct colors are used here.
X68k::Graph.gpalet(1, WHITE)
X68k::Graph.gpalet(2, CYAN)
X68k::Graph.gpalet(3, MAGENTA)
X68k::Graph.gpalet(4, GREEN)
X68k::Graph.symbol(24, 24, "IOCS GRAPH DEMO", WHITE, 1, 1)
X68k::Graph.symbol(24, 44, "line box fill pset point circle paint symbol", CYAN)
# Line fan.
i = 0
while i < 72
  color = (i & 1) == 0 ? MAGENTA : GREEN
  X68k::Graph.line(24, 88 + i * 2, 248, 232 - i * 2, color)
  i += 1
end
X68k::Graph.box(24, 80, 248, 240, WHITE)
X68k::Graph.symbol(32, 248, "line", WHITE)
# Boxes and filled rectangles.
X68k::Graph.box(288, 80, 448, 160, WHITE)
X68k::Graph.fill(312, 104, 424, 136, MAGENTA)
X68k::Graph.box(304, 176, 456, 248, CYAN)
X68k::Graph.paint(308, 180, DARK_GREEN)
X68k::Graph.symbol(300, 256, "box fill paint", WHITE)
# Points and psets.
i = 0
while i < 96
  x = 64 + i * 3
  y = 304 + ((i * i) % 48)
  X68k::Graph.pset(x, y, YELLOW)
  X68k::Graph.point(x, y + 56, GREEN)
  i += 1
end
X68k::Graph.symbol(64, 424, "pset / point", WHITE)
# Polyline and polygon helpers built on Graph.line.
X68k::Graph.polyline([[64, 456], [112, 408], [160, 456], [208, 408], [256, 456]], YELLOW)
X68k::Graph.polygon([[336, 392], [432, 420], [400, 480], [304, 480], [272, 420]], WHITE)
X68k::Graph.polyline([[320, 432], [360, 408], [408, 432], [392, 464], [336, 464], [320, 432]], CYAN)
X68k::Graph.symbol(64, 488, "polyline / polygon", WHITE)
# Circles and ellipse-like ratio test.
X68k::Graph.circle(360, 336, 64, CYAN)
X68k::Graph.circle(360, 336, 32, YELLOW, 0, 360, 0x80)
X68k::Graph.circle(360, 336, 18, RED)
X68k::Graph.symbol(304, 424, "circle", WHITE)

