
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static int
process_file(int fd)
{
    char c;
    int n;
    int number = 0;
    int in_token = 0;
    int valid = 1;
    int has_digit = 0;
    const char *separators = " -\r\t\n./,";

    while ((n = read(fd, &c, 1)) == 1) {
        if (strchr(separators, c) != 0) {
            if (in_token && valid && has_digit) {
                if (number % 5 == 0 || number % 6 == 0)
                    printf("%d\n", number);
            }

            number = 0;
            in_token = 0;
            valid = 1;
            has_digit = 0;
        } else {
            if (!in_token) {
                in_token = 1;
                valid = 1;
                has_digit = 0;
                number = 0;
            }

            if (c >= '0' && c <= '9' && valid) {
                has_digit = 1;
                number = number * 10 + (c - '0');
            } else {
                valid = 0;
            }
        }
    }

    if (n < 0) {
        printf("sixfive: read error\n");
        return -1;
    }

    if (in_token && valid && has_digit) {
        if (number % 5 == 0 || number % 6 == 0)
            printf("%d\n", number);
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    int fd;
    int i;
    int result = 0;

    if (argc == 1) {
        if (process_file(0) < 0)
            exit(1);
        exit(0);
    }

    for (i = 1; i < argc; i++) {
        fd = open(argv[i], O_RDONLY);

        if (fd < 0) {
            printf("sixfive: cannot open %s\n", argv[i]);
            result = 1;
            continue;
        }

        if (process_file(fd) < 0)
            result = 1;

        if (close(fd) < 0) {
            printf("sixfive: close failed for %s\n", argv[i]);
            result = 1;
        }
    }

    exit(result);
}
