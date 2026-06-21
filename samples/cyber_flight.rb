# CyberStick wireframe flight demo
# Run AJOY.X first for analog input.
#
#   mruby cyber_flight.rb
#   mruby cyber_flight.rb usbCyber

def x68k_color(r, g, b, i = 1)
  ((g & 31) << 11) | ((r & 31) << 6) | ((b & 31) << 1) | (i & 1)
end

PAL_BLACK = 0
PAL_GREEN = 130
PAL_CYAN = 199
PAL_DARK = 66
PAL_YELLOW = 248
PAL_WHITE = 255
SCREEN_W = 256
SCREEN_H = 256
CX = 128
CY = 122
PROJ = 92
GROUND_SCALE = 2600
GROUND_FAR = 100000
GROUND_NEAR = 22
GROUND_STEP = 22
GROUND_HALF = 5
SPEED_FP_BASE = 64
SPEED_FP_RANGE = 192
SPEED_FORWARD_DIV = 32
GROUND_Y_ROWS = [0, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 138]

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

def clamp(v, lo, hi)
  return lo if v < lo
  return hi if v > hi
  v
end

def out_code(x, y)
  c = 0
  c |= 1 if x < 0
  c |= 2 if x >= SCREEN_W
  c |= 4 if y < 0
  c |= 8 if y >= SCREEN_H
  c
end

def clip_line(x1, y1, x2, y2)
  c1 = out_code(x1, y1)
  c2 = out_code(x2, y2)

  loop do
    return [x1, y1, x2, y2] if (c1 | c2) == 0
    return nil if (c1 & c2) != 0

    c = c1 != 0 ? c1 : c2
    if (c & 8) != 0
      y = SCREEN_H - 1
      x = x1 + (x2 - x1) * (y - y1) / (y2 - y1)
    elsif (c & 4) != 0
      y = 0
      x = x1 + (x2 - x1) * (y - y1) / (y2 - y1)
    elsif (c & 2) != 0
      x = SCREEN_W - 1
      y = y1 + (y2 - y1) * (x - x1) / (x2 - x1)
    else
      x = 0
      y = y1 + (y2 - y1) * (x - x1) / (x2 - x1)
    end

    if c == c1
      x1 = x
      y1 = y
      c1 = out_code(x1, y1)
    else
      x2 = x
      y2 = y
      c2 = out_code(x2, y2)
    end
  end
end

def draw_line(seg, color = nil)
  clipped = clip_line(seg[0], seg[1], seg[2], seg[3])
  return unless clipped

  c = color || seg[4]
  X68k::Graph.line(clipped[0], clipped[1], clipped[2], clipped[3], c)
end

def erase_segments(segments)
  i = 0
  while i < segments.length
    draw_line(segments[i], PAL_BLACK)
    i += 1
  end
end

def draw_segments(segments)
  i = 0
  while i < segments.length
    draw_line(segments[i])
    i += 1
  end
end

def sin_approx(a)
  a %= 1024
  sign = 1
  if a >= 512
    a -= 512
    sign = -1
  end
  a = 512 - a if a > 256
  sign * ((a * (512 - a)) / 256)
end

def cos_approx(a)
  sin_approx(a + 256)
end

def rotate3d(x, y, z, roll, pitch)
  sr = sin_approx(roll) / 2
  cr = cos_approx(roll) / 2
  sp = sin_approx(pitch) / 2
  cp = cos_approx(pitch) / 2

  x1 = (x * cr - y * sr) / 128
  y1 = (x * sr + y * cr) / 128
  y2 = (y1 * cp - z * sp) / 128
  z2 = (y1 * sp + z * cp) / 128
  [x1, y2, z2]
end

def project3d(x, y, z)
  z += 184
  z = 24 if z < 24
  [CX + (x * PROJ) / z, CY - (y * PROJ) / z]
end

def add_plane_segments(segments, roll, pitch)
  points = []
  i = 0
  while i < PLANE_POINTS.length
    p = PLANE_POINTS[i]
    rx, ry, rz = rotate3d(p[0], p[1], p[2], -roll * 2, -pitch * 2)
    points << project3d(rx, ry - 42, rz)
    i += 1
  end

  i = 0
  while i < BODY_EDGES.length
    e = BODY_EDGES[i]
    a = points[e[0]]
    b = points[e[1]]
    segments << [a[0], a[1], b[0], b[1], PAL_WHITE]
    i += 1
  end

  i = 0
  while i < WING_EDGES.length
    e = WING_EDGES[i]
    a = points[e[0]]
    b = points[e[1]]
    segments << [a[0], a[1], b[0], b[1], PAL_CYAN]
    i += 1
  end

  i = 0
  while i < TAIL_EDGES.length
    e = TAIL_EDGES[i]
    a = points[e[0]]
    b = points[e[1]]
    segments << [a[0], a[1], b[0], b[1], PAL_YELLOW]
    i += 1
  end
end

def ground_point(x, dist, horizon, roll)
  y = horizon + GROUND_SCALE / dist
  spread = 18 + (y - horizon) * 2
  shift = (roll * (y - horizon)) / 74
  sx = CX + (x * spread) / GROUND_HALF + shift
  sy = y + (roll * (sx - CX)) / SCREEN_W
  [sx, sy]
end

def add_world_segments(segments, roll, pitch, forward, speed)
  horizon = 105 + pitch / 3

  i = -GROUND_HALF
  while i <= GROUND_HALF
    far = ground_point(i, GROUND_FAR, horizon, roll)
    near = ground_point(i, GROUND_NEAR, horizon, roll)
    segments << [far[0], far[1], near[0], near[1], PAL_DARK]
    i += 1
  end

  scroll = forward % 32
  i = 0
  while i < GROUND_Y_ROWS.length
    base_dy = GROUND_Y_ROWS[i]
    dy = base_dy
    if i > 0
      dy += (scroll * (base_dy + 8)) / 96
    end
    if dy == 0
      dist = GROUND_FAR
    else
      dist = GROUND_SCALE / dy
    end
    y = horizon + dy
    if y >= horizon && y < SCREEN_H
      a = ground_point(-GROUND_HALF, dist, horizon, roll)
      b = ground_point(GROUND_HALF, dist, horizon, roll)
      segments << [a[0], a[1], b[0], b[1], PAL_GREEN]
    end
    i += 1
  end

  segments << [0, horizon - roll / 2, SCREEN_W - 1, horizon + roll / 2, PAL_CYAN]
end

def build_frame(roll, pitch, forward, speed)
  segments = []
  add_world_segments(segments, roll, pitch, forward, speed)
  add_plane_segments(segments, roll, pitch)
  segments
end

def speed_text(speed_fp)
  whole = speed_fp / 64
  frac = ((speed_fp % 64) * 10) / 64
  "#{whole}.#{frac}"
end

def button_text(buttons)
  return "-" unless buttons && buttons.length > 0
  buttons.join(" ")
end

def show_status(roll, pitch, speed_fp, use_ajoy, preset, buttons)
  X68k::Text.locate(0, 0)
  X68k::Text.print("CyberStick wire flight      ")
  X68k::Text.locate(0, 1)
  X68k::Text.print("ESC/Q quit #{use_ajoy ? preset : 'key'}          ")
  X68k::Text.locate(0, 2)
  X68k::Text.print("r#{roll.to_s.rjust(4)} p#{pitch.to_s.rjust(4)} sp#{speed_text(speed_fp)}      ")
  X68k::Text.locate(0, 3)
  X68k::Text.print("btn #{button_text(buttons).ljust(24)}")
end

def read_key_control(state)
  if X68k::Key.sense != 0
    key = X68k::Key.input & 0xff
    return false if key == 27 || key == 81 || key == 113

    state[0] -= 10 if key == 97 || key == 65
    state[0] += 10 if key == 100 || key == 68
    state[1] -= 10 if key == 119 || key == 87
    state[1] += 10 if key == 115 || key == 83
    state[2] -= 8 if key == 122 || key == 90
    state[2] += 8 if key == 120 || key == 88
    state[0] = clamp(state[0], -96, 96)
    state[1] = clamp(state[1], -96, 96)
    state[2] = clamp(state[2], 0, 255)
  end
  true
end

def init_palette
  # Graph.line/fill use direct 16-bit color values in this mode.
end

X68k::Screen.set_mode(10)
X68k::Screen.clear
X68k::Text.clear
X68k::Text.cursor_off
init_palette

X68k::Screen.apage(0)
X68k::Graph.fill(0, 0, SCREEN_W - 1, SCREEN_H - 1, PAL_BLACK)
X68k::Screen.apage(1)
X68k::Graph.fill(0, 0, SCREEN_W - 1, SCREEN_H - 1, PAL_BLACK)
X68k::Screen.vpage(1)

preset = ARGV.length > 0 ? ARGV[0] : "real"
use_ajoy = X68k::Ajoy.available?

old_mode = nil
old_speed = nil
if use_ajoy
  old_mode = X68k::Ajoy.mode
  old_speed = X68k::Ajoy.speed
  X68k::Ajoy.button_preset = preset == "usbCyber" ? "usbCyber" : "real"
  X68k::Ajoy.throttle_reverse = preset == "usbCyber"
  X68k::Ajoy.mode = 1
  X68k::Ajoy.speed = 0
end

begin
  center_ud = 128
  center_lr = 128
  if use_ajoy
    v = X68k::Ajoy.read
    if v
      center_ud = v[0]
      center_lr = v[1]
    end
  end

  roll = 0
  pitch = 0
  throttle = 128
  forward_fp = 0
  key_state = [0, 0, 128]
  draw_page = 1
  frame_count = 0

  loop do
    if use_ajoy
      v = X68k::Ajoy.read
      buttons = X68k::Ajoy.buttons
      if v
        roll_target = clamp(v[1] - center_lr, -96, 96)
        pitch_target = clamp(v[0] - center_ud, -96, 96)
        throttle = clamp(v[2], 0, 255)
      else
        roll_target = roll
        pitch_target = pitch
      end
    else
      break unless read_key_control(key_state)
      roll_target = key_state[0]
      pitch_target = key_state[1]
      throttle = key_state[2]
      buttons = []
    end

    if X68k::Key.sense != 0
      key = X68k::Key.input & 0xff
      break if key == 27 || key == 81 || key == 113
    end

    roll += (roll_target - roll) / 4
    pitch += (pitch_target - pitch) / 4
    speed_fp = SPEED_FP_BASE + throttle * SPEED_FP_RANGE / 255
    forward_fp += speed_fp
    forward = forward_fp / SPEED_FORWARD_DIV

    X68k::Screen.apage(draw_page)
    X68k::Graph.fill(0, 0, SCREEN_W - 1, SCREEN_H - 1, PAL_BLACK)
    segments = build_frame(roll, pitch, forward, speed_fp)
    draw_segments(segments)
    show_status(roll, pitch, speed_fp, use_ajoy, preset, buttons)
    segments = nil

    frame_count += 1
    GC.start if (frame_count & 31) == 0

    X68k::Crtc.wait_vblank
    X68k::Screen.vpage(1 << draw_page)
    draw_page = 1 - draw_page
  end
ensure
  X68k::Screen.apage(0)
  X68k::Graph.fill(0, 0, SCREEN_W - 1, SCREEN_H - 1, PAL_BLACK)
  X68k::Screen.apage(1)
  X68k::Graph.fill(0, 0, SCREEN_W - 1, SCREEN_H - 1, PAL_BLACK)
  X68k::Screen.apage(0)
  X68k::Screen.vpage(1)
  if use_ajoy
    X68k::Ajoy.mode = old_mode if old_mode
    X68k::Ajoy.speed = old_speed if old_speed
  end
  X68k::Text.cursor_on
  X68k::Text.clear
  X68k::Screen.clear
end