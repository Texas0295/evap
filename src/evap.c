#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define VERSION "0.1"

struct Config {
    const char *editor;
    int keep;
    int no_output;
    int null_end;
};

void print_help(void) {
    puts("Usage: evap [OPTIONS]");
    puts("A temporary, no-trace editing buffer that evaporates after use.\n");
    puts("Options:");
    puts("  --editor=CMD     Use specified editor instead of $EDITOR or vi");
    puts("  --keep           Keep temporary file after use (debugging)");
    puts("  --no-output      Do not print buffer to stdout");
    puts("  --null-end       End output with NUL byte instead of newline");
    puts("  --help           Show this help message");
    puts("  --version        Show version information");
}

int parse_args(int argc, char **argv, struct Config *cfg) {
    int i;
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--keep") == 0) {
            cfg->keep = 1;
        } else if (strcmp(arg, "--no-output") == 0) {
            cfg->no_output = 1;
        } else if (strcmp(arg, "--null-end") == 0) {
            cfg->null_end = 1;
        } else if (strcmp(arg, "--help") == 0) {
            print_help();
            exit(0);
        } else if (strcmp(arg, "--version") == 0) {
            printf("evap version %s\n", VERSION);
            exit(0);
        } else if (strncmp(arg, "--editor=", 9) == 0) {
            cfg->editor = arg + 9;
        } else if (strncmp(arg, "--", 2) == 0) {
            fprintf(stderr, "Invalid option: '%s'. Use --help for usage.\n", arg);
            return 1;
        } else {
            fprintf(stderr, "Unrecognized argument: '%s'. Use --help for usage.\n", arg);
            return 1;
        }
    }
    return 0;
}

char *make_tempfile(char *buf, size_t size) {
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir || strlen(tmpdir) > 200) tmpdir = "/tmp";
    snprintf(buf, size, "%s/evapXXXXXX", tmpdir);
    int fd = mkstemp(buf);
    if (fd < 0) return NULL;
    fchmod(fd, S_IRUSR | S_IWUSR);
    close(fd);
    return buf;
}

int launch_editor(const char *editor, const char *path) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int tty = open("/dev/tty", O_RDWR);
        if (tty >= 0) {
            dup2(tty, STDIN_FILENO);
            dup2(tty, STDOUT_FILENO);
            dup2(tty, STDERR_FILENO);
            if (tty > STDERR_FILENO)
                close(tty);
        }

        execlp(editor, editor, path, (char *)NULL);
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1)
        return -1;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

int print_buffer(const char *path, int null_end) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        fwrite(buf, 1, n, stdout);
    fclose(f);

    if (null_end)
        fputc('\0', stdout);
    return 0;
}

static void secure_wipe(const char *path) {
    int fd = -1;
    struct stat stbuf;
    off_t size;
    char buf[4096];
    ssize_t n, written, ret;

    fd = open(path, O_WRONLY);
    if (fd < 0)
        return;

    if (fstat(fd, &stbuf) != 0)
        goto cleanup;

    size = stbuf.st_size;
    memset(buf, 0, sizeof(buf));

    while (size > 0) {
        n = (size > (off_t)sizeof(buf)) ? sizeof(buf) : size;
        written = 0;

        while (written < n) {
            ret = write(fd, buf + written, n - written);
            if (ret <= 0)
                goto cleanup;
            written += ret;
        }

        size -= n;
    }

    fsync(fd);

cleanup:
    close(fd);
}

int main(int argc, char **argv) {
    struct rlimit rl = {0,0};
    setrlimit(RLIMIT_CORE,&rl); /* disable core dumps */

    struct Config cfg = {
        .editor = getenv("EDITOR"),
        .keep = 0,
        .no_output = 0,
        .null_end = 0
    };

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "[EVAP] [ERROR] standard input is not supported. Pipe into evap is invalid.\n");
        return 1;
    }

    if (!cfg.editor || geteuid() != getuid())
        cfg.editor = "vi";

    if (parse_args(argc, argv, &cfg))
        return 1;

    char path[256];
    if (!make_tempfile(path, sizeof(path))) {
        perror("mkstemp");
        return 1;
    }

    if (launch_editor(cfg.editor, path) != 0) {
        fprintf(stderr, "[EVAP] [ERROR] Editor exited abnormally.\n");
        unlink(path);
        return 1;
    }

    if (!cfg.no_output)
        print_buffer(path, cfg.null_end);

    if (!cfg.keep) {
        secure_wipe(path);
        unlink(path);
    } else {
        fprintf(stderr, "[EVAP] [INFO] Kept temporary file: %s\n", path);
    }

    return 0;
}
