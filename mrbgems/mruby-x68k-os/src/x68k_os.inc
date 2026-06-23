/* Included by x68k_os.c. */
static void
x68k_set_last_status(mrb_state *mrb, int status)
{
  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$?"), mrb_fixnum_value((mrb_int)status));
}

static void
x68k_file_free(mrb_state *mrb, void *ptr)
{
  x68k_file *file = (x68k_file*)ptr;

  if (file != NULL) {
    if (file->fp != NULL) {
      fclose(file->fp);
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

static char *x68k_env_find_entry_ci(const char *name);

static const char *
x68k_env_get_ci_cstr(const char *name)
{
  char *entry;
  char *eq;

  entry = x68k_env_find_entry_ci(name);
  if (entry == NULL) {
    return NULL;
  }

  eq = strchr(entry, '=');
  if (eq == NULL) {
    return NULL;
  }

  return eq + 1;
}

static void
x68k_make_backquote_tmpname(mrb_state *mrb, char *buf, size_t bufsiz)
{
  const char *dir;
  const char *sep;
  size_t len;
  char name[16];
  int n;

  n = snprintf(name, sizeof(name), "MRBBQ%03u.TMP", x68k_backquote_counter++ % 1000);
  if (n < 0 || n >= (int)sizeof(name)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "temporary file name too long");
  }

  dir = x68k_env_get_ci_cstr("TMP");
  if (dir == NULL || dir[0] == '\0') {
    dir = x68k_env_get_ci_cstr("TEMP");
  }

  if (dir == NULL || dir[0] == '\0') {
    n = snprintf(buf, bufsiz, "%s", name);
  } else {
    len = strlen(dir);
    sep = (dir[len - 1] == '/' || dir[len - 1] == '\\' || dir[len - 1] == ':') ? "" : "/";
    n = snprintf(buf, bufsiz, "%s%s%s", dir, sep, name);
  }

  if (n < 0 || n >= (int)bufsiz) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "temporary file path too long");
  }
}

static mrb_value
x68k_kernel_system(mrb_state *mrb, mrb_value self)
{
  char *cmd;
  int status;

  mrb_get_args(mrb, "z", &cmd);
  mrb_full_gc(mrb);
  status = system(cmd);
  x68k_set_last_status(mrb, status);

  return status == 0 ? mrb_true_value() : mrb_false_value();
}

static mrb_value
x68k_kernel_backquote(mrb_state *mrb, mrb_value self)
{
  char *cmd;
  char tmpname[128];
  char syscmd[256];
  FILE *fp;
  mrb_value result;
  char buffer[256];
  size_t nread;
  int n;

  mrb_get_args(mrb, "z", &cmd);
  mrb_full_gc(mrb);

  x68k_make_backquote_tmpname(mrb, tmpname, sizeof(tmpname));

  n = snprintf(syscmd, sizeof(syscmd), "%s > %s", cmd, tmpname);
  if (n < 0 || n >= (int)sizeof(syscmd)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "command too long");
  }

  remove(tmpname);
  x68k_set_last_status(mrb, system(syscmd));

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
  struct stat st;

  mrb_get_args(mrb, "z", &path);
  return stat(path, &st) == 0 ? mrb_true_value() : mrb_false_value();
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

static int
x68k_path_is_separator(char c)
{
  return c == '/' || c == '\\' || c == ':';
}

static const char*
x68k_path_last_separator(const char *path)
{
  const char *last = NULL;
  const char *p;

  for (p = path; *p != '\0'; p++) {
    if (x68k_path_is_separator(*p)) {
      last = p;
    }
  }

  return last;
}

static mrb_value
x68k_file_basename(mrb_state *mrb, mrb_value self)
{
  char *path;
  const char *sep;
  const char *base;
  size_t len;

  mrb_get_args(mrb, "z", &path);
  len = strlen(path);
  while (len > 0 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
    len--;
  }
  if (len == 0) {
    return mrb_str_new_lit(mrb, "");
  }

  sep = NULL;
  for (base = path; base < path + len; base++) {
    if (x68k_path_is_separator(*base)) {
      sep = base;
    }
  }

  base = sep == NULL ? path : sep + 1;
  return mrb_str_new(mrb, base, (mrb_int)(path + len - base));
}

static mrb_value
x68k_file_dirname(mrb_state *mrb, mrb_value self)
{
  char *path;
  const char *sep;
  size_t len;

  mrb_get_args(mrb, "z", &path);
  len = strlen(path);
  while (len > 0 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
    len--;
  }
  if (len == 0) {
    return mrb_str_new_lit(mrb, ".");
  }

  sep = NULL;
  {
    const char *p;
    for (p = path; p < path + len; p++) {
      if (x68k_path_is_separator(*p)) {
        sep = p;
      }
    }
  }

  if (sep == NULL) {
    return mrb_str_new_lit(mrb, ".");
  }
  if (*sep == ':' && sep == path + 1) {
    return mrb_str_new(mrb, path, 2);
  }
  if (sep == path) {
    return mrb_str_new(mrb, path, 1);
  }

  return mrb_str_new(mrb, path, (mrb_int)(sep - path));
}

static mrb_value
x68k_file_extname(mrb_state *mrb, mrb_value self)
{
  char *path;
  const char *sep;
  const char *base;
  const char *dot;
  const char *p;
  size_t len;

  mrb_get_args(mrb, "z", &path);
  len = strlen(path);
  while (len > 0 && (path[len - 1] == '/' || path[len - 1] == '\\')) {
    len--;
  }
  if (len == 0) {
    return mrb_str_new_lit(mrb, "");
  }

  sep = x68k_path_last_separator(path);
  base = sep == NULL ? path : sep + 1;
  dot = NULL;
  for (p = base; p < path + len; p++) {
    if (*p == '.') {
      dot = p;
    }
  }

  if (dot == NULL || dot == base || dot == path + len - 1) {
    return mrb_str_new_lit(mrb, "");
  }

  return mrb_str_new(mrb, dot, (mrb_int)(path + len - dot));
}

static int
x68k_path_has_drive(const char *path)
{
  return path[0] != '\0' && path[1] == ':';
}

static int
x68k_path_is_rooted(const char *path)
{
  return path[0] == '/' || path[0] == '\\';
}

static void
x68k_path_append_part(mrb_state *mrb, mrb_value parts, const char *part, size_t len)
{
  mrb_int i;
  mrb_int n;

  if (len == 0) {
    return;
  }
  if (len == 1 && part[0] == '.') {
    return;
  }
  if (len == 2 && part[0] == '.' && part[1] == '.') {
    n = RARRAY_LEN(parts);
    if (n > 0) {
      mrb_ary_pop(mrb, parts);
    }
    return;
  }

  i = RARRAY_LEN(parts);
  mrb_ary_set(mrb, parts, i, mrb_str_new(mrb, part, (mrb_int)len));
}

static mrb_value
x68k_file_expand_path(mrb_state *mrb, mrb_value self)
{
  char *path;
  char cwd[256];
  const char *body;
  char drive[3];
  mrb_bool absolute;
  mrb_value parts;
  mrb_value result;
  const char *p;
  const char *start;
  mrb_int i;
  mrb_int count;

  mrb_get_args(mrb, "z", &path);

  drive[0] = '\0';
  drive[1] = '\0';
  drive[2] = '\0';
  absolute = FALSE;
  parts = mrb_ary_new(mrb);

  if (x68k_path_has_drive(path)) {
    drive[0] = path[0];
    drive[1] = ':';
    body = path + 2;
    absolute = TRUE;
  } else if (x68k_path_is_rooted(path)) {
    body = path;
    absolute = TRUE;
  } else {
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      mrb_sys_fail(mrb, "getcwd");
    }
    if (x68k_path_has_drive(cwd)) {
      drive[0] = cwd[0];
      drive[1] = ':';
      body = cwd + 2;
      absolute = TRUE;
    } else {
      body = cwd;
      absolute = x68k_path_is_rooted(cwd) ? TRUE : FALSE;
    }

    start = body;
    for (p = body; ; p++) {
      if (*p == '\0' || *p == '/' || *p == '\\') {
        x68k_path_append_part(mrb, parts, start, (size_t)(p - start));
        if (*p == '\0') {
          break;
        }
        start = p + 1;
      }
    }
    body = path;
  }

  start = body;
  for (p = body; ; p++) {
    if (*p == '\0' || *p == '/' || *p == '\\') {
      x68k_path_append_part(mrb, parts, start, (size_t)(p - start));
      if (*p == '\0') {
        break;
      }
      start = p + 1;
    }
  }

  result = mrb_str_new_cstr(mrb, "");
  if (drive[0] != '\0') {
    mrb_str_cat_cstr(mrb, result, drive);
  }
  if (absolute) {
    mrb_str_cat_lit(mrb, result, "/");
  }

  count = RARRAY_LEN(parts);
  for (i = 0; i < count; i++) {
    if (RSTRING_LEN(result) > 0) {
      const char *s = RSTRING_PTR(result);
      char last = s[RSTRING_LEN(result) - 1];
      if (last != '/' && last != '\\' && last != ':') {
        mrb_str_cat_lit(mrb, result, "/");
      }
    }
    mrb_str_cat_str(mrb, result, mrb_ary_ref(mrb, parts, i));
  }

  if (RSTRING_LEN(result) == 0) {
    return mrb_str_new_lit(mrb, ".");
  }

  return result;
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
x68k_file_readlines(mrb_state *mrb, mrb_value self)
{
  char *path;
  FILE *fp;
  mrb_value lines;
  mrb_value line;
  int ch;

  mrb_get_args(mrb, "z", &path);
  fp = fopen(path, "rb");
  if (fp == NULL) {
    mrb_sys_fail(mrb, path);
  }

  lines = mrb_ary_new(mrb);
  line = mrb_str_new_capa(mrb, 128);
  while ((ch = fgetc(fp)) != EOF) {
    char c = (char)ch;
    mrb_str_cat(mrb, line, &c, 1);
    if (c == '\n') {
      mrb_ary_push(mrb, lines, line);
      line = mrb_str_new_capa(mrb, 128);
    }
  }

  if (ferror(fp)) {
    fclose(fp);
    mrb_sys_fail(mrb, path);
  }
  fclose(fp);

  if (RSTRING_LEN(line) > 0) {
    mrb_ary_push(mrb, lines, line);
  }

  return lines;
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
x68k_file_delete(mrb_state *mrb, mrb_value self)
{
  char *path;

  mrb_get_args(mrb, "z", &path);
  if (remove(path) != 0) {
    mrb_sys_fail(mrb, path);
  }

  return mrb_fixnum_value(1);
}

static mrb_value
x68k_file_rename(mrb_state *mrb, mrb_value self)
{
  char *old_path;
  char *new_path;

  mrb_get_args(mrb, "zz", &old_path, &new_path);
  if (rename(old_path, new_path) != 0) {
    mrb_sys_fail(mrb, old_path);
  }

  return mrb_fixnum_value(0);
}

static mrb_value
x68k_file_copy(mrb_state *mrb, mrb_value self)
{
  char *src;
  char *dst;
  FILE *in;
  FILE *out;
  char buffer[1024];
  size_t nread;

  mrb_get_args(mrb, "zz", &src, &dst);
  in = fopen(src, "rb");
  if (in == NULL) {
    mrb_sys_fail(mrb, src);
  }
  out = fopen(dst, "wb");
  if (out == NULL) {
    fclose(in);
    mrb_sys_fail(mrb, dst);
  }

  while ((nread = fread(buffer, 1, sizeof(buffer), in)) > 0) {
    if (fwrite(buffer, 1, nread, out) != nread) {
      fclose(in);
      fclose(out);
      mrb_sys_fail(mrb, dst);
    }
  }
  if (ferror(in)) {
    fclose(in);
    fclose(out);
    mrb_sys_fail(mrb, src);
  }
  if (fclose(out) != 0) {
    fclose(in);
    mrb_sys_fail(mrb, dst);
  }
  fclose(in);

  return mrb_fixnum_value(0);
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
x68k_dir_pwd(mrb_state *mrb, mrb_value self)
{
  char path[256];

  if (getcwd(path, sizeof(path)) == NULL) {
    mrb_sys_fail(mrb, "getcwd");
  }

  return mrb_str_new_cstr(mrb, path);
}

static mrb_value
x68k_dir_chdir(mrb_state *mrb, mrb_value self)
{
  char *path;

  mrb_get_args(mrb, "z", &path);
  if (chdir(path) != 0) {
    mrb_sys_fail(mrb, path);
  }

  return mrb_fixnum_value(0);
}

static mrb_value
x68k_dir_mkdir(mrb_state *mrb, mrb_value self)
{
  char *path;

  mrb_get_args(mrb, "z", &path);
  if (mkdir(path, 0777) != 0) {
    mrb_sys_fail(mrb, path);
  }

  return mrb_fixnum_value(0);
}

static mrb_value
x68k_dir_delete(mrb_state *mrb, mrb_value self)
{
  char *path;

  mrb_get_args(mrb, "z", &path);
  if (rmdir(path) != 0) {
    mrb_sys_fail(mrb, path);
  }

  return mrb_fixnum_value(0);
}

static int
x68k_ascii_tolower(int ch)
{
  if (ch >= 'A' && ch <= 'Z') {
    return ch + ('a' - 'A');
  }
  return ch;
}

static int
x68k_env_name_eq_ci(const char *entry, const char *name)
{
  const char *p = entry;
  const char *q = name;

  while (*p != '\0' && *p != '=' && *q != '\0') {
    if (x68k_ascii_tolower((unsigned char)*p) != x68k_ascii_tolower((unsigned char)*q)) {
      return 0;
    }
    p++;
    q++;
  }

  return (*p == '=') && (*q == '\0');
}

static char *
x68k_env_find_entry_ci(const char *name)
{
  char **env;

  if (environ == NULL) {
    return NULL;
  }

  for (env = environ; *env != NULL; env++) {
    if (x68k_env_name_eq_ci(*env, name)) {
      return *env;
    }
  }

  return NULL;
}

static char *
x68k_env_dup_entry_name(const char *entry)
{
  const char *eq;
  char *name;
  size_t len;

  eq = strchr(entry, '=');
  if (eq == NULL) {
    return NULL;
  }

  len = (size_t)(eq - entry);
  name = (char *)malloc(len + 1);
  if (name == NULL) {
    return NULL;
  }

  memcpy(name, entry, len);
  name[len] = '\0';
  return name;
}

static mrb_value
x68k_env_get(mrb_state *mrb, mrb_value self)
{
  char *name;
  char *entry;
  char *eq;

  mrb_get_args(mrb, "z", &name);
  entry = x68k_env_find_entry_ci(name);
  if (entry == NULL) {
    return mrb_nil_value();
  }

  eq = strchr(entry, '=');
  if (eq == NULL) {
    return mrb_nil_value();
  }

  return mrb_str_new_cstr(mrb, eq + 1);
}

static mrb_value
x68k_env_set(mrb_state *mrb, mrb_value self)
{
  char *name;
  char *value;
  char *entry;
  char *actual_name;

  mrb_get_args(mrb, "zz", &name, &value);
  entry = x68k_env_find_entry_ci(name);
  if (entry != NULL) {
    actual_name = x68k_env_dup_entry_name(entry);
    if (actual_name == NULL) {
      mrb_raise(mrb, E_RUNTIME_ERROR, "failed to copy environment name");
    }
    if (setenv(actual_name, value, 1) != 0) {
      free(actual_name);
      mrb_sys_fail(mrb, name);
    }
    free(actual_name);
    return mrb_str_new_cstr(mrb, value);
  }

  if (setenv(name, value, 1) != 0) {
    mrb_sys_fail(mrb, name);
  }

  return mrb_str_new_cstr(mrb, value);
}

static mrb_value
x68k_env_delete(mrb_state *mrb, mrb_value self)
{
  char *name;
  char *entry;
  char *eq;
  char *actual_name;
  mrb_value old_value;

  mrb_get_args(mrb, "z", &name);
  entry = x68k_env_find_entry_ci(name);
  if (entry == NULL) {
    return mrb_nil_value();
  }

  eq = strchr(entry, '=');
  old_value = eq == NULL ? mrb_nil_value() : mrb_str_new_cstr(mrb, eq + 1);
  if (eq == NULL) {
    return old_value;
  }

  actual_name = x68k_env_dup_entry_name(entry);
  if (actual_name == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "failed to copy environment name");
  }
  if (unsetenv(actual_name) != 0) {
    free(actual_name);
    mrb_sys_fail(mrb, name);
  }
  free(actual_name);

  return old_value;
}

static mrb_value
x68k_env_keys(mrb_state *mrb, mrb_value self)
{
  mrb_value keys;
  char **env;
  char *eq;

  keys = mrb_ary_new(mrb);
  if (environ == NULL) {
    return keys;
  }

  for (env = environ; *env != NULL; env++) {
    eq = strchr(*env, '=');
    if (eq != NULL) {
      mrb_ary_push(mrb, keys, mrb_str_new(mrb, *env, (mrb_int)(eq - *env)));
    }
  }

  return keys;
}


