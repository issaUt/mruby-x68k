/* Included by x68k_hardware.c. */
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

