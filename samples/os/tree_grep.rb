# Recursive source search using Dir.glob and File.readlines.
#
# Usage:
#   mruby samples/os/tree_grep.rb rb ZMusic
#   mruby samples/os/tree_grep.rb c directory
#
# Arguments:
#   ARGV[0] extension without dot, default: rb
#   ARGV[1] keyword to search, default: X68k

target_ext = ARGV[0] || "rb"
keyword = ARGV[1] || "X68k"

target_ext = target_ext[1..-1] if target_ext[0, 1] == "."
target_ext = target_ext.downcase
pattern = "**/*." + target_ext

def search_file(path, keyword)
  count = 0
  File.readlines(path).each_with_index do |line, i|
    line = line.chomp
    if line.index(keyword)
      printf("%s:%d: %s\n", path, i + 1, line)
      count += 1
    end
  end
  count
end

print "glob=", pattern, " keyword=", keyword, "\n"
count = 0
Dir.glob(pattern).each do |path|
  next unless File.file?(path)
  count += search_file(path, keyword)
end
print "matches=", count, "\n"
