# Recursive grep sample using an external grep command.
#
# Usage:
#   mruby samples/os/bq_grep_cmd.rb rb X68k
#   mruby samples/os/bq_grep_cmd.rb c directory
#
# Arguments:
#   ARGV[0] extension without dot, default: rb
#   ARGV[1] grep pattern, default: X68k
#
# Dir.glob is used for traversal.
# Kernel#` is used to run grep, and all command output is collected in Ruby
# before printing the final report.

target_ext = ARGV[0] || "rb"
pattern = ARGV[1] || "X68k"

target_ext = target_ext[1..-1] if target_ext[0, 1] == "."
target_ext = target_ext.downcase

files = Dir.glob("**/*." + target_ext).select { |path| File.file?(path) }

results = []
files.each do |path|
  out = `grep #{pattern} #{path}`
  lines = []
  out.each_line do |line|
    line = line.chomp
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
