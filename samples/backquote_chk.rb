puts "Backquote test"
cmd = ARGV[0] || "dir"
puts "command: #{cmd}"
out = `#{cmd}`
puts "--- output ---"
puts out
puts "--- bytes=#{out.size} ---"
