#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define PGSIZE 4096
#define DATASIZE (64 * PGSIZE)

static int
scan(uint64 start, uint64 end, char *marker, int marker_len)
{
  uint64 i;
  int j;

  for (i = start; i + marker_len <= end; i++) {
    for (j = 0; j < marker_len; j++) {
      if (((char *)i)[j] != marker[j])
        break;
    }

    if (j == marker_len) {
      uint64 secret = i + marker_len;
      int len = 0;

      while (secret + len < end && len < 100 &&
             ((char *)secret)[len] != '\0')
        len++;

      if (len > 0) {
        write(2, (char *)secret, len);
        write(2, "\n", 1);
        return 0;
      }
    }
  }

  return -1;
}

int
main(int argc, char *argv[])
{
  char marker[] = "Here it is: ";
  int marker_len = sizeof(marker) - 1;
  char *heap;
  uint64 end;

  heap = sbrk(DATASIZE);
  if (heap == (char *)-1)
    exit(1);

  end = (uint64)(heap + DATASIZE);

  // Skip the unmapped guard page at 0x2000.
  if (scan(0, 2 * PGSIZE, marker, marker_len) == 0)
    exit(0);

  if (end > 3 * PGSIZE &&
      scan(3 * PGSIZE, end, marker, marker_len) == 0)
    exit(0);

  exit(1);
}
