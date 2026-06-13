bgm_path = ARGV.length > 0 ? ARGV[0] : "ZMD/simplebgm.ZMD"
se_path = ARGV.length > 1 ? ARGV[1] : "ZMD/laser.ZMD"
pcm_path = ARGV.length > 2 ? ARGV[2] : "ZMD/snare.pcm"

bgm_volume = ARGV.length > 3 ? ARGV[3].to_i : 100
se_volume = ARGV.length > 4 ? ARGV[4].to_i : 90
pcm_pan = ARGV.length > 5 ? ARGV[5].to_i : 3
pcm_frq = ARGV.length > 6 ? ARGV[6].to_i : 4

puts "ZMusic game audio test"
puts "available: #{X68k::ZMusic.available?}"
puts "pcm8: #{X68k::ZMusic.pcm8?}"
p X68k::ZMusic.status
puts "bgm: #{bgm_path}"
puts "se:  #{se_path}"
puts "pcm: #{pcm_path}"
puts "Z: play SE"
puts "X: play ADPCM"
puts "ESC: fadeout and exit"

if !X68k::ZMusic.available?
  puts "Z-MUSIC is not available"
  exit
end

result = X68k::ZMusic.play_bgm(bgm_path, bgm_volume, false)
puts "play_bgm: #{result}"
if result != 0
  exit
end

running = true
while running
  if X68k::Key.sense != 0
    ch = X68k::Key.input & 0xff
    if ch == 27
      puts "fadeout"
      X68k::ZMusic.fadeout(20)
      X68k::Sys.wait(600000)
      running = false
    elsif ch == 90 || ch == 122
      result = X68k::ZMusic.play_se(7, se_path, se_volume)
      puts "play_se: #{result}"
    elsif ch == 88 || ch == 120
      result = X68k::ZMusic.play_adpcm(pcm_path, pcm_pan, pcm_frq)
      puts "play_adpcm: #{result}"
    end

    while X68k::Key.sense != 0
      X68k::Key.input
    end
  end

  X68k::Sys.wait(48000)
end

X68k::ZMusic.stop
puts "done"
