.text
.globl _start, map_fixture, fail_test
_start:
    push %eax
    call test_main
    add $4,%esp
    push %eax
    call fail_test
fail_test:
    mov 4(%esp),%eax
    ljmp *0x30000000
map_fixture:
    mov 4(%esp),%eax
    ret
# Recreate only the live locals at the audited splices. The game's original
# sort/assignment or rounding/epilogue executes in the mapped image.
.globl run_rating_loop, rating_return, run_wealth_division, run_strength_division
run_rating_loop:
    push %ebx
    push %esi
    push %edi
    push %ebp
    mov %esp,%ebp
    sub $0x14,%esp
    mov 20(%ebp),%eax
    mov %eax,-4(%ebp)
    mov 24(%ebp),%eax
    mov %eax,-8(%ebp)
    mov $0x7eab81,%eax
    jmp *%eax
rating_return:
    mov %ebp,%esp
    pop %ebp
    pop %edi
    pop %esi
    pop %ebx
    ret
run_wealth_division:
    push %ebx
    push %esi
    push %edi
    push %ebp
    sub $0x18,%esp
    mov 0x2c(%esp),%eax
    mov %eax,4(%esp)
    mov 0x30(%esp),%eax
    mov %eax,8(%esp)
    mov 0x34(%esp),%eax
    mov %eax,12(%esp)
    mov 0x38(%esp),%esi
    mov $0x7ea942,%eax
    jmp *%eax
run_strength_division:
    push %ebx
    push %esi
    push %edi
    push %ebp
    sub $0x14,%esp
    mov 0x28(%esp),%eax
    mov %eax,4(%esp)
    mov 0x2c(%esp),%eax
    mov %eax,8(%esp)
    mov 0x30(%esp),%esi
    mov $0x7eaadb,%eax
    jmp *%eax
# Recreate the native item-card EBP local, then call the relocated target.
.globl run_item_load
run_item_load:
    push %ebp
    mov %esp,%ebp
    sub $8,%esp
    mov 16(%ebp),%ecx
    mov %ecx,-8(%ebp)
    mov 8(%ebp),%eax
    mov 12(%ebp),%edx
    call *20(%ebp)
    leave
    ret
.section .note.GNU-stack,"",@progbits

.text
# Run the exact native tail of TArtefactCustom.LoadFromBuffer from its config
# call through the optional strings. The fixture redirects the SEH cleanup.
.globl run_artefact_body, run_artefact_bitmap, take_artefact_bitmap_key
run_artefact_body:
    push %ebp
    mov %esp,%ebp
    push $0
    push $0
    push $0
    push %ebx
    push %esi
    mov 8(%ebp),%ebx
    mov 12(%ebp),%esi
    mov $0x70f83e,%eax
    jmp *%eax
run_artefact_bitmap:
    push %ebx
    mov 8(%esp),%ebx
    mov $0x70e72e,%eax
    jmp *%eax
take_artefact_bitmap_key:
    add $4,%esp
    pop %eax
    pop %ebx
    ret

.globl run_extra_module, take_extra_module, skip_extra_module
run_extra_module:
    push %esi
    push %edi
    mov 12(%esp),%esi
    mov 16(%esp),%edi
    mov $0x6f76a6,%eax
    jmp *%eax
skip_extra_module:
    xor %edi,%edi
take_extra_module:
    mov %edi,%eax
    pop %edi
    pop %esi
    ret
