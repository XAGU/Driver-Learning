.CODE
 
Int_3 PROC
		MOV EAX, 1234  ;·µ»Ø1234
		RET
Int_3 ENDP
 
 
MY_TEST PROC
		MOV EAX, 23 ;·µ»Ø23
		RET
MY_TEST ENDP


myAdd PROC
    add rcx,rdx
    mov rax,rcx
    ret
myAdd ENDP

PageProtectOff PROC
	cli
;	mov  eax,BYTE PTR cr0
	and  eax,not 10000h
	;mov  cr0,BYTE PTR eax
PageProtectOff ENDP


PageProtectOn PROC
	;mov  eax,BYTE PTR cr0
	or   eax,10000h
	;mov  cr0,BYTE PTR eax
	sti
PageProtectOn ENDP

END


