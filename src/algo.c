#include <algo.h>
#include <types.h>

int find_extreme_index(const int *arr, int len, cmp_fn cmp)
{
	if (arr == NULL || len <= 0)
		return -1;

	int idx = 0;
	for (int i = 1; i < len; i++) {
		if (cmp(arr[i], arr[idx])) {
			idx = i;
		}
	}
	return idx;
}

// 使用
int min_cmp(int a, int b) { return a < b; }
int max_cmp(int a, int b) { return a > b; }