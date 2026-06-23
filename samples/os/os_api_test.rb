puts "OS API test"

$fails = 0

def check(name, actual, expected)
  if actual == expected
    puts "ok  " + name
  else
    puts "ng  " + name
    puts "    expected: " + expected.inspect
    puts "    actual  : " + actual.inspect
    $fails += 1
  end
end

def check_true(name, value)
  check(name, value ? true : false, true)
end

def check_false(name, value)
  check(name, value ? true : false, false)
end

def remove_file(path)
  File.delete(path) if File.file?(path)
end

def remove_dir(path)
  Dir.delete(path) if Dir.exist?(path)
end

root = "tmp_osapi"
file1 = root + "/hello.txt"
file2 = root + "/copy.txt"
file3 = root + "/moved.txt"
subdir = root + "/sub"
file4 = subdir + "/nested.txt"

remove_file(file1)
remove_file(file2)
remove_file(file3)
remove_file(file4)
remove_dir(subdir)
remove_dir(root)

puts "[Dir/File]"
Dir.mkdir(root)
check_true("Dir.exist?", Dir.exist?(root))
check_true("File.exist? directory", File.exist?(root))
check_true("File.directory?", File.directory?(root))
check_false("File.file? directory", File.file?(root))

File.write(file1, "abc\n")
Dir.mkdir(subdir)
File.write(file4, "nested\n")
check_true("File.exist? file", File.exist?(file1))
check_true("File.file?", File.file?(file1))
check_false("File.directory? file", File.directory?(file1))
check("File.read", File.read(file1), "abc\n")
check("File.readlines", File.readlines(file1), ["abc\n"])
check("File.size", File.size(file1), 4)

entries = Dir.entries(root)
check_true("Dir.entries includes file", entries.include?("hello.txt"))
check_true("Dir.glob", Dir.glob(root + "/*.txt").include?(file1))
check_true("Dir.glob recursive", Dir.glob(root + "/**/*.txt").include?(file4))

File.copy(file1, file2)
check("File.copy", File.read(file2), "abc\n")
File.rename(file2, file3)
check_false("File.rename old gone", File.exist?(file2))
check_true("File.rename new exists", File.exist?(file3))

puts "[File path]"
check("File.basename backslash", File.basename("A:\\DIR\\FILE.TXT"), "FILE.TXT")
check("File.dirname backslash", File.dirname("A:\\DIR\\FILE.TXT"), "A:\\DIR")
check("File.basename slash", File.basename("A:/DIR/FILE.TXT"), "FILE.TXT")
check("File.dirname slash", File.dirname("A:/DIR/FILE.TXT"), "A:/DIR")
check("File.extname", File.extname("A:/DIR/FILE.TXT"), ".TXT")
check("File.extname none", File.extname("A:/DIR/FILE"), "")
check("File.expand_path dotdot", File.expand_path("A:/DIR/../FILE.TXT"), "A:/FILE.TXT")
check_true("File.expand_path relative", File.expand_path("README.md").index("README.md") != nil)

puts "[Pathname]"
pn = Pathname.new(root).join("hello.txt")
check("Pathname#to_s", pn.to_s, file1)
check("Pathname#basename", pn.basename.to_s, "hello.txt")
check("Pathname#dirname", pn.dirname.to_s, root)
check("Pathname#extname", pn.extname, ".txt")
check_true("Pathname#file?", pn.file?)
check("Pathname#read", pn.read, "abc\n")
check_true("Pathname#children", Pathname.new(root).children.map { |p| p.basename.to_s }.include?("hello.txt"))

puts "[ENV]"
old_tmp = ENV["MRUBY_X68K_TEST"]
ENV["MRUBY_X68K_TEST"] = "ok"
check("ENV[]=", ENV["MRUBY_X68K_TEST"], "ok")
check("ENV[] case insensitive get", ENV["mruby_x68k_test"], "ok")
ENV["mruby_x68k_test"] = "ok2"
check("ENV[] case insensitive set", ENV["MRUBY_X68K_TEST"], "ok2")
check_true("ENV.keys", ENV.keys.include?("MRUBY_X68K_TEST"))
check("ENV.delete case insensitive", ENV.delete("mruby_x68k_test"), "ok2")
check("ENV deleted", ENV["MRUBY_X68K_TEST"], nil)
ENV["MRUBY_X68K_TEST"] = old_tmp if old_tmp

puts "[system/backquote]"
check_true("system dir", system("dir " + root))
check("$? after system", $?, 0)
out = `dir #{root}`
check("$? after backquote", $?, 0)
check_true("backquote output", out.index("hello") != nil || out.index("HELLO") != nil)

puts "[FileUtils]"
remove_file(file3)
FileUtils.cp(file1, file2)
check_true("FileUtils.cp", File.file?(file2))
FileUtils.mv(file2, file3)
check_true("FileUtils.mv", File.file?(file3))
FileUtils.rm(file3)
check_false("FileUtils.rm", File.exist?(file3))

remove_file(file1)
remove_file(file4)
remove_dir(subdir)
remove_dir(root)

if $fails == 0
  puts "PASS"
else
  puts "FAIL " + $fails.to_s
end
