
#include <stdbool.h>
#include <stdint.h>

#include "helpers.h"

void Helper_FloatToString(char *buffer, uint32_t whole, uint32_t frac,
                          bool bAddEnd) {
  char *ptr = buffer;
  uint32_t temp = whole;

  if (temp == 0) {
    *ptr++ = '0';
  } else {
    char *start = ptr;
    while (temp > 0) {
      *ptr++ = (temp % 10) + '0';
      temp /= 10;
    }
    char *end = ptr - 1;
    while (start < end) {
      char t = *start;
      *start++ = *end;
      *end-- = t;
    }
  }
  if (bAddEnd)
    *ptr++ = '\0';
}


