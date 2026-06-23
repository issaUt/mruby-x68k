puts "BG map"

X68k::Screen.set_mode(10)
X68k::Graph.wipe
X68k::Text.cursor_off

X68k::Sprite.init
X68k::Sprite.palette(0, 1, 0x0000)
X68k::Sprite.palette(1, 1, 0xffff)
X68k::Sprite.palette(2, 1, 0x07ff)
X68k::Sprite.palette(3, 1, 0xf81f)
X68k::Sprite.palette(4, 1, 0xffe0)

grass = []
16.times do |y|
  16.times do |x|
    grass << (((x + y) % 5) == 0 ? 2 : 0)
  end
end

water = []
16.times do |y|
  16.times do |x|
    water << ((y == 3 || y == 9 || ((x + y) % 11) == 0) ? 3 : 0)
  end
end

wall = []
16.times do |y|
  16.times do |x|
    wall << ((x == 0 || y == 0 || x == 15 || y == 15 || x == y) ? 4 : 0)
  end
end

road = []
16.times do |y|
  16.times do |x|
    road << ((x > 5 && x < 10) || (y > 5 && y < 10) ? 1 : 0)
  end
end

blank = Array.new(16 * 16, 0)

X68k::Sprite.def16(0, grass.pack("C*"))
X68k::Sprite.def16(1, water.pack("C*"))
X68k::Sprite.def16(2, wall.pack("C*"))
X68k::Sprite.def16(3, road.pack("C*"))
X68k::Sprite.def16(4, blank.pack("C*"))
X68k::Sprite.on

map_w = 64
map_h = 64
map = []

map_h.times do |y|
  map_w.times do |x|
    if x == 0 || y == 0 || x == map_w - 1 || y == map_h - 1
      map << [8, 1]
    elsif (x - 32) * (x - 32) + (y - 30) * (y - 30) < 90
      map << [4, 1]
    elsif x == y || x == 63 - y
      map << [12, 1]
    elsif ((x / 4) + (y / 4)) % 2 == 0
      map << [0, 1]
    else
      map << [0, 2]
    end
  end
end

X68k::Bg.main_setup
X68k::Bg.main_clear(16, 1)
X68k::Bg.map_draw_full(map, map_w, 16, 1)

128.times do |i|
  X68k::Bg.main_scroll(i, i / 2)
  X68k::Sys.wait(96000)
end

X68k::Bg.off
X68k::Text.locate(0, 0)
X68k::Text.print("done")
X68k::Text.cursor_on
