
.macro zero_bss
	@ BSS start and end MUST be word-aligned.
	ldr	r0, =_bss_start
	ldr	r1, =_bss_end
	add	r1, r1, #4
	mov	r2, #0

1:
	str	r2, [r0]

	add	r0, r0, #4
	cmp	r0, r1
	blt	1b
.endm
