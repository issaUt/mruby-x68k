/* Included by x68k_hardware.c. */
static mrb_value
x68k_graph_line(mrb_state *mrb, mrb_value self)
{
  mrb_int x1, y1, x2, y2, color;
  struct iocs_lineptr line;

  mrb_get_args(mrb, "iiiii", &x1, &y1, &x2, &y2, &color);
  line.x1 = (short)x1;
  line.y1 = (short)y1;
  line.x2 = (short)x2;
  line.y2 = (short)y2;
  line.color = (short)color;
  line.linestyle = 0xffff;

  return mrb_fixnum_value(_iocs_line(&line));
}

static mrb_value
x68k_graph_point(mrb_state *mrb, mrb_value self)
{
  mrb_int x, y, color;
  struct iocs_pointptr point;

  mrb_get_args(mrb, "iii", &x, &y, &color);
  point.x = (short)x;
  point.y = (short)y;
  point.color = (short)color;

  return mrb_fixnum_value(_iocs_point(&point));
}

static mrb_value
x68k_graph_box(mrb_state *mrb, mrb_value self)
{
  mrb_int x1, y1, x2, y2, color;
  struct iocs_boxptr box;

  mrb_get_args(mrb, "iiiii", &x1, &y1, &x2, &y2, &color);
  box.x1 = (short)x1;
  box.y1 = (short)y1;
  box.x2 = (short)x2;
  box.y2 = (short)y2;
  box.color = (short)color;
  box.linestyle = 0xffff;

  return mrb_fixnum_value(_iocs_box(&box));
}

static mrb_value
x68k_graph_fill(mrb_state *mrb, mrb_value self)
{
  mrb_int x1, y1, x2, y2, color;
  struct iocs_fillptr fill;

  mrb_get_args(mrb, "iiiii", &x1, &y1, &x2, &y2, &color);
  fill.x1 = (short)x1;
  fill.y1 = (short)y1;
  fill.x2 = (short)x2;
  fill.y2 = (short)y2;
  fill.color = (short)color;

  return mrb_fixnum_value(_iocs_fill(&fill));
}

static mrb_value
x68k_graph_circle(mrb_state *mrb, mrb_value self)
{
  mrb_int x, y, radius, color;
  mrb_int start = 0;
  mrb_int end = 360;
  mrb_int ratio = 0x100;
  struct iocs_circleptr circle;

  mrb_get_args(mrb, "iiii|iii", &x, &y, &radius, &color, &start, &end, &ratio);
  circle.x = (short)x;
  circle.y = (short)y;
  circle.radius = (unsigned short)radius;
  circle.color = (short)color;
  circle.start = (short)start;
  circle.end = (short)end;
  circle.ratio = (unsigned short)ratio;

  return mrb_fixnum_value(_iocs_circle(&circle));
}

static mrb_value
x68k_graph_paint(mrb_state *mrb, mrb_value self)
{
  mrb_int x, y, color;
  struct iocs_paintptr paint;

  mrb_get_args(mrb, "iii", &x, &y, &color);
  paint.x = (short)x;
  paint.y = (short)y;
  paint.color = (short)color;
  paint.buf_start = x68k_paint_buffer;
  paint.buf_end = x68k_paint_buffer + sizeof(x68k_paint_buffer);

  return mrb_fixnum_value(_iocs_paint(&paint));
}

static mrb_value
x68k_graph_pset(mrb_state *mrb, mrb_value self)
{
  mrb_int x, y, color;
  struct iocs_psetptr pset;

  mrb_get_args(mrb, "iii", &x, &y, &color);
  pset.x = (short)x;
  pset.y = (short)y;
  pset.color = (short)color;

  return mrb_fixnum_value(_iocs_pset(&pset));
}

static mrb_value
x68k_graph_symbol(mrb_state *mrb, mrb_value self)
{
  mrb_int x, y, color;
  mrb_int mag_x = 1;
  mrb_int mag_y = 1;
  mrb_int font_type = 0;
  mrb_int angle = 0;
  mrb_value str;
  struct iocs_symbolptr symbol;

  mrb_get_args(mrb, "iiSi|iiii", &x, &y, &str, &color, &mag_x, &mag_y, &font_type, &angle);
  symbol.x1 = (short)x;
  symbol.y1 = (short)y;
  symbol.string_address = (const unsigned char*)RSTRING_CSTR(mrb, str);
  symbol.mag_x = (unsigned char)mag_x;
  symbol.mag_y = (unsigned char)mag_y;
  symbol.color = (short)color;
  symbol.font_type = (unsigned char)font_type;
  symbol.angle = (unsigned char)angle;

  return mrb_fixnum_value(_iocs_symbol(&symbol));
}

static mrb_value
x68k_graph_wipe(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(_iocs_wipe());
}

static mrb_value
x68k_graph_clear_on(mrb_state *mrb, mrb_value self)
{
  _iocs_g_clr_on();
  return mrb_nil_value();
}

static mrb_value
x68k_graph_gpalet(mrb_state *mrb, mrb_value self)
{
  mrb_int palno, color;

  mrb_get_args(mrb, "ii", &palno, &color);
  return mrb_fixnum_value(_iocs_gpalet((short)palno, (short)color));
}

static mrb_value
x68k_graph_crtmod(mrb_state *mrb, mrb_value self)
{
  mrb_int mode;

  mrb_get_args(mrb, "i", &mode);
  return mrb_fixnum_value(_iocs_crtmod((short)mode));
}

static mrb_value
x68k_screen_contrast(mrb_state *mrb, mrb_value self)
{
  mrb_int value;

  mrb_get_args(mrb, "i", &value);
  return mrb_fixnum_value(_iocs_contrast((short)value));
}

static mrb_value
x68k_screen_home(mrb_state *mrb, mrb_value self)
{
  mrb_int mode, x, y;

  mrb_get_args(mrb, "iii", &mode, &x, &y);
  return mrb_fixnum_value(_iocs_home((short)mode, (short)x, (short)y));
}

static mrb_value
x68k_screen_tpalet(mrb_state *mrb, mrb_value self)
{
  mrb_int palno, color;

  mrb_get_args(mrb, "ii", &palno, &color);
  return mrb_fixnum_value(_iocs_tpalet((short)palno, (short)color));
}

static mrb_value
x68k_screen_tpalet2(mrb_state *mrb, mrb_value self)
{
  mrb_int palno, color;

  mrb_get_args(mrb, "ii", &palno, &color);
  return mrb_fixnum_value(_iocs_tpalet2((short)palno, (short)color));
}

static mrb_value
x68k_screen_apage(mrb_state *mrb, mrb_value self)
{
  mrb_int page;

  mrb_get_args(mrb, "i", &page);
  return mrb_fixnum_value(_iocs_apage((short)page));
}

static mrb_value
x68k_screen_vpage(mrb_state *mrb, mrb_value self)
{
  mrb_int page;

  mrb_get_args(mrb, "i", &page);
  return mrb_fixnum_value(_iocs_vpage((short)page));
}

static mrb_value
x68k_screen_window(mrb_state *mrb, mrb_value self)
{
  mrb_int x1, y1, x2, y2;

  mrb_get_args(mrb, "iiii", &x1, &y1, &x2, &y2);
  return mrb_fixnum_value(_iocs_window((short)x1, (short)y1, (short)x2, (short)y2));
}

static mrb_value
x68k_text_color(mrb_state *mrb, mrb_value self)
{
  mrb_int color;

  mrb_get_args(mrb, "i", &color);
  return mrb_fixnum_value(_iocs_b_color((short)color));
}

static mrb_value
x68k_text_locate(mrb_state *mrb, mrb_value self)
{
  mrb_int x, y;

  mrb_get_args(mrb, "ii", &x, &y);
  return mrb_fixnum_value(_iocs_b_locate((short)x, (short)y));
}

static mrb_value
x68k_text_print(mrb_state *mrb, mrb_value self)
{
  mrb_value str;

  mrb_get_args(mrb, "S", &str);
  return mrb_fixnum_value(_iocs_b_print(RSTRING_CSTR(mrb, str)));
}

static mrb_value
x68k_text_putc(mrb_state *mrb, mrb_value self)
{
  mrb_int ch;

  mrb_get_args(mrb, "i", &ch);
  return mrb_fixnum_value(_iocs_b_putc((short)ch));
}

static mrb_value
x68k_text_clear_all(mrb_state *mrb, mrb_value self)
{
  _iocs_b_clr_al();
  return mrb_nil_value();
}

static mrb_value
x68k_text_clear_eol(mrb_state *mrb, mrb_value self)
{
  _iocs_b_clr_ed();
  return mrb_nil_value();
}

static mrb_value
x68k_text_clear_bol(mrb_state *mrb, mrb_value self)
{
  _iocs_b_clr_st();
  return mrb_nil_value();
}

static mrb_value
x68k_text_erase_all(mrb_state *mrb, mrb_value self)
{
  _iocs_b_era_al();
  return mrb_nil_value();
}

static mrb_value
x68k_text_erase_eol(mrb_state *mrb, mrb_value self)
{
  _iocs_b_era_ed();
  return mrb_nil_value();
}

static mrb_value
x68k_text_erase_bol(mrb_state *mrb, mrb_value self)
{
  _iocs_b_era_st();
  return mrb_nil_value();
}

static mrb_value
x68k_text_box(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, x1, y1;
  mrb_int style = 0xffff;
  struct iocs_tboxptr box;

  mrb_get_args(mrb, "iiiii|i", &page, &x, &y, &x1, &y1, &style);
  box.vram_page = (unsigned short)page;
  box.x = (short)x;
  box.y = (short)y;
  box.x1 = (short)x1;
  box.y1 = (short)y1;
  box.line_style = (unsigned short)style;
  _iocs_txbox(&box);

  return mrb_nil_value();
}

static mrb_value
x68k_text_fill(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, x1, y1, pattern;
  struct iocs_txfillptr fill;

  mrb_get_args(mrb, "iiiiii", &page, &x, &y, &x1, &y1, &pattern);
  fill.vram_page = (unsigned short)page;
  fill.x = (short)x;
  fill.y = (short)y;
  fill.x1 = (short)x1;
  fill.y1 = (short)y1;
  fill.fill_patn = (unsigned short)pattern;
  _iocs_txfill(&fill);

  return mrb_nil_value();
}

static mrb_value
x68k_text_rev(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, x1, y1;
  struct iocs_trevptr rev;

  mrb_get_args(mrb, "iiiii", &page, &x, &y, &x1, &y1);
  rev.vram_page = (unsigned short)page;
  rev.x = (short)x;
  rev.y = (short)y;
  rev.x1 = (short)x1;
  rev.y1 = (short)y1;
  _iocs_txrev(&rev);

  return mrb_nil_value();
}

static mrb_value
x68k_text_line(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, x1, y1;
  mrb_int style = 0xffff;
  struct iocs_tlineptr line;

  mrb_get_args(mrb, "iiiii|i", &page, &x, &y, &x1, &y1, &style);
  line.vram_page = (unsigned short)page;
  line.x = (short)x;
  line.y = (short)y;
  line.x1 = (short)x1;
  line.y1 = (short)y1;
  line.line_style = (unsigned short)style;
  _iocs_txline(&line);

  return mrb_nil_value();
}

static mrb_value
x68k_text_xline(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, x1;
  mrb_int style = 0xffff;
  struct iocs_xlineptr line;

  mrb_get_args(mrb, "iiii|i", &page, &x, &y, &x1, &style);
  line.vram_page = (unsigned short)page;
  line.x = (short)x;
  line.y = (short)y;
  line.x1 = (short)x1;
  line.line_style = (unsigned short)style;
  _iocs_txxline(&line);

  return mrb_nil_value();
}

static mrb_value
x68k_text_yline(mrb_state *mrb, mrb_value self)
{
  mrb_int page, x, y, y1;
  mrb_int style = 0xffff;
  struct iocs_ylineptr line;

  mrb_get_args(mrb, "iiii|i", &page, &x, &y, &y1, &style);
  line.vram_page = (unsigned short)page;
  line.x = (short)x;
  line.y = (short)y;
  line.y1 = (short)y1;
  line.line_style = (unsigned short)style;
  _iocs_txyline(&line);

  return mrb_nil_value();
}

static mrb_value
x68k_text_cursor_on(mrb_state *mrb, mrb_value self)
{
  _iocs_b_curon();
  _iocs_os_curon();
  return mrb_nil_value();
}

static mrb_value
x68k_text_cursor_off(mrb_state *mrb, mrb_value self)
{
  _iocs_b_curoff();
  _iocs_os_curof();
  return mrb_nil_value();
}

static mrb_value
x68k_key_sense(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(_iocs_b_keysns());
}

static mrb_value
x68k_key_input(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(_iocs_b_keyinp());
}

static mrb_value
x68k_key_bit(mrb_state *mrb, mrb_value self)
{
  mrb_int group;

  mrb_get_args(mrb, "i", &group);
  return mrb_fixnum_value(_iocs_bitsns((int)group));
}


