module X68k
  module Screen
    MODE_INFO = {
      0 => [512, 512, 16, 1],
      1 => [512, 480, 16, 1],
      2 => [256, 256, 16, 1],
      3 => [256, 240, 16, 1],
      4 => [512, 512, 16, 4],
      5 => [512, 480, 16, 4],
      6 => [256, 256, 16, 4],
      7 => [256, 240, 16, 4],
      8 => [512, 512, 256, 2],
      9 => [512, 480, 256, 2],
      10 => [256, 256, 256, 2],
      11 => [256, 240, 256, 2],
      12 => [512, 512, 65536, 1],
      13 => [512, 480, 65536, 1],
      14 => [256, 256, 65536, 1],
      15 => [256, 240, 65536, 1],
      16 => [768, 512, 16, 1],
      17 => [1024, 424, 16, 1],
      18 => [1024, 848, 16, 1]
    }

    def self.mode
      crtmod(-1)
    end

    def self.mode_info(mode_no = nil)
      mode_no = mode if mode_no.nil?
      info = MODE_INFO[mode_no]
      return nil if info.nil?

      {
        :mode => mode_no,
        :width => info[0],
        :height => info[1],
        :colors => info[2],
        :pages => info[3]
      }
    end

    def self.size(mode_no = nil)
      info = mode_info(mode_no)
      return nil if info.nil?
      [info[:width], info[:height]]
    end

    def self.width(mode_no = nil)
      info = mode_info(mode_no)
      return nil if info.nil?
      info[:width]
    end

    def self.height(mode_no = nil)
      info = mode_info(mode_no)
      return nil if info.nil?
      info[:height]
    end

    def self.set_mode(mode_no, do_clear = true, page = 0)
      result = crtmod(mode_no)
      vpage(page)
      clear if do_clear
      result
    end
  end
end