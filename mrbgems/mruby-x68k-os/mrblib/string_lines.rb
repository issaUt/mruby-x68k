class String
  def lines
    list = []
    start = 0
    i = 0

    while i < length
      ch = self[i, 1]
      if ch == "\r"
        if self[i + 1, 1] == "\n"
          i += 1
        end
        list << self[start..i]
        i += 1
        start = i
      elsif ch == "\n"
        list << self[start..i]
        i += 1
        start = i
      else
        i += 1
      end
    end

    list << self[start..-1] if start < length
    list
  end

  def each_line
    if block_given?
      lines.each do |line|
        yield line
      end
      self
    else
      lines
    end
  end
end
