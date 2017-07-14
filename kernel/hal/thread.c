#include <jinue/descriptors.h>
#include <hal/cpu.h>
#include <hal/cpu_data.h>
#include <hal/irq.h>
#include <hal/kernel.h>
#include <hal/thread.h>
#include <hal/vm.h>
#include <assert.h>
#include <pfalloc.h>
#include <stddef.h>
#include <vm_alloc.h>


/* defined in thread.asm */
void thread_context_switch_stack(
        thread_context_t *from_ctx,
        thread_context_t *to_ctx,
        bool destroy_from);

/* For each thread, a page is allocated which contains:
 *  - The thread structure (thread_t); and
 *  - This thread's kernel stack.
 *
 * Switching thread context (see thread_context_switch()) basically means
 * switching the kernel stack.
 *
 * The layout of this page is as follow:
 *
 *  +--------v-----------------v--------+ thread
 *  |                                   |   +(THREAD_CONTEXT_SIZE == PAGE_SIZE)
 *  |                                   |
 *  |                                   |
 *  |            Kernel stack           |
 *  |                                   |
 *  |                                   |
 *  |                                   |
 *  +-----------------------------------+ thread
 *  |                                   |   + sizeof(thread_t)
 *  |          Thread structure         |
 *  |             (thread_t)            |
 *  |                                   |
 *  +-----------------------------------+ thread
 *
 * The start of this page, and from there the thread structure, and kernel stack
 * base, can be found quickly by masking the least significant bits of the stack
 * pointer (with THREAD_CONTEXT_MASK).
 *
 * All members of the thread structure (thread_t) used by the HAL are grouped
 * in the thread context (sub-)structure (thread_context_t).
 */

thread_t *thread_page_create(
        addr_t           entry,
        addr_t           user_stack) {

    /* allocate thread context */
    thread_t *thread = (thread_t *)vm_alloc( global_page_allocator );

    if(thread != NULL) {
        pfaddr_t pf = pfalloc();

        if(pf == PFNULL) {
            vm_free(global_page_allocator, (addr_t)thread);
            return NULL;
        }

        vm_map_global((addr_t)thread, pf, VM_FLAG_KERNEL | VM_FLAG_READ_WRITE | VM_FLAG_GLOBAL);

        /* initialize fields */
        thread_context_t *thread_ctx = &thread->thread_ctx;
        
        thread_ctx->local_storage_addr  = NULL;

        /* setup stack for initial return to user space */
        uint32_t *kernel_stack_base = (uint32_t *)get_kernel_stack_base(thread_ctx);
        int idx = 0;

        /* the following values are put on the stack for use by the iret instruction */
        kernel_stack_base[--idx]   = 8 * GDT_USER_DATA + 3;     /* user stack segment (ss), rpl = 3 */
        kernel_stack_base[--idx]   = (uint32_t)user_stack;      /* user stack pointer (esp) */
        kernel_stack_base[--idx]   = 2;                         /* flags register (eflags) */
        kernel_stack_base[--idx]   = 8 * GDT_USER_CODE + 3;     /* user code segment (cs), rpl/cpl = 3 */
        kernel_stack_base[--idx]   = (uint32_t)entry;           /* user code entry point */
        
        /* the following values are popped by return_from_interrupt() */
        kernel_stack_base[--idx]   = 0;                         /* ebp */
        kernel_stack_base[--idx]   = 8 * GDT_USER_DATA + 3;     /* gs: user data segment, rpl = 3 */
        kernel_stack_base[--idx]   = 8 * GDT_USER_DATA + 3;     /* fs: user data segment, rpl = 3 */
        kernel_stack_base[--idx]   = 8 * GDT_USER_DATA + 3;     /* es: user data segment, rpl = 3 */
        kernel_stack_base[--idx]   = 8 * GDT_USER_DATA + 3;     /* ds: user data segment, rpl = 3 */
        kernel_stack_base[--idx]   = 0;                         /* ecx */
        kernel_stack_base[--idx]   = 0;                         /* edx */
        kernel_stack_base[--idx]   = 0;                         /* edi */
        kernel_stack_base[--idx]   = 0;                         /* esi */
        kernel_stack_base[--idx]   = 0;                         /* ebx */
        kernel_stack_base[--idx]   = 0;                         /* eax */        
        kernel_stack_base[--idx]   = 0;                         /* no error code */
        kernel_stack_base[--idx]   = 0;                         /* in_kernel */
        
        /* this is the address thread_context_switch_stack() will return to */
        kernel_stack_base[--idx]   = (uint32_t)return_from_interrupt;
        
        /* the following values are popped by address thread_context_switch_stack()
         * as part of its cleanup before returning (saved registers). */
        kernel_stack_base[--idx]   = 0;                         /* ebx */
        kernel_stack_base[--idx]   = 0;                         /* esi */
        kernel_stack_base[--idx]   = 0;                         /* edi */
        kernel_stack_base[--idx]   = 0;                         /* ebp */
        
        /* set thread stack pointer */
        thread_ctx->saved_stack_pointer = (addr_t)&kernel_stack_base[idx];
    }

    return thread;
}

void thread_page_destroy(thread_t *thread) {
    pfaddr_t pfaddr = vm_lookup_pfaddr(NULL, (addr_t)thread);
    vm_unmap_global((addr_t)thread);
    vm_free(global_page_allocator, (addr_t)thread);
    pffree(pfaddr);
}

void thread_context_switch(
        thread_context_t    *from_ctx,
        thread_context_t    *to_ctx,
        bool                 destroy_from) {

    /** ASSERTION: to_ctx argument must not be NULL */
    assert(to_ctx != NULL);
    
    /** ASSERTION: from_ctx argument must not be NULL if destroy_from is true */
    assert(from_ctx != NULL || ! destroy_from);

    /* nothing to do if this is already the current thread */
    if(from_ctx != to_ctx) {
        /* setup TSS with kernel stack base for this thread context */
        addr_t kernel_stack_base = get_kernel_stack_base(to_ctx);
        tss_t *tss = get_tss();

        tss->esp0 = kernel_stack_base;
        tss->esp1 = kernel_stack_base;
        tss->esp2 = kernel_stack_base;

        /* update kernel stack address for SYSENTER instruction */
        if(cpu_has_feature(CPU_FEATURE_SYSENTER)) {
            wrmsr(MSR_IA32_SYSENTER_ESP, (uint64_t)(uintptr_t)kernel_stack_base);
        }

        /* switch thread context stack */
        thread_context_switch_stack(from_ctx, to_ctx, destroy_from);
    }
}
