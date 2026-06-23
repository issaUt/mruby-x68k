# load / require sample.
#
# This checks:
#   - require loads a .rb file once
#   - load executes a .rb file every time
#   - $LOAD_PATH can be extended from Ruby

$LOAD_PATH << "samples/os/require_test"

puts "require test"
p $LOAD_PATH

p require("hello_lib")
p require("hello_lib")
puts hello_from_required_file
puts "$hello_lib_count=#{$hello_lib_count}"

load "samples/os/require_test/load_lib.rb"
load "samples/os/require_test/load_lib.rb"
puts "$load_lib_count=#{$load_lib_count}"

puts "$LOADED_FEATURES"
p $LOADED_FEATURES
