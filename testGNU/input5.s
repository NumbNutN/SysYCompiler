.section .text
.global _start
_start:
        PUSH    {R7, R14}
        SUB     R7, R7, #28
        SUB     R13, R13, #28
        LDR     R1, =10
        MOV     R5, R1
        LDR     R1, =120
        STR     R1, [R7, #24]
label1:
        CMP     R5, #5
        MOVLT   R6, #0
        MOV     R6, #1
        CMP     R6, #1
        BNE     label2
label3:
        LDR     R1, =1
        SUB     R2, R5, R1
        MOV     R7, R2
        LDR     R1, [R7, #24]
        CMP     R1, #70
        MOVLT   R8, #0
        MOV     R8, #1
        CMP     R8, #1
        BNE     label4
label5:
        LDR     R1, =100
        MOV     R4, R1
label6:
        STR     R4, [R7, #24]
        MOV     R5, R7
label4:
        LDR     R1, [R7, #24]
        LDR     R2, =10
        SUB     R3, R1, R2
        MOV     R0, R3
        MOV     R4, R0
label2:
end_label:
        POP     R7
        POP     R15
        ADD     R7, R7, #28
        ADD     R13, R13, #28
        POP     {R7, R15}