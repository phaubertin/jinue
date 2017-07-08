#include <jinue/descriptors.h>
#include <hal/cpu.h>
#include <hal/cpu_data.h>
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
        thread_context_t *to_ctx);

/* For each thread, a page is allocated which contains three things:
 *  - The thread structure (thread_t);
 *  - The message buffer in which messages sent from this thread are copied; and
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
 *  |                                   |   + THREAD_CONTEXT_MESSAGE_OFFSET
 *  |                                   |   + JINUE_SEND_MAX_SIZE
 *  |           Message buffer          |
 *  |                                   |
 *  |                                   |
 *  +-----------------------------------+ thread
 *  |               unused              |   + THREAD_CONTEXT_MESSAGE_OFFSET
 *  +-----------------------------------+ thread
 *  |                                   |   + sizeof(thread_t)
 *  |     Thread context structure      |
 *  |             (thread_t)            |
 *  |                                   |
 *  +-----------------------------------+ thread
 *
 * The start of this page, and from there the thread structure, message buffer
 * and kernel stack base, can be found quickly by masking the least significant
 * bits of the stack pointer (with THREAD_CONTEXT_MASK).
 */

thread_t *thread_page_create(
        addr_space_t    *addr_space,
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

        /* initialize fields
         *
         * A NULL saved stack pointer indicates to the thread context switching code
         * that this is a new thread that needs to return directly to user space. */
        thread_context_t *thread_ctx = &thread->thread_ctx;
         
        thread_ctx->saved_stack_pointer = NULL;
        thread_ctx->addr_space          = addr_space;
        thread_ctx->local_storage_addr  = NULL;

        /* setup stack for initial return to user space */
        uint32_t *kernel_stack_base = (uint32_t *)get_kernel_stack_base(thread_ctx);

        kernel_stack_base[-1]   = 8 * GDT_USER_DATA + 3;    /* user stack segment (ss), rpl = 3 */
        kernel_stack_base[-2]   = (uint32_t)user_stack;     /* user stack pointer (esp) */
        kernel_stack_base[-3]   = 2;                        /* flags register (eflags) */
        kernel_stack_base[-4]   = 8 * GDT_USER_CODE + 3;    /* user  code segment (cs), rpl/cpl = 3 */
        kernel_stack_base[-5]   = (uint32_t)entry;          /* entry point */
    }

    return thread;
}

void thread_context_switch(thread_context_t *from_ctx, thread_context_t *to_ctx) {
    /** ASSERTION: to_ctx argument must not be NULL */
    assert(to_ctx != NULL);

    /* nothing to do if this is already the current thread */
    if(from_ctx != to_ctx) {
        /* Current thread context is NULL on boot before we switch to the first
         * thread. */
        if(from_ctx == NULL || from_ctx->addr_space != to_ctx->addr_space) {
            vm_switch_addr_space(to_ctx->addr_space);
        }

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
        thread_context_switch_stack(from_ctx, to_ctx);
    }
}
