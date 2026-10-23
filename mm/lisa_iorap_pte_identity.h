/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LISA_IORAP_PTE_IDENTITY_H
#define LISA_IORAP_PTE_IDENTITY_H
/* A growing end may preserve the entire original VMA. Never accept shrink,
 * moved start, anon identity change, file replacement or unreadable memory.
 */
static inline int lisa_iorap_pte_vma_covers(unsigned long old_start,
 unsigned long old_end, unsigned long start, unsigned long end,
 int same_anon, int file_backed, int readable)
{
 return old_start < old_end && start == old_start && end >= old_end &&
  same_anon && !file_backed && readable;
}
#endif
