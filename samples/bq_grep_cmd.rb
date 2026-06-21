# Recursive grep sample using an external grep command.
#
# Usage:
#   mruby bq_grep_cmd.rb rb X68k
#   mruby bq_grep_cmd.rb c directory
#
# Arguments:
#   ARGV[0] extension without dot, default: rb
#   ARGV[1] grep pattern, default: X68k
#
# Dir.entries and File.directory? are used for traversal.
# Kernel#` is used to run grep, and all command output is collected in Ruby
# before printing the final report.

target_ext = ARGV[0] || "rb"
pattern = ARGV[1] || "X68k"

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

def collect_files(dir, ext, files)
  Dir.entries(dir).each do |name|
    next if name == "." || name == ".."

    path = join_path(dir, name)
    if File.directory?(path)
      collect_files(path, ext, files)
    elsif File.file?(path) && match_ext?(path, ext)
      files << path
    end
  end
end

files = []
collect_files(".", target_ext, files)

results = []
files.each do |path|
  out = `grep #{pattern} #{path}`
  lines = []
  each_line(out).each do |line|
    next if line.length == 0
    lines << line
  end

  if lines.length > 0
    results << {
      "path" => path,
      "lines" => lines
    }
  end
end

print "ext=.", target_ext, " pattern=", pattern, "\n"
print "files=", files.length, " matched_files=", results.length, "\n"

match_count = 0
results.each do |result|
  print "\n", result["path"], "\n"
  result["lines"].each do |line|
    print "  ", line, "\n"
    match_count += 1
  end
end

print "\nmatches=", match_count, "\n"
