#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <linux/connector.h>
#include <linux/netlink.h>
#include <linux/cn_proc.h>

#define max(x, y) ((y)<(x)?(x):(y))
#define min(x, y) ((y)>(x)?(x):(y))

#define SEND_MESSAGE_LEN (NLMSG_LENGTH(sizeof (struct cn_msg) + \
                                       sizeof (enum proc_cn_mcast_op)))
#define RECV_MESSAGE_LEN (NLMSG_LENGTH(sizeof (struct cn_msg) + \
                                       sizeof (struct proc_event)))

#define SEND_MESSAGE_SIZE    (NLMSG_SPACE(SEND_MESSAGE_LEN))
#define RECV_MESSAGE_SIZE    (NLMSG_SPACE(RECV_MESSAGE_LEN))

#define BUFF_SIZE (max(max(SEND_MESSAGE_SIZE, RECV_MESSAGE_SIZE), 4096+CONNECTOR_MAX_MSG_SIZE))

#define TRUNCATED_S "...<truncated>"
#define CMDLINE_MAX_CHARS      (65536 - sizeof(TRUNCATED_S))
char cmdline[CMDLINE_MAX_CHARS + sizeof(TRUNCATED_S)] = {0};

pid_t rootPid = 1;
int show_args = 1;
int show_cwd = 0;
int show_env = 0;
FILE *output;
sig_atomic_t quit = 0;

static void
sigint(int sig) {
  (void) sig;
  quit = 1;
}

static void
sigchld(int sig) {
  (void) sig;
  while (waitpid(-1, NULL, WNOHANG) > 0);
  quit = 1;
}

static void _printQuoted(const char *s, const char *meta_chars) {
  if (*s && !strpbrk(s, meta_chars)) {
    fprintf(output, "%s", s);
    return;
  }

  putc('\'', output);
  for (; *s; s++)
    if (*s == '\'')
      fprintf(output, "'\\''");
    else if (*s == '\n')
      fprintf(output, "'$'\\n''");
    else
      //todo: need more escape
      putc(*s, output);
  putc('\'', output);
}

static void printQuoted(const char *s) {
  printQuoted(s, "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020"
      "\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\040"
      "`^#*[]|\\?${}()'\"<>&;\177");
}

static void printQuoted_env(const char *s) {
  printQuoted(s, "\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020"
      "\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\040"
      "`^#*[]|\\?${}()'\"<>&;\177=");
}

static bool isDescendantPid(pid_t pid, pid_t rootPid) {
  char proc_path[PATH_MAX];
  snprintf(proc_path, sizeof proc_path, "/proc/%d/stat", pid);

  pid_t ppid = 0;
  FILE *f = fopen(proc_path, "r");
  if (f != NULL) {
    if (fscanf(f, "%*d (%*[^)]) %*c %d", &ppid) < 0) {
      fclose(f);
      return false;
    }
    fclose(f);
  } else {
    return false;
  }

  if (ppid == rootPid)
    return true;

  return isDescendantPid(ppid, rootPid);
}

static void
handle_msg(struct cn_msg *cn_hdr) {
  char proc_path[PATH_MAX];

  struct proc_event *ev = (struct proc_event *) cn_hdr->data;
  if (ev->what != PROC_EVENT_EXEC) return;

  pid_t pid = ev->event_data.exec.process_tgid;

  if (rootPid != 1 && !isDescendantPid(pid, rootPid))
    return;

  char ppid_s[16] = "?";
  char uid_s[16] = "?";
  char gid_s[16] = "?";
  snprintf(proc_path, sizeof proc_path, "/proc/%d/status", pid);
  {
    FILE *f = fopen(proc_path, "r");
    if (f != nil) {
      char *line = 0, *eq = 0;
      size_t lineLen = 0;
      while (getline(&line, &lineLen, f) >= 0) {
        if (sscanf(line, "PPid:\t%10s", ppid_s) == 1) {
        } else if (sscanf(line, "Uid:\t%10s", uid_s) == 1) {
        } else if (sscanf(line, "Gid:\t%10s", gid_s) == 1) {
          break;
        }
      }
      free(line);
      fclose(f);
    }
  }

  {
    char *comm = NULL;
    snprintf(proc_path, sizeof proc_path, "/proc/%d/comm", pid);
    {
      FILE *f = fopen(proc_path, "r");
      if (f != nil) {
        size_t size = 0;
        getline(&comm, &size, f);
        fclose(f);
      }
    }

    fprintf(output, "%d %s %s:%s %s", pid, ppid_s, uid_s, gid_s, comm == NULL ? "" : comm);

    free(comm);
  }

  {
    snprintf(proc_path, sizeof proc_path, "/proc/%d/cwd", pid);
    char cwd[PATH_MAX] = "";
    int len = readlink(proc_path, cwd, sizeof(cwd) - 1);
    if (len > 0)
      cwd[len] = 0;
    fprintf(output, "    cd ");
    printQuoted(cwd);
    fprintf(output, " & \\\n");
  }

  if (show_env) {
    snprintf(proc_path, sizeof proc_path, "/proc/%d/environ", pid);
    FILE* f = fopen(proc_path, "r");
    if (f != NULL) {
      char *kv = 0, *eq = 0;
      size_t size = 0;
      while (getdelim(&kv, &size, '\0', env) >= 0) {
        fprintf(output, "     ");
        if ((eq = strchr(kv, '='))) {
          *eq = 0;
          printQuoted_env(kv);
          fprintf(output, "=");
          printQuoted_env(eq + 1);
        } else {
          printQuoted_env(kv);
          fprintf(output, "=");
        }
        fprintf(output, " \\\n");
      }
      free(kv);
      fclose(env);
    } else {
      fprintf(output, " -");
    }
  }

  char exe[PATH_MAX] = "";
  snprintf(proc_path, sizeof proc_path, "/proc/%d/exe", pid);
  {
    int len = readlink(proc_path, exe, sizeof(exe) - 1);
    if (len > 0)
      exe[len] = 0;
  }

  snprintf(proc_path, sizeof proc_path, "/proc/%d/cmdline", pid);
  {
    FILE* f = fopen(proc_path, "r");
    char* arg = NULL;
    size_t size = 0;
    int count = 0;
    while (getdelim(&arg, &size, '\0', cmdline) != -1) {
      if (count == 0) {
        if (arg[0] == '/' || (arg[0] == '.' && arg[0] == '/') || (arg[0] == '..' && arg[0] == '/')) {
          fprintf(output, "$___ ");
          printQuoted(exe);
          fprintf(output, " \\\n");
        } else {
          fprintf(output, "$___ exec -a ");
          printQuoted(arg);
          fprintf(output, " ");
          printQuoted(exe);
          fprintf(output, " \\\n");
        }
      } else {
        fprintf(output, "$___    ");
        printQuoted(arg);
      }
      fprintf(output, " \\\n");
      count++;
    }
    free(arg);
    fclose(f);

    if (count == 0) {
      printQuoted(exe);
    }
  }
  fprintf(output, "\n");

  fflush(output);
}

const char my_short_opts[] = "efht";
struct option my_long_opts[] = {
    {"exec",   0, NULL, 'e'},
    {"fork",   0, NULL, 'f'},
    {"help",   0, NULL, 'h'},
    {"thread", 0, NULL, 't'},
    {}
};

int
main(int argc, char *argv[]) {
  int sk_nl;
  struct sockaddr_nl my_nla, kern_nla, from_nla;
  socklen_t from_nla_len;
  char buff[BUFF_SIZE];
  struct nlmsghdr *nl_hdr, *nlh;
  struct cn_msg *cn_hdr;
  enum proc_cn_mcast_op *mcop_msg;
  size_t recv_len = 0;
  int rc = -1, opt;

  const char *exeName = strrchr(argv[0], '/');
  if (!exeName) exeName = argv[0];

  output = stdout;

  while ((opt = getopt(argc, argv, "+delfo:p:qw")) != -1)
    switch (opt) {
      case 'd':
        show_cwd = 1;
        break;
      case 'e':
        show_env = 1;
        break;
      case 'l': /* obsoleted, ignore */;
        break;
      case 'f': /* obsoleted, ignore */;
        break;
      case 'p':
        rootPid = atoi(optarg);
        break;
      case 'q':
        show_args = 0;
        break;
      case 'o':
        output = fopen(optarg, "w");
        if (!output) {
          perror("fopen");
          exit(1);
        }
        break;
      case 'w': /* obsoleted, ignore */;
        break;
      default:
        goto usage;
    }

  if (rootPid != 1 && optind != argc) {
    usage:
    fprintf(stderr, "Usage: %s [-d|--cwd] [-e|--env] [-q|--no-args]\n", exeName);
    fprintf(stderr, "       %.*s [-o FILE] [-p PID|CMD...]\n", strlen(exeName));
    exit(1);
  }

  sk_nl = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
  if (sk_nl == -1) {
    perror("socket NETLINK_CONNECTOR");
    exit(1);
  }

  my_nla.nl_family = AF_NETLINK;
  my_nla.nl_groups = CN_IDX_PROC;
  my_nla.nl_pid = getpid();

  kern_nla.nl_family = AF_NETLINK;
  kern_nla.nl_groups = CN_IDX_PROC;
  kern_nla.nl_pid = 1;

  if (bind(sk_nl, (struct sockaddr *) &my_nla, sizeof my_nla) == -1) {
    perror("bind CN_IDX_PROC");
    goto close_and_exit;
  }
  nl_hdr = (struct nlmsghdr *) buff;
  cn_hdr = (struct cn_msg *) NLMSG_DATA(nl_hdr);
  mcop_msg = (enum proc_cn_mcast_op *) &cn_hdr->data[0];

  memset(buff, 0, sizeof buff);
  *mcop_msg = PROC_CN_MCAST_LISTEN;

  nl_hdr->nlmsg_len = SEND_MESSAGE_LEN;
  nl_hdr->nlmsg_type = NLMSG_DONE;
  nl_hdr->nlmsg_flags = 0;
  nl_hdr->nlmsg_seq = 0;
  nl_hdr->nlmsg_pid = getpid();

  cn_hdr->id.idx = CN_IDX_PROC;
  cn_hdr->id.val = CN_VAL_PROC;
  cn_hdr->seq = 0;
  cn_hdr->ack = 0;
  cn_hdr->len = sizeof(enum proc_cn_mcast_op);

  if (send(sk_nl, nl_hdr, nl_hdr->nlmsg_len, 0) != nl_hdr->nlmsg_len) {
    perror("send");
    goto close_and_exit;
  }

  if (*mcop_msg == PROC_CN_MCAST_IGNORE) {
    rc = 0;
    goto close_and_exit;
  }

  if (optind != argc) {
    pid_t child;

    rootPid = getpid();
    signal(SIGCHLD, sigchld);

    child = fork();
    if (child == -1) {
      perror("fork");
      goto close_and_exit;
    }
    if (child == 0) {
      execvp(argv[optind], argv + optind);
      perror("execvp");
      goto close_and_exit;
    }
  }

  signal(SIGINT, sigint);

  rc = 0;
  while (!quit) {
    memset(buff, 0, sizeof buff);
    from_nla_len = sizeof from_nla;
    nlh = (struct nlmsghdr *) buff;
    memcpy(&from_nla, &kern_nla, sizeof from_nla);
    recv_len = recvfrom(sk_nl, buff, BUFF_SIZE, 0,
                        (struct sockaddr *) &from_nla, &from_nla_len);
    if (from_nla.nl_pid != 0 || recv_len < 1)
      continue;

    while (NLMSG_OK(nlh, recv_len)) {
      if (nlh->nlmsg_type == NLMSG_NOOP)
        continue;
      if (nlh->nlmsg_type == NLMSG_ERROR || nlh->nlmsg_type == NLMSG_OVERRUN)
        break;

      handle_msg(NLMSG_DATA(nlh));

      if (nlh->nlmsg_type == NLMSG_DONE)
        break;
      nlh = NLMSG_NEXT(nlh, recv_len);
    }
  }

  close_and_exit:
  close(sk_nl);
  return rc;
}
