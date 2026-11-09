.text
.globl enter_ia32, return_ia32
enter_ia32:
    push %rbx
    push %rbp
    mov %rsp, %r8
    mov %rdi, %rsp
    mov %esi, %eax
    mov $0x2b, %ecx
    mov %cx, %ds
    mov %cx, %es
    pushq $0x23
    push %rdx
    lretq
return_ia32:
    mov %r8,%rsp
    pop %rbp
    pop %rbx
    ret
.section .note.GNU-stack,"",@progbits
