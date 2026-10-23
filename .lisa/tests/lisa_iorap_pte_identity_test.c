#include <assert.h>
#include <stdio.h>
#include "../../mm/lisa_iorap_pte_identity.h"
int main(void) {
 assert(lisa_iorap_pte_vma_covers(4096,8192,4096,8192,1,0,1));
 assert(lisa_iorap_pte_vma_covers(4096,8192,4096,16384,1,0,1));
 assert(!lisa_iorap_pte_vma_covers(4096,8192,4096,6144,1,0,1));
 assert(!lisa_iorap_pte_vma_covers(4096,8192,0,16384,1,0,1));
 assert(!lisa_iorap_pte_vma_covers(4096,8192,4096,16384,0,0,1));
 assert(!lisa_iorap_pte_vma_covers(4096,8192,4096,16384,1,1,1));
 assert(!lisa_iorap_pte_vma_covers(4096,8192,4096,16384,1,0,0));
 assert(!lisa_iorap_pte_vma_covers(8192,4096,8192,16384,1,0,1));
 puts("PASS PTE original-range coverage: equal/growing end; reject shrink/moved start/anon replacement/file/unreadable/invalid original range");
}
