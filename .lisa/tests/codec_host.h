/* Host-only definitions for testing the unmodified codec implementation. */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define EXPORT_SYMBOL(x)
#define MODULE_LICENSE(x)
