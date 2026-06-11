#ifndef VFS_H
#define VFS_H

enum num_syscall
{
    SYS_OPEN,
    SYS_CLOSE,
    SYS_READ,
    SYS_WRITE,
    SYS_CREATE
};

int vfs_init(void);
void vfs_task(void *arg);

#endif // VFS_H