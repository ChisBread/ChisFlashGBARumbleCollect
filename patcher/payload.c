
asm(R"(
.thumb
rumble_active:
    # be patched machine code, 8 bytes
    nop
    nop

	push {r1-r5}
	ldr 	r1,=0xD200
	ldr 	r2,=0x1500
	ldr 	r3,=0x9fe0000
	strh 	r1,[r3]
	ldr 	r3,=0x8000000
	strh  r2,[r3]
	ldr 	r3,=0x8020000
	strh 	r1,[r3]
	ldr 	r3,=0x8040000
	strh  r2,[r3]
	ldr 	r3,=0x9E20000
	mov 	r1,#0xF1
	strh  r1,[r3]
	ldr 	r3,=0x9FC0000
	strh  r2,[r3]
	LDR		R4, =0x8001000
	
	mov		r5,#2
	strb  r5,[r4]	
	nop
	nop
	nop
	nop
# set_off
	mov		r5,#0
	strb  r5,[r4]
# exit
	pop {r1-r5}
	bx  lr
)");

asm(R"(
# The following footer must come last.
.balign 4
.ascii "<3 from ChisBreadRumble"

)");