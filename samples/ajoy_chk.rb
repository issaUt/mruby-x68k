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
  line("usage: mruby ajoy_chk.rb [real|usbCyber]")
  line("available?: #{X68k::Ajoy.available?}")
  line("mode: #{mode}  speed: #{speed}  preset: #{preset}")
  line("throttle_reverse: #{X68k::Ajoy.throttle_reverse?}")
  line

  if raw && values
    raw_ud, raw_lr, raw_throttle, raw_trigger = raw
    ud, lr, throttle, trigger = values
    line "stick U/D : #{ud.to_s.rjust(3)}  #{hex4(ud)}"
    line "stick L/R : #{lr.to_s.rjust(3)}  #{hex4(lr)}"
    line "throttle  : #{throttle.to_s.rjust(3)}  #{hex4(throttle)}"
    line "thr raw   : #{raw_throttle.to_s.rjust(3)}  #{hex4(raw_throttle)}"
    line "trigger   : #{hex4(trigger)}"
    line "mask      : #{hex4(X68k::Ajoy.trigger_mask || 0)}"
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
if preset == "usbCyber"
  X68k::Ajoy.button_preset = "usbCyber"
  X68k::Ajoy.throttle_reverse = true
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
