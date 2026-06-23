cycle = ARGV.length > 0 ? ARGV[0].to_i : 1
disp = ARGV.length > 1 ? ARGV[1].to_i != 0 : false

puts "Interrupt poll test"
puts "VSync interrupt count is updated in C handler."
puts "ESC/Q: quit"

X68k::Text.cursor_off
X68k::Text.clear
X68k::Text.locate(0, 0)
puts "VSync interrupt poll test"
puts "ESC/Q: quit"
printf("disp=%s cycle=%d\n", disp ? "true" : "false", cycle)

result = X68k::Interrupt.vsync_enable(disp, cycle)
printf("vsync_enable: %d ($%08X)\n", result, result)
if result < 0
  X68k::Text.cursor_on
  exit
end

total = 0
last = 0
max_pending = 0
frames = 0
running = true

while running
  if X68k::Key.sense != 0
    ch = X68k::Key.input & 0xff
    running = false if ch == 27 || ch == 81 || ch == 113
    while X68k::Key.sense != 0
      X68k::Key.input
    end
  end

  if X68k::Interrupt.vsync?
    last = X68k::Interrupt.vsync_count
    total += last
    max_pending = last if last > max_pending
    X68k::Interrupt.vsync_clear
  else
    last = 0
  end

  frames += 1
  X68k::Text.locate(0, 6)
  printf("last pending: %8d\n", last)
  printf("max pending:  %8d\n", max_pending)
  printf("total:        %8d\n", total)
  printf("frames:       %8d\n", frames)
  printf("vblank?:      %-5s\n", X68k::Crtc.vblank? ? "true" : "false")
  X68k::Crtc.wait_vblank
end

X68k::Interrupt.vsync_disable
X68k::Text.cursor_on
X68k::Text.clear
puts "done"
