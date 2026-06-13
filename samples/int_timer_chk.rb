unit = ARGV.length > 0 ? ARGV[0].to_i : 7
cycle = ARGV.length > 1 ? ARGV[1].to_i : 200

puts "Timer-D poll test"
puts "unit=#{unit}, cycle=#{cycle}"
puts "ESC/Q: quit"

X68k::Text.cursor_off
X68k::Text.clear
X68k::Text.locate(0, 0)
puts "Timer-D poll test"
puts "ESC/Q: quit"
puts "unit=#{unit}, cycle=#{cycle}"
puts "unit 7 is 50us, cycle 200 is about 10ms."

result = X68k::Interrupt.timer_d_enable(unit, cycle)
printf("timer_d_enable: %d ($%08X)\n", result, result)
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

  if X68k::Interrupt.timer_d?
    last = X68k::Interrupt.timer_d_count
    total += last
    max_pending = last if last > max_pending
    X68k::Interrupt.timer_d_clear
  else
    last = 0
  end

  frames += 1
  X68k::Text.locate(0, 7)
  printf("last pending: %8d\n", last)
  printf("max pending:  %8d\n", max_pending)
  printf("total:        %8d\n", total)
  printf("frames:       %8d\n", frames)
  X68k::Crtc.wait_vblank
end

X68k::Interrupt.timer_d_disable
X68k::Text.cursor_on
X68k::Text.clear
puts "done"
