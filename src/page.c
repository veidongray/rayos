#include "page.h"
#include "print.h"

static uint32_t global_page_directory_entry[1024] __attribute__((aligned(4096)));
static uint32_t global_page_table_entry[1024][1024] __attribute__((aligned(4096)));

int page_init(void)
{
    uint32_t i, j, len, size;

    /* Init kernel page */
    size = (uint32_t)_kernel_end_aligned - (uint32_t)_kernel_start;
    len = (size / 0x400000) + ((size % 0x400000) ? 1 : 0);

    for (i = 0; i < len; ++i) {
        for (j = 0; j < 1024; ++j) {
            // 4MB per page == 0x400000 bytes
            global_page_table_entry[i][j] = ((i * 0x400000) + (j * 0x1000)) | 0x3;
        }
    }

    for (i = 0; i < 1024; ++i) {
        // fill global page directory
        global_page_directory_entry[i] = 0x00000000 | 0x2;
    }

    for (i = 0; i < len; ++i) {
        // for 0x00000000 ~ [size]
        global_page_directory_entry[i] = (uint32_t)global_page_table_entry[i] | 0x3;
    }

    for (i = 0; i < len; ++i) {
        // for 0xC0000000 ~ 0xFFFFFFFF
        global_page_directory_entry[i + 768] = (uint32_t)global_page_table_entry[i] | 0x3;
    }
    
    // And this inside a function
    loadPageDirectory(global_page_directory_entry);
    enablePaging();

    cga_printf("Pageing:\n");
    for (i = 768; i < len + 768; i += 4) {
        cga_printf("PDE[%u]: 0x%X PDE[%u]: 0x%X PDE[%u]: 0x%X PDE[%u]: 0x%X\n",
            i, global_page_directory_entry[i],
            i + 1, global_page_directory_entry[i + 1],
            i + 2, global_page_directory_entry[i + 2],
            i + 3, global_page_directory_entry[i + 3]);
    }
    return 0;
}