/* Included by x68k_hardware.c. */
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


