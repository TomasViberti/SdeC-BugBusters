global gini_convert

section .text

gini_convert:
    push    rbp
    mov    rbp, rsp

    ; Hago espacio en el stack
        sub     rsp, 8

    ; Uso del stack: 
    ;Guardo el double en el stack
        movsd   [rbp-8], xmm0
    ; Convierto a long truncandolo
        cvttsd2si rax, [rbp-8]
    ; Sumo 1 al long truncado
        add rax, 1
    leave
    ret

section .note.GNU-stack noalloc noexec nowrite progbits