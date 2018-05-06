#include <stdio.h>
#include "otg_common.c"

uint32_t buf[8192];

int main(int argc, char **argv) {
  size_t rd;
  
  for (;;) {
    rd = fread(buf, sizeof(uint32_t), ARRAYLEN(buf), stdin);
    if (rd <= 0)
      break;

    for (size_t i=0; i<rd; ++i)
      buf[i] = SwapEndian32(buf[i]);

    fwrite(buf, sizeof(uint32_t), rd, stdout);
  }

  return 0;
}
