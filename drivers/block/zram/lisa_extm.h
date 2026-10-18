/* SPDX-License-Identifier: GPL-2.0 */
/* Lisa ExtM age levels: see .lisa/hyperos-extm-abi.md for source provenance. */
#ifndef LISA_ZRAM_EXTM_H
#define LISA_ZRAM_EXTM_H
#define LISA_EXTM_LEVELS 10U
#define LISA_EXTM_AGE_MASK (15UL << LISA_EXTM_AGE_SHIFT)
struct lisa_extm_request {
 unsigned long max_pages;
 unsigned int min_age;
 bool idle;
};
static inline unsigned int lisa_extm_age(unsigned long flags)
{
 return (flags & LISA_EXTM_AGE_MASK) >> LISA_EXTM_AGE_SHIFT;
}
static inline unsigned long lisa_extm_clear_age(unsigned long flags)
{
 return flags & ~LISA_EXTM_AGE_MASK;
}
static inline unsigned long lisa_extm_mark(unsigned long flags)
{
 unsigned int age = lisa_extm_age(flags);
 if (age < LISA_EXTM_LEVELS)
  age++;
 return lisa_extm_clear_age(flags) | ((unsigned long)age << LISA_EXTM_AGE_SHIFT);
}
static inline bool lisa_extm_space(char c)
{
 return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
/* Bounded decimal parser: no allocation, truncation or partial output on error. */
static inline bool lisa_extm_parse(const char *s, unsigned long len,
                                  struct lisa_extm_request *out)
{
 struct lisa_extm_request req = { ~0UL, 1, true };
 unsigned long p = 4, values[2], n = 0;
 if (len < 4 || len > 128)
  return false;
 if (s[0] == 'h' && s[1] == 'u' && s[2] == 'g' && s[3] == 'e')
  req.idle = false;
 else if (!(s[0] == 'i' && s[1] == 'd' && s[2] == 'l' && s[3] == 'e'))
  return false;
 if (p < len && !lisa_extm_space(s[p]))
  return false;
 while (p < len) {
  unsigned long value = 0;
  while (p < len && lisa_extm_space(s[p]))
   p++;
  if (p == len)
   break;
  if (!req.idle || n == 2 || s[p] < '0' || s[p] > '9')
   return false;
  do {
   unsigned int digit = s[p++] - '0';
   if (value > (~0UL - digit) / 10)
    return false;
   value = value * 10 + digit;
  } while (p < len && s[p] >= '0' && s[p] <= '9');
  if (p < len && !lisa_extm_space(s[p]))
   return false;
  values[n++] = value;
 }
 if (n)
  req.max_pages = values[0];
 if (n > 1)
  req.min_age = values[1] > LISA_EXTM_LEVELS ? LISA_EXTM_LEVELS : values[1];
 *out = req;
 return true;
}
#endif
