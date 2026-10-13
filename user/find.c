
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

// Return the filename at the end of a path.
char*
get_name(char *path)
{
    char *p;

    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    return p + 1;
}

// Run the requested command with the matched file as its last argument.
void
run_command(char *path, char **cmdargs, int cmdargc)
{
    int pid;
    int i;
    char *args[MAXARG];

    pid = fork();

    if (pid < 0) {
        fprintf(2, "find: fork failed\n");
        return;
    }

    if (pid == 0) {
        // Copy the command's existing arguments.
        for (i = 0; i < cmdargc; i++)
            args[i] = cmdargs[i];

        // Append the path of the matched file.
        args[cmdargc] = path;
        args[cmdargc + 1] = 0;

        exec(args[0], args);

        // exec returns only if it fails.
        fprintf(2, "find: exec %s failed\n", args[0]);
        exit(1);
    }

    // The parent waits for the command to finish.
    wait(0);
}

// Search path recursively for files named target.
// If cmdargs is not 0, execute that command for every match.
void
find(char *path, char *target, char **cmdargs, int cmdargc)
{
    char buf[512];
    char *p;
    int fd;
    int n;
    struct stat st;
    struct dirent de;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // If path is a regular file, check its name.
    if (st.type == T_FILE) {
        if (strcmp(get_name(path), target) == 0) {
            if (cmdargs != 0)
                run_command(path, cmdargs, cmdargc);
            else
                printf("%s\n", path);
        }

        close(fd);
        return;
    }

    if (st.type != T_DIR) {
        close(fd);
        return;
    }

    // Make sure the path and a child filename fit in buf.
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
        fprintf(2, "find: path too long: %s\n", path);
        close(fd);
        return;
    }

    strcpy(buf, path);
    p = buf + strlen(buf);

    if (p > buf && *(p - 1) != '/')
        *p++ = '/';

    // Read each directory entry.
    while ((n = read(fd, &de, sizeof(de))) == sizeof(de)) {
        if (de.inum == 0)
            continue;

        // Do not recurse into the current or parent directory.
        if (strcmp(de.name, ".") == 0 ||
            strcmp(de.name, "..") == 0)
            continue;

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = '\0';

        if (stat(buf, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }

        if (st.type == T_FILE) {
            if (strcmp(de.name, target) == 0) {
                if (cmdargs != 0)
                    run_command(buf, cmdargs, cmdargc);
                else
                    printf("%s\n", buf);
            }
        } else if (st.type == T_DIR) {
            find(buf, target, cmdargs, cmdargc);
        }
    }

    if (n < 0)
        fprintf(2, "find: error reading %s\n", path);

    close(fd);
}

int
main(int argc, char *argv[])
{
    char **cmdargs = 0;
    int cmdargc = 0;
    

    // Normal form: find <directory> <filename>
    if (argc == 3) {
        find(argv[1], argv[2], 0, 0);
        exit(0);
    }

    // Extended form: find <directory> <filename> -exec <command> [args...]
    if (argc >= 5 && strcmp(argv[3], "-exec") == 0) {
        cmdargs = &argv[4];
        cmdargc = argc - 4;

        // We need room for the matched path and the final null pointer.
        if (cmdargc + 2 > MAXARG) {
            fprintf(2, "find: too many command arguments\n");
            exit(1);
        }

        find(argv[1], argv[2], cmdargs, cmdargc);
        exit(0);
    }

    fprintf(2, "Usage: find <directory> <filename> [-exec <command> [args...]]\n");
    exit(1);
}
