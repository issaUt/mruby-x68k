class Dir
  def self.glob(pattern)
    pattern = pattern.to_s
    recursive_at = pattern.index("**/")
    recursive_at = pattern.index("**\\") unless recursive_at

    if recursive_at
      base = pattern[0, recursive_at]
      rest = pattern[(recursive_at + 3)..-1]
      base = "." if base.nil? || base.empty?
      base = base[0..-2] if base.length > 1 && (base[-1, 1] == "/" || base[-1, 1] == "\\")

      list = []
      Dir.__x68k_glob_recursive(base, rest, list)
      return list
    end

    dir = File.dirname(pattern)
    mask = File.basename(pattern)
    dir = "." if dir.empty?

    list = []
    Dir.entries(dir).each do |name|
      next if name == "." || name == ".."
      next unless Dir.__x68k_glob_match?(mask, name)

      list << Dir.__x68k_join_path(dir, name)
    end
    list
  end

  def self.__x68k_join_path(dir, name)
    if dir == "." || dir.empty?
      name
    elsif dir[-1, 1] == "/" || dir[-1, 1] == "\\" || dir[-1, 1] == ":"
      dir + name
    else
      dir + "/" + name
    end
  end

  def self.__x68k_split_glob(pattern)
    slash = pattern.index("/")
    backslash = pattern.index("\\")

    if slash && backslash
      pos = slash < backslash ? slash : backslash
    else
      pos = slash || backslash
    end

    if pos
      [pattern[0, pos], pattern[(pos + 1)..-1]]
    else
      [pattern, nil]
    end
  end

  def self.__x68k_glob_recursive(dir, pattern, list)
    mask, rest = Dir.__x68k_split_glob(pattern)

    Dir.entries(dir).each do |name|
      next if name == "." || name == ".."

      path = Dir.__x68k_join_path(dir, name)
      if rest
        next unless File.directory?(path)
        next unless Dir.__x68k_glob_match?(mask, name)
        Dir.__x68k_glob_recursive(path, rest, list)
      else
        list << path if Dir.__x68k_glob_match?(mask, name)
      end
    end

    Dir.entries(dir).each do |name|
      next if name == "." || name == ".."

      path = Dir.__x68k_join_path(dir, name)
      Dir.__x68k_glob_recursive(path, pattern, list) if File.directory?(path)
    end
  end

  def self.__x68k_glob_match?(pattern, text)
    pi = 0
    ti = 0
    star = -1
    mark = 0

    while ti < text.length
      if pi < pattern.length && (pattern[pi, 1] == "?" || pattern[pi, 1] == text[ti, 1])
        pi += 1
        ti += 1
      elsif pi < pattern.length && pattern[pi, 1] == "*"
        star = pi
        pi += 1
        mark = ti
      elsif star >= 0
        pi = star + 1
        mark += 1
        ti = mark
      else
        return false
      end
    end

    pi += 1 while pi < pattern.length && pattern[pi, 1] == "*"
    pi == pattern.length
  end
end
