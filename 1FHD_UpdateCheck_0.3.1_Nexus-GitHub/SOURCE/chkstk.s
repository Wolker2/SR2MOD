.text
.globl __chkstk
__chkstk:
  pushl %ecx
  pushl %edx
  movl %esp, %ecx
  addl $12, %ecx
  movl %eax, %edx
.Lprobe:
  cmpl $4096, %edx
  jb .Llast
  subl $4096, %ecx
  testb $0, (%ecx)
  subl $4096, %edx
  jmp .Lprobe
.Llast:
  subl %edx, %ecx
  testb $0, (%ecx)
  popl %edx
  popl %ecx
  ret
