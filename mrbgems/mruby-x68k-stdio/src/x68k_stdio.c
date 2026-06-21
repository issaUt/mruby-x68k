#include <mruby.h>
#include <mruby/class.h>
#include <mruby/array.h>
#include <mruby/data.h>
#include <mruby/error.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <x68k/iocs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#ifndef nullptr
#define nullptr NULL
#endif
#include "libzm2.h"
#include "libzm2util.h"

typedef struct {
  FILE *fp;
} x68k_file;

static unsigned char x68k_paint_buffer[4096];
static mrb_bool x68k_super_mode = FALSE;
static int x68k_super_ssp = 0;

static volatile unsigned long x68k_int_vsync_count = 0;
static volatile unsigned long x68k_int_raster_count = 0;
static volatile unsigned long x68k_int_timer_d_count = 0;
static volatile unsigned long x68k_int_opm_count = 0;
static mrb_bool x68k_int_vsync_enabled = FALSE;
static mrb_bool x68k_int_raster_enabled = FALSE;
static mrb_bool x68k_int_timer_d_enabled = FALSE;
static mrb_bool x68k_int_opm_enabled = FALSE;

#define X68K_JOY_PORT1_ADDR 0x00e9a001
#define X68K_JOY_PORT2_ADDR 0x00e9a003
#define X68K_JOY_CTRL_ADDR  0x00e9a005
#define X68K_JOY_BSR_ADDR   0x00e9a007

#define X68K_ZMUSIC_SE_SLOTS 4
#define X68K_ZMUSIC_ADPCM_SLOTS 4

static unsigned char *x68k_zmusic_buffer = NULL;
static size_t x68k_zmusic_buffer_size = 0;
static unsigned char *x68k_zmusic_se_buffers[X68K_ZMUSIC_SE_SLOTS];
static size_t x68k_zmusic_se_buffer_sizes[X68K_ZMUSIC_SE_SLOTS];
static int x68k_zmusic_se_volumes[X68K_ZMUSIC_SE_SLOTS];
static char x68k_zmusic_se_paths[X68K_ZMUSIC_SE_SLOTS][96];
static unsigned char *x68k_zmusic_adpcm_buffers[X68K_ZMUSIC_ADPCM_SLOTS];
static size_t x68k_zmusic_adpcm_buffer_sizes[X68K_ZMUSIC_ADPCM_SLOTS];

static int
x68k_zmusic_signature_bytes(unsigned char sig_out[8])
{
  const volatile unsigned char *sig;
  unsigned long entry_addr;
  int old_super;
  int i;

  old_super = _iocs_b_super(0);
  entry_addr = *(volatile unsigned long *)0x8c;

  if (entry_addr < 8 || (entry_addr & 1) != 0) {
    if (old_super > 0) {
      _iocs_b_super(old_super);
    }
    return 0;
  }

  sig = (const volatile unsigned char *)(entry_addr - 8);
  for (i = 0; i < 8; i++) {
    sig_out[i] = sig[i];
  }

  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return 1;
}

static int
x68k_zmusic_available_raw(void)
{
  unsigned char sig[8];

  if (!x68k_zmusic_signature_bytes(sig)) {
    return 0;
  }

  return memcmp(sig, "ZmuSiC", 6) == 0;
}

static int
x68k_zmusic_version_raw(void)
{
  unsigned char sig[8];

  if (!x68k_zmusic_signature_bytes(sig) || memcmp(sig, "ZmuSiC", 6) != 0) {
    return -1;
  }

  return ((int)sig[6] << 8) | (int)sig[7];
}

static void
x68k_zmusic_free_buffer(mrb_state *mrb)
{
  if (x68k_zmusic_buffer != NULL) {
    mrb_free(mrb, x68k_zmusic_buffer);
    x68k_zmusic_buffer = NULL;
    x68k_zmusic_buffer_size = 0;
  }
}

static long
x68k_zmusic_adpcm_param(mrb_state *mrb, mrb_int pan, mrb_int frq, mrb_int priority)
{
  if (pan < 0 || pan > 3) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "pan must be 0..3");
  }
  if (frq < 0 || frq > 4) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "frq must be 0..4");
  }
  if (priority < 0 || priority > 255) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "priority must be 0..255");
  }

  return ((long)priority << 16) | ((long)frq << 8) | (long)pan;
}

static void
x68k_zmusic_free_adpcm_buffer(mrb_state *mrb, int slot)
{
  if (slot < 0 || slot >= X68K_ZMUSIC_ADPCM_SLOTS) {
    return;
  }
  if (x68k_zmusic_adpcm_buffers[slot] != NULL) {
    mrb_free(mrb, x68k_zmusic_adpcm_buffers[slot]);
    x68k_zmusic_adpcm_buffers[slot] = NULL;
    x68k_zmusic_adpcm_buffer_sizes[slot] = 0;
  }
}

static void
x68k_zmusic_free_adpcm_buffers(mrb_state *mrb)
{
  int i;

  for (i = 0; i < X68K_ZMUSIC_ADPCM_SLOTS; i++) {
    x68k_zmusic_free_adpcm_buffer(mrb, i);
  }
}
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
x68k_dir_entries(mrb_state *mrb, mrb_value self)
{
  char *path = (char *)".";
  DIR *dir;
  struct dirent *ent;
  mrb_value ary;

  mrb_get_args(mrb, "|z", &path);
  dir = opendir(path);
  if (dir == NULL) {
    mrb_sys_fail(mrb, path);
  }

  ary = mrb_ary_new(mrb);
  while ((ent = readdir(dir)) != NULL) {
    mrb_ary_push(mrb, ary, mrb_str_new_cstr(mrb, ent->d_name));
  }

  closedir(dir);
  return ary;
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

static unsigned int x68k_backquote_counter = 0;

static mrb_value
x68k_kernel_backquote(mrb_state *mrb, mrb_value self)
{
  char *cmd;
  char tmpname[16];
  char syscmd[256];
  FILE *fp;
  mrb_value result;
  char buffer[256];
  size_t nread;
  int n;

  mrb_get_args(mrb, "z", &cmd);

  n = snprintf(tmpname, sizeof(tmpname), "MRBBQ%03u.TMP", x68k_backquote_counter++ % 1000);
  if (n < 0 || n >= (int)sizeof(tmpname)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "temporary file name too long");
  }

  n = snprintf(syscmd, sizeof(syscmd), "%s > %s", cmd, tmpname);
  if (n < 0 || n >= (int)sizeof(syscmd)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "command too long");
  }

  remove(tmpname);
  system(syscmd);

  fp = fopen(tmpname, "rb");
  if (fp == NULL) {
    remove(tmpname);
    return mrb_str_new_lit(mrb, "");
  }

  result = mrb_str_new_capa(mrb, 256);
  while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
    mrb_str_cat(mrb, result, buffer, (mrb_int)nread);
  }

  if (ferror(fp)) {
    fclose(fp);
    remove(tmpname);
    mrb_sys_fail(mrb, tmpname);
  }

  fclose(fp);
  remove(tmpname);
  return result;
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
x68k_file_directory(mrb_state *mrb, mrb_value self)
{
  char *path;
  struct stat st;

  mrb_get_args(mrb, "z", &path);
  if (stat(path, &st) != 0) {
    return mrb_false_value();
  }

  return S_ISDIR(st.st_mode) ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_file_file(mrb_state *mrb, mrb_value self)
{
  char *path;
  struct stat st;

  mrb_get_args(mrb, "z", &path);
  if (stat(path, &st) != 0) {
    return mrb_false_value();
  }

  return S_ISREG(st.st_mode) ? mrb_true_value() : mrb_false_value();
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


static void
x68k_zmusic_free_se_buffer(mrb_state *mrb, int slot)
{
  if (slot < 0 || slot >= X68K_ZMUSIC_SE_SLOTS) {
    return;
  }
  if (x68k_zmusic_se_buffers[slot] != NULL) {
    mrb_free(mrb, x68k_zmusic_se_buffers[slot]);
    x68k_zmusic_se_buffers[slot] = NULL;
    x68k_zmusic_se_buffer_sizes[slot] = 0;
    x68k_zmusic_se_volumes[slot] = -1;
    x68k_zmusic_se_paths[slot][0] = '\0';
  }
}

static void
x68k_zmusic_free_se_buffers(mrb_state *mrb)
{
  int i;

  for (i = 0; i < X68K_ZMUSIC_SE_SLOTS; i++) {
    x68k_zmusic_free_se_buffer(mrb, i);
  }
}

static unsigned char*
x68k_zmusic_load_file(mrb_state *mrb, const char *path, size_t *size_out)
{
  FILE *fp;
  long file_size;
  unsigned char *buffer;
  size_t nread;

  fp = fopen(path, "rb");
  if (fp == NULL) {
    mrb_sys_fail(mrb, path);
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    mrb_sys_fail(mrb, path);
  }
  file_size = ftell(fp);
  if (file_size <= 0) {
    fclose(fp);
    mrb_raise(mrb, E_ARGUMENT_ERROR, "empty file");
  }
  if (fseek(fp, 0, SEEK_SET) != 0) {
    fclose(fp);
    mrb_sys_fail(mrb, path);
  }

  buffer = (unsigned char*)mrb_malloc(mrb, (size_t)file_size);
  nread = fread(buffer, 1, (size_t)file_size, fp);
  if (nread != (size_t)file_size || fclose(fp) != 0) {
    mrb_free(mrb, buffer);
    mrb_sys_fail(mrb, path);
  }

  *size_out = (size_t)file_size;
  return buffer;
}

static unsigned char*
x68k_zmusic_load_zmd(mrb_state *mrb, const char *path, size_t *size_out)
{
  unsigned char *buffer;
  size_t file_size;

  buffer = x68k_zmusic_load_file(mrb, path, &file_size);
  if (file_size < 8) {
    mrb_free(mrb, buffer);
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid ZMD data");
  }

  if (buffer[0] != 0x10 || memcmp(buffer + 1, "ZmuSiC", 6) != 0) {
    mrb_free(mrb, buffer);
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid ZMD data");
  }

  *size_out = (size_t)file_size;
  return buffer;
}

static void
x68k_zmusic_validate_volume(mrb_state *mrb, mrb_int volume)
{
  if (volume < 0 || volume > 100) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "volume must be 0..100");
  }
}

static unsigned char
x68k_zmusic_volume_attenuation(mrb_int volume)
{
  static const unsigned char attenuation[101] = {
    127, 53, 45, 41, 37, 35, 33, 31, 29, 28, 27, 26, 25, 24, 23, 22,
    21, 21, 20, 19, 19, 18, 18, 17, 17, 16, 16, 15, 15, 14, 14, 14,
    13, 13, 12, 12, 12, 12, 11, 11, 11, 10, 10, 10, 10, 9, 9, 9,
    9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 5,
    5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3,
    3, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0
  };

  return attenuation[volume];
}

static void
x68k_zmusic_scale_zmd_volume(mrb_state *mrb, unsigned char *buffer, size_t size, mrb_int volume)
{
  size_t i;
  unsigned char attenuation;

  x68k_zmusic_validate_volume(mrb, volume);
  if (volume == 100 || size < 10) {
    return;
  }

  attenuation = x68k_zmusic_volume_attenuation(volume);
  for (i = 8; i + 1 < size; i++) {
    if (buffer[i] == 0xb6 && buffer[i + 1] <= 127) {
      int scaled = (int)buffer[i + 1] + (int)attenuation;
      if (scaled > 127) {
        scaled = 127;
      }
      buffer[i + 1] = (unsigned char)scaled;
      i++;
    }
  }
}
static mrb_value
x68k_zmusic_available(mrb_state *mrb, mrb_value self)
{
  return x68k_zmusic_available_raw() ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_zmusic_version(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value((mrb_int)x68k_zmusic_version_raw());
}

static mrb_value
x68k_zmusic_stop(mrb_state *mrb, mrb_value self)
{
  int result;

  if (!x68k_zmusic_available_raw()) {
    return mrb_fixnum_value(-1);
  }

  result = zm2_m_stop_all();

  return mrb_fixnum_value(result);
}

static mrb_value
x68k_zmusic_fadeout(mrb_state *mrb, mrb_value self)
{
  mrb_int speed;

  mrb_get_args(mrb, "i", &speed);
  if (!x68k_zmusic_available_raw()) {
    return mrb_fixnum_value(-1);
  }

  return mrb_fixnum_value(zm2_fade_out((int32_t)speed));
}

static mrb_value
x68k_zmusic_play_bgm(mrb_state *mrb, mrb_value self)
{
  char *path;
  mrb_bool fast = FALSE;
  mrb_int volume = 100;
  unsigned char *buffer;
  size_t size;
  int result;

  mrb_get_args(mrb, "z|ib", &path, &volume, &fast);
  if (!x68k_zmusic_available_raw()) {
    return mrb_fixnum_value(-1);
  }

  buffer = x68k_zmusic_load_zmd(mrb, path, &size);

  zm2_m_stop_all();
  x68k_zmusic_free_buffer(mrb);
  x68k_zmusic_free_se_buffers(mrb);
  x68k_zmusic_buffer = buffer;
  x68k_zmusic_buffer_size = size;
  x68k_zmusic_scale_zmd_volume(mrb, x68k_zmusic_buffer, x68k_zmusic_buffer_size, volume);

  if (fast) {
    result = zm2_play_cnv_data_fast(x68k_zmusic_buffer + 7);
  }
  else {
    result = zm2_play_cnv_data((uint32_t)(x68k_zmusic_buffer_size - 7), x68k_zmusic_buffer + 7);
  }
  if (result != 0) {
    x68k_zmusic_free_buffer(mrb);
  }

  return mrb_fixnum_value(result);
}

static mrb_value
x68k_zmusic_status(mrb_state *mrb, mrb_value self)
{
  unsigned char *status;
  mrb_value hash;
  int pcm8_mode;

  if (!x68k_zmusic_available_raw()) {
    return mrb_nil_value();
  }

  status = (unsigned char*)zm2_zm_status();
  if (status == NULL) {
    return mrb_nil_value();
  }

  pcm8_mode = ((int)status[4] << 8) | (int)status[5];
  hash = mrb_hash_new(mrb);
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "midi")), status[0] ? mrb_true_value() : mrb_false_value());
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "midi_type")), mrb_fixnum_value((mrb_int)(signed char)status[1]));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "force_midi")), status[2] ? mrb_true_value() : mrb_false_value());
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "pcm8")), status[3] ? mrb_true_value() : mrb_false_value());
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "pcm8_mode")), mrb_fixnum_value((mrb_int)(short)pcm8_mode));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "resident_flags")), mrb_fixnum_value((mrb_int)status[6]));
  mrb_hash_set(mrb, hash, mrb_symbol_value(mrb_intern_lit(mrb, "play_state")), mrb_fixnum_value((mrb_int)(signed char)status[7]));

  return hash;
}

static mrb_value
x68k_zmusic_pcm8(mrb_state *mrb, mrb_value self)
{
  unsigned char *status;

  if (!x68k_zmusic_available_raw()) {
    return mrb_false_value();
  }

  status = (unsigned char*)zm2_zm_status();
  if (status == NULL) {
    return mrb_false_value();
  }

  return status[3] ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_zmusic_play_se(mrb_state *mrb, mrb_value self)
{
  mrb_int track;
  char *path;
  mrb_int slot = 0;
  mrb_int volume = 100;
  unsigned char *buffer;
  size_t size;
  int32_t table_offset;
  int reload;

  mrb_get_args(mrb, "iz|ii", &track, &path, &volume, &slot);
  if (!x68k_zmusic_available_raw()) {
    return mrb_fixnum_value(-1);
  }
  if (track < 1 || track > 32) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "track must be 1..32");
  }
  if (slot < 0 || slot >= X68K_ZMUSIC_SE_SLOTS) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "slot must be 0..3");
  }

  reload = x68k_zmusic_se_buffers[slot] == NULL ||
           x68k_zmusic_se_volumes[slot] != (int)volume ||
           strcmp(x68k_zmusic_se_paths[slot], path) != 0;
  if (reload) {
    buffer = x68k_zmusic_load_zmd(mrb, path, &size);
    table_offset = zm2_get_zmd_common_size(buffer, size);
    if (table_offset < 0) {
      mrb_free(mrb, buffer);
      mrb_raise(mrb, E_ARGUMENT_ERROR, "unsupported ZMD data");
    }

    x68k_zmusic_free_se_buffer(mrb, (int)slot);
    x68k_zmusic_se_buffers[slot] = buffer;
    x68k_zmusic_se_buffer_sizes[slot] = size;
    x68k_zmusic_se_volumes[slot] = (int)volume;
    strncpy(x68k_zmusic_se_paths[slot], path, sizeof(x68k_zmusic_se_paths[slot]) - 1);
    x68k_zmusic_se_paths[slot][sizeof(x68k_zmusic_se_paths[slot]) - 1] = '\0';
    x68k_zmusic_scale_zmd_volume(mrb, x68k_zmusic_se_buffers[slot], x68k_zmusic_se_buffer_sizes[slot], volume);
  }

  table_offset = zm2_get_zmd_common_size(x68k_zmusic_se_buffers[slot], x68k_zmusic_se_buffer_sizes[slot]);
  if (table_offset < 0) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "unsupported ZMD data");
  }
  zm2_se_play((uint32_t)track, x68k_zmusic_se_buffers[slot] + table_offset);

  return mrb_fixnum_value(0);
}

static mrb_value
x68k_zmusic_play_adpcm(mrb_state *mrb, mrb_value self)
{
  char *path;
  mrb_int pan = 3;
  mrb_int frq = 4;
  mrb_int priority = 0;
  mrb_int slot = 0;
  unsigned char *buffer;
  size_t size;
  int result;

  mrb_get_args(mrb, "z|iiii", &path, &pan, &frq, &priority, &slot);
  if (!x68k_zmusic_available_raw()) {
    return mrb_fixnum_value(-1);
  }
  if (slot < 0 || slot >= X68K_ZMUSIC_ADPCM_SLOTS) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "slot must be 0..3");
  }

  x68k_zmusic_adpcm_param(mrb, pan, frq, priority);
  buffer = x68k_zmusic_load_file(mrb, path, &size);

  x68k_zmusic_free_adpcm_buffer(mrb, (int)slot);
  x68k_zmusic_adpcm_buffers[slot] = buffer;
  x68k_zmusic_adpcm_buffer_sizes[slot] = size;

  zm2_se_adpcm1((uint32_t)x68k_zmusic_adpcm_buffer_sizes[slot],
                (uint8_t)priority, (uint8_t)frq, (uint8_t)pan,
                x68k_zmusic_adpcm_buffers[slot]);
  result = 0;

  return mrb_fixnum_value(result);
}

static mrb_value
x68k_zmusic_play_adpcm_note(mrb_state *mrb, mrb_value self)
{
  mrb_int note;
  mrb_int pan = 3;
  mrb_int frq = 4;
  mrb_int priority = 0;
  int result;

  mrb_get_args(mrb, "i|iii", &note, &pan, &frq, &priority);
  if (!x68k_zmusic_available_raw()) {
    return mrb_fixnum_value(-1);
  }
  if (note < 0 || note > 511) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "note must be 0..511");
  }

  x68k_zmusic_adpcm_param(mrb, pan, frq, priority);
  zm2_se_adpcm2((uint32_t)note, (uint8_t)priority, (uint8_t)frq, (uint8_t)pan);
  result = 0;

  return mrb_fixnum_value(result);
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

static int
x68k_crtc_vdisp_raw(void)
{
  return (*(volatile unsigned char*)0x00e88001 & 0x10) != 0;
}

static int
x68k_crtc_vdisp_super(void)
{
  int old_super;
  int vdisp;

  old_super = _iocs_b_super(0);
  vdisp = x68k_crtc_vdisp_raw();
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return vdisp;
}

static mrb_value
x68k_crtc_vdisp(mrb_state *mrb, mrb_value self)
{
  return x68k_crtc_vdisp_super() ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_crtc_vblank(mrb_state *mrb, mrb_value self)
{
  return x68k_crtc_vdisp_super() ? mrb_false_value() : mrb_true_value();
}

static mrb_value
x68k_crtc_wait_vblank(mrb_state *mrb, mrb_value self)
{
  int old_super;

  old_super = _iocs_b_super(0);
  while (!x68k_crtc_vdisp_raw()) {
  }
  while (x68k_crtc_vdisp_raw()) {
  }
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return mrb_nil_value();
}

static mrb_bool
x68k_sys_set_super(mrb_bool mode)
{
  mrb_bool old_mode = x68k_super_mode;

  if (mode && !x68k_super_mode) {
    x68k_super_ssp = _iocs_b_super(0);
    if (x68k_super_ssp < 0) {
      x68k_super_ssp = 0;
    }
    x68k_super_mode = TRUE;
  }
  else if (!mode && x68k_super_mode) {
    if (x68k_super_ssp != 0) {
      _iocs_b_super(x68k_super_ssp);
      x68k_super_ssp = 0;
    }
    x68k_super_mode = FALSE;
  }

  return old_mode;
}

static mrb_value
x68k_sys_super_p(mrb_state *mrb, mrb_value self)
{
  return x68k_super_mode ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_sys_super(mrb_state *mrb, mrb_value self)
{
  mrb_bool mode = TRUE;

  mrb_get_args(mrb, "|b", &mode);
  return x68k_sys_set_super(mode) ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_sys_yield_block(mrb_state *mrb, void *userdata)
{
  return mrb_yield_argv(mrb, *(mrb_value*)userdata, 0, NULL);
}

static mrb_value
x68k_sys_with_super(mrb_state *mrb, mrb_value self)
{
  mrb_value block;
  mrb_bool old_mode;
  mrb_bool error = FALSE;
  mrb_value result;

  mrb_get_args(mrb, "&", &block);
  if (mrb_nil_p(block)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "block required");
  }

  old_mode = x68k_sys_set_super(TRUE);
  result = mrb_protect_error(mrb, x68k_sys_yield_block, &block, &error);
  x68k_sys_set_super(old_mode);
  if (error) {
    mrb_exc_raise(mrb, result);
  }

  return result;
}

static unsigned short
x68k_sys_interrupt_disable_raw(void)
{
  unsigned short old_sr;

  __asm__ volatile ("movew %%sr,%0" : "=d"(old_sr));
  if ((old_sr & 0x2000) != 0) {
    __asm__ volatile ("movew %0,%%sr" : : "d"((unsigned short)(old_sr | 0x0700)) : "memory");
  }

  return old_sr;
}

static void
x68k_sys_interrupt_enable_raw(unsigned short old_sr)
{
  if ((old_sr & 0x2000) != 0) {
    __asm__ volatile ("movew %0,%%sr" : : "d"(old_sr) : "memory");
  }
}

static mrb_value
x68k_sys_interrupt_disable(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value((mrb_int)x68k_sys_interrupt_disable_raw());
}

static mrb_value
x68k_sys_interrupt_enable(mrb_state *mrb, mrb_value self)
{
  mrb_int old_sr;

  mrb_get_args(mrb, "i", &old_sr);
  x68k_sys_interrupt_enable_raw((unsigned short)old_sr);
  return mrb_nil_value();
}

static mrb_value
x68k_sys_with_interrupt_disabled(mrb_state *mrb, mrb_value self)
{
  mrb_value block;
  unsigned short old_sr;
  mrb_bool error = FALSE;
  mrb_value result;

  mrb_get_args(mrb, "&", &block);
  if (mrb_nil_p(block)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "block required");
  }

  old_sr = x68k_sys_interrupt_disable_raw();
  result = mrb_protect_error(mrb, x68k_sys_yield_block, &block, &error);
  x68k_sys_interrupt_enable_raw(old_sr);
  if (error) {
    mrb_exc_raise(mrb, result);
  }

  return result;
}

static long
x68k_iocs_arg_long(mrb_state *mrb, mrb_value value)
{
  if (mrb_string_p(value)) {
    return (long)RSTRING_PTR(value);
  }
  if (mrb_nil_p(value)) {
    return 0;
  }
  return (long)mrb_integer(value);
}

static unsigned long
x68k_ajoy_vector(void)
{
  unsigned long addr;

  __asm__ volatile (
    "movew #0x01f2,%%sp@-\n"
    ".word 0xff35\n"
    "addql #2,%%sp\n"
    "movel %%d0,%0\n"
    : "=d"(addr)
    :
    : "d0", "cc", "memory"
  );

  return addr;
}

static int
x68k_ajoy_available_raw(void)
{
  unsigned long addr = x68k_ajoy_vector();

  if (addr == 0 || addr > 0x00ffffff || addr < 8 || (addr & 1) != 0) {
    return 0;
  }

  return *(volatile unsigned long *)(addr - 8) == 0x4973696dUL &&
         *(volatile unsigned long *)(addr - 4) == 0x6f636869UL;
}

static long
x68k_ajoy_call(int md, int d2, void *buffer)
{
  long result;

  __asm__ volatile (
    "movel #0x000000f2,%%d0\n"
    "movel %1,%%d1\n"
    "movel %2,%%d2\n"
    "movel %3,%%a1\n"
    "trap #15\n"
    "movel %%d0,%0\n"
    : "=d"(result)
    : "d"((long)md), "d"((long)d2), "g"(buffer)
    : "d0", "d1", "d2", "a1", "cc", "memory"
  );

  return result;
}

#define X68K_AJOY_BUTTONS 16
#define X68K_AJOY_BUTTON_NAME_SIZE 16
#define X68K_AJOY_BUTTON_PATTERNS 16

typedef struct {
  const char *name;
  unsigned short mask;
} x68k_ajoy_button_pattern;

static const x68k_ajoy_button_pattern x68k_ajoy_patterns_real[] = {
  {"select", 0x0001}, {"start", 0x0002}, {"e2", 0x0004}, {"e1", 0x0008},
  {"d", 0x0010}, {"c", 0x0020},
  {"a", 0x0140}, {"a", 0x0440}, {"a_plus", 0x0440},
  {"b", 0x0280}, {"b", 0x0880}, {"b_plus", 0x0880}
};

static const x68k_ajoy_button_pattern x68k_ajoy_patterns_usb_cyber[] = {
  {"select", 0x0001}, {"start", 0x0002}, {"e2", 0x0004}, {"e1", 0x0008},
  {"d", 0x0010}, {"c", 0x0020}, {"a", 0x0a80}, {"b", 0x0540}
};

static char x68k_ajoy_button_map[X68K_AJOY_BUTTONS][X68K_AJOY_BUTTON_NAME_SIZE] = {
  "select", "start", "e2", "e1", "d", "c", "b_plus_b2", "a_plus_a2",
  "b2", "a2", "b", "a", "", "", "", ""
};
static char x68k_ajoy_button_pattern_names[X68K_AJOY_BUTTON_PATTERNS][X68K_AJOY_BUTTON_NAME_SIZE];
static unsigned short x68k_ajoy_button_pattern_masks[X68K_AJOY_BUTTON_PATTERNS];
static int x68k_ajoy_button_pattern_count = 0;
static char x68k_ajoy_button_preset[X68K_AJOY_BUTTON_NAME_SIZE] = "";
static int x68k_ajoy_throttle_reverse = 0;

static mrb_value
x68k_ajoy_read_raw(mrb_state *mrb, unsigned short buffer[5])
{
  long status;

  if (!x68k_ajoy_available_raw()) {
    return mrb_nil_value();
  }

  status = x68k_ajoy_call(0, 0, buffer);
  if (status < 0) {
    return mrb_nil_value();
  }

  return mrb_true_value();
}

static mrb_value
x68k_ajoy_button_map_get(mrb_state *mrb, mrb_value self)
{
  mrb_value result = mrb_ary_new_capa(mrb, X68K_AJOY_BUTTONS);
  int i;

  for (i = 0; i < X68K_AJOY_BUTTONS; i++) {
    if (x68k_ajoy_button_map[i][0] == '\0') {
      mrb_ary_push(mrb, result, mrb_nil_value());
    }
    else {
      mrb_ary_push(mrb, result, mrb_str_new_cstr(mrb, x68k_ajoy_button_map[i]));
    }
  }

  return result;
}

static mrb_value
x68k_ajoy_button_map_set(mrb_state *mrb, mrb_value self)
{
  mrb_value map;
  mrb_int len;
  int i;

  mrb_get_args(mrb, "A", &map);
  len = RARRAY_LEN(map);
  if (len > X68K_AJOY_BUTTONS) {
    len = X68K_AJOY_BUTTONS;
  }

  for (i = 0; i < X68K_AJOY_BUTTONS; i++) {
    x68k_ajoy_button_map[i][0] = '\0';
  }

  for (i = 0; i < len; i++) {
    mrb_value label = mrb_ary_ref(mrb, map, i);
    mrb_value str;
    mrb_int copy_len;

    if (mrb_nil_p(label)) {
      continue;
    }

    str = mrb_obj_as_string(mrb, label);
    copy_len = RSTRING_LEN(str);
    if (copy_len >= X68K_AJOY_BUTTON_NAME_SIZE) {
      copy_len = X68K_AJOY_BUTTON_NAME_SIZE - 1;
    }
    memcpy(x68k_ajoy_button_map[i], RSTRING_PTR(str), (size_t)copy_len);
    x68k_ajoy_button_map[i][copy_len] = '\0';
  }

  return map;
}


static void
x68k_ajoy_copy_label(char *dest, const char *src)
{
  size_t copy_len = strlen(src);
  if (copy_len >= X68K_AJOY_BUTTON_NAME_SIZE) {
    copy_len = X68K_AJOY_BUTTON_NAME_SIZE - 1;
  }
  memcpy(dest, src, copy_len);
  dest[copy_len] = '\0';
}

static void
x68k_ajoy_set_button_patterns_from_table(const x68k_ajoy_button_pattern *patterns, int count, const char *preset)
{
  int i;

  if (count > X68K_AJOY_BUTTON_PATTERNS) {
    count = X68K_AJOY_BUTTON_PATTERNS;
  }

  x68k_ajoy_button_pattern_count = 0;
  for (i = 0; i < count; i++) {
    x68k_ajoy_copy_label(x68k_ajoy_button_pattern_names[i], patterns[i].name);
    x68k_ajoy_button_pattern_masks[i] = patterns[i].mask;
    x68k_ajoy_button_pattern_count++;
  }
  x68k_ajoy_copy_label(x68k_ajoy_button_preset, preset);
}

static void
x68k_ajoy_set_button_preset_raw(const char *preset)
{
  if (strcmp(preset, "usbCyber") == 0) {
    x68k_ajoy_set_button_patterns_from_table(
      x68k_ajoy_patterns_usb_cyber,
      (int)(sizeof(x68k_ajoy_patterns_usb_cyber) / sizeof(x68k_ajoy_patterns_usb_cyber[0])),
      "usbCyber"
    );
    return;
  }
  x68k_ajoy_set_button_patterns_from_table(
    x68k_ajoy_patterns_real,
    (int)(sizeof(x68k_ajoy_patterns_real) / sizeof(x68k_ajoy_patterns_real[0])),
    "real"
  );
}

static mrb_value
x68k_ajoy_button_preset_get(mrb_state *mrb, mrb_value self)
{
  if (x68k_ajoy_button_preset[0] == '\0') {
    x68k_ajoy_set_button_preset_raw("real");
  }
  return mrb_str_new_cstr(mrb, x68k_ajoy_button_preset);
}

static mrb_value
x68k_ajoy_button_preset_set(mrb_state *mrb, mrb_value self)
{
  char *preset;

  mrb_get_args(mrb, "z", &preset);
  x68k_ajoy_set_button_preset_raw(preset);
  return mrb_str_new_cstr(mrb, x68k_ajoy_button_preset);
}

static mrb_value
x68k_ajoy_button_patterns_get(mrb_state *mrb, mrb_value self)
{
  mrb_value result = mrb_ary_new_capa(mrb, x68k_ajoy_button_pattern_count);
  int i;

  for (i = 0; i < x68k_ajoy_button_pattern_count; i++) {
    mrb_value pair = mrb_ary_new_capa(mrb, 2);
    mrb_ary_push(mrb, pair, mrb_str_new_cstr(mrb, x68k_ajoy_button_pattern_names[i]));
    mrb_ary_push(mrb, pair, mrb_fixnum_value((mrb_int)x68k_ajoy_button_pattern_masks[i]));
    mrb_ary_push(mrb, result, pair);
  }

  return result;
}

static mrb_value
x68k_ajoy_button_patterns_set(mrb_state *mrb, mrb_value self)
{
  mrb_value patterns;
  mrb_int len;
  int i;

  mrb_get_args(mrb, "A", &patterns);
  len = RARRAY_LEN(patterns);
  if (len > X68K_AJOY_BUTTON_PATTERNS) {
    len = X68K_AJOY_BUTTON_PATTERNS;
  }

  x68k_ajoy_button_pattern_count = 0;
  x68k_ajoy_button_preset[0] = '\0';
  for (i = 0; i < len; i++) {
    mrb_value pair = mrb_ary_ref(mrb, patterns, i);
    mrb_value label;
    mrb_value str;
    mrb_value mask_value;
    mrb_int copy_len;
    mrb_int mask;

    if (!mrb_array_p(pair) || RARRAY_LEN(pair) < 2) {
      mrb_raise(mrb, E_ARGUMENT_ERROR, "button pattern must be [name, mask]");
    }

    label = mrb_ary_ref(mrb, pair, 0);
    if (mrb_nil_p(label)) {
      continue;
    }

    mask_value = mrb_ary_ref(mrb, pair, 1);
    mask = mrb_fixnum(mask_value);
    str = mrb_obj_as_string(mrb, label);
    copy_len = RSTRING_LEN(str);
    if (copy_len >= X68K_AJOY_BUTTON_NAME_SIZE) {
      copy_len = X68K_AJOY_BUTTON_NAME_SIZE - 1;
    }

    memcpy(x68k_ajoy_button_pattern_names[x68k_ajoy_button_pattern_count], RSTRING_PTR(str), (size_t)copy_len);
    x68k_ajoy_button_pattern_names[x68k_ajoy_button_pattern_count][copy_len] = '\0';
    x68k_ajoy_button_pattern_masks[x68k_ajoy_button_pattern_count] = (unsigned short)(mask & 0xffff);
    x68k_ajoy_button_pattern_count++;
  }

  return patterns;
}

static unsigned short
x68k_ajoy_trigger_raw_bits(unsigned short trigger)
{
  return (unsigned short)((~trigger) & 0xffff);
}

static unsigned short
x68k_ajoy_trigger_mask_value(unsigned short trigger)
{
  unsigned short raw = x68k_ajoy_trigger_raw_bits(trigger);
  unsigned short mask = 0;

  if ((raw & 0x0140) == 0x0140 || (raw & 0x0440) == 0x0440 || (raw & 0x0a80) == 0x0a80) {
    mask |= 0x0001; /* a */
  }
  if ((raw & 0x0280) == 0x0280 || (raw & 0x0880) == 0x0880 || (raw & 0x0540) == 0x0540) {
    mask |= 0x0002; /* b */
  }
  if ((raw & 0x0020) != 0) {
    mask |= 0x0004; /* c */
  }
  if ((raw & 0x0010) != 0) {
    mask |= 0x0008; /* d */
  }
  if ((raw & 0x0008) != 0) {
    mask |= 0x0010; /* e1 */
  }
  if ((raw & 0x0004) != 0) {
    mask |= 0x0020; /* e2 */
  }
  if ((raw & 0x0002) != 0) {
    mask |= 0x0040; /* start */
  }
  if ((raw & 0x0001) != 0) {
    mask |= 0x0080; /* select */
  }
  if ((raw & 0x0440) == 0x0440) {
    mask |= 0x0100; /* a_plus */
  }
  if ((raw & 0x0880) == 0x0880) {
    mask |= 0x0200; /* b_plus */
  }

  return mask;
}

static mrb_value
x68k_ajoy_trigger_mask(mrb_state *mrb, mrb_value self)
{
  unsigned short buffer[5] = {0, 0, 0, 0, 0};

  if (mrb_nil_p(x68k_ajoy_read_raw(mrb, buffer))) {
    return mrb_nil_value();
  }

  return mrb_fixnum_value((mrb_int)x68k_ajoy_trigger_mask_value(buffer[4]));
}

static mrb_value
x68k_ajoy_buttons(mrb_state *mrb, mrb_value self)
{
  unsigned short buffer[5] = {0, 0, 0, 0, 0};
  unsigned short mask;
  mrb_value result;
  int i;

  if (mrb_nil_p(x68k_ajoy_read_raw(mrb, buffer))) {
    return mrb_nil_value();
  }

  mask = (unsigned short)((~buffer[4]) & 0xffff);
  result = mrb_ary_new(mrb);
  if (x68k_ajoy_button_pattern_count > 0) {
    for (i = 0; i < x68k_ajoy_button_pattern_count; i++) {
      unsigned short pattern = x68k_ajoy_button_pattern_masks[i];
      int duplicate = 0;
      int j;

      if (pattern == 0 || (mask & pattern) != pattern) {
        continue;
      }

      for (j = 0; j < i; j++) {
        unsigned short prev_pattern = x68k_ajoy_button_pattern_masks[j];
        if (strcmp(x68k_ajoy_button_pattern_names[j], x68k_ajoy_button_pattern_names[i]) == 0 &&
            prev_pattern != 0 && (mask & prev_pattern) == prev_pattern) {
          duplicate = 1;
          break;
        }
      }

      if (!duplicate) {
        mrb_ary_push(mrb, result, mrb_str_new_cstr(mrb, x68k_ajoy_button_pattern_names[i]));
      }
    }
  }
  else {
    for (i = 0; i < X68K_AJOY_BUTTONS; i++) {
      if ((mask & (1 << i)) != 0 && x68k_ajoy_button_map[i][0] != '\0') {
        mrb_ary_push(mrb, result, mrb_str_new_cstr(mrb, x68k_ajoy_button_map[i]));
      }
    }
  }

  return result;
}


static void
x68k_ajoy_apply_adjustments(unsigned short buffer[5])
{
  if (x68k_ajoy_throttle_reverse) {
    buffer[2] = (unsigned short)(255 - (buffer[2] & 0xff));
  }
}

static mrb_value
x68k_ajoy_read(mrb_state *mrb, mrb_value self)
{
  unsigned short buffer[5] = {0, 0, 0, 0, 0};
  mrb_value result;

  if (mrb_nil_p(x68k_ajoy_read_raw(mrb, buffer))) {
    return mrb_nil_value();
  }

  x68k_ajoy_apply_adjustments(buffer);

  result = mrb_ary_new_capa(mrb, 4);
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buffer[0]));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buffer[1]));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buffer[2]));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buffer[4]));

  return result;
}

static mrb_value
x68k_ajoy_throttle_reverse_get(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(x68k_ajoy_throttle_reverse);
}

static mrb_value
x68k_ajoy_throttle_reverse_set(mrb_state *mrb, mrb_value self)
{
  mrb_bool reverse;

  mrb_get_args(mrb, "b", &reverse);
  x68k_ajoy_throttle_reverse = reverse ? 1 : 0;
  return mrb_bool_value(x68k_ajoy_throttle_reverse);
}

static mrb_value
x68k_ajoy_available(mrb_state *mrb, mrb_value self)
{
  return mrb_bool_value(x68k_ajoy_available_raw());
}

static mrb_value
x68k_ajoy_read_raw_value(mrb_state *mrb, mrb_value self)
{
  unsigned short buffer[5] = {0, 0, 0, 0, 0};
  mrb_value result;

  if (mrb_nil_p(x68k_ajoy_read_raw(mrb, buffer))) {
    return mrb_nil_value();
  }

  result = mrb_ary_new_capa(mrb, 4);
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buffer[0]));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buffer[1]));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buffer[2]));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buffer[4]));

  return result;
}

static mrb_value
x68k_ajoy_mode(mrb_state *mrb, mrb_value self)
{
  mrb_int mode = -1;

  mrb_get_args(mrb, "|i", &mode);
  if (!x68k_ajoy_available_raw()) {
    return mrb_nil_value();
  }
  return mrb_fixnum_value((mrb_int)x68k_ajoy_call(1, (int)mode, NULL));
}

static mrb_value
x68k_ajoy_mode_set(mrb_state *mrb, mrb_value self)
{
  mrb_int mode;

  mrb_get_args(mrb, "i", &mode);
  if (!x68k_ajoy_available_raw()) {
    return mrb_nil_value();
  }
  x68k_ajoy_call(1, (int)mode, NULL);
  return mrb_fixnum_value(mode);
}

static mrb_value
x68k_ajoy_speed(mrb_state *mrb, mrb_value self)
{
  mrb_int speed = -1;

  mrb_get_args(mrb, "|i", &speed);
  if (!x68k_ajoy_available_raw()) {
    return mrb_nil_value();
  }
  return mrb_fixnum_value((mrb_int)x68k_ajoy_call(2, (int)speed, NULL));
}

static mrb_value
x68k_ajoy_speed_set(mrb_state *mrb, mrb_value self)
{
  mrb_int speed;

  mrb_get_args(mrb, "i", &speed);
  if (!x68k_ajoy_available_raw()) {
    return mrb_nil_value();
  }
  x68k_ajoy_call(2, (int)speed, NULL);
  return mrb_fixnum_value(speed);
}

static mrb_value
x68k_iocs_call(mrb_state *mrb, mrb_value self)
{
  mrb_value *values;
  mrb_int argc;
  long regs[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  mrb_int rd = 0;
  mrb_int ra = 0;
  mrb_value result;
  int i;

  argc = mrb_get_args(mrb, "*", &values);
  if (argc < 1 || argc > 10) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "wrong number of arguments");
  }

  for (i = 0; i < argc && i < 8; i++) {
    regs[i] = x68k_iocs_arg_long(mrb, values[i]);
  }
  if (argc > 8) {
    rd = mrb_integer(values[8]);
  }
  if (argc > 9) {
    ra = mrb_integer(values[9]);
  }
  if (rd < 0 || rd > 5) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "rd must be 0..5");
  }
  if (ra < 0 || ra > 2) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "ra must be 0..2");
  }

  __asm__ volatile (
    "moveml %0@, %%d0-%%d5/%%a1-%%a2\n"
    "trap #15\n"
    "moveml %%d0-%%d5/%%a1-%%a2, %0@\n"
    : : "a"(regs)
    : "d0", "d1", "d2", "d3", "d4", "d5", "a1", "a2", "cc", "memory"
  );

  if (rd + ra == 0) {
    return mrb_fixnum_value((mrb_int)regs[0]);
  }

  result = mrb_ary_new_capa(mrb, (mrb_int)(1 + rd + ra));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)regs[0]));
  for (i = 0; i < rd; i++) {
    mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)regs[1 + i]));
  }
  for (i = 0; i < ra; i++) {
    mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)regs[6 + i]));
  }

  return result;
}

__attribute__((interrupt))
static void
x68k_int_handle_vsync(void)
{
  x68k_int_vsync_count++;
}

__attribute__((interrupt))
static void
x68k_int_handle_raster(void)
{
  x68k_int_raster_count++;
}

__attribute__((interrupt))
static void
x68k_int_handle_timer_d(void)
{
  x68k_int_timer_d_count++;
}

__attribute__((interrupt))
static void
x68k_int_handle_opm(void)
{
  x68k_int_opm_count++;
}

static int
x68k_iocs_vdispst_super(const void *addr, int disp, int cycle)
{
  int old_super = _iocs_b_super(0);
  int result = _iocs_vdispst(addr, disp, cycle);
  if (old_super > 0) _iocs_b_super(old_super);
  return result;
}

static int
x68k_iocs_crtcras_super(const void *addr, int raster)
{
  int old_super = _iocs_b_super(0);
  int result = _iocs_crtcras(addr, raster);
  if (old_super > 0) _iocs_b_super(old_super);
  return result;
}

static int
x68k_iocs_timerdst_super(const void *addr, int unit, int cycle)
{
  int old_super = _iocs_b_super(0);
  int result = _iocs_timerdst(addr, unit, cycle);
  if (old_super > 0) _iocs_b_super(old_super);
  return result;
}

static mrb_value
x68k_interrupt_vsync_enable(mrb_state *mrb, mrb_value self)
{
  mrb_bool disp = FALSE;
  mrb_int cycle = 1;
  int result;

  mrb_get_args(mrb, "|bi", &disp, &cycle);
  if (x68k_int_vsync_enabled) {
    return mrb_fixnum_value(0);
  }
  x68k_int_vsync_count = 0;
  result = x68k_iocs_vdispst_super(x68k_int_handle_vsync, disp ? 1 : 0, (int)cycle);
  x68k_int_vsync_enabled = result >= 0;

  return mrb_fixnum_value((mrb_int)result);
}

static mrb_value
x68k_interrupt_vsync_disable(mrb_state *mrb, mrb_value self)
{
  int result = 0;

  if (x68k_int_vsync_enabled) {
    result = x68k_iocs_vdispst_super(0, 0, 0);
  }
  x68k_int_vsync_enabled = FALSE;
  return mrb_fixnum_value((mrb_int)result);
}

static mrb_value
x68k_interrupt_vsync_count(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value((mrb_int)x68k_int_vsync_count);
}

static mrb_value
x68k_interrupt_vsync_clear(mrb_state *mrb, mrb_value self)
{
  x68k_int_vsync_count = 0;
  return mrb_nil_value();
}

static mrb_value
x68k_interrupt_vsync_p(mrb_state *mrb, mrb_value self)
{
  return x68k_int_vsync_count != 0 ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_interrupt_raster_enable(mrb_state *mrb, mrb_value self)
{
  mrb_int raster;
  int result;

  mrb_get_args(mrb, "i", &raster);
  if (x68k_int_raster_enabled) {
    return mrb_fixnum_value(0);
  }
  x68k_int_raster_count = 0;
  result = x68k_iocs_crtcras_super(x68k_int_handle_raster, (int)raster);
  x68k_int_raster_enabled = result >= 0;

  return mrb_fixnum_value((mrb_int)result);
}

static mrb_value
x68k_interrupt_raster_disable(mrb_state *mrb, mrb_value self)
{
  int result = 0;

  if (x68k_int_raster_enabled) {
    result = x68k_iocs_crtcras_super(0, 0);
  }
  x68k_int_raster_enabled = FALSE;
  return mrb_fixnum_value((mrb_int)result);
}

static mrb_value
x68k_interrupt_raster_count(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value((mrb_int)x68k_int_raster_count);
}

static mrb_value
x68k_interrupt_raster_clear(mrb_state *mrb, mrb_value self)
{
  x68k_int_raster_count = 0;
  return mrb_nil_value();
}

static mrb_value
x68k_interrupt_raster_p(mrb_state *mrb, mrb_value self)
{
  return x68k_int_raster_count != 0 ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_interrupt_timer_d_enable(mrb_state *mrb, mrb_value self)
{
  mrb_int unit = 7;
  mrb_int cycle = 200;
  int result;

  mrb_get_args(mrb, "|ii", &unit, &cycle);
  if (x68k_int_timer_d_enabled) {
    return mrb_fixnum_value(0);
  }
  x68k_int_timer_d_count = 0;
  x68k_iocs_timerdst_super(0, 0, 0);
  result = x68k_iocs_timerdst_super(x68k_int_handle_timer_d, (int)unit, (int)cycle);
  x68k_int_timer_d_enabled = result >= 0;

  return mrb_fixnum_value((mrb_int)result);
}

static mrb_value
x68k_interrupt_timer_d_disable(mrb_state *mrb, mrb_value self)
{
  int result = 0;

  if (x68k_int_timer_d_enabled) {
    result = x68k_iocs_timerdst_super(0, 0, 0);
  }
  x68k_int_timer_d_enabled = FALSE;
  return mrb_fixnum_value((mrb_int)result);
}

static mrb_value
x68k_interrupt_timer_d_count(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value((mrb_int)x68k_int_timer_d_count);
}

static mrb_value
x68k_interrupt_timer_d_clear(mrb_state *mrb, mrb_value self)
{
  x68k_int_timer_d_count = 0;
  return mrb_nil_value();
}

static mrb_value
x68k_interrupt_timer_d_p(mrb_state *mrb, mrb_value self)
{
  return x68k_int_timer_d_count != 0 ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_interrupt_opm_enable(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(-2);
}

static mrb_value
x68k_interrupt_opm_disable(mrb_state *mrb, mrb_value self)
{
  x68k_int_opm_enabled = FALSE;
  return mrb_fixnum_value(0);
}

static mrb_value
x68k_interrupt_opm_count(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value((mrb_int)x68k_int_opm_count);
}

static mrb_value
x68k_interrupt_opm_clear(mrb_state *mrb, mrb_value self)
{
  x68k_int_opm_count = 0;
  return mrb_nil_value();
}

static mrb_value
x68k_interrupt_opm_p(mrb_state *mrb, mrb_value self)
{
  return x68k_int_opm_count != 0 ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_interrupt_disable_all(mrb_state *mrb, mrb_value self)
{
  if (x68k_int_vsync_enabled) {
    x68k_iocs_vdispst_super(0, 0, 0);
  }
  if (x68k_int_raster_enabled) {
    x68k_iocs_crtcras_super(0, 0);
  }
  if (x68k_int_timer_d_enabled) {
    x68k_iocs_timerdst_super(0, 0, 0);
  }
  x68k_int_vsync_enabled = FALSE;
  x68k_int_raster_enabled = FALSE;
  x68k_int_timer_d_enabled = FALSE;
  x68k_int_opm_enabled = FALSE;
  return mrb_nil_value();
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


static volatile unsigned char*
x68k_joy_port_addr(int port)
{
  return (volatile unsigned char *)(port == 1 ? X68K_JOY_PORT2_ADDR : X68K_JOY_PORT1_ADDR);
}

static void
x68k_joy_delay(mrb_int count)
{
  volatile mrb_int i;

  for (i = 0; i < count; i++) {
  }
}

static unsigned char
x68k_joy_read_port_super(int port)
{
  volatile unsigned char *joy_port = x68k_joy_port_addr(port);
  int old_super = _iocs_b_super(0);
  unsigned char value = *joy_port;

  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return value;
}

static unsigned char
x68k_joy_read_control_super(void)
{
  volatile unsigned char *ctrl = (volatile unsigned char *)X68K_JOY_CTRL_ADDR;
  int old_super = _iocs_b_super(0);
  unsigned char value = *ctrl;

  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return value;
}

static void
x68k_joy_set_control_bit(int bit, int value)
{
  volatile unsigned char *bsr = (volatile unsigned char *)X68K_JOY_BSR_ADDR;
  int old_super = _iocs_b_super(0);

  *bsr = (unsigned char)(((bit & 7) << 1) | (value ? 1 : 0));
  if (old_super > 0) {
    _iocs_b_super(old_super);
  }
}

static mrb_value
x68k_joy_direct_raw(mrb_state *mrb, mrb_value self)
{
  mrb_int port = 0;

  mrb_get_args(mrb, "|i", &port);
  return mrb_fixnum_value((mrb_int)x68k_joy_read_port_super((int)port));
}

static mrb_value
x68k_joy_control(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value((mrb_int)x68k_joy_read_control_super());
}

static mrb_value
x68k_joy_control_bit(mrb_state *mrb, mrb_value self)
{
  mrb_int bit;
  mrb_bool value;

  mrb_get_args(mrb, "ib", &bit, &value);
  if (bit < 0 || bit > 7) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "bit must be 0..7");
  }
  x68k_joy_set_control_bit((int)bit, value ? 1 : 0);

  return x68k_joy_control(mrb, self);
}

#define X68K_JOY_BTN_UP     (1 << 0)
#define X68K_JOY_BTN_DOWN   (1 << 1)
#define X68K_JOY_BTN_LEFT   (1 << 2)
#define X68K_JOY_BTN_RIGHT  (1 << 3)
#define X68K_JOY_BTN_A      (1 << 4)
#define X68K_JOY_BTN_B      (1 << 5)
#define X68K_JOY_BTN_C      (1 << 6)
#define X68K_JOY_BTN_X      (1 << 7)
#define X68K_JOY_BTN_Y      (1 << 8)
#define X68K_JOY_BTN_Z      (1 << 9)
#define X68K_JOY_BTN_START  (1 << 10)
#define X68K_JOY_BTN_MODE   (1 << 11)
#define X68K_JOY_DETECTED_6B (1 << 15)
#define X68K_JOY_SEGA6B_PHASES 5
#define X68K_JOY_SEGA6B_SCAN_PHASES 9
#define X68K_JOY_CYBER_NIBBLES 12

static int
x68k_joy_default_req_bit(int port)
{
  return port == 1 ? 5 : 4;
}

static int
x68k_joy_read_cyber_poll(int port, int req_bit, int ack_bit, mrb_int timeout, unsigned char values[X68K_JOY_CYBER_NIBBLES], mrb_int *edges, mrb_int *loops, unsigned char *first_value, unsigned char *last_value)
{
  volatile unsigned char *joy_port = x68k_joy_port_addr(port);
  volatile unsigned char *bsr = (volatile unsigned char *)X68K_JOY_BSR_ADDR;
  int old_super = _iocs_b_super(0);
  int count = 0;
  unsigned char value;
  int last_ack;

  if (timeout < 0) {
    timeout = 0;
  }

  value = *joy_port;
  *first_value = value;
  last_ack = (value & (1 << ack_bit)) == 0;
  *edges = 0;
  *loops = 0;

  *bsr = (unsigned char)(((req_bit & 7) << 1) | 0);
  while (count < X68K_JOY_CYBER_NIBBLES && *loops < timeout) {
    int ack;

    value = *joy_port;
    ack = (value & (1 << ack_bit)) == 0;
    if (!last_ack && ack) {
      values[count++] = (unsigned char)((value & 0x0f) << 4);
      (*edges)++;
      if (count == 1) {
        *bsr = (unsigned char)(((req_bit & 7) << 1) | 1);
      }
    }
    last_ack = ack;
    (*loops)++;
  }
  *bsr = (unsigned char)(((req_bit & 7) << 1) | 1);
  *last_value = *joy_port;

  if (old_super > 0) {
    _iocs_b_super(old_super);
  }

  return count;
}

static void
x68k_joy_cyber_args(mrb_state *mrb, int *port, mrb_int *timeout, int *ack_bit, int *req_bit)
{
  mrb_int port_arg = 0;
  mrb_int timeout_arg = 30000;
  mrb_int ack_arg = 6;
  mrb_int req_arg = -1;

  mrb_get_args(mrb, "|iiii", &port_arg, &timeout_arg, &ack_arg, &req_arg);
  if (ack_arg < 0 || ack_arg > 7) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "ack_bit must be 0..7");
  }
  if (req_arg > 7) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "req_bit must be 0..7");
  }

  *port = (int)port_arg;
  *timeout = timeout_arg;
  *ack_bit = (int)ack_arg;
  *req_bit = req_arg < 0 ? x68k_joy_default_req_bit(*port) : (int)req_arg;
}

static mrb_value
x68k_joy_cyber_raw(mrb_state *mrb, mrb_value self)
{
  int port;
  int req_bit;
  int ack_bit;
  mrb_int timeout;
  mrb_int edges;
  mrb_int loops;
  unsigned char first_value;
  unsigned char last_value;
  unsigned char values[X68K_JOY_CYBER_NIBBLES];
  int count;
  int i;
  mrb_value result;

  x68k_joy_cyber_args(mrb, &port, &timeout, &ack_bit, &req_bit);
  count = x68k_joy_read_cyber_poll(port, req_bit, ack_bit, timeout, values, &edges, &loops, &first_value, &last_value);

  result = mrb_ary_new_capa(mrb, count);
  for (i = 0; i < count; i++) {
    mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)values[i]));
  }

  return result;
}

static mrb_value
x68k_joy_cyber_scan_raw(mrb_state *mrb, mrb_value self)
{
  int port;
  int req_bit;
  int ack_bit;
  mrb_int timeout;
  mrb_int edges;
  mrb_int loops;
  unsigned char first_value;
  unsigned char last_value;
  unsigned char values[X68K_JOY_CYBER_NIBBLES];
  int count;
  int i;
  mrb_value raw;
  mrb_value result;

  x68k_joy_cyber_args(mrb, &port, &timeout, &ack_bit, &req_bit);
  count = x68k_joy_read_cyber_poll(port, req_bit, ack_bit, timeout, values, &edges, &loops, &first_value, &last_value);

  raw = mrb_ary_new_capa(mrb, count);
  for (i = 0; i < count; i++) {
    mrb_ary_push(mrb, raw, mrb_fixnum_value((mrb_int)values[i]));
  }

  result = mrb_ary_new_capa(mrb, 5);
  mrb_ary_push(mrb, result, raw);
  mrb_ary_push(mrb, result, mrb_fixnum_value(edges));
  mrb_ary_push(mrb, result, mrb_fixnum_value(loops));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)first_value));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)last_value));

  return result;
}

static mrb_value
x68k_joy_cyber(mrb_state *mrb, mrb_value self)
{
  int port;
  int req_bit;
  int ack_bit;
  mrb_int timeout;
  mrb_int edges;
  mrb_int loops;
  unsigned char first_value;
  unsigned char last_value;
  unsigned char values[X68K_JOY_CYBER_NIBBLES];
  int count;
  int x;
  int y;
  int throttle;
  int buttons;
  mrb_value result;

  x68k_joy_cyber_args(mrb, &port, &timeout, &ack_bit, &req_bit);
  count = x68k_joy_read_cyber_poll(port, req_bit, ack_bit, timeout, values, &edges, &loops, &first_value, &last_value);
  if (count < X68K_JOY_CYBER_NIBBLES) {
    return mrb_nil_value();
  }

  x = (values[3] & 0xf0) | ((values[7] >> 4) & 0x0f);
  y = (values[2] & 0xf0) | ((values[6] >> 4) & 0x0f);
  throttle = (values[4] & 0xf0) | ((values[8] >> 4) & 0x0f);
  buttons = (values[0] & 0xf0) | ((values[1] >> 4) & 0x0f);
  buttons = (0xff - buttons) & 0xff;

  {
    int raw10 = values[10] & 0xf0;
    int normal = buttons;
    buttons = 0;
    if ((normal & 0x40) != 0) buttons |= 0x0001; /* a */
    if ((normal & 0x80) != 0) buttons |= 0x0002; /* b */
    if ((normal & 0x20) != 0) buttons |= 0x0004; /* c */
    if ((normal & 0x10) != 0) buttons |= 0x0008; /* d */
    if ((normal & 0x08) != 0) buttons |= 0x0010; /* e1 */
    if ((normal & 0x04) != 0) buttons |= 0x0020; /* e2 */
    if ((normal & 0x02) != 0) buttons |= 0x0040; /* start */
    if ((normal & 0x01) != 0) buttons |= 0x0080; /* select */
    if ((normal & 0x40) != 0 && raw10 == 0xb0) buttons |= 0x0100; /* a_plus */
    if ((normal & 0x80) != 0 && raw10 == 0x70) buttons |= 0x0200; /* b_plus */
  }

  result = mrb_ary_new_capa(mrb, 4);
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)x));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)y));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)throttle));
  mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)buttons));

  return result;
}

static int
x68k_joy_default_sel_bit(int port)
{
  return port == 1 ? 5 : 4;
}

static void
x68k_joy_read_sega_phases(int port, int sel_bit, mrb_int wait_count, unsigned char *values, int phases)
{
  volatile unsigned char *joy_port = x68k_joy_port_addr(port);
  int i;
  int old_super = _iocs_b_super(0);

  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = 8;   /* PC4 low */
  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = 10;  /* PC5 low */
  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = 12;  /* PC6 low */
  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = 14;  /* PC7 low */

  for (i = 0; i < phases; i++) {
    *(volatile unsigned char *)X68K_JOY_BSR_ADDR = (unsigned char)(((sel_bit & 7) << 1) | ((i & 1) == 0 ? 1 : 0));
    x68k_joy_delay(wait_count);
    values[i] = *joy_port;
  }
  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = (unsigned char)((sel_bit & 7) << 1);

  if (old_super > 0) {
    _iocs_b_super(old_super);
  }
}

static void
x68k_joy_read_sega_sequence(int port, int sel_bit, mrb_int wait_count, unsigned char values[X68K_JOY_SEGA6B_PHASES])
{
  x68k_joy_read_sega_phases(port, sel_bit, wait_count, values, X68K_JOY_SEGA6B_PHASES);
}

static void
x68k_joy_read_sega_scan_sequence(int port, int sel_bit, mrb_int wait_count, unsigned char values[X68K_JOY_SEGA6B_SCAN_PHASES])
{
  x68k_joy_read_sega_phases(port, sel_bit, wait_count, values, X68K_JOY_SEGA6B_SCAN_PHASES);
}

static void
x68k_joy_read_sega3b_sequence(int port, int sel_bit, mrb_int wait_count, unsigned char values[2])
{
  volatile unsigned char *joy_port = x68k_joy_port_addr(port);
  int old_super = _iocs_b_super(0);

  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = 8;   /* PC4 low */
  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = 10;  /* PC5 low */
  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = 12;  /* PC6 low */
  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = 14;  /* PC7 low */

  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = (unsigned char)(((sel_bit & 7) << 1) | 1);
  x68k_joy_delay(wait_count);
  values[0] = *joy_port;

  *(volatile unsigned char *)X68K_JOY_BSR_ADDR = (unsigned char)((sel_bit & 7) << 1);
  x68k_joy_delay(wait_count);
  values[1] = *joy_port;

  if (old_super > 0) {
    _iocs_b_super(old_super);
  }
}

static int
x68k_joy_bit_down(unsigned char value, int bit)
{
  return (value & (1 << bit)) == 0;
}

static unsigned long
x68k_joy_decode_sega3b(const unsigned char values[2])
{
  unsigned char base_h = values[0];
  unsigned char base_l = values[1];
  unsigned long mask = 0;

  if (x68k_joy_bit_down(base_h, 0)) mask |= X68K_JOY_BTN_UP;
  if (x68k_joy_bit_down(base_h, 1)) mask |= X68K_JOY_BTN_DOWN;
  if (x68k_joy_bit_down(base_h, 2)) mask |= X68K_JOY_BTN_LEFT;
  if (x68k_joy_bit_down(base_h, 3)) mask |= X68K_JOY_BTN_RIGHT;

  /*
   * X68000 adapter mapping confirmed on hardware. 3B mode intentionally
   * does not enter the 6B extension sequence.
   */
  if (x68k_joy_bit_down(base_l, 5)) mask |= X68K_JOY_BTN_A;
  if (x68k_joy_bit_down(base_h, 5)) mask |= X68K_JOY_BTN_B;
  if (x68k_joy_bit_down(base_h, 6)) mask |= X68K_JOY_BTN_C;
  if (x68k_joy_bit_down(base_l, 6)) mask |= X68K_JOY_BTN_START;

  return mask;
}

static unsigned long
x68k_joy_decode_sega6b(const unsigned char *values)
{
  unsigned char base_h = values[0];
  unsigned char base_l = values[1];
  unsigned char marker = values[3];
  unsigned char ext_h = values[4];
  unsigned long mask = 0;

  if (x68k_joy_bit_down(base_h, 0)) mask |= X68K_JOY_BTN_UP;
  if (x68k_joy_bit_down(base_h, 1)) mask |= X68K_JOY_BTN_DOWN;
  if (x68k_joy_bit_down(base_h, 2)) mask |= X68K_JOY_BTN_LEFT;
  if (x68k_joy_bit_down(base_h, 3)) mask |= X68K_JOY_BTN_RIGHT;

  if ((marker & 0x0f) == 0) {
    mask |= X68K_JOY_DETECTED_6B;
  }

  /*
   * X68000 adapter mapping confirmed on hardware:
   * A=base_l bit5, B=base_h bit5, C=base_h bit6,
   * X=ext_h bit2, Y=ext_h bit1, Z=ext_h bit0, Start=base_l bit6.
   */
  if (x68k_joy_bit_down(base_l, 5)) mask |= X68K_JOY_BTN_A;
  if (x68k_joy_bit_down(base_h, 5)) mask |= X68K_JOY_BTN_B;
  if (x68k_joy_bit_down(base_h, 6)) mask |= X68K_JOY_BTN_C;
  if (x68k_joy_bit_down(ext_h, 2)) mask |= X68K_JOY_BTN_X;
  if (x68k_joy_bit_down(ext_h, 1)) mask |= X68K_JOY_BTN_Y;
  if (x68k_joy_bit_down(ext_h, 0)) mask |= X68K_JOY_BTN_Z;
  if (x68k_joy_bit_down(base_l, 6)) mask |= X68K_JOY_BTN_START;
  if (x68k_joy_bit_down(ext_h, 3)) mask |= X68K_JOY_BTN_MODE;

  return mask;
}

static mrb_value
x68k_joy_sega3b_raw(mrb_state *mrb, mrb_value self)
{
  mrb_int port = 0;
  mrb_int wait_count = 80;
  mrb_int sel_bit = -1;
  unsigned char values[2];
  mrb_value result;
  int i;

  mrb_get_args(mrb, "|iii", &port, &wait_count, &sel_bit);
  if (sel_bit < 0) {
    sel_bit = x68k_joy_default_sel_bit((int)port);
  }
  if (sel_bit > 7) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "sel_bit must be 0..7");
  }

  x68k_joy_read_sega3b_sequence((int)port, (int)sel_bit, wait_count, values);

  result = mrb_ary_new_capa(mrb, 2);
  for (i = 0; i < 2; i++) {
    mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)values[i]));
  }

  return result;
}

static mrb_value
x68k_joy_sega3b(mrb_state *mrb, mrb_value self)
{
  mrb_int port = 0;
  mrb_int wait_count = 80;
  unsigned char values[2];
  int sel_bit;

  mrb_get_args(mrb, "|ii", &port, &wait_count);
  sel_bit = x68k_joy_default_sel_bit((int)port);
  x68k_joy_read_sega3b_sequence((int)port, sel_bit, wait_count, values);

  return mrb_fixnum_value((mrb_int)x68k_joy_decode_sega3b(values));
}

static mrb_value
x68k_joy_sega6b_scan_raw(mrb_state *mrb, mrb_value self)
{
  mrb_int port = 0;
  mrb_int wait_count = 80;
  mrb_int sel_bit = -1;
  unsigned char values[X68K_JOY_SEGA6B_SCAN_PHASES];
  mrb_value result;
  int i;

  mrb_get_args(mrb, "|iii", &port, &wait_count, &sel_bit);
  if (sel_bit < 0) {
    sel_bit = x68k_joy_default_sel_bit((int)port);
  }
  if (sel_bit > 7) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "sel_bit must be 0..7");
  }

  x68k_joy_read_sega_scan_sequence((int)port, (int)sel_bit, wait_count, values);

  result = mrb_ary_new_capa(mrb, X68K_JOY_SEGA6B_SCAN_PHASES);
  for (i = 0; i < X68K_JOY_SEGA6B_SCAN_PHASES; i++) {
    mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)values[i]));
  }

  return result;
}

static mrb_value
x68k_joy_sega6b_raw(mrb_state *mrb, mrb_value self)
{
  mrb_int port = 0;
  mrb_int wait_count = 80;
  mrb_int sel_bit = -1;
  unsigned char values[X68K_JOY_SEGA6B_PHASES];
  mrb_value result;
  int i;

  mrb_get_args(mrb, "|iii", &port, &wait_count, &sel_bit);
  if (sel_bit < 0) {
    sel_bit = x68k_joy_default_sel_bit((int)port);
  }
  if (sel_bit > 7) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "sel_bit must be 0..7");
  }

  x68k_joy_read_sega_sequence((int)port, (int)sel_bit, wait_count, values);

  result = mrb_ary_new_capa(mrb, X68K_JOY_SEGA6B_PHASES);
  for (i = 0; i < X68K_JOY_SEGA6B_PHASES; i++) {
    mrb_ary_push(mrb, result, mrb_fixnum_value((mrb_int)values[i]));
  }

  return result;
}

static mrb_value
x68k_joy_sega6b(mrb_state *mrb, mrb_value self)
{
  mrb_int port = 0;
  mrb_int wait_count = 80;
  unsigned char values[X68K_JOY_SEGA6B_PHASES];
  int sel_bit;

  mrb_get_args(mrb, "|ii", &port, &wait_count);
  sel_bit = x68k_joy_default_sel_bit((int)port);
  x68k_joy_read_sega_sequence((int)port, sel_bit, wait_count, values);

  return mrb_fixnum_value((mrb_int)x68k_joy_decode_sega6b(values));
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
  struct RClass *dir;
  struct RClass *x68k_file;
  struct RClass *x68k;
  struct RClass *graph;
  struct RClass *screen;
  struct RClass *text;
  struct RClass *key;
  struct RClass *crtc;
  struct RClass *joy;
  struct RClass *ajoy;
  struct RClass *bg;
  struct RClass *sys;
  struct RClass *sprite;
  struct RClass *zmusic;
  struct RClass *iocs;
  struct RClass *interrupt;

  mrb_define_method(mrb, mrb->kernel_module, "puts", x68k_kernel_puts, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->kernel_module, "printf", x68k_kernel_printf, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->kernel_module, "open", x68k_kernel_open, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
  mrb_define_method(mrb, mrb->kernel_module, "`", x68k_kernel_backquote, MRB_ARGS_REQ(1));

  file = mrb_define_class(mrb, "File", mrb->object_class);
  mrb_define_class_method(mrb, file, "exist?", x68k_file_exist, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "exists?", x68k_file_exist, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "directory?", x68k_file_directory, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "file?", x68k_file_file, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "open", x68k_file_open, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, file, "read", x68k_file_read, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "size", x68k_file_size, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "write", x68k_file_write, MRB_ARGS_REQ(2));

  dir = mrb_define_class(mrb, "Dir", mrb->object_class);
  mrb_define_class_method(mrb, dir, "entries", x68k_dir_entries, MRB_ARGS_OPT(1));

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
  mrb_define_class_method(mrb, screen, "apage", x68k_screen_apage, MRB_ARGS_REQ(1));
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

  crtc = mrb_define_module_under(mrb, x68k, "Crtc");
  mrb_define_class_method(mrb, crtc, "vdisp?", x68k_crtc_vdisp, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, crtc, "vblank?", x68k_crtc_vblank, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, crtc, "wait_vblank", x68k_crtc_wait_vblank, MRB_ARGS_NONE());

  joy = mrb_define_module_under(mrb, x68k, "Joy");
  mrb_define_class_method(mrb, joy, "get", x68k_joy_get, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, joy, "raw", x68k_joy_get, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, joy, "direct_raw", x68k_joy_direct_raw, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, joy, "control", x68k_joy_control, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, joy, "control_bit", x68k_joy_control_bit, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, joy, "sega3b_raw", x68k_joy_sega3b_raw, MRB_ARGS_ARG(0, 3));
  mrb_define_class_method(mrb, joy, "sega3b", x68k_joy_sega3b, MRB_ARGS_ARG(0, 2));
  mrb_define_class_method(mrb, joy, "sega6b_raw", x68k_joy_sega6b_raw, MRB_ARGS_ARG(0, 3));
  mrb_define_class_method(mrb, joy, "sega6b", x68k_joy_sega6b, MRB_ARGS_ARG(0, 2));
  mrb_define_class_method(mrb, joy, "sega6b_scan_raw", x68k_joy_sega6b_scan_raw, MRB_ARGS_ARG(0, 3));
  mrb_define_class_method(mrb, joy, "cyber_raw", x68k_joy_cyber_raw, MRB_ARGS_ARG(0, 4));
  mrb_define_class_method(mrb, joy, "cyber_scan_raw", x68k_joy_cyber_scan_raw, MRB_ARGS_ARG(0, 4));
  mrb_define_class_method(mrb, joy, "cyber", x68k_joy_cyber, MRB_ARGS_ARG(0, 4));

  ajoy = mrb_define_module_under(mrb, x68k, "Ajoy");
  mrb_define_class_method(mrb, ajoy, "available?", x68k_ajoy_available, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "read_raw", x68k_ajoy_read_raw_value, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "read", x68k_ajoy_read, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "throttle_reverse?", x68k_ajoy_throttle_reverse_get, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "throttle_reverse=", x68k_ajoy_throttle_reverse_set, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, ajoy, "trigger_mask", x68k_ajoy_trigger_mask, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "buttons", x68k_ajoy_buttons, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "button_map", x68k_ajoy_button_map_get, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "button_map=", x68k_ajoy_button_map_set, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, ajoy, "button_patterns", x68k_ajoy_button_patterns_get, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "button_patterns=", x68k_ajoy_button_patterns_set, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, ajoy, "button_preset", x68k_ajoy_button_preset_get, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, ajoy, "button_preset=", x68k_ajoy_button_preset_set, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, ajoy, "mode", x68k_ajoy_mode, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, ajoy, "mode=", x68k_ajoy_mode_set, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, ajoy, "speed", x68k_ajoy_speed, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, ajoy, "speed=", x68k_ajoy_speed_set, MRB_ARGS_REQ(1));

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

  zmusic = mrb_define_module_under(mrb, x68k, "ZMusic");
  mrb_define_class_method(mrb, zmusic, "available?", x68k_zmusic_available, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, zmusic, "version", x68k_zmusic_version, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, zmusic, "play_bgm", x68k_zmusic_play_bgm, MRB_ARGS_ARG(1, 2));
  mrb_define_class_method(mrb, zmusic, "stop", x68k_zmusic_stop, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, zmusic, "fadeout", x68k_zmusic_fadeout, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, zmusic, "status", x68k_zmusic_status, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, zmusic, "pcm8?", x68k_zmusic_pcm8, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, zmusic, "play_se", x68k_zmusic_play_se, MRB_ARGS_ARG(2, 2));
  mrb_define_class_method(mrb, zmusic, "play_adpcm", x68k_zmusic_play_adpcm, MRB_ARGS_ARG(1, 4));
  mrb_define_class_method(mrb, zmusic, "play_adpcm_note", x68k_zmusic_play_adpcm_note, MRB_ARGS_ARG(1, 3));
  sys = mrb_define_module_under(mrb, x68k, "Sys");
  mrb_define_class_method(mrb, sys, "wait", x68k_sys_wait, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, sys, "super?", x68k_sys_super_p, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, sys, "super", x68k_sys_super, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, sys, "with_super", x68k_sys_with_super, MRB_ARGS_BLOCK());
  mrb_define_class_method(mrb, sys, "interrupt_disable", x68k_sys_interrupt_disable, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, sys, "interrupt_enable", x68k_sys_interrupt_enable, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, sys, "with_interrupt_disabled", x68k_sys_with_interrupt_disabled, MRB_ARGS_BLOCK());

  iocs = mrb_define_module_under(mrb, x68k, "Iocs");
  mrb_define_class_method(mrb, iocs, "call", x68k_iocs_call, MRB_ARGS_ANY());

  interrupt = mrb_define_module_under(mrb, x68k, "Interrupt");
  mrb_define_class_method(mrb, interrupt, "vsync_enable", x68k_interrupt_vsync_enable, MRB_ARGS_ARG(0, 2));
  mrb_define_class_method(mrb, interrupt, "vsync_disable", x68k_interrupt_vsync_disable, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "vsync_count", x68k_interrupt_vsync_count, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "vsync_clear", x68k_interrupt_vsync_clear, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "vsync?", x68k_interrupt_vsync_p, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "raster_enable", x68k_interrupt_raster_enable, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, interrupt, "raster_disable", x68k_interrupt_raster_disable, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "raster_count", x68k_interrupt_raster_count, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "raster_clear", x68k_interrupt_raster_clear, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "raster?", x68k_interrupt_raster_p, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "timer_d_enable", x68k_interrupt_timer_d_enable, MRB_ARGS_ARG(0, 2));
  mrb_define_class_method(mrb, interrupt, "timer_d_disable", x68k_interrupt_timer_d_disable, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "timer_d_count", x68k_interrupt_timer_d_count, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "timer_d_clear", x68k_interrupt_timer_d_clear, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "timer_d?", x68k_interrupt_timer_d_p, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "opm_enable", x68k_interrupt_opm_enable, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "opm_disable", x68k_interrupt_opm_disable, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "opm_count", x68k_interrupt_opm_count, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "opm_clear", x68k_interrupt_opm_clear, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "opm?", x68k_interrupt_opm_p, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, interrupt, "disable_all", x68k_interrupt_disable_all, MRB_ARGS_NONE());

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
