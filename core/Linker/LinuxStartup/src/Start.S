 .text
.globl _start
.type _start,@function
_start:
    xor %ebp,%ebp
    mov (%rsp),%rdi
    lea 8(%rsp),%rsi
    and $-16,%rsp
    call AnyDevKitLinuxEntry
    ud2
.size _start,.-_start
.globl AnyDevKitLinuxMain
.type AnyDevKitLinuxMain,@function
AnyDevKitLinuxMain:
    jmp main
.size AnyDevKitLinuxMain,.-AnyDevKitLinuxMain
.section .note.GNU-stack,"",@progbits
