#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *)&s, sizeof(s));

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      int nn = read(0, data + n, sizeof(data) - n);
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}


static void
print_hex(uint64 value)
{
    char digits[] = "0123456789ABCDEF";
    char buf[16];
    int n = 0;

    do {
        buf[n++] = digits[value & 0xF];
        value >>= 4;
    } while (value != 0);

    while (n > 0)
        printf("%c", buf[--n]);

    printf("\n");
}

void
memdump(char *fmt, char *data, int len)
{
    int pos = 0;
    int i;
    int j;
    int value;
    uint64 raw;

    for (i = 0; fmt[i] != '\0'; i++) {
        switch (fmt[i]) {
        case 'i':
            if (len - pos < 4) {
                printf("memdump: not enough data for 'i'\n");
                return;
            }

            raw = 0;
            for (j = 0; j < 4; j++)
                raw |= (uint64)(uint8)data[pos + j] << (8 * j);

            printf("%d\n", (int)(uint32)raw);
            pos += 4;
            break;

        case 'p':
            if (len - pos < 8) {
                printf("memdump: not enough data for 'p'\n");
                return;
            }

            raw = 0;
            for (j = 0; j < 8; j++)
                raw |= (uint64)(uint8)data[pos + j] << (8 * j);

            print_hex(raw);
            pos += 8;
            break;

        case 'h':
            if (len - pos < 2) {
                printf("memdump: not enough data for 'h'\n");
                return;
            }

            value = (uint8)data[pos]
                  | ((uint8)data[pos + 1] << 8);

            if (value & 0x8000)
                value -= 65536;

            printf("%d\n", value);
            pos += 2;
            break;

        case 'c':
            if (len - pos < 1) {
                printf("memdump: not enough data for 'c'\n");
                return;
            }

            printf("%c\n", data[pos]);
            pos++;
            break;

        case 's':
            if (len - pos < 8) {
                printf("memdump: not enough data for 's'\n");
                return;
            }

            raw = 0;
            for (j = 0; j < 8; j++)
                raw |= (uint64)(uint8)data[pos + j] << (8 * j);

            if (raw == 0)
                printf("\n");
            else
                printf("%s\n", (char *)raw);

            pos += 8;
            break;

        case 'S':
            j = pos;

            while (j < len && data[j] != '\0') {
                printf("%c", data[j]);
                j++;
            }

            printf("\n");
            pos = j;

            if (pos < len && data[pos] == '\0')
                pos++;

            break;

        default:
            printf("memdump: unknown format character '%c'\n",
                   fmt[i]);
            return;
        }
    }
}
