#include <bitmap.h>

// 初始化位图
void bitmap_init(bitmap_t *bmp, uint64_t *data, size_t num_bits)
{
	bmp->size = num_bits;
	bmp->map = data;

	// 清空整个位图
	size_t words = (num_bits + 63) / 64;
	for (size_t i = 0; i < words; i++) {
		data[i] = 0;
	}
}

// 设置特定位
void bitmap_set(bitmap_t *bmp, size_t bit)
{
	if (bit >= bmp->size)
		return;

	size_t word_idx = bit / 64;
	size_t bit_idx = bit % 64;

	bmp->map[word_idx] |= (1ULL << bit_idx);
}

// 清除特定位
void bitmap_clear(bitmap_t *bmp, size_t bit)
{
	if (bit >= bmp->size)
		return;

	size_t word_idx = bit / 64;
	size_t bit_idx = bit % 64;

	bmp->map[word_idx] &= ~(1ULL << bit_idx);
}

// 测试特定位是否设置
int bitmap_test(const bitmap_t *bmp, size_t bit)
{
	if (bit >= bmp->size)
		return 0;

	size_t word_idx = bit / 64;
	size_t bit_idx = bit % 64;

	return (bmp->map[word_idx] & (1ULL << bit_idx)) != 0;
}

// 查找第一个清零的位（从start开始）
int bitmap_find_first_zero(bitmap_t *bmp, size_t start)
{
	size_t word_start = start / 64;
	size_t bit_start = start % 64;

	for (size_t i = word_start; i < (bmp->size + 63) / 64; i++) {
		uint64_t word = bmp->map[i];

		// 如果整个字都是1，跳到下一个字
		if (word == UINT64_MAX) {
			continue;
		}

		// 从起始位开始查找
		for (size_t j = (i == word_start ? bit_start : 0); j < 64;
		     j++) {
			size_t bit_pos = i * 64 + j;
			if (bit_pos >= bmp->size)
				break;

			if (!(word & (1ULL << j))) {
				return bit_pos;
			}
		}
	}

	return -1; // 没有找到
}

// 查找第一个设置的位（从start开始）
int bitmap_find_first_set(bitmap_t *bmp, size_t start)
{
	size_t word_start = start / 64;
	size_t bit_start = start % 64;

	for (size_t i = word_start; i < (bmp->size + 63) / 64; i++) {
		uint64_t word = bmp->map[i];

		// 如果整个字都是0，跳到下一个字
		if (word == 0) {
			continue;
		}

		// 从起始位开始查找
		for (size_t j = (i == word_start ? bit_start : 0); j < 64;
		     j++) {
			size_t bit_pos = i * 64 + j;
			if (bit_pos >= bmp->size)
				break;

			if (word & (1ULL << j)) {
				return bit_pos;
			}
		}
	}

	return -1; // 没有找到
}

// 分配一个连续的位块
int bitmap_alloc_range(bitmap_t *bmp, size_t count, size_t start)
{
	if (count == 0)
		return -1;

	for (size_t i = start; i <= bmp->size - count; i++) {
		int found = 1;

		// 检查是否有连续的count个零位
		for (size_t j = 0; j < count; j++) {
			if (bitmap_test(bmp, i + j)) {
				found = 0;
				break;
			}
		}

		if (found) {
			// 设置这count个位
			for (size_t j = 0; j < count; j++) {
				bitmap_set(bmp, i + j);
			}
			return i; // 返回起始位置
		}
	}

	return -1; // 没有找到足够的连续空间
}

// 释放一个连续的位块
void bitmap_free_range(bitmap_t *bmp, size_t start, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (start + i < bmp->size) {
			bitmap_clear(bmp, start + i);
		}
	}
}

// 统计已设置的位数
size_t bitmap_count_set(bitmap_t *bmp)
{
	size_t count = 0;
	size_t total_words = (bmp->size + 63) / 64;

	for (size_t i = 0; i < total_words; i++) {
		uint64_t word = bmp->map[i];
		if (i == total_words - 1 && bmp->size % 64 != 0) {
			// 处理最后一个不完整的字
			size_t valid_bits = bmp->size % 64;
			uint64_t mask = (1ULL << valid_bits) - 1;
			word &= mask;
		}

		// 手动计算置位数
		while (word) {
			count++;
			word &= word - 1;
		}
	}

	return count;
}

// 获取位图使用率百分比
int bitmap_usage_percent(bitmap_t *bmp)
{
	size_t total = bmp->size;
	size_t used = bitmap_count_set(bmp);
	return (int)((used * 100) / total);
}

// 原子操作版本（用于多核环境）
void bitmap_set_atomic(bitmap_t *bmp, size_t bit)
{
	if (bit >= bmp->size)
		return;

	size_t word_idx = bit / 64;
	size_t bit_idx = bit % 64;
	uint64_t mask = 1ULL << bit_idx;

	__atomic_fetch_or(&bmp->map[word_idx], mask, __ATOMIC_SEQ_CST);
}

void bitmap_clear_atomic(bitmap_t *bmp, size_t bit)
{
	if (bit >= bmp->size)
		return;

	size_t word_idx = bit / 64;
	size_t bit_idx = bit % 64;
	uint64_t mask = ~(1ULL << bit_idx);

	__atomic_fetch_and(&bmp->map[word_idx], mask, __ATOMIC_SEQ_CST);
}

// 批量操作：设置多个连续位
void bitmap_set_range(bitmap_t *bmp, size_t start, size_t count)
{
	for (size_t i = 0; i < count && (start + i) < bmp->size; i++) {
		bitmap_set(bmp, start + i);
	}
}

// 批量操作：清除多个连续位
void bitmap_clear_range(bitmap_t *bmp, size_t start, size_t count)
{
	for (size_t i = 0; i < count && (start + i) < bmp->size; i++) {
		bitmap_clear(bmp, start + i);
	}
}