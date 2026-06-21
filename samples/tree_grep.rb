# Recursive source search using Dir.entries and File APIs.
#
# Usage:
#   mruby tree_grep.rb rb ZMusic
#   mruby tree_grep.rb c directory
#
# Arguments:
#   ARGV[0] extension without dot, default: rb
#   ARGV[1] keyword to search, default: X68k

target_ext = ARGV[0] || "rb"
keyword = ARGV[1] || "X68k"

target_ext = target_ext[1..-1] if target_ext[0, 1] == "."
target_ext = target_ext.downcase

def each_line(text)
  lf = 10.chr
  cr = 13.chr
  text.gsub(cr + lf, lf).gsub(cr, lf).split(lf)
end

def join_path(dir, name)
  return name if dir == "."
  dir + "/" + name
end

def match_ext?(path, ext)
  dot = path.rindex(".")
  return false unless dot
  path[(dot + 1)..-1].downcase == ext
end

def search_file(path, ext, keyword)
  return 0 unless File.file?(path)
  return 0 unless match_ext?(path, ext)

  count = 0
  each_line(File.read(path)).each_with_index do |line, i|
    if line.index(keyword)
      printf("%s:%d: %s\n", path, i + 1, line)
      count += 1
    end
  end
  count
end

def search_tree(dir, ext, keyword)
  total = 0

  Dir.entries(dir).each do |name|
    next if name == "." || name == ".."

    path = join_path(dir, name)
    if File.directory?(path)
      total += search_tree(path, ext, keyword)
    else
      total += search_file(path, ext, keyword)
    end
  end

  total
end

print "ext=.", target_ext, " keyword=", keyword, "\n"
count = search_tree(".", target_ext, keyword)
print "matches=", count, "\n"
