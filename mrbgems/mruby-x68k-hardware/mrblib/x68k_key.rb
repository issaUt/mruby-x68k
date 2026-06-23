module X68k
  module Key
    NUM_2 = [0x09, 0x10]
    NUM_4 = [0x08, 0x80]
    NUM_6 = [0x09, 0x02]
    NUM_8 = [0x08, 0x10]
    SPACE = [0x06, 0x20]
    Z = [0x05, 0x04]
    X = [0x05, 0x08]

    def self.down?(group, mask)
      (bit(group) & mask) != 0
    end

    def self.pressed?(key)
      down?(key[0], key[1])
    end

    def self.num2?
      pressed?(NUM_2)
    end

    def self.num4?
      pressed?(NUM_4)
    end

    def self.num6?
      pressed?(NUM_6)
    end

    def self.num8?
      pressed?(NUM_8)
    end

    def self.space?
      pressed?(SPACE)
    end

    def self.z?
      pressed?(Z)
    end

    def self.x?
      pressed?(X)
    end
  end
end
