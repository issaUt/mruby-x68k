MRuby::Gem::Specification.new('mruby-x68k-stdio') do |spec|
  spec.license = 'MIT'
  spec.author  = 'X68000 mruby port'
  spec.summary = 'small stdio helpers for X68000 mruby'
  spec.cc.include_paths << File.expand_path('../../third_party/libzm2/include', __dir__)
end
