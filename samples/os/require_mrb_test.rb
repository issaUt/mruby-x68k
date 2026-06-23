# .mrb require sample for mrb.x / mrb-os.x.
#
# Build mrb_lib.mrb first:
#   mrbc -o samples/os/require_test/mrb_lib.mrb samples/os/require_test/mrb_lib.rb
#
# Then run:
#   mrb samples/os/require_mrb_test.mrb

$LOAD_PATH << "samples/os/require_test"

p require("mrb_lib")
p require("mrb_lib")
puts hello_from_mrb
puts "$mrb_lib_count=#{$mrb_lib_count}"
