# Basic shell-script style APIs.

print "pwd: ", Dir.pwd, "\n"
print "PATH: ", ENV["PATH"], "\n"

Dir.mkdir("tmp_mrb") unless Dir.exist?("tmp_mrb")
File.write("tmp_mrb/hello.txt", "hello from mruby-x68k\n")
File.copy("tmp_mrb/hello.txt", "tmp_mrb/copy.txt")
File.rename("tmp_mrb/copy.txt", "tmp_mrb/moved.txt")

print "entries:\n"
Dir.entries("tmp_mrb").each do |name|
  print "  ", name, "\n"
end

ok = system("dir tmp_mrb")
print "system ok=", ok, " status=", $?, "\n"

File.delete("tmp_mrb/hello.txt")
FileUtils.rm("tmp_mrb/moved.txt")
Dir.delete("tmp_mrb")
