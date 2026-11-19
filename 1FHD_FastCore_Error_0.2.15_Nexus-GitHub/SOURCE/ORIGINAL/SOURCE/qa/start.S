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

.text
# Native TGalaxy.Load loop with the same EBP counter and live ESI/EDI.
.globl run_script_loop, finish_script_loop
run_script_loop:
    push %ebp
    mov %esp,%ebp
    sub $0x78,%esp
    push %ebx
    push %esi
    push %edi
    mov 8(%ebp),%esi
    mov 12(%ebp),%edi
    mov 16(%ebp),%eax
    mov %eax,-0x78(%ebp)
    mov $0x7d2b9e,%eax
    jmp *%eax
finish_script_loop:
    pop %edi
    pop %esi
    pop %ebx
    leave
    ret

.text
.globl run_saved_planet_factory, finish_saved_planet_factory
run_saved_planet_factory:
    push %edi
    push %esi
    mov 12(%esp),%edi
    mov 16(%esp),%esi
    mov 20(%esp),%edx
    mov 24(%esp),%ecx
    mov $0x72e4b0,%eax
    push $0x72e19e
    ret
finish_saved_planet_factory:
    pop %esi
    pop %edi
    ret

.text
.globl run_saved_ship_factory, finish_saved_ship_factory
run_saved_ship_factory:
    push %ebx
    push %esi
    mov 12(%esp),%ebx
    mov 16(%esp),%esi
    mov 20(%esp),%edx
    mov 24(%esp),%ecx
    cmpl $0,28(%esp)
    je 1f
    mov $0x6c4bf4,%eax
    push $0x6c4405
    ret
1:
    mov $0x6c4c5c,%eax
    push $0x6c448e
    ret
finish_saved_ship_factory:
    pop %esi
    pop %ebx
    ret

.text
.globl run_scene_config, finish_scene_config
run_scene_config:
    push %esi
    mov 8(%esp),%esi
    mov 12(%esp),%eax
    mov 16(%esp),%edx
    push $0x7865fd
    ret
finish_scene_config:
    pop %esi
    ret

.text
.globl run_script_prepare, run_script_finish, finish_script_prepare, finish_script_cleanup
run_script_prepare:
    mov 4(%esp),%eax
    push $0x7d2b87
    ret
finish_script_prepare:
    ret
run_script_finish:
    push %esi
    push %edi
    mov 12(%esp),%esi
    mov 16(%esp),%edi
    push $0x7d4347
    ret
finish_script_cleanup:
    pop %edi
    pop %esi
    ret

.text
.globl run_saved_message, finish_saved_message
run_saved_message:
    mov 4(%esp),%eax
    mov 8(%esp),%edx
    push $0x5e64e0
    ret
finish_saved_message:
    ret

.globl run_bridge_panel, run_hide_bridge_panels, finish_bridge_panel, finish_hide_bridge_panels
run_bridge_panel:
    push %ebp
    mov %esp,%ebp
    sub $16,%esp
    mov 8(%ebp),%eax
    mov %eax,-4(%ebp)
    mov 12(%ebp),%eax
    push $0x5586fe
    ret
finish_bridge_panel:
    leave
    ret
run_hide_bridge_panels:
    push %ebp
    mov %esp,%ebp
    sub $16,%esp
    mov 8(%ebp),%eax
    mov %eax,-4(%ebp)
    push $0x558634
    ret
finish_hide_bridge_panels:
    mov -16(%ebp),%eax
    leave
    ret

.globl run_cleanup_world, finish_cleanup_world, run_cleanup_image
.globl run_cleanup_shot, finish_cleanup_shot, skip_cleanup_shot
run_cleanup_world:
    push %ebp
    mov %esp,%ebp
    push %esi
    push %edi
    sub $0x68,%esp
    mov 8(%ebp),%esi
    mov 12(%ebp),%edi
    push $0x7d2e07
    ret
finish_cleanup_world:
    mov -0x68(%ebp),%eax
    mov -8(%ebp),%edi
    mov -4(%ebp),%esi
    leave
    ret
run_cleanup_image:
    push %esi
    mov 8(%esp),%esi
    mov 12(%esp),%eax
    mov 16(%esp),%edx
    call 1f
    pop %esi
    ret
1:
    push $0x7d386f
    ret
run_cleanup_shot:
    push %esi
    mov 8(%esp),%esi
    push $0x7f98d4
    ret
finish_cleanup_shot:
    cmp $3,%dl
    jne 2f
    cmp 8(%esi),%eax
    jne 2f
    xor %eax,%eax
    pop %esi
    ret
skip_cleanup_shot:
    mov $1,%eax
    pop %esi
    ret
2:
    mov $2,%eax
    pop %esi
    ret

.globl run_countable_size, finish_countable_missing, finish_countable_found
.globl run_countable_merge, finish_countable_merge
run_countable_size:
    push %ebx
    push %esi
    mov 12(%esp),%ebx
    mov 16(%esp),%eax
    mov 20(%esp),%edx
    push $0x7076be
    ret
finish_countable_missing:
    mov %esi,%eax
    pop %esi
    pop %ebx
    ret
finish_countable_found:
    test %eax,%eax
    mov $7,%eax
    jnz 1f
    xor %eax,%eax
1:
    pop %esi
    pop %ebx
    ret
run_countable_merge:
    push %edi
    push %esi
    mov 12(%esp),%edi
    mov 16(%esp),%esi
    mov %edi,%eax
    push $0x7079ca
    ret
finish_countable_merge:
    pop %esi
    pop %edi
    ret

.globl run_live_missiles_open, run_live_missiles_refresh
.globl finish_live_missiles_open, finish_live_missiles_refresh
run_live_missiles_open:
    push %ebp
    mov %esp,%ebp
    push 8(%ebp)
    push $0x7e2619
    ret
finish_live_missiles_open:
    leave
    ret
run_live_missiles_refresh:
    push 4(%esp)
    push $0x7e27c1
    ret
finish_live_missiles_refresh:
    add $4,%esp
    ret
