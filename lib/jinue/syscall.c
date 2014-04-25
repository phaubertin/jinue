#include <jinue/syscall.h>
#include <stddef.h>


syscall_stub_t syscall = NULL;


int get_syscall_method(void) {
    return syscall_intr(NULL, NULL, SYSCALL_FUNCT_SYSCALL_METHOD, NULL, NULL);
}

int set_syscall_method(void) {
    int method = get_syscall_method();
    syscall = syscall_stubs[method];
    
    return method;
}

void set_errno_addr(int *perrno) {
    (void)syscall(
        NULL,
        NULL,
        SYSCALL_FUNCT_SET_ERRNO_ADDR,
        (unsigned int)perrno,
        NULL );
}
