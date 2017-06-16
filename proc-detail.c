#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

int show_env = 0;

void _print_quoted(const char *s, const char *meta_chars) {
  if (*s && !strpbrk(s, meta_chars)) {
    printf("%s", s);
    return;
  }
  /*
  Words of the form $'string' are treated specially.  The word expands to string, with backslash-escaped characters
  replaced as specified by the ANSI C standard.  Backslash escape sequences, if present, are decoded as follows:
         \a     alert (bell)
         \b     backspace
         \e     an escape character
         \f     form feed
         \n     new line
         \r     carriage return
         \t     horizontal tab
         \v     vertical tab
         \\     backslash
         \'     single quote
         \nnn   the eight-bit character whose value is the octal value nnn (one to three digits)
         \xHH   the eight-bit character whose value is the hexadecimal value HH (one or two hex digits)
         \cx    a control-x character
  The expanded result is single-quoted, as if the dollar sign had not been present.
  */
  putc('\'', stdout);
  for (; *s; s++)
    if (*s == '\'')
      printf("'\\''");
    else if (*s == '\n')
      printf("'$'\\n''");
    else if (*s == '\t')
      printf("'$'\\t''");
    else if (*s >= 0x01 && *s <= 0x31 || *s == 0x7f)
      printf("'$'\\x%02x''", *s);
    else
      putc(*s, stdout);
  putc('\'', stdout);
}

void print_quoted(const char *s) {
  _print_quoted(s, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
      " `^#*[]|\\?${}()'\"<>&;\177");
}

void print_quoted_env(const char *s) {
  _print_quoted(s, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
      "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
      " `^#*[]|\\?${}()'\"<>&;\177=");
}

void print_cmdline(pid_t pid) {

  char proc_path[PATH_MAX];
  {
    snprintf(proc_path, sizeof proc_path, "/proc/%d/cwd", pid);
    char cwd[PATH_MAX] = "";
    int len = readlink(proc_path, cwd, sizeof(cwd) - 1);
    if (len > 0) {
      printf("    cd ");
      print_quoted(cwd);
      printf(" & \\\n");
    }
  }

  if (show_env) {
    snprintf(proc_path, sizeof proc_path, "/proc/%d/environ", pid);
    FILE* f = fopen(proc_path, "r");
    if (f != NULL) {
      char *kv = 0, *eq = 0;
      size_t bufCap = 0;
      while (getdelim(&kv, &bufCap, '\0', f) >= 0) {
        printf("     ");
        if ((eq = strchr(kv, '='))) {
          *eq = 0;
          print_quoted_env(kv);
          printf("=");
          print_quoted_env(eq + 1);
        } else {
          print_quoted_env(kv);
          printf("=");
        }
        printf(" \\\n");
      }
      free(kv);
      fclose(f);
    }
  }

  char exe[PATH_MAX] = "?";
  snprintf(proc_path, sizeof proc_path, "/proc/%d/exe", pid);
  {
    int len = readlink(proc_path, exe, sizeof(exe) - 1);
    if (len > 0)
      exe[len] = 0;
  }

  snprintf(proc_path, sizeof proc_path, "/proc/%d/cmdline", pid);
  {
    int count = 0;
    FILE* f = fopen(proc_path, "r");
    if (f != NULL) {
      char *arg = NULL;
      size_t bufCap = 0;
      while (getdelim(&arg, &bufCap, '\0', f) != -1) {
        if (count == 0) {
          if (arg[0] == '/' || (arg[0] == '.' && arg[0] == '/') || (arg[0] == '.' && arg[0] == '/')) {
            printf("$___ ");
            print_quoted(exe);
            printf(" \\\n");
          } else {
            printf("$___ exec -a ");
            print_quoted(arg);
            printf(" ");
            print_quoted(exe);
            printf(" \\\n");
          }
        } else {
          printf("$___    ");
          print_quoted(arg);
        }
        printf(" \\\n");
        count++;
      }
      free(arg);
      fclose(f);
    }

    if (count == 0) {
      print_quoted(exe);
    }
  }
  printf("\n");

  fflush(stdout);
}

const char *exeName;

void usage() {
  printf("Usage1: %s [-e|--env] pid ...\n", exeName);
  printf("Usage2: ps | %s [-e|--env]\n", exeName);
}

int main(int argc, char *argv[]) {

  exeName = strrchr(argv[0], '/');
  if (!exeName) exeName = argv[0];

  struct option long_options[] = {
      {"env", no_argument, NULL, 'e'},
      {0, 0, 0, 0}
  };
  int opt, option_index;
  while ((opt = getopt_long(argc, argv, "e", long_options, &option_index)) != -1) {
    switch (opt) {
      case 'e':
        show_env = 1;
        break;
      default:
        usage();
        exit(EXIT_FAILURE);
    }
  }

  if (optind < argc) {
    while (optind < argc) {
      pid_t pid = 0;
      if (sscanf(argv[optind], "%d", &pid) == 1) {
        print_cmdline(pid);
      }
      optind++;
    }
  } else {
    char *line = NULL;
    size_t bufCap = 0;
    while (getline(&line, &bufCap, stdin) != -1) {
      printf("%s", line);
      fflush(stdout);
      pid_t pid = 0;
      if (sscanf(line, "%d", &pid) == 1) {
        print_cmdline(pid);
        fflush(stdout);
      }
    }
    int e = errno;
    if (!feof(stdin)) {
      errno = e;
      perror("getline");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
