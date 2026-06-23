module X68k
  module Graph
    def self.polyline(points, color)
      return nil if points.length < 2

      i = 0
      last = points.length - 1
      while i < last
        p1 = points[i]
        p2 = points[i + 1]
        line(p1[0], p1[1], p2[0], p2[1], color)
        i += 1
      end

      nil
    end

    def self.polygon(points, color)
      return nil if points.length < 2

      polyline(points, color)
      p1 = points[points.length - 1]
      p2 = points[0]
      line(p1[0], p1[1], p2[0], p2[1], color)

      nil
    end
  end
end
