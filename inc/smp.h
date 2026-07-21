#ifndef SMP_H
#define SMP_H

#include <types.h>

int smp_init(void);
__u32 __get_current_apic_id(void);

#endif /* SMP_H */