#include <jinue/asm/descriptors.h>
#include <hal/asm/thread.h>

    bits 32
    
    extern in_kernel
    extern thread_page_destroy

; ------------------------------------------------------------------------------
; FUNCTION: thread_context_switch_stack
; C PROTOTYPE: void thread_context_switch_stack(
;               thread_context_t *from_ctx,
;               thread_context_t *to_ctx,
;               bool destroy_from);
; ------------------------------------------------------------------------------
    global thread_context_switch_stack
thread_context_switch_stack:
    ; System V ABI calling convention: these four registers must be preserved
    ;
    ; We must store these registers whether we are actually using them here or
    ; not since we are about to switch to another thread that will.
    push ebp
    mov ebp, esp    ; setup frame pointer
    push ebx
    push esi
    push edi
    
    ; At this point, the stack looks like this:
    ;
    ; esp+28  destroy_from boolean (third function argument)
    ; esp+24  to thread context pointer (second function argument)
    ; esp+20  from thread context pointer (first function argument)
    ; esp+16  return address
    ; esp+12  ebx
    ; esp+ 8  esi
    ; esp+ 4  edi
    ; esp+ 0  ebp
    
    ; retrieve the from thread context argument
    mov edi, [esp+20]   ; from thread context (first function argument)

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
    mov esi, [esp+24]   ; to thread context (second function argument)
    
    ; This is where we actually switch thread by loading the stack pointer.
    mov esp, [esi]      ; saved stack pointer is the first member
    
    ; Now that we switched stack, see if the caller requested the from thread
    ; context be destroyed.
    mov eax, [esp+28]   ; destroy_from boolean (third function argument)
    or eax, eax
    jz .skip_destroy
    
    ; destroy from thread context
    and edi, THREAD_CONTEXT_MASK
    push edi
    call thread_page_destroy
    
    ; cleanup thread_page_destroy() arguments from stack
    add esp, 4

.skip_destroy:
    pop edi
    pop esi
    pop ebx
    pop ebp
    
    ret
