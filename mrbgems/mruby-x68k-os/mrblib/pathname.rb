class Pathname
  def initialize(path)
    @path = path.to_s
  end

  def to_s
    @path
  end

  def inspect
    "#<Pathname:#{@path}>"
  end

  def ==(other)
    other.respond_to?(:to_s) && @path == other.to_s
  end

  def +(other)
    join(other)
  end

  def join(*parts)
    path = @path
    parts.each do |part|
      s = part.to_s
      next if s.empty?
      last = path.empty? ? "" : path[path.length - 1, 1]
      if path.empty? || last == "/" || last == "\\" || last == ":"
        path = path + s
      else
        path = path + "/" + s
      end
    end
    Pathname.new(path)
  end

  def basename
    Pathname.new(File.basename(@path))
  end

  def dirname
    Pathname.new(File.dirname(@path))
  end

  def extname
    File.extname(@path)
  end

  def expand_path
    Pathname.new(File.expand_path(@path))
  end

  def exist?
    File.exist?(@path)
  end

  def file?
    File.file?(@path)
  end

  def directory?
    File.directory?(@path)
  end

  def read
    File.read(@path)
  end

  def write(data)
    File.write(@path, data)
  end

  def size
    File.size(@path)
  end

  def children
    list = []
    Dir.entries(@path).each do |name|
      next if name == "." || name == ".."
      list << Pathname.new(@path).join(name)
    end
    list
  end
end
