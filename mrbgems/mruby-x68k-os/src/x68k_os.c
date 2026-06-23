#include <mruby.h>
#include <mruby/class.h>
#include <mruby/array.h>
#include <mruby/data.h>
#include <mruby/error.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#ifndef nullptr
#define nullptr NULL
#endif

extern char *getcwd(char *buf, size_t size);
extern int chdir(const char *path);
extern int rmdir(const char *path);
extern char **environ;

typedef struct {
  FILE *fp;
} x68k_file;

void x68k_require_init(mrb_state *mrb);

#include "x68k_os.inc"

void
mrb_mruby_x68k_os_gem_init(mrb_state *mrb)
{
  struct RClass *file;
  struct RClass *dir;
  struct RClass *x68k_file;

  mrb_define_method(mrb, mrb->kernel_module, "puts", x68k_kernel_puts, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->kernel_module, "printf", x68k_kernel_printf, MRB_ARGS_ANY());
  mrb_define_method(mrb, mrb->kernel_module, "open", x68k_kernel_open, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
  mrb_define_method(mrb, mrb->kernel_module, "`", x68k_kernel_backquote, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, mrb->kernel_module, "system", x68k_kernel_system, MRB_ARGS_REQ(1));

  file = mrb_define_class(mrb, "File", mrb->object_class);
  mrb_define_class_method(mrb, file, "exist?", x68k_file_exist, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "exists?", x68k_file_exist, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "directory?", x68k_file_directory, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "file?", x68k_file_file, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "basename", x68k_file_basename, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "dirname", x68k_file_dirname, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "extname", x68k_file_extname, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "expand_path", x68k_file_expand_path, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "absolute_path", x68k_file_expand_path, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "open", x68k_file_open, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, file, "read", x68k_file_read, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "readlines", x68k_file_readlines, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "size", x68k_file_size, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "write", x68k_file_write, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, file, "delete", x68k_file_delete, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "unlink", x68k_file_delete, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, file, "rename", x68k_file_rename, MRB_ARGS_REQ(2));
  mrb_define_class_method(mrb, file, "copy", x68k_file_copy, MRB_ARGS_REQ(2));

  dir = mrb_define_class(mrb, "Dir", mrb->object_class);
  mrb_define_class_method(mrb, dir, "entries", x68k_dir_entries, MRB_ARGS_OPT(1));
  mrb_define_class_method(mrb, dir, "pwd", x68k_dir_pwd, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, dir, "getwd", x68k_dir_pwd, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, dir, "chdir", x68k_dir_chdir, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, dir, "mkdir", x68k_dir_mkdir, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, dir, "exist?", x68k_file_directory, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, dir, "exists?", x68k_file_directory, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, dir, "delete", x68k_dir_delete, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, dir, "rmdir", x68k_dir_delete, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, dir, "rename", x68k_file_rename, MRB_ARGS_REQ(2));

  {
    struct RClass *env = mrb_define_module(mrb, "ENV");
    mrb_define_class_method(mrb, env, "[]", x68k_env_get, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, env, "[]=", x68k_env_set, MRB_ARGS_REQ(2));
    mrb_define_class_method(mrb, env, "delete", x68k_env_delete, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, env, "keys", x68k_env_keys, MRB_ARGS_NONE());
  }

  {
    struct RClass *fileutils = mrb_define_module(mrb, "FileUtils");
    mrb_define_class_method(mrb, fileutils, "cp", x68k_file_copy, MRB_ARGS_REQ(2));
    mrb_define_class_method(mrb, fileutils, "copy", x68k_file_copy, MRB_ARGS_REQ(2));
    mrb_define_class_method(mrb, fileutils, "mv", x68k_file_rename, MRB_ARGS_REQ(2));
    mrb_define_class_method(mrb, fileutils, "move", x68k_file_rename, MRB_ARGS_REQ(2));
    mrb_define_class_method(mrb, fileutils, "mkdir", x68k_dir_mkdir, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, fileutils, "rm", x68k_file_delete, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, fileutils, "remove", x68k_file_delete, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, fileutils, "rmdir", x68k_dir_delete, MRB_ARGS_REQ(1));
  }

  x68k_set_last_status(mrb, 0);
  x68k_require_init(mrb);

  x68k_file = mrb_define_class(mrb, "X68kFile", mrb->object_class);
  MRB_SET_INSTANCE_TT(x68k_file, MRB_TT_DATA);
  mrb_define_method(mrb, x68k_file, "gets", x68k_file_obj_gets, MRB_ARGS_NONE());
  mrb_define_method(mrb, x68k_file, "write", x68k_file_obj_write, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, x68k_file, "print", x68k_file_obj_print, MRB_ARGS_ANY());
  mrb_define_method(mrb, x68k_file, "close", x68k_file_obj_close, MRB_ARGS_NONE());
}

void
mrb_mruby_x68k_os_gem_final(mrb_state *mrb)
{
}
