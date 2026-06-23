puts "Pathname/File path test"

paths = [
  "A:\\DIR\\FILE.TXT",
  "A:/DIR/FILE.TXT",
  "samples/tree_grep.rb",
  "README.md"
]

paths.each do |path|
  puts path
  puts "  dirname : " + File.dirname(path)
  puts "  basename: " + File.basename(path)
  puts "  extname : " + File.extname(path)
  puts "  expand  : " + File.expand_path(path)
end

p = Pathname.new("samples")
puts "children of " + p.to_s
p.children.each do |child|
  puts "  " + child.to_s
end
