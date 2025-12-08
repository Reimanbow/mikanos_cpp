; asmfunc.asm
; System V AMD64 ABIの仕様より、RAMレジスタに設定した値が戻り値になる
; Registers: RDI, RSI, RDX, RCX, R8, R9

bits 64
section .text

global IoOut32	; void IoOut32(uint16_t addr, uint32_t data);
IoOut32:
	mov dx, di		; dx = addr
	mov eax, esi	; eax = data
	out dx, eax		; DXに設定されたIOポートアドレスに対してEAXに設定された32ビット整数を出力する
	ret

global IoIn32	; uint32_t IoIn32(uint16_t addr);
IoIn32:
	mov dx, di	; dx = addr
	in eax, dx	; DXに設定されたIOポートアドレスから32ビット整数を入力してEAXに設定する
	ret

global GetCS
GetCS:
	xor eax, eax
	mov ax, cs
	ret

global LoadIDT
LoadIDT:
	push rbp
	mov rbp, rsp
	sub rsp, 10
	mov [rsp], di		; limit
	mov [rsp + 2], rsi	; offset
	lidt [rsp]
	mov rsp, rbp
	pop rbp
	ret