# CyberStick direct polling experiment
#
# This is an experimental reader based on the Arduino CyberStick sketch.
# It is intentionally separate from X68k::Ajoy, which uses AJOY.X.

RCV_SIZE = 12

port = ARGV.length > 0 ? ARGV[0].to_i : 0
timeout = ARGV.length > 1 ? ARGV[1].to_i : 30000
ack_bit = ARGV.length > 2 ? ARGV[2].to_i : 6
req_bit = ARGV.length > 3 ? ARGV[3].to_i : (port == 1 ? 5 : 4)

def hex2(n)
  sprintf("$%02X", n & 0xff)
end

def bin8(n)
  sprintf("%08b", n & 0xff)
end

def line(s = "")
  printf("%-62s\n", s)
end

def decode_cyber(rcv)
  return nil if rcv.length < RCV_SIZE

  x = (rcv[3] & 0xf0) | ((rcv[7] >> 4) & 0x0f)
  y = (rcv[2] & 0xf0) | ((rcv[6] >> 4) & 0x0f)
  throttle = (rcv[4] & 0xf0) | ((rcv[8] >> 4) & 0x0f)
  raw_buttons = (rcv[0] & 0xf0) | ((rcv[1] >> 4) & 0x0f)
  normal = (0xff - raw_buttons) & 0xff
  raw10 = rcv[10] & 0xf0
  mask = 0

  mask |= (1 << 0) if (normal & 0x40) != 0 # a
  mask |= (1 << 1) if (normal & 0x80) != 0 # b
  mask |= (1 << 2) if (normal & 0x20) != 0 # c
  mask |= (1 << 3) if (normal & 0x10) != 0 # d
  mask |= (1 << 4) if (normal & 0x08) != 0 # e1
  mask |= (1 << 5) if (normal & 0x04) != 0 # e2
  mask |= (1 << 6) if (normal & 0x02) != 0 # start
  mask |= (1 << 7) if (normal & 0x01) != 0 # select
  mask |= (1 << 8) if (normal & 0x40) != 0 && raw10 == 0xb0 # a_plus
  mask |= (1 << 9) if (normal & 0x80) != 0 && raw10 == 0x70 # b_plus

  [x, y, throttle, mask]
end

def button_names(mask)
  names = []
  names << "a"      if (mask & (1 << 0)) != 0
  names << "b"      if (mask & (1 << 1)) != 0
  names << "c"      if (mask & (1 << 2)) != 0
  names << "d"      if (mask & (1 << 3)) != 0
  names << "e1"     if (mask & (1 << 4)) != 0
  names << "e2"     if (mask & (1 << 5)) != 0
  names << "start"  if (mask & (1 << 6)) != 0
  names << "select" if (mask & (1 << 7)) != 0
  names << "a_plus" if (mask & (1 << 8)) != 0
  names << "b_plus" if (mask & (1 << 9)) != 0
  names
end

def draw(port, req_bit, ack_bit, timeout, scan)
  rcv, edges, loops, first_value, last_value = scan
  decoded = decode_cyber(rcv)

  X68k::Text.locate(0, 0)
  line("CyberStick direct polling experiment")
  line("ESC/Q: quit")
  line("usage: mruby samples/input/cyber_poll.rb [port=0] [timeout=30000] [ack_bit=6] [req_bit]")
  line(sprintf("port=%d req=%d ack=%d timeout=%d", port, req_bit, ack_bit, timeout))
  line(sprintf("count=%d edges=%d loops=%d", rcv.length, edges, loops))
  line(sprintf("first=%s %s  last=%s %s",
               hex2(first_value), bin8(first_value),
               hex2(last_value), bin8(last_value)))
  line

  if decoded
    x, y, throttle, buttons = decoded
    names = button_names(buttons)
    line(sprintf("x=%3d y=%3d throttle=%3d buttons=%s",
                 x, y, throttle, names.length == 0 ? "(none)" : names.join(" ")))
  else
    line("x=--- y=--- throttle=--- buttons=---")
  end

  line
  line("raw nibbles:")
  i = 0
  while i < RCV_SIZE
    if i < rcv.length
      line(sprintf("%2d %s %s", i, hex2(rcv[i]), bin8(rcv[i])))
    else
      line(sprintf("%2d -- --------", i))
    end
    i += 1
  end
end

X68k::Text.cursor_off
X68k::Text.clear

begin
  loop do
    scan = X68k::Joy.cyber_scan_raw(port, timeout, ack_bit, req_bit)
    draw(port, req_bit, ack_bit, timeout, scan)

    if X68k::Key.sense != 0
      ch = X68k::Key.input & 0xff
      break if ch == 27 || ch == 81 || ch == 113
      while X68k::Key.sense != 0
        X68k::Key.input
      end
    end
  end
ensure
  X68k::Text.cursor_on
  X68k::Text.clear
  puts "done"
end

