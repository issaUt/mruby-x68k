puts "ZMusic SE test"
puts "available: #{X68k::ZMusic.available?}"
p X68k::ZMusic.status

if ARGV.length == 0
  puts "usage:"
  puts "  mruby samples/sound/zm_se.rb SE.ZMD [track] [volume] [slot]"
  exit
end

path = ARGV[0]
track = ARGV.length > 1 ? ARGV[1].to_i : 7
volume = ARGV.length > 2 ? ARGV[2].to_i : 100
slot = ARGV.length > 3 ? ARGV[3].to_i : 0

puts "play_se: #{path}"
puts "track: #{track}"
puts "volume: #{volume}"
puts "slot: #{slot}"
p X68k::ZMusic.play_se(track, path, volume, slot)

X68k::Sys.wait(300000)
