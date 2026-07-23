#ifndef SMP_H
#define SMP_H

#include <types.h>

#ifndef MAX_CPUS
#define MAX_CPUS 1
#endif /* MAX_CPUS */

int smp_init(void);
uint64_t get_bsp_id(void);
uint32_t *get_ap_ids(void);
__u32 __get_current_apic_id(void);

#define get_current_cpuid() __get_current_apic_id()

#endif /* SMP_H */