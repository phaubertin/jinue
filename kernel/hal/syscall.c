#include <printk.h>
#include <syscall.h>
#include <stddef.h>

int syscall_method;

syscall_funct_t hal_syscall_funct;


void hal_syscall_dispatch(syscall_params_t *syscall_params) {
    hal_syscall_funct(syscall_params);
}

void set_syscall_funct(syscall_funct_t syscall_funct) {
    if(syscall_funct == NULL) {
        hal_syscall_funct = default_syscall_funct;
    }
    else {
        hal_syscall_funct = syscall_funct;
    }
}

void default_syscall_funct(syscall_params_t *syscall_params) {
    
    printk("SYSCALL: ref 0x%x funct %u: arg1=%u(0x%x) arg2=%u(0x%x) method=%u(0x%x) \n",
        syscall_params->args.dest,
        syscall_params->args.funct,
        syscall_params->args.arg1,   syscall_params->args.arg1,
        syscall_params->args.arg2,   syscall_params->args.arg2,
        syscall_params->args.method, syscall_params->args.method );
}
