puts "CRTC vblank test"
puts "ESC/Q: quit"

X68k::Text.cursor_off
X68k::Text.clear

frame = 0
running = true
marker_width = 40

X68k::Text.locate(0, 0)
puts "CRTC vblank test"
puts "ESC/Q: quit"
puts ""
puts "wait_vblank waits for the next vertical blank."
puts "The counter should advance at the display frame rate."

while running
  if X68k::Key.sense != 0
    ch = X68k::Key.input & 0xff
    running = false if ch == 27 || ch == 81 || ch == 113

    while X68k::Key.sense != 0
      X68k::Key.input
    end
  end

  X68k::Crtc.wait_vblank
  frame += 1

  vdisp = X68k::Crtc.vdisp?
  vblank = X68k::Crtc.vblank?
  marker_pos = frame % marker_width
  marker = ""
  marker_width.times do |i|
    marker << (i == marker_pos ? "*" : ".")
  end

  X68k::Text.locate(0, 7)
  printf("frame:  %8d\n", frame)
  printf("vdisp?: %-5s\n", vdisp ? "true" : "false")
  printf("vblank?: %-5s\n", vblank ? "true" : "false")
  printf("[%s]\n", marker)
end

X68k::Text.cursor_on
X68k::Text.clear
puts "done"
