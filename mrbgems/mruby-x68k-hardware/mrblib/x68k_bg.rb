module X68k
  module Bg
    MAIN_PAGE = 1
    MAIN_REG = 0
    DEFAULT_BLANK_CODE = 8
    DEFAULT_COLOR = 1

    def self.main_clear(code = DEFAULT_BLANK_CODE, color = DEFAULT_COLOR)
      fill_page(MAIN_PAGE, code, color)
    end

    def self.main_put(x, y, code, color = DEFAULT_COLOR, h_flip = 0, v_flip = 0)
      put(MAIN_PAGE, x, y, code, color, h_flip, v_flip)
    end

    def self.main_fill(x, y, w, h, code, color = DEFAULT_COLOR, h_flip = 0, v_flip = 0)
      fill(MAIN_PAGE, x, y, w, h, code, color, h_flip, v_flip)
    end

    def self.main_scroll(x, y)
      scroll_direct(MAIN_REG, x, y)
    end

    def self.main_setup
      ctrlst(0, MAIN_PAGE, 0)
      main_scroll(0, 0)
      on
    end

    def self.map_tile(map, map_width, x, y, default_tile = DEFAULT_BLANK_CODE)
      return default_tile if x < 0 || y < 0 || map_width <= 0
      index = y * map_width + x
      return default_tile if index < 0 || index >= map.length
      map[index]
    end

    def self.map_draw(map, map_width, map_x, map_y, screen_x, screen_y, width, height, default_tile = DEFAULT_BLANK_CODE, default_color = DEFAULT_COLOR)
      map_blit(map, map_width, map_x, map_y, screen_x, screen_y, width, height, MAIN_PAGE, default_tile, default_color)
    end

    def self.map_draw_full(map, map_width, default_tile = DEFAULT_BLANK_CODE, default_color = DEFAULT_COLOR)
      map_draw(map, map_width, 0, 0, 0, 0, 64, 64, default_tile, default_color)
    end
  end
end
