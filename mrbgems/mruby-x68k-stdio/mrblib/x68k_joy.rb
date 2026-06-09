module X68k
  module Joy
    PORT1 = 0
    PORT2 = 1
    UP = 0x01
    DOWN = 0x02
    LEFT = 0x04
    RIGHT = 0x08
    A = 0x20
    B = 0x40
    X = 0x60
    START = UP | DOWN
    SELECT = LEFT | RIGHT

    def self.port1
      get(PORT1)
    end

    def self.port2
      get(PORT2)
    end

    def self.down?(port, mask)
      (get(port) & mask) == 0
    end

    def self.up?(port = PORT1)
      down?(port, UP)
    end

    def self.down_button?(port = PORT1)
      down?(port, DOWN)
    end

    def self.left?(port = PORT1)
      down?(port, LEFT)
    end

    def self.right?(port = PORT1)
      down?(port, RIGHT)
    end

    def self.a?(port = PORT1)
      down?(port, A)
    end

    def self.b?(port = PORT1)
      down?(port, B)
    end

    def self.x?(port = PORT1)
      down?(port, X)
    end

    def self.start?(port = PORT1)
      down?(port, START)
    end

    def self.select?(port = PORT1)
      down?(port, SELECT)
    end
  end
end
