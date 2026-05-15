#include "cpuid.h"

void cpuid(struct cpuid_result_t *c, uint32_t leaf)
{
    asm volatile(
        "cpuid"
        : "=a"(c->eax), "=b"(c->ebx),
          "=c"(c->ecx), "=d"(c->edx)
        : "a"(leaf), "c"(0));
}

void cpuid_ext(struct cpuid_result_t *c, uint32_t leaf, uint32_t subleaf)
{
    asm volatile(
        "cpuid"
        : "=a"(c->eax), "=b"(c->ebx),
          "=c"(c->ecx), "=d"(c->edx)
        : "a"(leaf), "c"(subleaf));
}

int is_apic_supported(void)
{
    struct cpuid_result_t c;

    cpuid(&c, CPUID_FEATURE_INFO);

    if (c.edx & (1 << 9))
        return 1;
    return 0;
}

int is_tsc_supported(void)
{
    struct cpuid_result_t c;

    cpuid(&c, CPUID_FEATURE_INFO);

    if (c.edx & (1 << 4))
        return 1;
    return 0;
}

int is_cmov_supported(void)
{
    struct cpuid_result_t c;

    cpuid(&c, CPUID_FEATURE_INFO);

    if (c.edx & (1 << 15))
        return 1;
    return 0;
}

int is_clflush_supported(void)
{
    struct cpuid_result_t c;

    cpuid(&c, CPUID_FEATURE_INFO);

    if (c.edx & (1 << 19))
        return 1;
    return 0;
}

int is_mmx_supported(void)
{
    struct cpuid_result_t c;

    cpuid(&c, CPUID_FEATURE_INFO);

    if (c.edx & (1 << 23))
        return 1;
    return 0;
}

int is_sse_supported(void)
{
    struct cpuid_result_t c;

    cpuid(&c, CPUID_FEATURE_INFO);

    if (c.edx & (1 << 25))
        return 1;
    return 0;
}

int is_sse2_supported(void)
{
    struct cpuid_result_t c;

    cpuid(&c, CPUID_FEATURE_INFO);

    if (c.edx & (1 << 26))
        return 1;
    return 0;
}