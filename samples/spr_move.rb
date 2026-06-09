puts "Sprite move"

X68k::Screen.set_mode(10)
info = X68k::Screen.mode_info
printf("mode %d %dx%d\n", info[:mode], info[:width], info[:height])

X68k::Graph.box(0, 0, 255, 255, 0xffff)
X68k::Graph.line(0, 0, 255, 255, 0x07e0)
X68k::Graph.line(0, 255, 255, 0, 0xf800)
X68k::Graph.symbol(20, 24, "sprite move", 0xffff)

printf("sp_init %d\n", X68k::Sprite.init)
X68k::Sprite.palette(0, 1, 0x0000)
X68k::Sprite.palette(1, 1, 0xffff)
X68k::Sprite.palette(2, 1, 0xf81f)
X68k::Sprite.palette(3, 1, 0x07ff)
X68k::Sprite.palette(4, 1, 0xffe0)
X68k::Sprite.palette(5, 1, 0xf800)

ball = []
16.times do |y|
  16.times do |x|
    dx = x - 7
    dx = -dx if dx < 0
    dy = y - 7
    dy = -dy if dy < 0
    ball << ((dx + dy) < 8 ? 2 : 0)
  end
end

ship = []
16.times do |y|
  16.times do |x|
    if x < 2 || y < 2 || y > 13
      ship << 0
    elsif x > y - 2 && x > 13 - y
      ship << 4
    else
      ship << 0
    end
  end
end

box = []
16.times do |y|
  16.times do |x|
    box << (((x + y) % 2) == 0 ? 1 : 3)
  end
end

X68k::Sprite.def16(0, ball.pack("C*"))
X68k::Sprite.def16(1, ship.pack("C*"))
X68k::Sprite.def16(2, box.pack("C*"))

printf("sp_on %d\n", X68k::Sprite.on)

x0 = 24
y0 = 88
dx0 = 2
x1 = 220
y1 = 128
dx1 = -3
x2 = 96
y2 = 190
dy2 = -2

240.times do |i|
  X68k::Sprite.put(0, x0, y0, 0, 1, 3)
  X68k::Sprite.put(1, x1, y1, 1, 1, 3)
  X68k::Sprite.put(2, x2, y2, 2, 1, 3)

  x0 += dx0
  dx0 = -dx0 if x0 < 16 || x0 > 224
  x1 += dx1
  dx1 = -dx1 if x1 < 16 || x1 > 224
  y2 += dy2
  dy2 = -dy2 if y2 < 64 || y2 > 224

  600.times do
  end
end

puts "done"
