puts "X68k::Graph demo"

X68k::Screen.set_mode(12)
X68k::Screen.clear

# Draw a color fan with IOCS-based graphics primitives.
i = 0
while i < 128
  color = (i * 2) | ((i * 2) << 8)
  X68k::Graph.line(0, i * 2, 255, 255 - i * 2, color)
  i += 1
end

X68k::Graph.box(16, 16, 112, 72, 0xffff)
X68k::Graph.fill(144, 48, 224, 104, 0xf81f)

puts "done"
