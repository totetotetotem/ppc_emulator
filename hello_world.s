.text
    .align 2
    .global _start

_start:
    lis r3,repeat@ha
    addi r3,r3,repeat@l
    lwz r0,0(r3)
    mtctr r0
    lis r4,msg@ha
    addi r4,r4,msg@l
1:    li  r3,1
    li  r5,msg_sz
    li  r0,4
    sc
    bdnz 1b
    li  r3,0
    li  r0,1
    sc

repeat: .long 5

.data
    .align 2

msg:   .asciz "hello,world\n"

    .align 2
msg_sz = . - msg

.section .bss
    .align 2

data: .long 0


