	.arch armv5t
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 6
	.eabi_attribute 34, 0
	.eabi_attribute 18, 4
	.file	"dymatic_array.c"
	.text
	.section	.rodata
	.align	2
.LC0:
	.ascii	"%d\000"
	.text
	.align	2
	.global	main
	.syntax unified
	.arm
	.type	main, %function
main:
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 1, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, fp, lr}
	add	fp, sp, #24
	sub	sp, sp, #28
	ldr	r3, .L6
	ldr	r3, [r3]
	str	r3, [fp, #-32]
	mov	r3, #0
	mov	r3, sp
	mov	r8, r3
	sub	r3, fp, #48
	mov	r1, r3
	ldr	r0, .L6+4
	bl	scanf
	ldr	r1, [fp, #-48]
	sub	r3, r1, #1
	str	r3, [fp, #-40]
	mov	r2, r1
	mov	r3, #0
	mov	r6, r2
	mov	r7, r3
	mov	r2, #0
	mov	r3, #0
	lsl	r3, r7, #5
	orr	r3, r3, r6, lsr #27
	lsl	r2, r6, #5
	mov	r2, r1
	mov	r3, #0
	mov	r4, r2
	mov	r5, r3
	mov	r2, #0
	mov	r3, #0
	lsl	r3, r5, #5
	orr	r3, r3, r4, lsr #27
	lsl	r2, r4, #5
	mov	r3, r1
	lsl	r3, r3, #2
	add	r3, r3, #7
	lsr	r3, r3, #3
	lsl	r3, r3, #3
	sub	sp, sp, r3
	mov	r3, sp
	add	r3, r3, #3
	lsr	r3, r3, #2
	lsl	r3, r3, #2
	str	r3, [fp, #-36]
	mov	r3, #0
	str	r3, [fp, #-44]
	b	.L2
.L3:
	ldr	r3, [fp, #-36]
	ldr	r2, [fp, #-44]
	mov	r1, #0
	str	r1, [r3, r2, lsl #2]
	ldr	r3, [fp, #-44]
	add	r3, r3, #1
	str	r3, [fp, #-44]
.L2:
	ldr	r3, [fp, #-48]
	ldr	r2, [fp, #-44]
	cmp	r2, r3
	blt	.L3
	mov	sp, r8
	mov	r3, #0
	ldr	r2, .L6
	ldr	r1, [r2]
	ldr	r2, [fp, #-32]
	eors	r1, r2, r1
	mov	r2, #0
	beq	.L5
	bl	__stack_chk_fail
.L5:
	mov	r0, r3
	sub	sp, fp, #24
	@ sp needed
	pop	{r4, r5, r6, r7, r8, fp, pc}
.L7:
	.align	2
.L6:
	.word	__stack_chk_guard
	.word	.LC0
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.2.0-17ubuntu1) 11.2.0"
	.section	.note.GNU-stack,"",%progbits
