/* Included by x68k_hardware.c. */
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


