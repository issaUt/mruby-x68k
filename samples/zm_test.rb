puts "ZMusic test"
puts "[1] available?"
available = X68k::ZMusic.available?
puts "available: #{available}"
puts "[2] version"
printf("version: %04X\n", X68k::ZMusic.version)
puts "[3] pcm8?"
puts "pcm8: #{X68k::ZMusic.pcm8?}"
puts "[4] status"
p X68k::ZMusic.status

if ARGV.length > 0
  puts "[5] play_bgm"
  result = X68k::ZMusic.play_bgm(ARGV[0], 100, false)
  puts "play_bgm: #{result}"
  puts "[6] wait"
  X68k::Sys.wait(600000)

  if ARGV.length > 1
    puts "[7] play_se"
    se_volume = ARGV.length > 2 ? ARGV[2].to_i : 100
    puts "se_volume: #{se_volume}"
    X68k::ZMusic.play_se(7, ARGV[1], se_volume)
    puts "[8] wait"
    X68k::Sys.wait(600000)
  end

  puts "[9] fadeout"
  X68k::ZMusic.fadeout(20)
  puts "[10] wait"
  X68k::Sys.wait(600000)
  puts "[11] stop"
  X68k::ZMusic.stop
end
