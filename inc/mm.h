#ifndef MM_H
#define MM_H

#include <list.h>
#include <stddef.h>
#include <stdint.h>

struct vmap_area {
	uint64_t va_start;   // 虚拟地址起始位置
	uint64_t va_end;     // 虚拟地址结束位置
	uint64_t va_nrpages; // 总的占用的页数量
	struct list_head list;
};

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *krealloc(void *ptr, size_t new_size);
void kfree(void *virtaddr);
void mm_init(void);

#endif // MM_H