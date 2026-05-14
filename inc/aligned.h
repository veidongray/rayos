#ifndef ALIGNED_H
#define ALIGNED_H

#include <stdint.h>

#define ALIGN_UP(x, align) (((x) + (align)) & ~((align) - 1))
#define ALIGN_4K(x) ALIGN_UP((x), 4096)
#define ALIGN_4B(x) ALIGN_UP((x), 4)

#define ALIGN_ATTR(align) __attribute__((aligned((align))))

#endif // ALIGNED_H