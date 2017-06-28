#include <jinue/asm/descriptors.h>
#include <hal/asm/thread.h>

    bits 32
    
    extern in_kernel

; ------------------------------------------------------------------------------
; FUNCTION: thread_context_switch_stack
; C PROTOTYPE: void thread_context_switch_stack(
;               thread_context_t *from_ctx,
;               thread_context_t *to_ctx)
; ------------------------------------------------------------------------------
    global thread_context_switch_stack
thread_context_switch_stack:
    ; System V ABI calling convention: these four registers must be preserved
    ;
    ; We must store these registers whether we are actually using them here or
    ; not since we are about to switch to another thread that will.
    push ebx
    push esi
    push edi
    push ebp
    
    ; At this point, the stack looks like this:
    ;
    ; esp+24  to thread context pointer (second function argument)
    ; esp+20  from thread context pointer (first function argument)
    ; esp+16  return address
    ; esp+12  ebx
    ; esp+ 8  esi
    ; esp+ 4  edi
    ; esp+ 0  ebp
    
    ; retrieve the from thread context argument
    mov edi, [esp+20]

    ; On the first thread context switch after boot, the kernel is using a
    ; temporary stack and the from/current thread context is NULL. Skip saving
    ; the current stack pointer in that case.
    or edi, edi
    jz .do_switch

    ; store the current stack pointer in the first member of the thread context
    ; structure
    mov [edi], esp

.do_switch:
    ; load the saved stack pointer from the thread context to which we are
    ; switching
    mov esi, [esp+24]   ; thread context pointer (first function argument)
    mov eax, [esi]      ; saved stack pointer is the first member
    
    ; If the saved stack pointer is NULL, this is a new thread that never ran
    ; before and that will start by "returning" directly to user space.
    or eax, eax
    jz .new_thread
    
    ; This is where we actually switch thread by loading the stack pointer.
    mov esp, eax

    pop ebp
    pop edi
    pop esi
    pop ebx
    
    ret
    
.new_thread:
    ; leaving the kernel
    mov [in_kernel], dword 0
    
    ; set user space data segment selectors
    mov eax, 8 * GDT_USER_DATA + 3  ; user data segment, rpl=3
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    
    ; The thread context creation code has already setup the stack with the
    ; following data for an interrupt return to user space with iretd:
    ;
    ;   - user stack segment (ss)
    ;   - user stack pointer (esp)
    ;   - flags register (eflags)
    ;   - code segment (cs)
    ;   - return address (entry point)
    ;
    ; ... for a total of 20 bytes on the stack.
    add esi, THREAD_CONTEXT_SIZE - 20
    mov esp, esi
    
    ; clear registers
    xor eax, eax
    mov ebx, eax
    mov ecx, eax
    mov edx, eax
    mov esi, eax
    mov edi, eax
    mov ebp, eax
    
    iretd
