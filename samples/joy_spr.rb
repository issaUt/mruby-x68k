puts "Joy sprite"

X68k::Screen.set_mode(10)
X68k::Graph.box(0, 0, 255, 255, 0xffff)
X68k::Graph.symbol(16, 16, "joy move, START quit", 0xffff)
X68k::Text.cursor_off

X68k::Sprite.init
X68k::Sprite.palette(0, 1, 0x0000)
X68k::Sprite.palette(1, 1, 0xffff)
X68k::Sprite.palette(2, 1, 0x07ff)
X68k::Sprite.palette(3, 1, 0xf81f)
X68k::Sprite.palette(4, 1, 0xffe0)
X68k::Sprite.palette(5, 1, 0xf800)

ship = []
16.times do |y|
  16.times do |x|
    if x < 2 || y < 2 || y > 13
      ship << 0
    elsif x > y - 2 && x > 13 - y
      ship << ((x + y) % 3) + 2
    else
      ship << 0
    end
  end
end

button = []
16.times do |y|
  16.times do |x|
    if x == 0 || y == 0 || x == 15 || y == 15
      button << 1
    else
      button << 5
    end
  end
end

X68k::Sprite.def16(0, ship.pack("C*"))
X68k::Sprite.def16(1, button.pack("C*"))
X68k::Sprite.on

x = 120
y = 120
quit = false

while !quit
  if X68k::Key.sense != 0
    ch = X68k::Key.input & 0xff
    quit = true if ch == 81 || ch == 113 || ch == 27
  end
  quit = true if X68k::Joy.start?

  x -= 3 if X68k::Joy.left?
  x += 3 if X68k::Joy.right?
  y -= 3 if X68k::Joy.up?
  y += 3 if X68k::Joy.down_button?

  x = 16 if x < 16
  x = 224 if x > 224
  y = 48 if y < 48
  y = 224 if y > 224

  X68k::Sprite.put(0, x, y, 0, 1, 3)

  if X68k::Joy.a? || X68k::Joy.b? || X68k::Joy.x?
    bx = x + 18
    bx = x - 18 if bx > 224
    X68k::Sprite.put(1, bx, y, 1, 1, 3)
  else
    X68k::Sprite.put(1, 0, 0, 1, 1, 0)
  end

  X68k::Sys.wait(48000)
end

X68k::Sprite.off
X68k::Text.cursor_on
puts "done"
