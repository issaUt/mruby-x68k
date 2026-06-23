/* Included by x68k_hardware.c. */
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

