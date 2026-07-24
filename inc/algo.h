#ifndef ALGO_H
#define ALGO_H

typedef int (*cmp_fn)(int a, int b);

int find_extreme_index(const int *arr, int len, cmp_fn cmp);

int min_cmp(int a, int b);
int max_cmp(int a, int b);

#endif /* ALGO_H */