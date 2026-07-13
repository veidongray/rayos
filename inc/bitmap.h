#ifndef BITMAP_H
#define BITMAP_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	size_t size;   // 总位数
	uint64_t *map; // 位图数组指针
} bitmap_t;

// 初始化位图
void bitmap_init(bitmap_t *bmp, uint64_t *data, size_t num_bits);
// 设置特定位
void bitmap_set(bitmap_t *bmp, size_t bit);
// 清除特定位
void bitmap_clear(bitmap_t *bmp, size_t bit);
// 测试特定位是否设置
int bitmap_test(const bitmap_t *bmp, size_t bit);
// 查找第一个清零的位（从start开始）
int bitmap_find_first_zero(bitmap_t *bmp, size_t start);
// 查找第一个设置的位（从start开始）
int bitmap_find_first_set(bitmap_t *bmp, size_t start);
// 分配一个连续的位块
int bitmap_alloc_range(bitmap_t *bmp, size_t count, size_t start);
// 释放一个连续的位块
void bitmap_free_range(bitmap_t *bmp, size_t start, size_t count);
// 统计已设置的位数
size_t bitmap_count_set(bitmap_t *bmp);
// 获取位图使用率百分比
int bitmap_usage_percent(bitmap_t *bmp);
// 原子操作版本（用于多核环境）
void bitmap_set_atomic(bitmap_t *bmp, size_t bit);
void bitmap_clear_atomic(bitmap_t *bmp, size_t bit);
// 批量操作：设置多个连续位
void bitmap_set_range(bitmap_t *bmp, size_t start, size_t count);
// 批量操作：清除多个连续位
void bitmap_clear_range(bitmap_t *bmp, size_t start, size_t count);

#endif /* BITMAP_H */