sorttype = ARGV[0] || "name"

out = `dir`

total_lines = 0
elements = []
out.each_line do |line|
  total_lines += 1
  next if line =~ /^\s/

  fields = line.strip.split(" ")
  next if fields.length != 5

  # Skip the temporary file created by Kernel#` while this sample is running.
  next if fields[2].to_i == 0

  elements << {
    "name" => fields[0].strip,
    "ext" => fields[1].strip,
    "size" => fields[2].to_i
  }
end

print "Total line = ", total_lines, "\n"
print "SortType: "
case sorttype
when "size"
  print "size"
  elements.sort! { |x, y| x["size"] <=> y["size"] }
when "downcase"
  print "downcase"
  elements.sort! { |x, y| x["name"].downcase <=> y["name"].downcase }
else
  print "name"
  elements.sort! { |x, y| x["name"] <=> y["name"] }
end
print "\n\n"

print "File count = ", elements.length, "\n"

elements.each do |value|
  printf("%8s: %s.%s\n", value["size"], value["name"], value["ext"])
end
