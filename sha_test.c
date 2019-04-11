#include "stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sha_funcs.h"

int main(int args, char *argv[]) {
  printf("Algorithm: %s \n", argv[1]);
  printf("Data: %s \n", argv[2]);
  if (strcmp(argv[1], "SHA256") == 0) {
    uint8_t result[32];
    Sha2_256(argv[2], strlen(argv[2]), result );
    print_sha(result, 32);
  }
  else if (strcmp(argv[1], "SHA1") == 0) {
    uint8_t result[20];
    Sha1(argv[2], strlen(argv[2]), result );
    print_sha(result, 20);
  }
  else {
    printf("Unknown algorithm: %s\n", argv[1]);
    return 1;
  }

  return 0;
}
