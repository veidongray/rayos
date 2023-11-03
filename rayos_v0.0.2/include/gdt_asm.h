#ifndef __GDTASM_H__
#define __GDTASM_H__

#define GDTSEGASM(BASE, LIMIT, G, DoB, AVL, P, DPL, S, TYPE) \
.word (LIMIT & 0xffff), (BASE & 0xffff);\
.byte ((BASE >> 16) & 0xff), (((P << 7) | (DPL << 5) | (S << 4)) | TYPE),\
(((G << 7) | (DoB << 6) | 0 | (AVL << 4)) | ((LIMIT >> 16) & 0xf)),\
((BASE >> 24) & 0xff)

#endif // __GDTASM_H__