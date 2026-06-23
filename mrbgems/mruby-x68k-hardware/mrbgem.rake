MRuby::Gem::Specification.new('mruby-x68k-hardware') do |spec|
  spec.license = 'MIT'
  spec.author  = 'X68000 mruby port'
  spec.summary = 'X68000 hardware helpers for mruby'
  spec.cc.include_paths << File.expand_path('../../third_party/libzm2/include', __dir__)
end
