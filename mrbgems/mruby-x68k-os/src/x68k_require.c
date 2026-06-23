/*
** x68k_require.c - load/require support for mruby-x68k
**
** This implementation is adapted from mattn/mruby-require:
**   https://github.com/mattn/mruby-require
**
** mruby-require is licensed under the MIT License.
** The dynamic shared-object loading path from the original implementation is
** intentionally omitted because Human68k does not provide POSIX dlopen-style
** shared libraries.
*/

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/dump.h>
#include <mruby/error.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/variable.h>

#ifdef MRB_X68K_ENABLE_RB_REQUIRE
#include <mruby/compile.h>
#endif

#include <stdio.h>
#include <string.h>

extern char **environ;

#define X68K_REQUIRE_MAXPATH 256

static int
x68k_req_casecmp(const char *a, const char *b, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++) {
    unsigned char ca = (unsigned char)a[i];
    unsigned char cb = (unsigned char)b[i];

    if (ca >= 'a' && ca <= 'z') ca = (unsigned char)(ca - 'a' + 'A');
    if (cb >= 'a' && cb <= 'z') cb = (unsigned char)(cb - 'a' + 'A');
    if (ca != cb) return (int)ca - (int)cb;
  }

  return 0;
}

static const char*
x68k_req_getenv_ci(const char *name)
{
  char **env;
  size_t len = strlen(name);

  if (environ == NULL) return NULL;
  for (env = environ; *env != NULL; env++) {
    char *eq = strchr(*env, '=');
    if (eq == NULL) continue;
    if ((size_t)(eq - *env) == len && x68k_req_casecmp(*env, name, len) == 0) {
      return eq + 1;
    }
  }

  return NULL;
}

static int
x68k_req_is_sep(char c)
{
  return c == '/' || c == '\\';
}

static int
x68k_req_has_drive(const char *path)
{
  return path[0] != '\0' && path[1] == ':';
}

static int
x68k_req_is_absolute_or_explicit(const char *path)
{
  if (x68k_req_has_drive(path)) return TRUE;
  if (x68k_req_is_sep(path[0])) return TRUE;
  if (path[0] == '.') return TRUE;
  return FALSE;
}

static int
x68k_req_has_path_sep(const char *path)
{
  const char *p;

  for (p = path; *p != '\0'; p++) {
    if (x68k_req_is_sep(*p) || *p == ':') return TRUE;
  }

  return FALSE;
}

static int
x68k_req_has_ext(const char *path)
{
  const char *p;
  const char *base = path;

  for (p = path; *p != '\0'; p++) {
    if (x68k_req_is_sep(*p) || *p == ':') base = p + 1;
  }
  for (p = base; *p != '\0'; p++) {
    if (*p == '.') return TRUE;
  }

  return FALSE;
}

static mrb_value
x68k_req_load_error_class(mrb_state *mrb)
{
  return mrb_obj_value(mrb_class_get(mrb, "LoadError"));
}

static void
x68k_req_load_fail(mrb_state *mrb, mrb_value path, const char *err)
{
  mrb_value mesg;
  mrb_value exc;

  mesg = mrb_str_new_cstr(mrb, err);
  mrb_str_cat_lit(mrb, mesg, " -- ");
  mrb_str_cat_str(mrb, mesg, path);
  exc = mrb_funcall(mrb, x68k_req_load_error_class(mrb), "new", 1, mesg);
  mrb_iv_set(mrb, exc, mrb_intern_lit(mrb, "path"), path);
  mrb_exc_raise(mrb, exc);
}

static mrb_value
x68k_req_loaded_features(mrb_state *mrb, mrb_bool replace_new)
{
  mrb_value ary = mrb_gv_get(mrb, mrb_intern_lit(mrb, "$\""));
  ary = mrb_check_array_type(mrb, ary);

  if (mrb_nil_p(ary) && replace_new) {
    ary = mrb_ary_new(mrb);
    mrb_gv_set(mrb, mrb_intern_lit(mrb, "$\""), ary);
  }

  return ary;
}

static mrb_value
x68k_req_expand_path(mrb_state *mrb, mrb_value path)
{
  struct RClass *file = mrb_class_get(mrb, "File");
  return mrb_funcall(mrb, mrb_obj_value(file), "expand_path", 1, path);
}

static mrb_value
x68k_req_path_join(mrb_state *mrb, mrb_value dir, mrb_value name, const char *ext)
{
  const char *d = RSTRING_PTR(dir);
  mrb_int dlen = RSTRING_LEN(dir);
  mrb_value path = mrb_str_dup(mrb, dir);

  if (dlen > 0 && !(dlen == 1 && d[0] == '.')) {
    char last = d[dlen - 1];
    if (!x68k_req_is_sep(last) && last != ':') {
      mrb_str_cat_lit(mrb, path, "/");
    }
  } else if (dlen == 1 && d[0] == '.') {
    path = mrb_str_new_cstr(mrb, "");
  }

  mrb_str_cat_str(mrb, path, name);
  if (ext != NULL) {
    mrb_str_cat_cstr(mrb, path, ext);
  }

  return path;
}

static mrb_bool
x68k_req_file_exists(mrb_state *mrb, mrb_value path)
{
  FILE *fp = fopen(mrb_string_value_cstr(mrb, &path), "rb");
  if (fp == NULL) return FALSE;
  fclose(fp);
  return TRUE;
}

static mrb_value
x68k_req_find_file_check(mrb_state *mrb, mrb_value dir, mrb_value filename, const char *ext)
{
  mrb_value path;

  if (x68k_req_is_absolute_or_explicit(RSTRING_PTR(filename)) || x68k_req_has_path_sep(RSTRING_PTR(filename))) {
    path = mrb_str_dup(mrb, filename);
    if (ext != NULL) mrb_str_cat_cstr(mrb, path, ext);
  } else {
    path = x68k_req_path_join(mrb, dir, filename, ext);
  }

  if (!x68k_req_file_exists(mrb, path)) {
    return mrb_nil_value();
  }

  return x68k_req_expand_path(mrb, path);
}

static mrb_value
x68k_req_find_file(mrb_state *mrb, mrb_value filename, mrb_bool completion)
{
  mrb_value load_path;
  mrb_value filepath;
  const char *exts[3];
  int ext_count;
  int i;
  int j;

  if (completion && !x68k_req_has_ext(RSTRING_PTR(filename))) {
#ifdef MRB_X68K_ENABLE_RB_REQUIRE
    exts[0] = ".rb";
    exts[1] = ".mrb";
#else
    exts[0] = ".mrb";
#endif
#ifdef MRB_X68K_ENABLE_RB_REQUIRE
    ext_count = 2;
#else
    ext_count = 1;
#endif
  } else {
    exts[0] = NULL;
    ext_count = 1;
  }

  if (x68k_req_is_absolute_or_explicit(RSTRING_PTR(filename)) || x68k_req_has_path_sep(RSTRING_PTR(filename))) {
    load_path = mrb_ary_new_capa(mrb, 1);
    mrb_ary_push(mrb, load_path, mrb_str_new_lit(mrb, "."));
  } else {
    load_path = mrb_obj_dup(mrb, mrb_gv_get(mrb, mrb_intern_lit(mrb, "$:")));
    load_path = mrb_check_array_type(mrb, load_path);
    if (mrb_nil_p(load_path)) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "invalid $:");
    }
  }

  for (i = 0; i < RARRAY_LEN(load_path); i++) {
    mrb_value dir = mrb_ary_entry(load_path, i);
    if (!mrb_string_p(dir)) continue;

    for (j = 0; j < ext_count; j++) {
      filepath = x68k_req_find_file_check(mrb, dir, filename, exts[j]);
      if (!mrb_nil_p(filepath)) {
        return filepath;
      }
    }
  }

  x68k_req_load_fail(mrb, filename, "cannot load such file");
  return mrb_nil_value();
}

#ifdef MRB_X68K_ENABLE_RB_REQUIRE
static void
x68k_req_load_rb_file(mrb_state *mrb, mrb_value filepath)
{
  FILE *fp;
  const char *fpath = mrb_string_value_cstr(mrb, &filepath);
  mrb_ccontext *cxt;
  int ai;

  fp = fopen(fpath, "r");
  if (fp == NULL) {
    x68k_req_load_fail(mrb, filepath, "cannot load such file");
    return;
  }

  ai = mrb_gc_arena_save(mrb);
  cxt = mrbc_context_new(mrb);
  mrbc_filename(mrb, cxt, fpath);
  mrb_load_file_cxt(mrb, fp, cxt);
  fclose(fp);
  mrbc_context_free(mrb, cxt);
  mrb_gc_arena_restore(mrb, ai);

  if (mrb->exc) {
    mrb_exc_raise(mrb, mrb_obj_value(mrb->exc));
  }
}
#endif

static void
x68k_req_load_mrb_file(mrb_state *mrb, mrb_value filepath)
{
  FILE *fp;
  const char *fpath = mrb_string_value_cstr(mrb, &filepath);
  mrb_irep *irep;
  int ai;

  fp = fopen(fpath, "rb");
  if (fp == NULL) {
    x68k_req_load_fail(mrb, filepath, "cannot load such file");
    return;
  }

  ai = mrb_gc_arena_save(mrb);
  irep = mrb_read_irep_file(mrb, fp);
  fclose(fp);
  mrb_gc_arena_restore(mrb, ai);

  if (irep) {
    struct RProc *proc;

    proc = mrb_proc_new(mrb, irep);
    MRB_PROC_SET_TARGET_CLASS(proc, mrb->object_class);

    ai = mrb_gc_arena_save(mrb);
    mrb_yield_with_class(mrb, mrb_obj_value(proc), 0, NULL, mrb_top_self(mrb), mrb->object_class);
    mrb_gc_arena_restore(mrb, ai);
  } else if (mrb->exc) {
    mrb_exc_raise(mrb, mrb_obj_value(mrb->exc));
  }
}

static void
x68k_req_load_file(mrb_state *mrb, mrb_value filepath)
{
  const char *path = RSTRING_PTR(filepath);
  const char *ext = strrchr(path, '.');

  if (ext != NULL && strcmp(ext, ".mrb") == 0) {
    x68k_req_load_mrb_file(mrb, filepath);
    return;
  }

#ifdef MRB_X68K_ENABLE_RB_REQUIRE
  x68k_req_load_rb_file(mrb, filepath);
#else
  x68k_req_load_fail(mrb, filepath, "Ruby source loading is not available in this build");
#endif
}

static mrb_value
x68k_req_load(mrb_state *mrb, mrb_value filename)
{
  mrb_value filepath = x68k_req_find_file(mrb, filename, FALSE);
  x68k_req_load_file(mrb, filepath);
  return mrb_true_value();
}

static mrb_value
x68k_req_kernel_load(mrb_state *mrb, mrb_value self)
{
  mrb_value filename;

  mrb_get_args(mrb, "S", &filename);
  return x68k_req_load(mrb, filename);
}

static mrb_bool
x68k_req_loaded_check(mrb_state *mrb, mrb_value filepath)
{
  mrb_value loading_files;
  mrb_value loaded_files = x68k_req_loaded_features(mrb, TRUE);
  int i;

  for (i = 0; i < RARRAY_LEN(loaded_files); i++) {
    mrb_value item = mrb_ary_entry(loaded_files, i);
    if (mrb_string_p(item) && mrb_str_cmp(mrb, item, filepath) == 0) {
      return FALSE;
    }
  }

  loading_files = mrb_gv_get(mrb, mrb_intern_lit(mrb, "$\"_"));
  if (mrb_nil_p(loading_files)) return TRUE;

  for (i = 0; i < RARRAY_LEN(loading_files); i++) {
    mrb_value item = mrb_ary_entry(loading_files, i);
    if (mrb_string_p(item) && mrb_str_cmp(mrb, item, filepath) == 0) {
      return FALSE;
    }
  }

  return TRUE;
}

static void
x68k_req_loading_add(mrb_state *mrb, mrb_value filepath)
{
  mrb_value loading_files = mrb_gv_get(mrb, mrb_intern_lit(mrb, "$\"_"));
  if (mrb_nil_p(loading_files)) {
    loading_files = mrb_ary_new(mrb);
    mrb_gv_set(mrb, mrb_intern_lit(mrb, "$\"_"), loading_files);
  }
  mrb_ary_push(mrb, loading_files, filepath);
}

static void
x68k_req_loading_delete(mrb_state *mrb, mrb_value filepath)
{
  mrb_value loading_files = mrb_gv_get(mrb, mrb_intern_lit(mrb, "$\"_"));
  if (mrb_array_p(loading_files)) {
    mrb_funcall(mrb, loading_files, "delete", 1, filepath);
  }
}

static mrb_value
x68k_req_require(mrb_state *mrb, mrb_value filename)
{
  mrb_value filepath = x68k_req_find_file(mrb, filename, TRUE);

  if (!mrb_nil_p(filepath) && x68k_req_loaded_check(mrb, filepath)) {
    x68k_req_loading_add(mrb, filepath);
    x68k_req_load_file(mrb, filepath);
    x68k_req_loading_delete(mrb, filepath);
    if (mrb->exc) {
      mrb_exc_raise(mrb, mrb_obj_value(mrb->exc));
    }
    mrb_ary_push(mrb, x68k_req_loaded_features(mrb, TRUE), filepath);
    return mrb_true_value();
  }

  return mrb_false_value();
}

static mrb_value
x68k_req_kernel_require(mrb_state *mrb, mrb_value self)
{
  mrb_value filename;

  mrb_get_args(mrb, "S", &filename);
  return x68k_req_require(mrb, filename);
}

static mrb_value
x68k_req_load_error_path(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "path"));
}

static void
x68k_req_push_path(mrb_state *mrb, mrb_value ary, const char *ptr, size_t len)
{
  if (len == 0) return;
  mrb_ary_push(mrb, ary, mrb_str_new(mrb, ptr, (mrb_int)len));
}

static void
x68k_req_push_env_paths(mrb_state *mrb, mrb_value ary, const char *env)
{
  const char *p;
  const char *start;

  if (env == NULL || env[0] == '\0') return;

  start = env;
  for (p = env; ; p++) {
    if (*p == '\0' || *p == ';' || *p == ',') {
      x68k_req_push_path(mrb, ary, start, (size_t)(p - start));
      if (*p == '\0') break;
      start = p + 1;
    }
  }
}

static mrb_value
x68k_req_init_load_path(mrb_state *mrb)
{
  mrb_value ary = mrb_ary_new(mrb);

  mrb_ary_push(mrb, ary, mrb_str_new_lit(mrb, "."));
  x68k_req_push_env_paths(mrb, ary, x68k_req_getenv_ci("MRBLIB"));
  x68k_req_push_env_paths(mrb, ary, x68k_req_getenv_ci("RUBYLIB"));

  return ary;
}

void
x68k_require_init(mrb_state *mrb)
{
  struct RClass *load_error;

  mrb_define_method(mrb, mrb->kernel_module, "load", x68k_req_kernel_load, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, mrb->kernel_module, "require", x68k_req_kernel_require, MRB_ARGS_REQ(1));

  load_error = mrb_define_class(mrb, "LoadError", E_SCRIPT_ERROR);
  mrb_define_method(mrb, load_error, "path", x68k_req_load_error_path, MRB_ARGS_NONE());

  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$:"), x68k_req_init_load_path(mrb));
  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$LOAD_PATH"), mrb_gv_get(mrb, mrb_intern_lit(mrb, "$:")));
  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$\""), mrb_ary_new(mrb));
  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$LOADED_FEATURES"), mrb_gv_get(mrb, mrb_intern_lit(mrb, "$\"")));
}
