## 开发顺序总结（清单式）
1.搭建交叉编译环境 + QEMU  
2.编写 MBR（实模式，加载 Loader）  
3.实现保护模式切换（GDT + CR0）  
4.从汇编跳转到 C 语言 main()  
5.实现屏幕打印（VGA）  
6.设置 IDT + 异常处理  
7.初始化 PIC + 时钟中断  
8.实现简易内存分配器  
9.实现任务上下文切换  
10.添加键盘输入、系统调用等  

## memory manager
描述的是kmalloc和kfree的实现，实现思路是创建所有可用内存的八分之一空间来作为kmalloc的cache，kmalloc的所有空间都从cache里分配；  
每一个分配的空间末尾都有一个struct mm_area结构体描述该内存空间的起始地址以及大小，并且使用list进行管理；  
底层需要alloc_page()的支持，所以page的初始化必须在mm之前；  
目前存在最大的问题是内存碎片问题，后期有需要再考虑解决方案。

## test
编译的代码必须0 Errors和0 Warnings；  
所有的修改必须通过qemu以及VirtualBox的进行多次重复的运行验证，有其中之一不能正常运行则不采用修改方案。