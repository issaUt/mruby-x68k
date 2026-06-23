# Graphic cyan candidate check
# ESC/Q: quit

X68k::Screen.set_mode(10)
X68k::Screen.clear
X68k::Text.clear
X68k::Text.cursor_off
X68k::Graph.fill(0, 0, 255, 255, 0)

X68k::Text.locate(0, 0)
X68k::Text.print("Cyan candidate")
X68k::Text.locate(0, 1)
X68k::Text.print("line / box / fill")
X68k::Text.locate(0, 2)
X68k::Text.print("ESC/Q: quit")

colors = [195, 199, 203, 207, 211, 215, 219, 223, 227, 231]
labels = ["195", "199", "203", "207", "211", "215", "219", "223", "227", "231"]

i = 0
while i < colors.length
  c = colors[i]
  y = 42 + i * 18
  X68k::Text.locate(0, y / 8)
  X68k::Text.print(labels[i])
  X68k::Graph.line(56, y, 104, y, c)
  X68k::Graph.box(120, y - 5, 160, y + 5, c)
  X68k::Graph.fill(176, y - 5, 216, y + 5, c)
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