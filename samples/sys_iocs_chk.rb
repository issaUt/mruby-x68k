puts "Sys/IOCS low level test"

puts "[1] supervisor"
puts "before: #{X68k::Sys.super?}"
block_super = nil
X68k::Sys.with_super do
  block_super = X68k::Sys.super?
end
puts "inside: #{block_super}"
puts "after:  #{X68k::Sys.super?}"

puts "[2] interrupt disable"
old_sr = X68k::Sys.interrupt_disable
X68k::Sys.interrupt_enable(old_sr)
printf("old sr: %04X\n", old_sr)

flag = 0
X68k::Sys.with_interrupt_disabled do
  flag = 123
end
puts "critical flag: #{flag}"

puts "[3] IOCS"
key_sense = X68k::Iocs.call(0x01)
printf("B_KEYSNS: %04X\n", key_sense)

key_bit0 = X68k::Iocs.call(0x04, 0)
printf("BITSNS(0): %04X\n", key_bit0)

puts "[4] IOCS string pointer"
message = "B_PRINT through X68k::Iocs.call\r\n\x00"
X68k::Iocs.call(0x21, 0, 0, 0, 0, 0, message)

puts "[5] block exception restore"
begin
  X68k::Sys.with_super do
    raise "restore check"
  end
rescue
  puts "rescued: #{X68k::Sys.super?}"
end

puts "done"
