#include <mruby.h>
#include <mruby/class.h>
#include <mruby/array.h>
#include <mruby/data.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <x68k/iocs.h>
#include <stdio.h>

typedef struct {
  FILE *fp;
} x68k_file;

static unsigned char x68k_paint_buffer[4096];

static void
x68k_file_free(mrb_state *mrb, void *ptr)
{
  x68k_file *file = (x68k_file*)ptr;

  if (file != NULL) {
    if (file->fp != NULL) {
      fclose(file->fp);
      file->fp = NULL;
    }
    mrb_free(mrb, file);
  }
}

static const struct mrb_data_type x68k_file_type = {
  "X68kFile", x68k_file_free
};

static x68k_file*
x68k_get_file(mrb_state *mrb, mrb_value self)
{
  x68k_file *file = DATA_GET_PTR(mrb, self, &x68k_file_type, x68k_file);

  if (file == NULL || file->fp == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "closed file");
  }

  return file;
}

static void
x68k_write_value(mrb_state *mrb, mrb_value value)
{
  if (mrb_nil_p(value)) {
    return;
  }

  if (!mrb_string_p(value)) {
    value = mrb_obj_as_string(mrb, value);
  }

  fwrite(RSTRING_PTR(value), (size_t)RSTRING_LEN(value), 1, stdout);
}

static mrb_value
x68k_kernel_puts(mrb_state *mrb, mrb_value self)
{
  mrb_int argc;
  mrb_value *argv;

  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc == 0) {
    fputc('\n', stdout);
  }
  else {
    for (mrb_int i = 0; i < argc; i++) {
      x68k_write_value(mrb, argv[i]);
      fputc('\n', stdout);
    }
  }
  fflush(stdout);

  return mrb_nil_value();
}

static mrb_value
x68k_kernel_printf(mrb_state *mrb, mrb_value self)
{
  mrb_int argc;
  mrb_value *argv;
  mrb_value str;

  mrb_get_args(mrb, "*", &argv, &argc);
  str = mrb_funcall_argv(mrb, self, mrb_intern_lit(mrb, "sprintf"), argc, argv);
  x68k_write_value(mrb, str);
  fflush(stdout);

  return mrb_nil_value();
}

static mrb_value
x68k_file_obj_gets(mrb_state *mrb, mrb_value self)
{
  x68k_file *file = x68k_get_file(mrb, self);
  mrb_value line = mrb_str_new_capa(mrb, 128);
  int ch;

  while ((ch = fgetc(file->fp)) != EOF) {
    char c = (char)ch;
    mrb_str_cat(mrb, line, &c, 1);
    if (c == '\n') {
      break;
    }
  }

  if (RSTRING_LEN(line) == 0 && ch == EOF) {
    return mrb_nil_value();
  }

  return line;
}

static mrb_value
x68k_file_obj_write(mrb_state *mrb, mrb_value self)
{
  x68k_file *file = x68k_get_file(mrb, self);
  mrb_value data;
  size_t len;
  size_t nwritten;

  mrb_get_args(mrb, "S", &data);
  len = (size_t)RSTRING_LEN(data);
  nwritten = fwrite(RSTRING_PTR(data), 1, len, file->fp);
  if (nwritten != len) {
    mrb_sys_fail(mrb, "write");
  }

  return mrb_fixnum_value((mrb_int)nwritten);
}

static mrb_value
x68k_file_obj_print(mrb_state *mrb, mrb_value self)
{
  x68k_file *file = x68k_get_file(mrb, self);
  mrb_int argc;
  mrb_value *argv;

  mrb_get_args(mrb, "*", &argv, &argc);
  for (mrb_int i = 0; i < argc; i++) {
    mrb_value str = argv[i];
    size_t len;

    if (!mrb_string_p(str)) {
      str = mrb_obj_as_string(mrb, str);
    }
    len = (size_t)RSTRING_LEN(str);
    if (fwrite(RSTRING_PTR(str), 1, len, file->fp) != len) {
      mrb_sys_fail(mrb, "print");
    }
  }

  return mrb_nil_value();
}

static mrb_value
x68k_file_obj_close(mrb_state *mrb, mrb_value self)
{
  x68k_file *file = x68k_get_file(mrb, self);

  if (fclose(file->fp) != 0) {
    file->fp = NULL;
    mrb_sys_fail(mrb, "close");
  }
  file->fp = NULL;

  return mrb_nil_value();
}

static mrb_value
x68k_open_file(mrb_state *mrb, struct RClass *file_class, const char *path, const char *mode)
{
  FILE *fp;
  x68k_file *file;

  fp = fopen(path, mode);
  if (fp == NULL) {
    mrb_sys_fail(mrb, path);
  }

  file = (x68k_file*)mrb_malloc(mrb, sizeof(x68k_file));
  file->fp = fp;

  return mrb_obj_value(mrb_data_object_alloc(mrb, file_class, file, &x68k_file_type));
}

static mrb_value
x68k_kernel_open(mrb_state *mrb, mrb_value self)
{
  char *path;
  char *mode = "r";
  struct RClass *file_class;

  mrb_get_args(mrb, "z|z", &path, &mode);
  file_class = mrb_class_get(mrb, "X68kFile");

  return x68k_open_file(mrb, file_class, path, mode);
}

static mrb_value
x68k_file_exist(mrb_state *mrb, mrb_value self)
{
  char *path;
  FILE *fp;

  mrb_get_args(mrb, "z", &path);
  fp = fopen(path, "rb");
  if (fp == NULL) {
    return mrb_false_value();
  }

  fclose(fp);
  return mrb_true_value();
}

static mrb_value
x68k_file_read(mrb_state *mrb, mrb_value self)
{
  char *path;
  FILE *fp;
  mrb_value result;
  char buffer[256];
  size_t nread;

  mrb_get_args(mrb, "z", &path);
  fp = fopen(path, "rb");
  if (fp == NULL) {
    mrb_sys_fail(mrb, path);
  }

  result = mrb_str_new_capa(mrb, 256);
  while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
    mrb_str_cat(mrb, result, buffer, (mrb_int)nread);
  }

  if (ferror(fp)) {
    fclose(fp);
    mrb_sys_fail(mrb, path);
  }

  fclose(fp);
  return result;
}

static mrb_value
x68k_file_size(mrb_state *mrb, mrb_value self)
{
  char *path;
  FILE *fp;
  long size;

  mrb_get_args(mrb, "z", &path);
  fp = fopen(path, "rb");
  if (fp == NULL) {
    mrb_sys_fail(mrb, path);
  }

  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    mrb_sys_fail(mrb, path);
  }

  size = ftell(fp);
  fclose(fp);
  if (size < 0) {
    mrb_sys_fail(mrb, path);
  }

  return mrb_fixnum_value((mrb_int)size);
}

static mrb_value
x68k_file_write(mrb_state *mrb, mrb_value self)
{
  char *path;
  mrb_value data;
  FILE *fp;
  size_t len;
  size_t nwritten;

  mrb_get_args(mrb, "zS", &path, &data);
  fp = fopen(path, "wb");
  if (fp == NULL) {
    mrb_sys_fail(mrb, path);
  }

  len = (size_t)RSTRING_LEN(data);
  nwritten = fwrite(RSTRING_PTR(data), 1, len, fp);
  if (nwritten != len || fclose(fp) != 0) {
    mrb_sys_fail(mrb, path);
  }

  return mrb_fixnum_value((mrb_int)nwritten);
}

static mrb_value
x68k_file_open(mrb_state *mrb, mrb_value self)
{
  char *path;
  char *mode = "r";
  struct RClass *file_class = mrb_class_get(mrb, "X68kFile");

  mrb_get_args(mrb, "z|z", &path, &mode);

  return x68k_open_file(mrb, file_class, path, mode);
}

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

static mrb_value
x68k_sys_wait(mrb_state *mrb, mrb_value self)
{
  mrb_int count;
  volatile mrb_int i;

  mrb_get_args(mrb, "i", &count);
  for (i = 0; i < count; i++) {
  }

  return mrb_nil_value();
}

static mrb_value
x68k_joy_get(mrb_state *mrb, mrb_value self)
{
  mrb_int port = 0;

  mrb_get_args(mrb, "|i", &port);
  return mrb_fixnum_value(_iocs_joyget((int)port));
}

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

void
mrb_mruby_x68k_stdio_gem_init(mrb_state *mrb)
{
  struct RClass *file;
  struct RClass *x68k_file;
  struct RClass *x68k;
  struct RClass *graph;
  struct RClass *screen;
  struct RClass *text;
  struct RClass *key;
  struct RClass *joy;
  struct RClass *bg;
  struct RClass *sys;
  struct RClass *sprite;

  mrb_define_method(mrb, mrb->kernel_module, "puts", x68k_kernel_puts, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->kernel_module, "printf", x68k_kernel_printf, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->kernel_module, "open", x68k_kernel_open, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));

  file = mrb_define_class(mrb, "File", mrb->object_class);
  mrb_define_class_method(mrb, file, "exist?", x68k_file_exist, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "exists?", x68k_file_exist, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "open", x68k_file_open, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, file, "read", x68k_file_read, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "size", x68k_file_size, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "write", x68k_file_write, MRB_ARGS_REQ(2));

  x68k_file = mrb_define_class(mrb, "X68kFile", mrb->object_class);
  MRB_SET_INSTANCE_TT(x68k_file, MRB_TT_DATA);
  mrb_define_method(mrb, x68k_file, "gets", x68k_file_obj_gets, MRB_ARGS_NONE());
  mrb_define_method(mrb, x68k_file, "write", x68k_file_obj_write, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, x68k_file, "print", x68k_file_obj_print, MRB_ARGS_ANY());
  mrb_define_method(mrb, x68k_file, "close", x68k_file_obj_close, MRB_ARGS_NONE());

  x68k = mrb_define_module(mrb, "X68k");
  screen = mrb_define_module_under(mrb, x68k, "Screen");
  mrb_define_class_method(mrb, screen, "crtmod", x68k_graph_crtmod, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, screen, "clear", x68k_graph_clear_on, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, screen, "clear_on", x68k_graph_clear_on, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, screen, "contrast", x68k_screen_contrast, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, screen, "wipe", x68k_graph_wipe, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, screen, "gpalet", x68k_graph_gpalet, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, screen, "home", x68k_screen_home, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, screen, "tpalet", x68k_screen_tpalet, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, screen, "tpalet2", x68k_screen_tpalet2, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, screen, "vpage", x68k_screen_vpage, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, screen, "window", x68k_screen_window, MRB_ARGS_REQ(4));

  text = mrb_define_module_under(mrb, x68k, "Text");
  mrb_define_class_method(mrb, text, "color", x68k_text_color, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, text, "locate", x68k_text_locate, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, text, "print", x68k_text_print, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, text, "putc", x68k_text_putc, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, text, "clear", x68k_text_clear_all, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "clear_all", x68k_text_clear_all, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "clear_eol", x68k_text_clear_eol, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "clear_bol", x68k_text_clear_bol, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "erase", x68k_text_erase_all, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "erase_all", x68k_text_erase_all, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "erase_eol", x68k_text_erase_eol, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "erase_bol", x68k_text_erase_bol, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "box", x68k_text_box, MRB_ARGS_ARG(5, 1));
  mrb_define_class_method(mrb, text, "fill", x68k_text_fill, MRB_ARGS_REQ(6));
  mrb_define_class_method(mrb, text, "rev", x68k_text_rev, MRB_ARGS_REQ(5));
  mrb_define_class_method(mrb, text, "line", x68k_text_line, MRB_ARGS_ARG(5, 1));
  mrb_define_class_method(mrb, text, "xline", x68k_text_xline, MRB_ARGS_ARG(4, 1));
  mrb_define_class_method(mrb, text, "yline", x68k_text_yline, MRB_ARGS_ARG(4, 1));
  mrb_define_class_method(mrb, text, "cursor_on", x68k_text_cursor_on, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "cursor_off", x68k_text_cursor_off, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "curon", x68k_text_cursor_on, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, text, "curoff", x68k_text_cursor_off, MRB_ARGS_NONE());

  key = mrb_define_module_under(mrb, x68k, "Key");
  mrb_define_class_method(mrb, key, "sense", x68k_key_sense, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, key, "input", x68k_key_input, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, key, "bit", x68k_key_bit, MRB_ARGS_REQ(1));

  joy = mrb_define_module_under(mrb, x68k, "Joy");
  mrb_define_class_method(mrb, joy, "get", x68k_joy_get, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, joy, "raw", x68k_joy_get, MRB_ARGS_OPT(1));

  bg = mrb_define_module_under(mrb, x68k, "Bg");
  mrb_define_class_method(mrb, bg, "ctrlst", x68k_bg_ctrlst, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, bg, "ctrlgt", x68k_bg_ctrlgt, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, bg, "scroll", x68k_bg_scroll, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, bg, "scroll_direct", x68k_bg_scroll_direct, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, bg, "text", x68k_bg_text, MRB_ARGS_REQ(4));
  mrb_define_class_method(mrb, bg, "textgt", x68k_bg_textgt, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, bg, "clear", x68k_bg_clear, MRB_ARGS_ARG(1, 1));
  mrb_define_class_method(mrb, bg, "put", x68k_bg_put, MRB_ARGS_ARG(4, 3));
  mrb_define_class_method(mrb, bg, "fill", x68k_bg_fill, MRB_ARGS_ARG(6, 3));
  mrb_define_class_method(mrb, bg, "fill_page", x68k_bg_fill_page, MRB_ARGS_ARG(2, 3));
  mrb_define_class_method(mrb, bg, "map_blit", x68k_bg_map_blit, MRB_ARGS_ARG(8, 3));
  mrb_define_class_method(mrb, bg, "on", x68k_bg_on, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, bg, "off", x68k_bg_off, MRB_ARGS_NONE());

  sys = mrb_define_module_under(mrb, x68k, "Sys");
  mrb_define_class_method(mrb, sys, "wait", x68k_sys_wait, MRB_ARGS_REQ(1));

  sprite = mrb_define_module_under(mrb, x68k, "Sprite");
  mrb_define_class_method(mrb, sprite, "init", x68k_sprite_init, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, sprite, "on", x68k_sprite_on, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, sprite, "off", x68k_sprite_off, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, sprite, "cgclr", x68k_sprite_cgclr, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, sprite, "defcg", x68k_sprite_defcg, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, sprite, "regst", x68k_sprite_regst, MRB_ARGS_REQ(6));
  mrb_define_class_method(mrb, sprite, "palet", x68k_sprite_palet, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, sprite, "palette", x68k_sprite_palet, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, sprite, "def16", x68k_sprite_def16, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, sprite, "put", x68k_sprite_put, MRB_ARGS_ARG(4, 5));

  graph = mrb_define_module_under(mrb, x68k, "Graph");
  mrb_define_class_method(mrb, graph, "line", x68k_graph_line, MRB_ARGS_REQ(5));
  mrb_define_class_method(mrb, graph, "point", x68k_graph_point, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, graph, "pset", x68k_graph_pset, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, graph, "box", x68k_graph_box, MRB_ARGS_REQ(5));
  mrb_define_class_method(mrb, graph, "fill", x68k_graph_fill, MRB_ARGS_REQ(5));
  mrb_define_class_method(mrb, graph, "circle", x68k_graph_circle, MRB_ARGS_ARG(4, 3));
  mrb_define_class_method(mrb, graph, "paint", x68k_graph_paint, MRB_ARGS_REQ(3));
  mrb_define_class_method(mrb, graph, "symbol", x68k_graph_symbol, MRB_ARGS_ARG(4, 4));
  mrb_define_class_method(mrb, graph, "wipe", x68k_graph_wipe, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, graph, "clear_on", x68k_graph_clear_on, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, graph, "gpalet", x68k_graph_gpalet, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, graph, "crtmod", x68k_graph_crtmod, MRB_ARGS_REQ(1));
}

void
mrb_mruby_x68k_stdio_gem_final(mrb_state *mrb)
{
}
