main:
        PUSH    {R7, R14}
        SUB     R7, R7, #24040
        SUB     R13, R13, #24040
        LDR     R0, =10
        LDR     R1, =10
        ADD     R2, R0, R1
        MOV     R5, R2
        LDR     R0, =600
        LDR     R1, =3
        MLA     R2, R0, R1, R4
        STR     R2, [R7, #-40]
        LDR     R0, =30
        LDR     R1, [R7, #-40]
        LDR     R2, =5
        MLA     R3, R0, R2, R1
        MOV     R6, R3
        LDR     R0, =1
        MLA     R1, R0, R5, R6
        STR     R1, [R7, #-36]
        LDR     R0, =600
        LDR     R1, =3
        MLA     R2, R0, R1, R4
        MOV     R8, R2
        LDR     R0, =30
        LDR     R1, =5
        MLA     R2, R0, R1, R8
        MOV     R9, R2
        LDR     R0, =10
        LDR     R1, =10
        ADD     R2, R0, R1
        MOV     R10, R2
        LDR     R0, =1
        MLA     R1, R0, R10, R9
        STR     R1, [R7, #-32]
end_label:
        POP     R7
        POP     R15
        ADD     R7, R7, #24040
        ADD     R13, R13, #24040 
        POP     {R7, R15}