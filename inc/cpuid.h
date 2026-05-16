#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>

typedef enum
{
    // Standard Function Numbers
    CPUID_VENDOR_INFO = 0x00000000,
    CPUID_FEATURE_INFO = 0x00000001,
    CPUID_CACHE_INFO = 0x00000002,
    CPUID_PROCESSOR_SERIAL = 0x00000003, // Deprecated
    CPUID_DETERMINISTIC_CACHE_PARAMS = 0x00000004,
    CPUID_MONITOR_MWAIT_PARAMS = 0x00000005,
    CPUID_THERMAL_POWER_MANAGEMENT = 0x00000006,
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS = 0x00000007,
    CPUID_RESERVED_8 = 0x00000008,
    CPUID_DIRECT_CACHE_ACCESS_INFO = 0x00000009,
    CPUID_ARCHITECTURAL_PERFORMANCE_MONITORING = 0x0000000A,
    CPUID_EXTENDED_TOPOLOGY = 0x0000000B,
    CPUID_EXTENDED_STATE_INFORMATION = 0x0000000D,
    CPUID_TRACENEXT_INFO = 0x0000000F,
    CPUID_QOS_MONITORING_ENUM = 0x00000010,
    CPUID_SOC_VENDOR_INFO = 0x00000017,
    CPUID_DETERMINISTIC_ADDRESS_TRANSLATION = 0x00000018,
    CPUID_V2_EXTENDED_TOPOLOGY = 0x0000001F,

    // Extended Function Numbers
    CPUID_EXTENDED_FUNCTION_MAX = 0x80000000,
    CPUID_EXTENDED_FEATURES = 0x80000001,
    CPUID_PROCESSOR_BRAND_STRING_1 = 0x80000002,
    CPUID_PROCESSOR_BRAND_STRING_2 = 0x80000003,
    CPUID_PROCESSOR_BRAND_STRING_3 = 0x80000004,
    CPUID_L1_CACHE_INFO = 0x80000005,
    CPUID_L2_L3_CACHE_INFO = 0x80000006,
    CPUID_ADVANCED_PM_INFO = 0x80000007,
    CPUID_VIRTUAL_PHYSICAL_ADDRESS_SIZES = 0x80000008,

    // Memory Encryption Function Numbers
    CPUID_MEMORY_ENCRYPTION = 0x8000001F,

    // Virtualization Function Numbers
    CPUID_HYPERVISOR_INFO = 0x40000000,

} cpuid_function_t;

struct cpuid_result_t
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
};

void cpuid(struct cpuid_result_t *c, uint32_t leaf);
void cpuid_ext(struct cpuid_result_t *c, uint32_t leaf, uint32_t subleaf);
int is_apic_supported(void);
int is_tsc_supported(void);
int is_cmov_supported(void);
int is_clflush_supported(void);
int is_mmx_supported(void);
int is_sse_supported(void);
int is_sse2_supported(void);

#endif // CPUID_H