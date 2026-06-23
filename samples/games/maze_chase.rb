puts "Maze chase"

FLOOR = 0
WALL = 4
DOT = 8
POWER = 12
PLAYER = 4
GHOST1 = 5
GHOST2 = 6
GHOST3 = 7
GHOST4 = 8
GHOST_FRIGHT = 9
HUD_BLANK = 16
HUD_DIGIT = 17
HUD_CLEAR = 27
HUD_COLOR = 15
KIND_PLAYER = 1
KIND_GHOST = 2
TILE_FLOOR = 0
TILE_WALL = 1
TILE_GATE = 2
TILE_DOT = 3
TILE_POWER = 4
DIR_STOP = 0
DIR_LEFT = 1
DIR_RIGHT = 2
DIR_UP = 3
DIR_DOWN = 4
DIR_DX = [0, -1, 1, 0, 0]
DIR_DY = [0, 0, 0, -1, 1]
DIR_BIT = [0, 1, 2, 4, 8]
JOY_UP = 0x01
JOY_DOWN = 0x02
JOY_LEFT = 0x04
JOY_RIGHT = 0x08
JOY_A = 0x20
JOY_B = 0x40
JOY_X = 0x60
JOY_START = JOY_UP | JOY_DOWN
AI_HUNTER = 0
AI_AMBUSHER = 1
AI_FLANKER = 2
AI_SHY = 3
INPUT_JOY = 0
INPUT_KEY = 1
INPUT_BOTH = 2
POWER_FRAMES = 174
POWER_WARN_FRAMES = 54
START_LIVES = 5
MISS_WAIT_FRAMES = 90

def first_existing(paths)
  i = 0
  while i < paths.length
    return paths[i] if File.exist?(paths[i])
    i += 1
  end
  nil
end

NORMAL_BGM = first_existing(["simplebgm.ZMD", "ZMD/simplebgm.ZMD", "../ZMD/simplebgm.ZMD"])
ATTACK_BGM = first_existing(["attackbgm.ZMD", "ZMD/attackbgm.ZMD", "../ZMD/attackbgm.ZMD"])
EAT_SE = first_existing(["eat.ZMD", "ZMD/eat.ZMD", "../ZMD/eat.ZMD"])
CHARGE_SE = first_existing(["charge.ZMD", "ZMD/charge.ZMD", "../ZMD/charge.ZMD"])
MISS_SE = first_existing(["miss.ZMD", "ZMD/miss.ZMD", "../ZMD/miss.ZMD"])
SE_TRACK = 7
EAT_SE_SLOT = 0
CHARGE_SE_SLOT = 1
MISS_SE_SLOT = 2

input_mode = INPUT_KEY
ghost_count = 4
clear_test = false
audio_requested = true
frame_wait = 1
i = 0
while i < ARGV.length
  arg = ARGV[i]
  if arg == "joy" || arg == "joystick"
    input_mode = INPUT_JOY
  elsif arg == "key" || arg == "keyboard"
    input_mode = INPUT_KEY
  elsif arg == "both"
    input_mode = INPUT_BOTH
  elsif arg[0, 2] == "g="
    ghost_count = arg[2, arg.length - 2].to_i
  elsif arg[0, 7] == "ghosts="
    ghost_count = arg[7, arg.length - 7].to_i
  elsif arg[0, 2] == "e="
    ghost_count = arg[2, arg.length - 2].to_i
  elsif arg == "clear" || arg == "testclear"
    clear_test = true
  elsif arg[0, 5] == "wait="
    frame_wait = arg[5, arg.length - 5].to_i
  elsif arg[0, 6] == "speed="
    speed = arg[6, arg.length - 6].to_i
    frame_wait = 60 / speed if speed > 0
  elsif arg[0, 1] >= "0" && arg[0, 1] <= "9"
    ghost_count = arg.to_i
  end
  i += 1
  audio_requested = false if arg == "noaudio" || arg == "no-audio"
end
ghost_count = 0 if ghost_count < 0
ghost_count = 4 if ghost_count > 4
frame_wait = 1 if frame_wait < 1
frame_wait = 6 if frame_wait > 6

MAP = [
  "############################",
  "#........... ##........... #",
  "#.   .     . ##.     .   . #",
  "#o ##. ####. ##. ####. ##o #",
  "#. ##. ####. ##. ####. ##. #",
  "#......................... #",
  "#.   .  .            .   . #",
  "#. ##. #. ########. #. ##. #",
  "#..... #.... ##.... #..... #",
  "#    . #   . ##.    #.     #",
  "#####. ####. ##. ####. #####",
  "#####. #........... #. #####",
  "#####. #.         . #. #####",
  "#####. #. ###--###. #. #####",
  "     .  . #      #.  .      ",
  "     .  . #      #.  .      ",
  "#####. #. ########. #. #####",
  "#####. #.         . #. #####",
  "#####. #.         . #. #####",
  "#####. #. ########. #. #####",
  "#........... ##........... #",
  "#.   .     . ##.     .   . #",
  "#. ##. ####. ##. ####. ##. #",
  "#o. #................. #.o #",
  "# . #.  .         .  . #.  #",
  "##. #. #. ########. #. #. ##",
  "#..... #.... ##.... #..... #",
  "#.     #   . ##.    #    . #",
  "#. ########. ##. ########. #",
  "#......................... #",
  "#                          #",
  "############################"
]

MAP_X = 16
MAP_Y = 0
SPR_OFS_X = 24
SPR_OFS_Y = 16
LOGIC_OFS_X = 8
LOGIC_OFS_Y = 0
STEP = 2
FIX_SHIFT = 8
FIX_ONE = 256
PLAYER_SPEED = 512
GHOST_SPEED = 384
GHOST_FRIGHT_SPEED = 256
GHOST_TUNNEL_SPEED = 256
WARP_ROW = 14
WARP_SLOW_CELLS = 7
MAP_W = MAP[0].length
MAP_H = MAP.length

MAP_DATA = []

def reset_map_data
  while MAP_DATA.length > 0
    MAP_DATA.pop
  end

  count = 0
  row = 0
  while row < MAP_H
    line = MAP[row]
    col = 0
    while col < MAP_W
      ch = line[col, 1]
      v = TILE_FLOOR
      v = TILE_WALL if ch == "#"
      v = TILE_GATE if ch == "-"
      v = TILE_DOT if ch == "."
      v = TILE_POWER if ch == "o"
      count += 1 if v == TILE_DOT || v == TILE_POWER
      MAP_DATA << v
      col += 1
    end
    row += 1
  end
  count
end

food_left = reset_map_data

def make_wall
  data = []
  16.times do |y|
    16.times do |x|
      xx = x & 7
      yy = y & 7
      data << ((xx == 0 || yy == 0 || xx == yy) ? 2 : 0)
    end
  end
  data
end

def make_dot
  data = Array.new(16 * 16, 0)
  data[5 * 16 + 5] = 4
  data[5 * 16 + 6] = 4
  data[6 * 16 + 5] = 4
  data[6 * 16 + 6] = 4
  data
end

def make_power
  data = Array.new(16 * 16, 0)
  2.times do |by|
    2.times do |bx|
      ox = bx * 8
      oy = by * 8
      y = 1
      while y < 7
        x = 1
        while x < 7
          dx = x - 3
          dx = -dx if dx < 0
          dy = y - 3
          dy = -dy if dy < 0
          data[(oy + y) * 16 + ox + x] = 4 if dx + dy < 5
          x += 1
        end
        y += 1
      end
    end
  end
  data
end

def make_player
  data = Array.new(16 * 16, 0)
  16.times do |y|
    16.times do |x|
      dx = x - 7
      dx = -dx if dx < 0
      dy = y - 7
      dy = -dy if dy < 0
      data[y * 16 + x] = 4 if dx + dy < 9
    end
  end
  data
end

def make_ghost(color)
  data = Array.new(16 * 16, 0)
  16.times do |y|
    16.times do |x|
      v = 0
      if y < 3
        v = color if x > 4 && x < 11
      elsif y < 12
        v = color if x > 2 && x < 13
      else
        v = color if x > 2 && x < 13 && ((x / 3) & 1) == 0
      end
      data[y * 16 + x] = v
    end
  end
  data
end

def make_hud_blank
  Array.new(16 * 16, 0)
end

def make_hud_digit(n)
  glyphs = [
    ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    ["11110", "00001", "00001", "01110", "00001", "00001", "11110"],
    ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    ["11111", "10000", "10000", "11110", "00001", "00001", "11110"],
    ["01110", "10000", "10000", "11110", "10001", "10001", "01110"],
    ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    ["01110", "10001", "10001", "01111", "00001", "00001", "01110"]
  ]
  data = Array.new(16 * 16, 0)
  glyph = glyphs[n]
  y = 0
  while y < 7
    row = glyph[y]
    x = 0
    while x < 5
      if row[x, 1] == "1"
        data[(y + 1) * 16 + x + 1] = HUD_COLOR
      end
      x += 1
    end
    y += 1
  end
  data
end

def make_hud_char(n)
  glyphs = [
    ["01110", "10001", "10000", "10000", "10000", "10001", "01110"],
    ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    ["11110", "10001", "10001", "11110", "10100", "10010", "10001"]
  ]
  data = Array.new(16 * 16, 0)
  glyph = glyphs[n]
  y = 0
  while y < 7
    row = glyph[y]
    x = 0
    while x < 5
      data[(y + 1) * 16 + x + 1] = HUD_COLOR if row[x, 1] == "1"
      x += 1
    end
    y += 1
  end
  data
end

def warp_row?(ty)
  ty == WARP_ROW
end

def warp_exit?(tx, ty, dir)
  return false unless warp_row?(ty)
  (dir == DIR_LEFT && tx == 0) || (dir == DIR_RIGHT && tx == MAP_W - 2)
end

def can_move?(x, y, actor, dir)
  return false if dir == DIR_STOP
  tx = (x + LOGIC_OFS_X - 8 - MAP_X) / 8
  ty = (y + LOGIC_OFS_Y - 8 - MAP_Y) / 8
  nx = tx + DIR_DX[dir]
  ny = ty + DIR_DY[dir]
  return true if warp_exit?(tx, ty, dir) && (nx < 0 || nx + 1 >= MAP_W)
  return false if nx < 0 || ny < 0 || nx + 1 >= MAP_W || ny + 1 >= MAP_H

  gate = false
  idx = ny * MAP_W + nx
  v = MAP_DATA[idx]
  return false if v == TILE_WALL
  gate = true if v == TILE_GATE

  v = MAP_DATA[idx + 1]
  return false if v == TILE_WALL
  gate = true if v == TILE_GATE

  idx += MAP_W
  v = MAP_DATA[idx]
  return false if v == TILE_WALL
  gate = true if v == TILE_GATE

  v = MAP_DATA[idx + 1]
  return false if v == TILE_WALL
  gate = true if v == TILE_GATE

  if gate
    return false if actor != KIND_GHOST
    return false if DIR_DY[dir] >= 0
  end
  true
end

def wall_cell?(tx, ty)
  return true if tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H
  MAP_DATA[ty * MAP_W + tx] == TILE_WALL
end

def blocked_left?(x, y)
  tx = cell_x(x) - 1
  ty = cell_y(y)
  wall_cell?(tx, ty) || wall_cell?(tx, ty + 1)
end

def blocked_right?(x, y)
  tx = cell_x(x) + 2
  ty = cell_y(y)
  wall_cell?(tx, ty) || wall_cell?(tx, ty + 1)
end

def blocked_up?(x, y)
  tx = cell_x(x)
  ty = cell_y(y) - 1
  wall_cell?(tx, ty) || wall_cell?(tx + 1, ty)
end

def blocked_down?(x, y)
  tx = cell_x(x)
  ty = cell_y(y) + 2
  wall_cell?(tx, ty) || wall_cell?(tx + 1, ty)
end

def centered?(x, y)
  ((x + LOGIC_OFS_X - MAP_X - 8) % 8) == 0 && ((y + LOGIC_OFS_Y - MAP_Y - 8) % 8) == 0
end

def next_center_x(x, dir)
  x + DIR_DX[dir] * 8
end

def next_center_y(y, dir)
  y + DIR_DY[dir] * 8
end

def cell_x(x)
  (x + LOGIC_OFS_X - 8 - MAP_X) / 8
end

def cell_y(y)
  (y + LOGIC_OFS_Y - 8 - MAP_Y) / 8
end

def pixel_x_from_cell(tx)
  MAP_X + tx * 8 + 8 - LOGIC_OFS_X
end

def pixel_y_from_cell(ty)
  MAP_Y + ty * 8 + 8 - LOGIC_OFS_Y
end

def warp_x(x, y)
  ty = cell_y(y)
  return x unless warp_row?(ty)
  tx = cell_x(x)
  return pixel_x_from_cell(MAP_W - 2) if tx < 0
  return pixel_x_from_cell(0) if tx > MAP_W - 2
  x
end

def tunnel_slow?(x, y)
  warp_row?(cell_y(y)) && (cell_x(x) < WARP_SLOW_CELLS || cell_x(x) > MAP_W - WARP_SLOW_CELLS - 2)
end

def clamp_cell_x(tx)
  tx = 1 if tx < 1
  tx = MAP_W - 3 if tx > MAP_W - 3
  tx
end

def clamp_cell_y(ty)
  ty = 1 if ty < 1
  ty = MAP_H - 3 if ty > MAP_H - 3
  ty
end

def opposite_dir?(a, b)
  return true if a == DIR_LEFT && b == DIR_RIGHT
  return true if a == DIR_RIGHT && b == DIR_LEFT
  return true if a == DIR_UP && b == DIR_DOWN
  return true if a == DIR_DOWN && b == DIR_UP
  false
end

def ghost_next_dir(x, y, dir, target_x, target_y)
  dx = target_x - x
  dy = target_y - y
  adx = dx
  adx = -adx if adx < 0
  ady = dy
  ady = -ady if ady < 0

  hdir = dx < 0 ? DIR_LEFT : DIR_RIGHT
  vdir = dy < 0 ? DIR_UP : DIR_DOWN
  hrev = dx < 0 ? DIR_RIGHT : DIR_LEFT
  vrev = dy < 0 ? DIR_DOWN : DIR_UP

  if adx >= ady
    d1 = hdir
    d2 = vdir
    d3 = vrev
    d4 = hrev
  else
    d1 = vdir
    d2 = hdir
    d3 = hrev
    d4 = vrev
  end

  return d1 if !opposite_dir?(d1, dir) && can_move?(x, y, KIND_GHOST, d1)
  return d2 if !opposite_dir?(d2, dir) && can_move?(x, y, KIND_GHOST, d2)
  return d3 if !opposite_dir?(d3, dir) && can_move?(x, y, KIND_GHOST, d3)
  return d4 if !opposite_dir?(d4, dir) && can_move?(x, y, KIND_GHOST, d4)

  return d1 if can_move?(x, y, KIND_GHOST, d1)
  return d2 if can_move?(x, y, KIND_GHOST, d2)
  return d3 if can_move?(x, y, KIND_GHOST, d3)
  return d4 if can_move?(x, y, KIND_GHOST, d4)
  dir
end

def ghost_flee_dir(x, y, dir, target_x, target_y)
  best_dir = dir
  best_score = -1

  d = DIR_LEFT
  while d <= DIR_DOWN
    if !opposite_dir?(d, dir) && can_move?(x, y, KIND_GHOST, d)
      nx = x + DIR_DX[d] * 8
      ny = y + DIR_DY[d] * 8
      dx = nx - target_x
      dy = ny - target_y
      score = dx * dx + dy * dy
      if score > best_score
        best_score = score
        best_dir = d
      end
    end
    d += 1
  end

  return best_dir if best_score >= 0

  d = DIR_LEFT
  while d <= DIR_DOWN
    if can_move?(x, y, KIND_GHOST, d)
      nx = x + DIR_DX[d] * 8
      ny = y + DIR_DY[d] * 8
      dx = nx - target_x
      dy = ny - target_y
      score = dx * dx + dy * dy
      if score > best_score
        best_score = score
        best_dir = d
      end
    end
    d += 1
  end

  best_dir
end

def ghost_house_dir(x, y)
  tx = cell_x(x)
  ty = cell_y(y)
  return DIR_STOP if ty < 14 || ty > 15 || tx < 11 || tx > 16

  return DIR_RIGHT if tx < 13 && can_move?(x, y, KIND_GHOST, DIR_RIGHT)
  return DIR_LEFT if tx > 13 && can_move?(x, y, KIND_GHOST, DIR_LEFT)
  return DIR_UP if can_move?(x, y, KIND_GHOST, DIR_UP)
  DIR_STOP
end

def draw_map
  origin_x = MAP_X / 8
  origin_y = MAP_Y / 8
  row = 0
  while row < MAP.length
    line = MAP[row]
    col = 0
    while col < line.length
      ch = line[col, 1]
      code = FLOOR
      code = WALL if ch == "#"
      code = DOT if ch == "."
      code = POWER if ch == "o"
      X68k::Bg.main_put(origin_x + col, origin_y + row, code, 1)
      col += 1
    end
    row += 1
  end
end

def draw_score(score)
  x = 1
  y = 0
  div = 10000
  i = 0
  started = false
  while i < 5
    digit = (score / div) % 10
    if digit != 0 || started || i == 4
      X68k::Bg.main_put(x + i, y, (HUD_DIGIT + digit) * 4, 1)
      started = true
    else
      X68k::Bg.main_put(x + i, y, HUD_BLANK * 4, 1)
    end
    div /= 10
    i += 1
  end
end

def draw_lives(lives)
  X68k::Bg.main_put(25, 0, (HUD_DIGIT + lives) * 4, 1)
end

def wait_game_frame(count)
  i = 0
  while i < count
    X68k::Crtc.wait_vblank
    i += 1
  end
end

def wait_escape_message(message)
  x = 13
  y = 17
  i = 0
  while i < 5
    X68k::Bg.main_put(x + i, y, (HUD_CLEAR + i) * 4, 1)
    i += 1
  end
  loop do
    if X68k::Key.sense != 0
      ch = X68k::Key.input & 0xff
      break if ch == 27
    end
    X68k::Crtc.wait_vblank
  end
end

def wait_frames(frames)
  i = 0
  while i < frames
    if X68k::Key.sense != 0
      ch = X68k::Key.input & 0xff
      return true if ch == 27
    end
    X68k::Crtc.wait_vblank
    i += 1
  end
  false
end

def eat_at(x, y)
  tx = cell_x(x)
  ty = cell_y(y)
  return 0 if tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H

  idx = ty * MAP_W + tx
  v = MAP_DATA[idx]
  return 0 unless v == TILE_DOT || v == TILE_POWER

  MAP_DATA[idx] = TILE_FLOOR
  X68k::Bg.main_put((MAP_X / 8) + tx, (MAP_Y / 8) + ty, FLOOR, 1)
  return 50 if v == TILE_POWER
  10
end

def reset_round(player, ghosts)
  player.reset_home!
  i = 0
  while i < ghosts.length
    ghosts[i].reset_home!
    i += 1
  end
end

def sprite_x(x)
  x - 8 + SPR_OFS_X
end

def sprite_y(y)
  y - 8 + SPR_OFS_Y
end

def touching?(a, b)
  dx = a.x - b.x
  dy = a.y - b.y
  dx * dx + dy * dy < 100
end

class Actor
  attr_accessor :x, :y, :dir, :desired
  attr_reader :sprite_no, :pattern, :kind, :color

  def initialize(sprite_no, pattern, x, y, dir, kind, color = 1)
    @sprite_no = sprite_no
    @pattern = pattern
    @x = x
    @y = y
    @fx = x * FIX_ONE
    @fy = y * FIX_ONE
    @home_x = x
    @home_y = y
    @home_dir = dir
    @dir = dir
    @desired = dir
    @kind = kind
    @color = color
  end

  def reset_home!
    @x = @home_x
    @y = @home_y
    @fx = @home_x * FIX_ONE
    @fy = @home_y * FIX_ONE
    @dir = @home_dir
    @desired = @home_dir
  end

  def move!(speed = PLAYER_SPEED)
    if centered?(@x, @y)
      @dir = @desired if can_move?(@x, @y, @kind, @desired)
      return unless can_move?(@x, @y, @kind, @dir)
    end

    next_fx = @fx + DIR_DX[@dir] * speed
    next_fy = @fy + DIR_DY[@dir] * speed
    next_x = next_fx / FIX_ONE
    next_y = next_fy / FIX_ONE

    if @dir == DIR_LEFT || @dir == DIR_RIGHT
      cur_cell = cell_x(@x)
      next_cell = cell_x(next_x)
      if cur_cell != next_cell
        if !can_move?(pixel_x_from_cell(cur_cell), @y, @kind, @dir)
          @x = pixel_x_from_cell(cur_cell)
          @fx = @x * FIX_ONE
          return
        end
        if @dir == DIR_RIGHT
          next_x = pixel_x_from_cell(next_cell)
          next_fx = next_x * FIX_ONE
        end
      end
    elsif @dir == DIR_UP || @dir == DIR_DOWN
      cur_cell = cell_y(@y)
      next_cell = cell_y(next_y)
      if cur_cell != next_cell
        if !can_move?(@x, pixel_y_from_cell(cur_cell), @kind, @dir)
          @y = pixel_y_from_cell(cur_cell)
          @fy = @y * FIX_ONE
          return
        end
        if @dir == DIR_DOWN
          next_y = pixel_y_from_cell(next_cell)
          next_fy = next_y * FIX_ONE
        end
      end
    end

    @fx = next_fx
    @fy = next_fy
    @x = next_x
    @y = next_y
    if @dir == DIR_LEFT || @dir == DIR_RIGHT
      @x = warp_x(@x, @y)
      @fx = @x * FIX_ONE
    end
  end

  def draw!(pattern = nil)
    pattern = @pattern if pattern == nil
    X68k::Sprite.put(@sprite_no, sprite_x(@x), sprite_y(@y), pattern, @color, 3)
  end

  def hide!
    X68k::Sprite.put(@sprite_no, 0, 0, @pattern, @color, 0)
  end
end

class GhostActor < Actor
  attr_accessor :vulnerable

  def initialize(sprite_no, pattern, x, y, dir, kind, color = 1, phase = 0, ai_type = AI_HUNTER, scatter_tx = 1, scatter_ty = 1, chase_frames = 320, wander_frames = 180, mode_phase = 0)
    super(sprite_no, pattern, x, y, dir, kind, color)
    @vulnerable = false
    @phase = phase
    @ai_type = ai_type
    @scatter_tx = scatter_tx
    @scatter_ty = scatter_ty
    @chase_frames = chase_frames
    @wander_frames = wander_frames
    @mode_phase = mode_phase
    @target_x = x
    @target_y = y
  end

  def reset_home!
    super
    @target_x = @home_x
    @target_y = @home_y
    @vulnerable = false
  end

  def frightened?(power_timer)
    power_timer > 0 && @vulnerable
  end

  def chase!(target, frame, frightened = false)
    if centered?(@x, @y)
      house_dir = ghost_house_dir(@x, @y)
      if house_dir != DIR_STOP
        @desired = house_dir
      elsif frightened
        @desired = ghost_flee_dir(@x, @y, @dir, target.x, target.y)
      else
        choose_target!(target, frame)
        @desired = ghost_next_dir(@x, @y, @dir, @target_x, @target_y)
      end
    end
    old_x = @x
    old_y = @y
    speed = frightened ? GHOST_FRIGHT_SPEED : GHOST_SPEED
    speed = GHOST_TUNNEL_SPEED if tunnel_slow?(@x, @y) && !frightened
    move!(speed)
    if @x == old_x && @y == old_y && centered?(@x, @y)
      reverse!
    end
  end

  def reverse!
    if @dir == DIR_LEFT
      @dir = DIR_RIGHT
    elsif @dir == DIR_RIGHT
      @dir = DIR_LEFT
    elsif @dir == DIR_UP
      @dir = DIR_DOWN
    elsif @dir == DIR_DOWN
      @dir = DIR_UP
    end
    @desired = @dir
  end

  def chase_mode?(frame)
    cycle = @chase_frames + @wander_frames
    ((frame + @mode_phase) % cycle) < @chase_frames
  end

  def choose_target!(target, frame)
    unless chase_mode?(frame)
      @target_x = pixel_x_from_cell(@scatter_tx)
      @target_y = pixel_y_from_cell(@scatter_ty)
      return
    end

    tx = cell_x(target.x)
    ty = cell_y(target.y)

    if @ai_type == AI_AMBUSHER
      tx += DIR_DX[target.dir] * 4
      ty += DIR_DY[target.dir] * 4
    elsif @ai_type == AI_FLANKER
      ahead_x = tx + DIR_DX[target.dir] * 2
      ahead_y = ty + DIR_DY[target.dir] * 2
      tx = ahead_x - DIR_DY[target.dir] * 3
      ty = ahead_y + DIR_DX[target.dir] * 3
    elsif @ai_type == AI_SHY
      dx = cell_x(@x) - tx
      dy = cell_y(@y) - ty
      if dx * dx + dy * dy < 36
        tx = @scatter_tx
        ty = @scatter_ty
      end
    end

    tx = clamp_cell_x(tx)
    ty = clamp_cell_y(ty)
    @target_x = pixel_x_from_cell(tx)
    @target_y = pixel_y_from_cell(ty)
  end
end

blank = Array.new(16 * 16, 0)

X68k::Screen.set_mode(10)
X68k::Graph.wipe
X68k::Text.cursor_off

X68k::Sprite.init
X68k::Sprite.palette(0, 1, 0x0000)
X68k::Sprite.palette(1, 1, 0xffff)
X68k::Sprite.palette(2, 1, 0x001f)
X68k::Sprite.palette(3, 1, 0x07ff)
X68k::Sprite.palette(4, 1, 0xffe0)
X68k::Sprite.palette(5, 1, 0xf800)
X68k::Sprite.palette(6, 1, 0xf81f)
X68k::Sprite.palette(7, 1, 0x07ff)
X68k::Sprite.palette(8, 1, 0xff80)
X68k::Sprite.palette(HUD_COLOR, 1, 0xffff)

X68k::Sprite.def16(0, blank.pack("C*"))
X68k::Sprite.def16(1, make_wall.pack("C*"))
X68k::Sprite.def16(2, make_dot.pack("C*"))
X68k::Sprite.def16(3, make_power.pack("C*"))
X68k::Sprite.def16(PLAYER, make_player.pack("C*"))
X68k::Sprite.def16(GHOST1, make_ghost(5).pack("C*"))
X68k::Sprite.def16(GHOST2, make_ghost(6).pack("C*"))
X68k::Sprite.def16(GHOST3, make_ghost(7).pack("C*"))
X68k::Sprite.def16(GHOST4, make_ghost(8).pack("C*"))
X68k::Sprite.def16(GHOST_FRIGHT, make_ghost(2).pack("C*"))
i = 0
while i < 10
  X68k::Sprite.def16(HUD_DIGIT + i, make_hud_digit(i).pack("C*"))
  i += 1
end
i = 0
while i < 5
  X68k::Sprite.def16(HUD_CLEAR + i, make_hud_char(i).pack("C*"))
  i += 1
end
X68k::Sprite.def16(HUD_BLANK, make_hud_blank.pack("C*"))
X68k::Sprite.on

X68k::Bg.main_setup
X68k::Bg.main_clear(FLOOR, 1)
X68k::Bg.main_scroll(0, 0)
draw_map
score = 0
lives = START_LIVES
draw_score(score)
draw_lives(lives)

if clear_test
  wait_escape_message("CLEAR")
  X68k::Sprite.off
  X68k::Bg.off
  X68k::Text.cursor_on
  puts "done"
  exit
end

player = Actor.new(0, PLAYER, MAP_X + 13 * 8, MAP_Y + 24 * 8, DIR_STOP, KIND_PLAYER)
ghosts = [
  GhostActor.new(1, GHOST1, MAP_X + 11 * 8, MAP_Y + 120, DIR_RIGHT, KIND_GHOST, 1, 0, AI_HUNTER, 1, 1, 360, 160, 0),
  GhostActor.new(2, GHOST2, MAP_X + 13 * 8, MAP_Y + 120, DIR_UP, KIND_GHOST, 1, 1, AI_AMBUSHER, MAP_W - 3, 1, 430, 130, 90),
  GhostActor.new(3, GHOST3, MAP_X + 15 * 8, MAP_Y + 120, DIR_LEFT, KIND_GHOST, 1, 2, AI_FLANKER, 1, MAP_H - 3, 300, 210, 170),
  GhostActor.new(4, GHOST4, MAP_X + 13 * 8, MAP_Y + 120, DIR_UP, KIND_GHOST, 1, 3, AI_SHY, MAP_W - 3, MAP_H - 3, 260, 240, 250)
]
while ghosts.length > ghost_count
  ghost = ghosts.pop
  ghost.hide!
end

audio_enabled = audio_requested && X68k::ZMusic.available? && NORMAL_BGM != nil
audio_mode = 0
if audio_enabled
  audio_enabled = X68k::ZMusic.play_bgm(NORMAL_BGM, 100, false) == 0
end

quit = false
clear = false
frame = 0
power_timer = 0
while !quit
  joy = 0xffff
  joy = X68k::Joy.get(0) if input_mode != INPUT_KEY

  if X68k::Key.sense != 0
    ch = X68k::Key.input & 0xff
    quit = true if ch == 81 || ch == 113 || ch == 27
  end

  quit = true if input_mode != INPUT_KEY && ((joy & JOY_START) == 0 || (joy & JOY_X) == 0)
  quit = true if input_mode != INPUT_JOY && X68k::Key.x?

  if input_mode != INPUT_KEY && (joy & JOY_LEFT) == 0
    player.desired = DIR_LEFT
  elsif input_mode != INPUT_KEY && (joy & JOY_RIGHT) == 0
    player.desired = DIR_RIGHT
  elsif input_mode != INPUT_KEY && (joy & JOY_UP) == 0
    player.desired = DIR_UP
  elsif input_mode != INPUT_KEY && (joy & JOY_DOWN) == 0
    player.desired = DIR_DOWN
  elsif input_mode != INPUT_JOY && X68k::Key.num4?
    player.desired = DIR_LEFT
  elsif input_mode != INPUT_JOY && X68k::Key.num6?
    player.desired = DIR_RIGHT
  elsif input_mode != INPUT_JOY && X68k::Key.num8?
    player.desired = DIR_UP
  elsif input_mode != INPUT_JOY && X68k::Key.num2?
    player.desired = DIR_DOWN
  end

  player.move!(PLAYER_SPEED)
  add_score = eat_at(player.x, player.y)
  if add_score != 0
    score += add_score
    food_left -= 1
    draw_score(score)
    if add_score == 50
      power_timer = POWER_FRAMES
      if audio_enabled && ATTACK_BGM != nil && audio_mode != 1
        audio_mode = X68k::ZMusic.play_bgm(ATTACK_BGM, 100, false) == 0 ? 1 : audio_mode
      end
      X68k::ZMusic.play_se(SE_TRACK, CHARGE_SE, 90, CHARGE_SE_SLOT) if audio_enabled && CHARGE_SE != nil
      i = 0
      while i < ghosts.length
        ghosts[i].vulnerable = true
        i += 1
      end
    elsif audio_enabled && EAT_SE != nil
      X68k::ZMusic.play_se(SE_TRACK, EAT_SE, 85, EAT_SE_SLOT)
    end
    if food_left <= 0
      clear = true
      quit = true
    end
  end

  if power_timer == 0
    i = 0
    while i < ghosts.length
      ghosts[i].vulnerable = false
      i += 1
    end
  end
  if audio_enabled && power_timer == 1 && audio_mode != 0
    audio_mode = X68k::ZMusic.play_bgm(NORMAL_BGM, 100, false) == 0 ? 0 : audio_mode
  end

  i = 0
  while i < ghosts.length
    ghosts[i].chase!(player, frame, ghosts[i].frightened?(power_timer))
    i += 1
  end

  unless clear
    i = 0
    while i < ghosts.length
      if touching?(player, ghosts[i])
        if ghosts[i].frightened?(power_timer)
          score += 200
          draw_score(score)
          X68k::ZMusic.play_se(SE_TRACK, CHARGE_SE, 95, CHARGE_SE_SLOT) if audio_enabled && CHARGE_SE != nil
          ghosts[i].reset_home!
        else
          lives -= 1
          draw_lives(lives)
          power_timer = 0
          if audio_enabled
            if MISS_SE != nil
              X68k::ZMusic.play_bgm(MISS_SE, 100, false)
            else
              X68k::ZMusic.stop
            end
            audio_mode = -1
          end
          quit = wait_frames(MISS_WAIT_FRAMES) if audio_enabled && MISS_SE != nil
          reset_round(player, ghosts) unless quit
          if !quit && lives <= 0
            score = 0
            lives = START_LIVES
            food_left = reset_map_data
            X68k::Bg.main_clear(FLOOR, 1)
            draw_map
            draw_score(score)
            draw_lives(lives)
          end
          if !quit && audio_enabled && NORMAL_BGM != nil
            audio_mode = X68k::ZMusic.play_bgm(NORMAL_BGM, 100, false) == 0 ? 0 : audio_mode
          end
          i = ghosts.length
        end
      end
      i += 1
    end
  end

  player.draw!
  i = 0
  while i < ghosts.length
    if ghosts[i].frightened?(power_timer) && (power_timer > POWER_WARN_FRAMES || ((power_timer / 2) & 1) == 0)
      ghosts[i].draw!(GHOST_FRIGHT)
    else
      ghosts[i].draw!
    end
    i += 1
  end
  power_timer -= 1 if power_timer > 0
  frame += 1
  wait_game_frame(frame_wait)
end

wait_escape_message("CLEAR") if clear
X68k::Sprite.off
X68k::Bg.off
X68k::ZMusic.stop if audio_enabled
X68k::Text.cursor_on
puts "clear" if clear
puts "done"
