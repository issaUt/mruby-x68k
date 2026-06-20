# AJOY.X analog joystick test
# Run AJOY.X first, then:
#   mruby ajoy_chk.rb

def hex4(n)
  sprintf("$%04X", n & 0xffff)
end

def line(s = "")
  printf("%-62s\n", s)
end

def draw(raw, values, mode, speed, preset)
  X68k::Text.locate(0, 0)
  line("AJOY.X analog joystick test")
  line("ESC/Q: quit")
  line("usage: mruby ajoy_chk.rb [real|merge|emu]")
  line("available?: #{X68k::Ajoy.available?}")
  line("mode: #{mode}  speed: #{speed}  preset: #{preset}")
  line("throttle_reverse: #{X68k::Ajoy.throttle_reverse?}")
  line

  if raw && values
    ud, lr, throttle, option, trigger = raw
    line "stick U/D : #{ud.to_s.rjust(3)}  #{hex4(ud)}"
    line "stick L/R : #{lr.to_s.rjust(3)}  #{hex4(lr)}"
    adj_throttle = values[2]
    line "throttle  : #{throttle.to_s.rjust(3)}  #{hex4(throttle)}"
    line "thr adj   : #{adj_throttle.to_s.rjust(3)}  #{hex4(adj_throttle)}"
    line "option    : #{option.to_s.rjust(3)}  #{hex4(option)}"
    line "trigger   : #{hex4(trigger)}"
    line "mask raw  : #{hex4(X68k::Ajoy.trigger_mask_raw || 0)}"
    line "mask merge: #{hex4(X68k::Ajoy.trigger_mask_merged || 0)}"
    line
    buttons = X68k::Ajoy.buttons
    line "buttons   : #{buttons && buttons.length > 0 ? buttons.join(" ") : "(none)"}"
    line
    line "trigger bits are active-low in AJOY.X."
  else
    line "read: failed"
  end
end

X68k::Text.cursor_off
X68k::Text.clear

unless X68k::Ajoy.available?
  line "AJOY.X is not resident."
  X68k::Text.cursor_on
  exit
end

preset = ARGV.length > 0 ? ARGV[0] : "real"
if preset == "emu"
  X68k::Ajoy.button_preset = "emu"
  X68k::Ajoy.throttle_reverse = true
elsif preset == "merge" || preset == "merged"
  X68k::Ajoy.button_preset = "merge"
  X68k::Ajoy.throttle_reverse = false
  preset = "merge"
else
  X68k::Ajoy.button_preset = "real"
  X68k::Ajoy.throttle_reverse = false
  preset = "real"
end

old_mode = X68k::Ajoy.mode
old_speed = X68k::Ajoy.speed

begin
  X68k::Ajoy.mode = 1
  X68k::Ajoy.speed = 0

  loop do
    raw = X68k::Ajoy.read_raw
    values = X68k::Ajoy.read
    draw(raw, values, X68k::Ajoy.mode, X68k::Ajoy.speed, preset)

    if X68k::Key.sense != 0
      key = X68k::Key.input & 0xff
      break if key == 27 || key == 81 || key == 113
      while X68k::Key.sense != 0
        X68k::Key.input
      end
    end

    X68k::Crtc.wait_vblank
  end
ensure
  X68k::Ajoy.mode = old_mode if old_mode
  X68k::Ajoy.speed = old_speed if old_speed
  X68k::Text.cursor_on
  X68k::Text.clear
end
