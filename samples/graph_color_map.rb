# Graphic color number map
# ESC/Q: quit

SCREEN_W = 256
SCREEN_H = 256

X68k::Screen.set_mode(10)
X68k::Screen.clear
X68k::Text.clear
X68k::Text.cursor_off
X68k::Graph.fill(0, 0, SCREEN_W - 1, SCREEN_H - 1, 0)

X68k::Text.locate(0, 0)
X68k::Text.print("Graph color map mode10")
X68k::Text.locate(0, 1)
X68k::Text.print("color numbers 0..31")
X68k::Text.locate(0, 2)
X68k::Text.print("ESC/Q: quit")

n = 0
while n < 32
  x = 16 + (n % 8) * 28
  y = 50 + (n / 8) * 36
  X68k::Graph.fill(x, y, x + 20, y + 18, n)
  X68k::Text.locate(x / 8, (y + 21) / 8)
  X68k::Text.print(n.to_s.rjust(2))
  n += 1
end

loop do
  if X68k::Key.sense != 0
    key = X68k::Key.input & 0xff
    break if key == 27 || key == 81 || key == 113
  end
end

X68k::Text.cursor_on
X68k::Text.clear
X68k::Screen.clear