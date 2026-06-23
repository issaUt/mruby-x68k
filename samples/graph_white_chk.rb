# Graphic white candidate check
# ESC/Q: quit

X68k::Screen.set_mode(10)
X68k::Screen.clear
X68k::Text.clear
X68k::Text.cursor_off
X68k::Graph.fill(0, 0, 255, 255, 0)

X68k::Text.locate(0, 0)
X68k::Text.print("Graph white candidate")
X68k::Text.locate(0, 1)
X68k::Text.print("line / box / fill")
X68k::Text.locate(0, 2)
X68k::Text.print("ESC/Q: quit")

colors = [7, 15, 31, 63, 127, 255, 0x7fff, 0xffff]
labels = ["7", "15", "31", "63", "127", "255", "7fff", "ffff"]

i = 0
while i < colors.length
  c = colors[i]
  y = 48 + i * 22
  X68k::Text.locate(0, y / 8)
  X68k::Text.print(labels[i])
  X68k::Graph.line(56, y, 104, y, c)
  X68k::Graph.box(120, y - 6, 160, y + 6, c)
  X68k::Graph.fill(176, y - 6, 216, y + 6, c)
  i += 1
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