#ifndef TSC_H
#define TSC_H

#include <types.h>

int tsc_init(void);
// ===== 对外接口 =====

uint64_t tsc_read(void);
uint64_t tsc_read_ns(void);
uint64_t tsc_read_us(void);
uint64_t tsc_read_ms(void);
// 精确忙等
void tsc_delay_us(uint64_t us);
void tsc_delay_ns(uint64_t ns);

#endif /* TSC_H */