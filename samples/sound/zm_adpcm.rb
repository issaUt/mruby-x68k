puts "ZMusic ADPCM test"
puts "available: #{X68k::ZMusic.available?}"
puts "pcm8: #{X68k::ZMusic.pcm8?}"
p X68k::ZMusic.status

if ARGV.length == 0
  puts "usage:"
  puts "  mruby samples/sound/zm_adpcm.rb SE.ADP [pan] [frq] [priority]"
  puts "  mruby samples/sound/zm_adpcm.rb note NOTE [pan] [frq] [priority]"
  exit
end

if ARGV[0] == "note"
  note = ARGV[1].to_i
  pan = ARGV.length > 2 ? ARGV[2].to_i : 3
  frq = ARGV.length > 3 ? ARGV[3].to_i : 4
  priority = ARGV.length > 4 ? ARGV[4].to_i : 0
  puts "play_adpcm_note: #{note}"
  p X68k::ZMusic.play_adpcm_note(note, pan, frq, priority)
else
  pan = ARGV.length > 1 ? ARGV[1].to_i : 3
  frq = ARGV.length > 2 ? ARGV[2].to_i : 4
  priority = ARGV.length > 3 ? ARGV[3].to_i : 0
  puts "play_adpcm: #{ARGV[0]}"
  p X68k::ZMusic.play_adpcm(ARGV[0], pan, frq, priority)
end

X68k::Sys.wait(300000)
