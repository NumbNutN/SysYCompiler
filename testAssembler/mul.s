.section .text, "x"
.global main
main:
    MOV R2,#2
    MUL R0,R2,R0
    MUL R0,R1,R2

    MOV R0,R0, LSL #1
    MOV R0,R0, LSL #1

    ;ADD R0,R1,LSL #2,R2
    ADD R0,R1,R2,LSL #2