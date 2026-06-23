puts "BG main"

X68k::Screen.set_mode(10)
X68k::Graph.wipe
X68k::Text.cursor_off

X68k::Sprite.init
X68k::Sprite.palette(0, 1, 0x0000)
X68k::Sprite.palette(1, 1, 0xffff)
X68k::Sprite.palette(2, 1, 0x07ff)
X68k::Sprite.palette(3, 1, 0xf81f)
X68k::Sprite.palette(4, 1, 0xffe0)

grid = []
16.times do |y|
  16.times do |x|
    grid << ((x == 0 || y == 0 || x == 8 || y == 8) ? 2 : 0)
  end
end

dot = []
16.times do |y|
  16.times do |x|
    dot << (((x + y) % 2) == 0 ? 3 : 0)
  end
end

blank = Array.new(16 * 16, 0)

X68k::Sprite.def16(0, grid.pack("C*"))
X68k::Sprite.def16(1, dot.pack("C*"))
X68k::Sprite.def16(2, blank.pack("C*"))
X68k::Sprite.on

X68k::Bg.main_setup
X68k::Bg.main_clear
X68k::Bg.main_fill(0, 0, 64, 64, 0, 1)

8.times do |i|
  X68k::Bg.main_put(i * 4, i * 2, 4, 1)
end

96.times do |i|
  X68k::Bg.main_scroll(i * 2, i)
  X68k::Sys.wait(96000)
end

X68k::Bg.off
X68k::Text.locate(0, 0)
X68k::Text.print("done")
X68k::Text.cursor_on
