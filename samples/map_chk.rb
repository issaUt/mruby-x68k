puts "Map check"

FLOOR = 0
WALL = 4
DOT = 8
POWER = 12

MAP = [
  "############################",
  "#........... ##........... #",
  "#.   .     . ##.     .   . #",
  "#o ##. ####. ##. ####. ##o #",
  "#. ##. ####. ##. ####. ##. #",
  "#......................... #",
  "#.   .  .            .   . #",  
  "#. ##. #. ########. #. ##. #",
  "#.   . #.... ##.... #.   . #",
  "#..... #   . ##.    #..... #",  
  "#    . ####. ##. ####.     #",
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
  "# . #. #           .#. #.  #",
  "##. #. #. ######## .#. #. ##",
  "#..... #.... ##.....#..... #",
  "#.     #   . ##.    #    . #",
  "#. ########. ##. ########. #",
  "#......................... #", 
  "#                          #",   
  "############################"
]

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

X68k::Sprite.def16(0, blank.pack("C*"))
X68k::Sprite.def16(1, make_wall.pack("C*"))
X68k::Sprite.def16(2, make_dot.pack("C*"))
X68k::Sprite.def16(3, make_power.pack("C*"))
X68k::Sprite.on

X68k::Bg.main_setup
X68k::Bg.main_clear(FLOOR, 1)
X68k::Bg.main_scroll(0, 0)

origin_x = 2
origin_y = 0

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

quit = false
while !quit
  if X68k::Key.sense != 0
    ch = X68k::Key.input & 0xff
    quit = true if ch == 81 || ch == 113 || ch == 27
  end
  quit = true if X68k::Key.x? || X68k::Joy.start?
  X68k::Sys.wait(48000)
end

X68k::Bg.off
X68k::Text.cursor_on
puts "done"
