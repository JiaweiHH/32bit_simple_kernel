#include "mstring.h"

inline void memcpy(uint8_t *dest, const uint8_t *src, uint32_t len) {
  for(; len; --len) {
    *dest++ = *src++;
  }
}

inline void memset(void *dest, uint8_t val, uint32_t len) {
  uint8_t *dst = (uint8_t *)dest;
  for(; len; --len) {
    *dst++ = val;
  }
}

inline void bzero(void *dest, uint32_t len) {
  uint8_t *dst = (uint8_t *)dest;
  for(; len; --len) {
    *dst++ = 0;
  }
}

inline int strcmp(const char *lhs, const char *rhs) {
  int i;
  for(i = 0; lhs[i] != '\0' && rhs[i] !='\0'; ++i) {
    if(lhs[i] == rhs[i])
      continue;
    if(lhs[i] > rhs[i])
      return 1;
    if(lhs[i] < rhs[i])
      return -1;
  }
  if(lhs[i] != '\0')
    return 1;
  if(rhs[i] != '\0')
    return -1;
  return 0;
}

inline int strlen(const char *src) {
  int cnt = 0;
  while(*src++ != '\0') {
    cnt++;
  }
  return cnt;
}

inline char *strcat(char *dest, const char *src) {
  int l_len = strlen(dest);
  int r_len = strlen(src);
  memcpy(dest + l_len, src, r_len);
  return dest;
}