/* Included by x68k_hardware.c. */
static mrb_value
x68k_bg_ctrlst(mrb_state *mrb, mrb_value self)
{
  mrb_int bg, page, mode;

  mrb_get_args(mrb, "iii", &bg, &page, &mode);
  return mrb_fixnum_value(_iocs_bgctrlst((int)bg, (int)page, (int)mode));
}

static mrb_value
x68k_bg_ctrlgt(mrb_state *mrb, mrb_value self)
{
  mrb_int bg;

  mrb_get_args(mrb, "i", &bg);
  return mrb_fixnum_value(_iocs_bgctrlgt((int)bg));
}

static mrb_value
x68k_bg_scroll(mrb_state *mrb, mrb_value self)
{
  mrb_int bg, x, y;

  mrb_get_args(mrb, "iii", &bg, &x, &y);
  return mrb_fixnum_value(_iocs_bgscrlst((int)bg, (int)x, (int)y));
}

static mrb_value
x68k_bg_text(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, value;

  mrb_get_args(mrb, "iiii", &page, &x, &y, &value);
  return mrb_fixnum_value(_iocs_bgtextst((int)page, (int)x, (int)y, (int)value));
}

static mrb_value
x68k_bg_textgt(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y;

  mrb_get_args(mrb, "iii", &page, &x, &y);
  return mrb_fixnum_value(_iocs_bgtextgt((int)page, (int)x, (int)y));
}

static mrb_value
x68k_bg_clear(mrb_state *mrb, mrb_value self)
{
  mrb_int page;
  mrb_int value = 0;

  mrb_get_args(mrb, "i|i", &page, &value);
  return mrb_fixnum_value(_iocs_bgtextcl((int)page, (int)value));
}

static unsigned short
x68k_bg_attr(mrb_int code, mrb_int color, mrb_int h_flip, mrb_int v_flip)
{
  return (unsigned short)(((v_flip & 1) << 15) |
                          ((h_flip & 1) << 14) |
                          ((color & 0x0f) << 8) |
                          (code & 0xff));
}

static volatile unsigned short *
x68k_bg_page_addr(mrb_int page)
{
  if (page == 0) {
    return (volatile unsigned short *)0x00ebc000;
  }
  if (page == 1) {
    return (volatile unsigned short *)0x00ebe000;
  }
  return NULL;
}

static mrb_value
x68k_bg_put(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, code;
  mrb_int color = 1;
  mrb_int h_flip = 0;
  mrb_int v_flip = 0;
  volatile unsigned short *base;
  int old_super;

  mrb_get_args(mrb, "iiii|iii", &page, &x, &y, &code, &color, &h_flip, &v_flip);
  if (x < 0 || x >= 64 || y < 0 || y >= 64) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "BG text coordinate out of range");
  }

  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 &= 0xfdfc;
  base = x68k_bg_page_addr(page);
  if (base == NULL) {
    if (old_super > 0) {
      _iocs_b_super(old_super);
    }
    mrb_raise(mrb, E_ARGUMENT_ERROR, "BG text page must be 0 or 1");
  }
  base[(int)y * 64 + (int)x] = x68k_bg_attr(code, color, h_flip, v_flip);
  *(volatile unsigned short *)0x00eb0808 |= 0x0203;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static mrb_value
x68k_bg_fill(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, w, h, code;
  mrb_int color = 1;
  mrb_int h_flip = 0;
  mrb_int v_flip = 0;
  volatile unsigned short *base;
  unsigned short attr;
  int old_super;

  mrb_get_args(mrb, "iiiiii|iii", &page, &x, &y, &w, &h, &code, &color, &h_flip, &v_flip);
  if (x < 0 || x >= 64 || y < 0 || y >= 64 || w < 0 || h < 0 || x + w > 64 || y + h > 64) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "BG fill rectangle out of range");
  }

  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 &= 0xfdfc;
  base = x68k_bg_page_addr(page);
  if (base == NULL) {
    if (old_super > 0) {
      _iocs_b_super(old_super);
    }
    mrb_raise(mrb, E_ARGUMENT_ERROR, "BG text page must be 0 or 1");
  }

  attr = x68k_bg_attr(code, color, h_flip, v_flip);
  for (int yy = 0; yy < (int)h; yy++) {
    volatile unsigned short *row = base + ((int)y + yy) * 64 + (int)x;
    for (int xx = 0; xx < (int)w; xx++) {
      row[xx] = attr;
    }
  }
  *(volatile unsigned short *)0x00eb0808 |= 0x0203;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static mrb_value
x68k_bg_fill_page(mrb_state *mrb, mrb_value self)
{
  mrb_int page, code;
  mrb_int color = 1;
  mrb_int h_flip = 0;
  mrb_int v_flip = 0;
  volatile unsigned short *base;
  unsigned short attr;
  int old_super;

  mrb_get_args(mrb, "ii|iii", &page, &code, &color, &h_flip, &v_flip);

  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 &= 0xfdfc;
  base = x68k_bg_page_addr(page);
  if (base == NULL) {
    if (old_super > 0) {
      _iocs_b_super(old_super);
    }
    mrb_raise(mrb, E_ARGUMENT_ERROR, "BG text page must be 0 or 1");
  }

  attr = x68k_bg_attr(code, color, h_flip, v_flip);
  for (int i = 0; i < 64 * 64; i++) {
    base[i] = attr;
  }
  *(volatile unsigned short *)0x00eb0808 |= 0x0203;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static unsigned short
x68k_bg_tile_attr_from_value(mrb_state *mrb, mrb_value tile, mrb_int default_color)
{
  mrb_int code;
  mrb_int color;
  mrb_int h_flip = 0;
  mrb_int v_flip = 0;

  if (mrb_array_p(tile)) {
    mrb_int len = RARRAY_LEN(tile);
    if (len <= 0) {
      code = 0;
      color = default_color;
    } else {
      code = mrb_integer(mrb_ary_entry(tile, 0));
      color = len > 1 ? mrb_integer(mrb_ary_entry(tile, 1)) : default_color;
      h_flip = len > 2 ? mrb_integer(mrb_ary_entry(tile, 2)) : 0;
      v_flip = len > 3 ? mrb_integer(mrb_ary_entry(tile, 3)) : 0;
    }
  } else {
    code = mrb_integer(tile);
    color = default_color;
  }

  return x68k_bg_attr(code, color, h_flip, v_flip);
}

static mrb_value
x68k_bg_map_blit(mrb_state *mrb, mrb_value self)
{
  mrb_value map;
  mrb_int map_width, map_x, map_y, screen_x, screen_y, width, height;
  mrb_int page = 1;
  mrb_int default_code = 8;
  mrb_int default_color = 1;
  volatile unsigned short *base;
  unsigned short default_attr;
  int old_super;

  mrb_get_args(mrb, "Aiiiiiii|iii", &map, &map_width, &map_x, &map_y, &screen_x, &screen_y,
               &width, &height, &page, &default_code, &default_color);

  if (map_width <= 0 || width < 0 || height < 0 ||
      screen_x < 0 || screen_y < 0 || screen_x + width > 64 || screen_y + height > 64) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "BG map blit rectangle out of range");
  }

  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 &= 0xfdfc;
  base = x68k_bg_page_addr(page);
  if (base == NULL) {
    if (old_super > 0) {
      _iocs_b_super(old_super);
    }
    mrb_raise(mrb, E_ARGUMENT_ERROR, "BG text page must be 0 or 1");
  }

  default_attr = x68k_bg_attr(default_code, default_color, 0, 0);
  for (int dy = 0; dy < (int)height; dy++) {
    volatile unsigned short *row = base + ((int)screen_y + dy) * 64 + (int)screen_x;
    int src_y = (int)map_y + dy;
    for (int dx = 0; dx < (int)width; dx++) {
      int src_x = (int)map_x + dx;
      mrb_int index = (mrb_int)src_y * map_width + src_x;
      if (src_x < 0 || src_y < 0 || index < 0 || index >= RARRAY_LEN(map)) {
        row[dx] = default_attr;
      } else {
        row[dx] = x68k_bg_tile_attr_from_value(mrb, mrb_ary_entry(map, index), default_color);
      }
    }
  }
  *(volatile unsigned short *)0x00eb0808 |= 0x0203;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static mrb_value
x68k_bg_on(mrb_state *mrb, mrb_value self)
{
  int old_super;

  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 |= 0x0203;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static mrb_value
x68k_bg_off(mrb_state *mrb, mrb_value self)
{
  int old_super;

  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 &= 0xfffc;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static mrb_value
x68k_bg_scroll_direct(mrb_state *mrb, mrb_value self)
{
  mrb_int bg, x, y;
  volatile unsigned short *regs;
  int old_super;

  mrb_get_args(mrb, "iii", &bg, &x, &y);
  if (bg < 0 || bg > 1) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "BG number must be 0 or 1");
  }

  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 &= 0xfdfc;
  regs = (volatile unsigned short *)(0x00eb0800 + ((int)bg * 4));
  regs[0] = (unsigned short)x;
  regs[1] = (unsigned short)y;
  *(volatile unsigned short *)0x00eb0808 |= 0x0203;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static mrb_value
x68k_sprite_init(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(_iocs_sp_init());
}

static mrb_value
x68k_sprite_on(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(_iocs_sp_on());
}

static mrb_value
x68k_sprite_off(mrb_state *mrb, mrb_value self)
{
  _iocs_sp_off();
  return mrb_nil_value();
}

static mrb_value
x68k_sprite_cgclr(mrb_state *mrb, mrb_value self)
{
  mrb_int code;

  mrb_get_args(mrb, "i", &code);
  return mrb_fixnum_value(_iocs_sp_cgclr((short)code));
}

static mrb_value
x68k_sprite_defcg(mrb_state *mrb, mrb_value self)
{
  mrb_int code, mode;
  mrb_value data;

  mrb_get_args(mrb, "iiS", &code, &mode, &data);
  return mrb_fixnum_value(_iocs_sp_defcg((short)code, (short)mode, RSTRING_PTR(data)));
}

static mrb_value
x68k_sprite_regst(mrb_state *mrb, mrb_value self)
{
  mrb_int no, x, y, code, prw, attr;

  mrb_get_args(mrb, "iiiiii", &no, &x, &y, &code, &prw, &attr);
  return mrb_fixnum_value(_iocs_sp_regst((short)no, (short)x, (short)y, (short)code, (short)prw, (short)attr));
}

static mrb_value
x68k_sprite_palet(mrb_state *mrb, mrb_value self)
{
  mrb_int pal_code, block, color;

  mrb_get_args(mrb, "iii", &pal_code, &block, &color);
  return mrb_fixnum_value(_iocs_spalet((int)(0x80000000u | ((unsigned int)pal_code & 0x0f)), (short)block, (short)color));
}

static unsigned short
x68k_pack_sprite_nibbles(const unsigned char *p)
{
  return (unsigned short)(((p[0] & 0x0f) << 12) |
                          ((p[1] & 0x0f) << 8) |
                          ((p[2] & 0x0f) << 4) |
                          (p[3] & 0x0f));
}

static mrb_value
x68k_sprite_def16(mrb_state *mrb, mrb_value self)
{
  mrb_int code;
  mrb_value data;
  unsigned short *reg0;
  unsigned short *reg1;
  const unsigned char *src;
  int old_super;

  mrb_get_args(mrb, "iS", &code, &data);
  if (RSTRING_LEN(data) < 256) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "sprite pattern must be 256 bytes");
  }

  src = (const unsigned char *)RSTRING_PTR(data);
  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 &= 0xfdfc;
  reg0 = (unsigned short *)(0x00eb8000 + ((int)code * 128));
  reg1 = (unsigned short *)(0x00eb8040 + ((int)code * 128));

  for (int y = 0; y < 16; y++) {
    const unsigned char *row = src + y * 16;
    *reg0++ = x68k_pack_sprite_nibbles(row + 0);
    *reg0++ = x68k_pack_sprite_nibbles(row + 4);
    *reg1++ = x68k_pack_sprite_nibbles(row + 8);
    *reg1++ = x68k_pack_sprite_nibbles(row + 12);
  }

  *(volatile unsigned short *)0x00eb0808 |= 0x0203;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static mrb_value
x68k_sprite_put(mrb_state *mrb, mrb_value self)
{
  mrb_int no, x, y, code;
  mrb_int color = 1;
  mrb_int prw = 3;
  mrb_int h_flip = 0;
  mrb_int v_flip = 0;
  mrb_int ext = 0;
  volatile unsigned short *reg;
  unsigned short ctrl;
  unsigned short pwr;
  int old_super;

  mrb_get_args(mrb, "iiii|iiiii", &no, &x, &y, &code, &color, &prw, &h_flip, &v_flip, &ext);

  ctrl = (unsigned short)(((v_flip & 1) << 15) |
                          ((h_flip & 1) << 14) |
                          ((color & 0x0f) << 8) |
                          (code & 0xff));
  pwr = (unsigned short)(((ext & 1) << 2) | (prw & 0x03));

  old_super = _iocs_b_super(0);
  *(volatile unsigned short *)0x00eb0808 &= 0xfdfc;
  reg = (volatile unsigned short *)(0x00eb0000 + ((int)no * 8));
  reg[0] = (unsigned short)x;
  reg[1] = (unsigned short)y;
  reg[2] = ctrl;
  reg[3] = pwr;
  *(volatile unsigned short *)0x00eb0808 |= 0x0203;
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}


