# Plane wire color check
# ESC/Q: quit

def x68k_color(r, g, b, i = 1)
  ((g & 31) << 11) | ((r & 31) << 6) | ((b & 31) << 1) | (i & 1)
end

PAL_BLACK = 0
PAL_GREEN = 224
PAL_CYAN = 199
PAL_DARK = 64
PAL_YELLOW = 248
PAL_WHITE = 255
SCREEN_W = 256
SCREEN_H = 256

PLANE_POINTS = [
  [0, 0, 50],
  [-8, 0, 16], [8, 0, 16],
  [-54, 0, 2], [54, 0, 2],
  [0, 0, -26],
  [-18, 0, -38], [18, 0, -38],
  [0, 15, -34]
]

BODY_EDGES = [
  [0, 1], [0, 2], [1, 5], [2, 5], [0, 5], [5, 8]
]

WING_EDGES = [
  [1, 3], [2, 4], [3, 4]
]

TAIL_EDGES = [
  [5, 6], [5, 7], [6, 8], [7, 8]
]

def project_point(p, ox, oy, scale)
  [ox + p[0] * scale / 16, oy - p[2] * scale / 32 - p[1] * scale / 32]
end

def draw_edges(edges, color, ox, oy, scale)
  i = 0
  while i < edges.length
    e = edges[i]
    a = project_point(PLANE_POINTS[e[0]], ox, oy, scale)
    b = project_point(PLANE_POINTS[e[1]], ox, oy, scale)
    X68k::Graph.line(a[0], a[1], b[0], b[1], color)
    i += 1
  end
end

def init_palette
  # Graph.line/fill use direct 16-bit color values in this mode.
end

X68k::Screen.set_mode(10)
X68k::Screen.clear
X68k::Text.clear
X68k::Text.cursor_off
init_palette

X68k::Graph.fill(0, 0, SCREEN_W - 1, SCREEN_H - 1, PAL_BLACK)

X68k::Text.locate(0, 0)
X68k::Text.print("Plane color check")
X68k::Text.locate(0, 1)
X68k::Text.print("BODY=WHITE WING=CYAN TAIL=YELLOW")
X68k::Text.locate(0, 2)
X68k::Text.print("ESC/Q: quit")

# Left: all parts together using intended colors.
draw_edges(BODY_EDGES, PAL_WHITE, 64, 150, 16)
draw_edges(WING_EDGES, PAL_CYAN, 64, 150, 16)
draw_edges(TAIL_EDGES, PAL_YELLOW, 64, 150, 16)
X68k::Text.locate(3, 22)
X68k::Text.print("combined")

# Right: separated strips, same geometry and same intended colors.
draw_edges(BODY_EDGES, PAL_WHITE, 180, 92, 14)
X68k::Text.locate(23, 10)
X68k::Text.print("body")
draw_edges(WING_EDGES, PAL_CYAN, 180, 145, 14)
X68k::Text.locate(23, 17)
X68k::Text.print("wing")
draw_edges(TAIL_EDGES, PAL_YELLOW, 180, 200, 14)
X68k::Text.locate(23, 24)
X68k::Text.print("tail")

loop do
  if X68k::Key.sense != 0
    key = X68k::Key.input & 0xff
    break if key == 27 || key == 81 || key == 113
  end
end

X68k::Text.cursor_on
X68k::Text.clear
X68k::Screen.clear