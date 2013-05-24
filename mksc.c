/*
 * mksc.c
 *
 * Copyright (c) 2013, Nathan Forbes
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *     Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Creates a basic script template.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MKSC_DEFAULT_PROGRAM_NAME "mksc"
#define MKSC_VERSION "0.4"

#define MKSC_DATE_STRING_BUFFER_SIZE 12
#define MKSC_INTRP_BUFFER_SIZE 64

#define MKSC_ASH_NAME "ash"
#define MKSC_AWK_NAME "awk"
#define MKSC_BASH_NAME "bash"
#define MKSC_BUSYBOX_NAME "busybox"
#define MKSC_CSH_NAME "csh"
#define MKSC_DASH_NAME "dash"
#define MKSC_KSH_NAME "ksh"
#define MKSC_LUA_NAME "lua"
#define MKSC_MKSH_NAME "mksh"
#define MKSC_PDKSH_NAME "pdksh"
#define MKSC_PERL_NAME "perl"
#define MKSC_PHP_NAME "php"
#define MKSC_PYTHON_NAME "python"
#define MKSC_RUBY_NAME "ruby"
#define MKSC_SH_NAME "sh"
#define MKSC_TCSH_NAME "tcsh"
#define MKSC_ZSH_NAME "zsh"

typedef enum mksc_script_intrp mksc_script_intrp_t;
typedef struct mksc_script mksc_script_t;

typedef enum
{
  MKSC_FALSE,
  MKSC_TRUE
} mksc_boolean_t;

enum mksc_script_intrp
{
  MKSC_ASH_TYPE,
  MKSC_AWK_TYPE,
  MKSC_BASH_TYPE,
  MKSC_BUSYBOX_TYPE,
  MKSC_CSH_TYPE,
  MKSC_CUSTOM_TYPE,
  MKSC_DASH_TYPE,
  MKSC_KSH_TYPE,
  MKSC_LUA_TYPE,
  MKSC_MKSH_TYPE,
  MKSC_PDKSH_TYPE,
  MKSC_PERL_TYPE,
  MKSC_PHP_TYPE,
  MKSC_PYTHON_TYPE,
  MKSC_RUBY_TYPE,
  MKSC_SH_TYPE,
  MKSC_TCSH_TYPE,
  MKSC_UNKNOWN_TYPE,
  MKSC_ZSH_TYPE
};

struct mksc_script
{
  mksc_script_intrp_t type;
  char *intrp_path;
  char *path;
  mksc_script_t *next;
  mksc_script_t *prev;
};

extern char **environ;
const char *g_program_name;

mksc_script_t *g_list = NULL;

static mksc_boolean_t
mksc_string_equals (const char *s1, const char *s2)
{
  size_t n;

  n = strlen (s1);
  if ((strlen (s2) == n) && (memcmp (s1, s2, n) == 0))
    return MKSC_TRUE;
  return MKSC_FALSE;
}

static mksc_boolean_t
mksc_string_startswith (const char *s, const char *p)
{
  size_t n;

  n = strlen (p);
  if ((strlen (s) >= n) && (memcmp (s, p, n) == 0))
    return MKSC_TRUE;
  return MKSC_FALSE;
}

static void
mksc_die (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "%s: error: ", g_program_name);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (EXIT_FAILURE);
}

static void
mksc_usage (mksc_boolean_t error)
{
  fprintf (!error ? stdout : stderr,
      "Usage: %s [-aAbBcdDklmpPrtxyz] [-C INTERPRETER] FILENAME\n",
      g_program_name);
}

static void
mksc_help (void)
{
  mksc_usage (MKSC_FALSE);
  fputs ("Options:\n"
      "  -a, --ash          Create an Almquist Shell script\n"
      "  -A, --awk          Create an AWK script\n"
      "  -b, --sh           Create a Bourne Shell script [DEFAULT]\n"
      "  -B, --bash         Create a Bourne-Again Shell script\n"
      "  -c, --csh          Create a C Shell script\n"
      "  -C INTERPRETER, --custom=INTERPRETER\n"
      "                     Create a script that uses INTERPRETER as the\n"
      "                     interpreter. If INTERPRETER is not an absolute\n"
      "                     path, an attempt to find it in the system's\n"
      "                     PATH environment variable will be made.\n"
      "  -d, --dash         Create a Debian Almquist Shell script\n"
      "  -D, --pdksh        Create a Public Domain Korn Shell script\n"
      "  -k, --ksh          Create a Korn Shell script\n"
      "  -l, --lua          Create a Lua script\n"
      "  -m, --mksh         Create a MirBSD Korn Shell script\n"
      "  -p, --perl         Create a Perl script\n"
      "  -P, --php          Create a PHP script\n"
      "  -r, --ruby         Create a Ruby script\n"
      "  -t, --tcsh         Create a TENEX C Shell script\n"
      "  -x, --busybox      Create a BusyBox script\n"
      "  -y, --python       Create a Python script\n"
      "  -z, --zsh          Create a Z Shell script\n"
      "  -?, -h, --help     Display this text and exit\n"
      "  -v, --version      Display version information and exit\n"
      "If no script type option is given, `--sh' is assumed.\n",
    stdout);
  exit (EXIT_SUCCESS);
}

static void
mksc_version (void)
{
  fputs (MKSC_DEFAULT_PROGRAM_NAME " " MKSC_VERSION "\n"
      "Written by Nathan Forbes (2013)\n",
      stdout);
  exit (EXIT_SUCCESS);
}

static char *
mksc_date_string (void)
{
  time_t t;
  struct tm *tmp;
  char *date;

  date = (char *) malloc (MKSC_DATE_STRING_BUFFER_SIZE);
  if (date == NULL)
    mksc_die ("out of memory");

  time (&t);
  tmp = localtime (&t);
  if (tmp != NULL &&
      strftime (date, MKSC_DATE_STRING_BUFFER_SIZE, "%F", tmp) > 0)
    return date;
  return NULL;
}

static char *
mksc_basename (char *p)
{
  char *x;

  if (p != NULL)
  {
    x = strrchr (p, '/');
    if (x != NULL)
      return ++x;
    return p;
  }
  return NULL;
}

static void
mksc_set_proper_program_name (char *n)
{
  char *x;

  x = mksc_basename (n);
  if (x != NULL)
    g_program_name = x;
  else
    g_program_name = MKSC_DEFAULT_PROGRAM_NAME;
}

static mksc_boolean_t
mksc_file_exists (const char *p)
{
  struct stat s;

  memset (&s, 0, sizeof (struct stat));
  if (stat (p, &s) == 0 && S_ISREG (s.st_mode))
    return MKSC_TRUE;
  return MKSC_FALSE;
}

static mksc_boolean_t
mksc_can_exec (const char *p)
{
  if (access (p, X_OK) == 0)
    return MKSC_TRUE;
  return MKSC_FALSE;
}

static const char *
mksc_getenv (const char *key)
{
  int x;
  size_t n_key;
  const char *value;

  n_key = strlen (key);
  for (x = 0; environ[x] != NULL; ++x)
  {
    if (mksc_string_startswith (environ[x], key) && environ[x][n_key] == '=')
    {
      value = environ[x] + (n_key + 1);
      return value;
    }
  }
  return NULL;
}

static char *
mksc_find_intrp_abspath (char *intrp)
{
  int x;
  int y;
  int pos;
  size_t n;
  size_t n_intrp;
  mksc_boolean_t complete;
  const char *path;

  n_intrp = strlen (intrp);
  if (n_intrp > 0)
  {
    if (intrp[0] == '/')
    {
      if (mksc_file_exists (intrp))
      {
        if (mksc_can_exec (intrp))
          return strndup (intrp, n_intrp);
        else
          mksc_die ("`%s' is not executable", intrp);
      }
      else
        mksc_die ("`%s' does not exist", intrp);
    }
    else
    {
      path = mksc_getenv ("PATH");
      if (path == NULL)
        mksc_die ("unable to find environment variable 'PATH'");
      n = 0;
      pos = 0;
      for (x = 0; path[x] != '\0'; ++x)
      {
        if (path[x] == ':' || path[x] == '\0')
        {
          if (pos == 0)
            n = x - 1;
          else if ((x - pos) > n)
            n = (x - pos) - 1;
          pos = x - 1;
        }
      }
      n += n_intrp + 1;
      pos = 0;
      complete = MKSC_FALSE;
      char buffer[n];
      for (x = 0; path[x] != '\0'; ++x)
      {
        if (path[x] == ':')
          complete = MKSC_TRUE;
        else
          buffer[pos++] = path[x];
        if (path[x + 1] == '\0' && !complete)
          complete = MKSC_TRUE;
        if (complete)
        {
          buffer[pos++] = '/';
          for (y = 0; intrp[y] != '\0'; ++y)
            buffer[pos++] = intrp[y];
          buffer[pos] = '\0';
          if (mksc_file_exists (buffer) && mksc_can_exec (buffer))
            return strndup (buffer, pos);
          pos = 0;
          complete = MKSC_FALSE;
        }
      }
    }
  }
  return NULL;
}

static void
mksc_write_template (const char *sc_path, const char *intrp_path)
{
  FILE *sc;
  char *d_str;

  sc = fopen (sc_path, "w");
  if (sc == NULL)
  {
    fprintf (stderr, "%s: warning: failed to open `%s' (%s)\n",
        g_program_name, sc_path, strerror (errno));
    return;
  }

  /*
     #!<interpreter path>
     #
     # <filename>
     #
     # <username>
     # <date>
     #
   */
  d_str = mksc_date_string ();
  if (d_str != NULL)
  {
    fprintf (sc,
        "#!%s\n"
        "#\n"
        "# %s\n"
        "#\n"
        "# %s\n"
        "# %s\n"
        "#\n",
        intrp_path,
        mksc_basename ((char *) sc_path),
        mksc_getenv ("USER"),
        d_str);
    free (d_str);
    d_str = NULL;
  }
  else
    fprintf (sc,
        "#!%s\n"
        "#\n"
        "# %s\n"
        "# %s\n"
        "#\n",
        intrp_path,
        mksc_basename ((char *) sc_path),
        mksc_getenv ("USER"));

  if (fclose (sc) != 0)
    fprintf (stderr, "%s: warning: failed to close `%s' (%s)\n",
        g_program_name, sc_path, strerror (errno));
}

static void
mksc_make_exec (const char *sc_path)
{
  if (chmod (sc_path, S_IRWXU) == -1)
    fprintf (stderr, "%s: warning: failed to make `%s' executable (%s)\n",
        g_program_name, sc_path, strerror (errno));
}

static void
mksc_exit (void)
{
  if (g_list != NULL)
  {
    for (; g_list != NULL; g_list = g_list->next)
    {
      if (g_list->intrp_path != NULL)
      {
        free (g_list->intrp_path);
        g_list->intrp_path = NULL;
      }
      if (g_list->prev != NULL)
      {
        free (g_list->prev);
        g_list->prev = NULL;
      }
      if (g_list->next == NULL)
        break;
    }
    if (g_list != NULL)
    {
      free (g_list);
      g_list = NULL;
    }
  }
}

static const char *
mksc_def_intrp_name (mksc_script_intrp_t *i)
{
  switch (*i)
  {
    case MKSC_ASH_TYPE:
      return MKSC_ASH_NAME;
    case MKSC_AWK_TYPE:
      return MKSC_AWK_NAME;
    case MKSC_BASH_TYPE:
      return MKSC_BASH_NAME;
    case MKSC_BUSYBOX_TYPE:
      return MKSC_BUSYBOX_NAME;
    case MKSC_CSH_TYPE:
      return MKSC_CSH_NAME;
    /*case MKSC_CUSTOM_TYPE:
      return "[CUSTOM]";*/
    case MKSC_DASH_TYPE:
      return MKSC_DASH_NAME;
    case MKSC_KSH_TYPE:
      return MKSC_KSH_NAME;
    case MKSC_LUA_TYPE:
      return MKSC_LUA_NAME;
    case MKSC_MKSH_TYPE:
      return MKSC_MKSH_NAME;
    case MKSC_PDKSH_TYPE:
      return MKSC_PDKSH_NAME;
    case MKSC_PERL_TYPE:
      return MKSC_PERL_NAME;
    case MKSC_PHP_TYPE:
      return MKSC_PHP_NAME;
    case MKSC_PYTHON_TYPE:
      return MKSC_PYTHON_NAME;
    case MKSC_RUBY_TYPE:
      return MKSC_RUBY_NAME;
    case MKSC_SH_TYPE:
      return MKSC_SH_NAME;
    case MKSC_TCSH_TYPE:
      return MKSC_TCSH_NAME;
    case MKSC_ZSH_TYPE:
      return MKSC_ZSH_NAME;
    default:
      return NULL/*"[UNKNOWN]"*/;
  }
}

static void
mksc_discern_long_opt (const char *o,
                       mksc_script_intrp_t *i,
                       char *intrp_buffer)
{
  size_t n;
  char *x;
  const char *d_intrp;

  if (mksc_string_equals (o, "almquist-shell"))
    *i = MKSC_ASH_TYPE;
  else if (mksc_string_equals (o, "awk"))
    *i = MKSC_AWK_TYPE;
  else if (mksc_string_equals (o, "bourne-shell"))
    *i = MKSC_SH_TYPE;
  else if (mksc_string_equals (o, "bourne-again-shell"))
    *i = MKSC_BASH_TYPE;
  else if (mksc_string_equals (o, "busybox"))
    *i = MKSC_BUSYBOX_TYPE;
  else if (mksc_string_equals (o, "c-shell"))
    *i = MKSC_CSH_TYPE;
  else if (mksc_string_equals (o, "custom"))
    *i = MKSC_CUSTOM_TYPE;
  else if (mksc_string_startswith (o, "custom="))
  {
    *i = MKSC_CUSTOM_TYPE;
    x = strrchr (o, '=');
    n = strlen (++x);
    strncpy (intrp_buffer, x, n);
    intrp_buffer[n] = '\0';
  }
  else if (mksc_string_equals (o, "debian-almquist-shell"))
    *i = MKSC_DASH_TYPE;
  else if (mksc_string_equals (o, "help"))
    mksc_help ();
  else if (mksc_string_equals (o, "korn-shell"))
    *i = MKSC_KSH_TYPE;
  else if (mksc_string_equals (o, "lua"))
    *i = MKSC_LUA_TYPE;
  else if (mksc_string_equals (o, "mirbsd-korn-shell"))
    *i = MKSC_MKSH_TYPE;
  else if (mksc_string_equals (o, "perl"))
    *i = MKSC_PERL_TYPE;
  else if (mksc_string_equals (o, "php"))
    *i = MKSC_PHP_TYPE;
  else if (mksc_string_equals (o, "public-domain-korn-shell"))
    *i = MKSC_PDKSH_TYPE;
  else if (mksc_string_equals (o, "python"))
    *i = MKSC_PYTHON_TYPE;
  else if (mksc_string_equals (o, "ruby"))
    *i = MKSC_RUBY_TYPE;
  else if (mksc_string_equals (o, "tenex-c-shell"))
    *i = MKSC_TCSH_TYPE;
  else if (mksc_string_equals (o, "version"))
    mksc_version ();
  else if (mksc_string_equals (o, "z-shell"))
    *i = MKSC_ZSH_TYPE;
  else
  {
    fprintf (stderr, "%s: error: `--%s' unrecognized\n", g_program_name, o);
    mksc_usage (MKSC_TRUE);
    exit (EXIT_FAILURE);
  }

  if (*i != MKSC_CUSTOM_TYPE)
  {
    d_intrp = mksc_def_intrp_name (i);
    n = strlen (d_intrp);
    strncpy (intrp_buffer, d_intrp, n);
    intrp_buffer[n] = '\0';
  }
}

static void
mksc_discern_short_opt (const char o,
                        mksc_script_intrp_t *i,
                        char *intrp_buffer)
{
  size_t n;
  const char *d_intrp;

  switch (o)
  {
    case '?':
      mksc_help ();
    case 'a':
      *i = MKSC_ASH_TYPE;
      break;
    case 'A':
      *i = MKSC_AWK_TYPE;
      break;
    case 'b':
      *i = MKSC_SH_TYPE;
      break;
    case 'B':
      *i = MKSC_BASH_TYPE;
      break;
    case 'c':
      *i = MKSC_CSH_TYPE;
      break;
    case 'C':
      *i = MKSC_CUSTOM_TYPE;
      break;
    case 'd':
      *i = MKSC_DASH_TYPE;
      break;
    case 'D':
      *i = MKSC_PDKSH_TYPE;
      break;
    case 'h':
      mksc_help ();
    case 'k':
      *i = MKSC_KSH_TYPE;
      break;
    case 'l':
      *i = MKSC_LUA_TYPE;
      break;
    case 'm':
      *i = MKSC_MKSH_TYPE;
      break;
    case 'p':
      *i = MKSC_PERL_TYPE;
      break;
    case 'P':
      *i = MKSC_PHP_TYPE;
      break;
    case 'r':
      *i = MKSC_RUBY_TYPE;
      break;
    case 't':
      *i = MKSC_TCSH_TYPE;
      break;
    case 'v':
      mksc_version ();
    case 'x':
      *i = MKSC_BUSYBOX_TYPE;
      break;
    case 'y':
      *i = MKSC_PYTHON_TYPE;
      break;
    case 'z':
      *i = MKSC_ZSH_TYPE;
      break;
    default:
      fprintf (stderr, "%s: error: `-%c' unrecognized\n", g_program_name, o);
      mksc_usage (MKSC_TRUE);
      exit (EXIT_FAILURE);
  }

  if (*i != MKSC_CUSTOM_TYPE)
  {
    d_intrp = mksc_def_intrp_name (i);
    n = strlen (d_intrp);
    strncpy (intrp_buffer, d_intrp, n);
    intrp_buffer[n] = '\0';
  }
}

static void
mksc_parse_cmd (char **v)
{
  int x;
  size_t n;
  char intrp[MKSC_INTRP_BUFFER_SIZE];
  char *intrp_abspath;
  mksc_script_intrp_t i;
  mksc_script_t *l;

  mksc_set_proper_program_name (v[0]);
  i = MKSC_SH_TYPE;
  intrp[0] = '\0';

  for (x = 1; v[x] != NULL; ++x)
  {
    if (v[x][0] == '-')
    {
      if (v[x][1] != '\0' && v[x][1] == '-')
        mksc_discern_long_opt (v[x] + 2, &i, intrp);
      else if (v[x][1] != '\0')
        mksc_discern_short_opt (v[x][1], &i, intrp);
      else
      {
        fprintf (stderr, "%s: error: `-' unrecognized\n", g_program_name);
        mksc_usage (MKSC_TRUE);
        exit (EXIT_FAILURE);
      }
      if (i == MKSC_CUSTOM_TYPE && intrp[0] == '\0')
      {
        if (v[x + 1] == NULL)
        {
          fprintf (stderr, "%s: error: `%s' requires an argument\n",
              g_program_name, v[x]);
          mksc_usage (MKSC_TRUE);
          exit (EXIT_FAILURE);
        }
        n = strlen (v[++x]);
        strncpy (intrp, v[x], n);
        intrp[n] = '\0';
      }
    }
    else
    {
      intrp_abspath = mksc_find_intrp_abspath (intrp);
      if (intrp_abspath == NULL)
      {
        fprintf (stderr, "%s: warning: unable to find `%s' in PATH\n",
            g_program_name, intrp);
        fprintf (stderr, "%s: warning: make sure `%s' is properly "
            "installed\n", g_program_name, intrp);
        continue;
      }
      if (g_list == NULL)
      {
        l = (mksc_script_t *) malloc (sizeof (mksc_script_t));
        l->prev = NULL;
      }
      else
      {
        for (l = g_list; l != NULL; l = l->next)
          if (l->next == NULL)
            break;
        l->next = (mksc_script_t *) malloc (sizeof (mksc_script_t));
        l->next->prev = l;
        l = l->next;
      }
      l->type = i;
      l->intrp_path = intrp_abspath;
      intrp[0] = '\0';
      l->path = v[x];
      l->next = NULL;
      if (l->prev != NULL)
        for (; l != NULL; l = l->prev)
          if (l->prev == NULL)
            break;
      g_list = l;
    }
  }
}

static void
mksc_perform (void)
{
  mksc_script_t *l;

  if (g_list == NULL)
  {
    fprintf (stderr, "%s: error: nothing to do\n", g_program_name);
    mksc_usage (MKSC_TRUE);
    exit (EXIT_FAILURE);
  }

  for (l = g_list; l != NULL; l = l->next)
  {
    mksc_write_template (l->path, l->intrp_path);
    mksc_make_exec (l->path);
  }
}

int
main (int argc, char **argv)
{
  atexit (mksc_exit);
  mksc_parse_cmd (argv);
  mksc_perform ();
  exit (EXIT_SUCCESS);
  return 0; /* for compiler */
}

