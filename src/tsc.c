#include <cpuid.h>
#include <printk.h>
#include <timer.h>
#include <types.h>

static uint64_t tsc_freq_hz;                 // TSC 频率（Hz）
static uint64_t tsc_freq_khz;                // TSC 频率（kHz）
static uint32_t invariant_tsc_supported = 0; // 为 0 则不支持

static inline uint64_t rdtsc(void)
{
	uint32_t lo, hi;
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t rdtscp(void)
{
	uint32_t lo, hi, aux;
	asm volatile("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
	return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t tsc_calibrate_lapic(void)
{
	// 用 500ms 提高精度（误差从 ~1% 降到 ~0.2%）
	const int delay_ms = 500;

	// 序列化起点
	uint32_t lo, hi;
	asm volatile("lfence\n\trdtsc" : "=a"(lo), "=d"(hi));
	uint64_t t1 = ((uint64_t)hi << 32) | lo;

	mdelay(delay_ms);

	// 序列化终点
	asm volatile("rdtscp" : "=a"(lo), "=d"(hi)::"ecx");
	uint64_t t2 = ((uint64_t)hi << 32) | lo;

	// 500ms → ×2 = 1s
	return (t2 - t1) * (1000 / delay_ms);
}

// CPUID leaf 0x15: TSC / Core Crystal Clock 比值
// CPUID leaf 0x16: CPU 基频 / 最大频率（MHz）
static inline uint64_t tsc_freq_from_cpuid(void)
{
	uint32_t eax, ebx, ecx, edx;

	// 方法 1：leaf 0x15（Skylake+）
	__cpuid(0x15, eax, ebx, ecx, edx);
	if (eax && ebx && ecx) {
		// TSC freq = ecx * ebx / eax
		return (uint64_t)ecx * ebx / eax;
	}

	// 方法 2：leaf 0x16（基频 MHz）
	__cpuid(0x16, eax, ebx, ecx, edx);
	if (eax) {
		return (uint64_t)(eax & 0xFFFF) * 1000000ULL;
	}

	return 0; // 读不到，需要方法 B
}

int tsc_init(void)
{
	// 1. 检查 Invariant TSC
	uint32_t eax, ebx, ecx, edx;
	__cpuid(0x80000007, eax, ebx, ecx, edx);
	if (!(edx & (1 << 8))) {
		printk("[TSC] WARNING: no invariant TSC");
		return -1;
	}
	invariant_tsc_supported = 1;

	// 2. 尝试 CPUID 直接读频率
	tsc_freq_hz = tsc_freq_from_cpuid();

	// 3. 读不到就 LAPIC Timer 校准
	if (tsc_freq_hz == 0) {
		tsc_freq_hz = tsc_calibrate_lapic();
	}

	if (tsc_freq_hz == 0) {
		printk("[TSC] ERROR: calibration failed\n");
		return -1;
	}

	tsc_freq_khz = tsc_freq_hz / 1000;

	printk("[TSC] frequency: %lu.%03lu MHz\n", tsc_freq_hz / 1000000,
	       (tsc_freq_hz % 1000000) / 1000);

	return 0;
}

// ===== 对外接口 =====

uint64_t tsc_read(void) { return rdtsc(); }

uint64_t tsc_read_ns(void)
{
	if (invariant_tsc_supported) {
		uint64_t t = rdtsc();
		return t * 1000000000ULL / tsc_freq_hz;
	} else {
		return 0;
	}
}

uint64_t tsc_read_us(void)
{
	if (invariant_tsc_supported) {
		uint64_t t = rdtsc();
		return t / tsc_freq_khz;
	} else {
		return 0;
	}
}

uint64_t tsc_read_ms(void)
{
	if (invariant_tsc_supported) {
		uint64_t t = rdtsc();
		return t / (tsc_freq_khz * 1000);
	} else {
		return 0;
	}
}

// 精确忙等
void tsc_delay_us(uint64_t us)
{
	if (invariant_tsc_supported) {
		uint64_t target = rdtsc() + us * tsc_freq_khz;
		while (rdtsc() < target)
			asm volatile("pause");
	}
}

void tsc_delay_ns(uint64_t ns)
{
	if (invariant_tsc_supported) {
		uint64_t cycles = ns * tsc_freq_hz / 1000000000ULL;
		uint64_t target = rdtsc() + cycles;
		while (rdtsc() < target)
			asm volatile("pause");
	}
}