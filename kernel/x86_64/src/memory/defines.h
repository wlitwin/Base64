#ifndef __X86_64_MEMORY_DEFINES_H__
#define __X86_64_MEMORY_DEFINES_H__

#define _1_KIB 1024ULL
#define _2_KIB (2*_1_KIB)
#define _4_KIB (4*_1_KIB)
#define _1_MIB (1024ULL*_1_KIB)
#define _2_MiB (2ULL*_1_MIB)
#define _512_MiB (512ULL*_1_MIB)
#define _1_GIB (1024ULL*_1_MIB)
#define _512_GIB (512ULL*_1_GIB)

#define PG_FLAG_RW 0x2
#define PG_FLAG_USER 0x4
#define PG_FLAG_PWT 0x8
#define PG_FLAG_PCD 0x10
#define PG_FLAG_XD 0x8000000000000000

#define PAGE_SMALL 0x1
#define PAGE_LARGE 0x2

#define PAGE_SMALL_SIZE 0x1000
#define PAGE_LARGE_SIZE 0x200000

#define PDT_PER_PDPT 512ULL
#define PDT_ENTRIES 512ULL
#define PDPT_ENTRIES 512ULL
#define PML4_ENTRIES 512ULL

#define ALIGN_2MIB(X) (((X) & 0xFFFFFFFFFFE00000) + ((((X) & 0x1FFFFF) > 0) * _2_MiB))
#define ALIGN_4KIB(X) (((X) & 0xFFFFFFFFFFFFF000) + ((((X) & 0xFFF) > 0) * _4_KIB))

#define MASK_2MIB(X) (((uint64_t)X) & 0xFFFFFFFFFFE00000)
#define MASK_4KIB(X) (((uint64_t)X) & 0xFFFFFFFFFFFFF000)

#define PT_PRESENT 0x1
#define PT_WRITABLE 0x2

#define PDT_PRESENT 0x1
#define PDT_WRITABLE 0x2
#define PDT_PAGE_SIZE 0x80

#define PDPT_PRESENT 0x1
#define PDPT_WRITABLE 0x2

#define PML4_PRESENT 0x1
#define PML4_WRITABLE 0x2

#define PG_SAFE_FLAGS (PG_FLAG_RW | PG_FLAG_USER | PG_FLAG_PWT | PG_FLAG_PCD | PG_FLAG_XD)

#endif
