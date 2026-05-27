#ifndef ALIGNES_H
#define ALIGNES_H

#include <stdint.h>

#define ALIGNED_UP(val, align) (((uint64_t)(val) + (align)) & ~((align) - 1))

#endif // ALIGNES_H