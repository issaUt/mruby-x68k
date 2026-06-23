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
#include <sys/stat.h>

#ifndef nullptr
#define nullptr NULL
#endif
#include "libzm2.h"
#include "libzm2util.h"

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

#include "x68k_zmusic.inc"
#include "x68k_graph_text.inc"
#include "x68k_system.inc"
#include "x68k_input.inc"
#include "x68k_interrupt.inc"
#include "x68k_bg_sprite.inc"

void
mrb_mruby_x68k_hardware_gem_init(mrb_state *mrb)
{
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
mrb_mruby_x68k_hardware_gem_final(mrb_state *mrb)
{
  x68k_zmusic_free_buffer(mrb);
  x68k_zmusic_free_se_buffers(mrb);
  x68k_zmusic_free_adpcm_buffers(mrb);
  x68k_interrupt_disable_all(mrb, mrb_nil_value());
}
