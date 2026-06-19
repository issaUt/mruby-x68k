port = ARGV.length > 0 ? ARGV[0].to_i : 0
map = ARGV.length > 1 ? ARGV[1].to_i : 0
wait_count = ARGV.length > 2 ? ARGV[2].to_i : 80

BTN_UP     = 1 << 0
BTN_DOWN   = 1 << 1
BTN_LEFT   = 1 << 2
BTN_RIGHT  = 1 << 3
BTN_A      = 1 << 4
BTN_B      = 1 << 5
BTN_C      = 1 << 6
BTN_X      = 1 << 7
BTN_Y      = 1 << 8
BTN_Z      = 1 << 9
BTN_START  = 1 << 10
BTN_MODE   = 1 << 11
DETECTED_6B = 1 << 15

BUTTONS = [
  ["Up",    BTN_UP],
  ["Down",  BTN_DOWN],
  ["Left",  BTN_LEFT],
  ["Right", BTN_RIGHT],
  ["A",     BTN_A],
  ["B",     BTN_B],
  ["C",     BTN_C],
  ["X",     BTN_X],
  ["Y",     BTN_Y],
  ["Z",     BTN_Z],
  ["Start", BTN_START],
  ["Mode",  BTN_MODE],
]

def bits(v)
  sprintf("%08b", v & 0xff)
end

def mask_bits(v)
  sprintf("$%04X", v & 0xffff)
end

def down?(v, bit)
  (v & (1 << bit)) == 0
end

def decode_from_raw(raw, map)
  base_h = raw[0]
  base_l = raw[1]
  marker = raw[3]
  ext_h = raw[4]
  mask = 0

  mask |= BTN_UP if down?(base_h, 0)
  mask |= BTN_DOWN if down?(base_h, 1)
  mask |= BTN_LEFT if down?(base_h, 2)
  mask |= BTN_RIGHT if down?(base_h, 3)
  mask |= DETECTED_6B if (marker & 0x0f) == 0

  if map == 1
    mask |= BTN_A if down?(base_h, 6)
    mask |= BTN_B if down?(base_h, 5)
    mask |= BTN_C if down?(ext_h, 0)
    mask |= BTN_X if down?(ext_h, 2)
    mask |= BTN_Y if down?(base_l, 5)
    mask |= BTN_Z if down?(ext_h, 1)
  else
    mask |= BTN_A if down?(base_l, 5)
    mask |= BTN_B if down?(base_h, 5)
    mask |= BTN_C if down?(base_h, 6)
    mask |= BTN_X if down?(ext_h, 2)
    mask |= BTN_Y if down?(ext_h, 1)
    mask |= BTN_Z if down?(ext_h, 0)
  end
  mask |= BTN_START if down?(base_l, 6)
  mask |= BTN_MODE if down?(ext_h, 3)
  mask
end

def pressed_names(mask)
  names = []
  i = 0
  while i < BUTTONS.length
    name = BUTTONS[i][0]
    bit = BUTTONS[i][1]
    names << name if (mask & bit) != 0
    i += 1
  end
  names
end

def put_line(s)
  printf("%-58s\n", s)
end

puts "SEGA 3B/6B mask test"
puts "usage: mruby joy_sega_dump.rb [port=0] [map=0] [wait=80]"
puts "ESC/Q: quit"

X68k::Text.cursor_off
X68k::Text.clear

running = true
while running
  if X68k::Key.sense != 0
    ch = X68k::Key.input & 0xff
    running = false if ch == 27 || ch == 81 || ch == 113
    while X68k::Key.sense != 0
      X68k::Key.input
    end
  end

  raw = X68k::Joy.sega6b_raw(port, wait_count)
  mask = decode_from_raw(raw, map)
  names = pressed_names(mask)

  X68k::Text.locate(0, 0)
  put_line("SEGA 3B/6B mask test")
  put_line("ESC/Q: quit")
  put_line(sprintf("port=%d map=%d wait=%d", port, map, wait_count))
  put_line(sprintf("mask: %s  6B:%s", mask_bits(mask), (mask & DETECTED_6B) != 0 ? "yes" : "no"))
  put_line("")
  put_line("buttons:")
  put_line(names.length == 0 ? "(none)" : names.join(" "))
  put_line("")
  put_line("raw sequence:")
  raw.each_with_index do |v, i|
    th = (i & 1) == 0 ? "H" : "L"
    put_line(sprintf("%2d TH=%s $%02X %s", i, th, v & 0xff, bits(v)))
  end

  X68k::Crtc.wait_vblank
end

X68k::Joy.control_bit(4, false)
X68k::Joy.control_bit(5, false)
X68k::Joy.control_bit(6, false)
X68k::Joy.control_bit(7, false)
X68k::Text.cursor_on
X68k::Text.clear
puts "done"
