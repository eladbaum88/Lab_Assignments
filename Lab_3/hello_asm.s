section .data
msg db "hello world", 10     ; 10 = '\n'
len equ $ - msg    

section .text
global _start

_start:

; write(1, msg, len)
    mov eax, 4               ; SYS_write
    mov ebx, 1               ; fd = STDOUT
    mov ecx, msg             ; buffer address
    mov edx, len             ; buffer length
    int 0x80                 ; kernel syscall

    ; exit(0)
    mov eax, 1               ; SYS_exit
    mov ebx, 0               ; exit code = 0
    int 0x80
