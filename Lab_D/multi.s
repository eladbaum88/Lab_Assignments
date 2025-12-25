
; 32-bit Linux, NASM, CDECL

global main
global print_multi
global getmulti
global maxmin
global add_multi
global rand_num
global PRmulti

extern printf
extern puts
extern fgets
extern stdin
extern strlen
extern malloc
extern free
extern strcmp

default rel

section .rodata
    fmt_byte:   db "%02hhx", 0
    empty_str:  db "", 0
    fmt_first:  db "%x", 0


    flag_I:     db "-I", 0
    flag_R:     db "-R", 0

section .data
    ; default numbers
    x_struct:
        dd 5
        db 0xaa, 1, 2, 0x44, 0x4f

    y_struct:
        dd 6
        db 0xaa, 1, 2, 3, 0x44, 0x4f

    ; PRNG state (must be non-zero)
    state:  dw 0xACE1
    mask:   dw 0xB400

section .bss
    input_buf:  resb 601

section .text

; ------------------------------------------------------------
; hex_nibble: AL ascii -> AL 0..15
; ------------------------------------------------------------
hex_nibble:
    cmp al, '0'
    jb  .bad
    cmp al, '9'
    jbe .digit
    or  al, 0x20
    cmp al, 'a'
    jb  .bad
    cmp al, 'f'
    ja  .bad
    sub al, ('a' - 10)
    ret
.digit:
    sub al, '0'
    ret
.bad:
    xor al, al
    ret

; ------------------------------------------------------------
; void print_multi(struct multi *p)
; prints all bytes MSB->LSB, newline
; ------------------------------------------------------------
print_multi:
    push ebp
    mov  ebp, esp

    push ebx
    push esi
    push edi

    mov  edi, [ebp+8]          ; p
    mov  ebx, [edi]            ; size
    lea  esi, [edi+4]          ; &num[0]
    dec  ebx                   ; i = size-1 (MSB index)

; Skip leading zero BYTES, but keep at least one byte
.skip_zero_bytes:
    cmp  ebx, 0
    jle  .print_loop           ; if only one byte left, don't skip it
    cmp  byte [esi+ebx], 0
    jne  .print_loop
    dec  ebx
    jmp  .skip_zero_bytes

.print_loop:
    ; Print bytes from current ebx down to 0 using %02hhx
    movzx eax, byte [esi+ebx]
    push eax
    lea  edx, [fmt_byte]       ; "%02hhx"
    push edx
    call printf
    add  esp, 8

    dec  ebx
    jns  .print_loop

    ; newline
    lea  eax, [empty_str]
    push eax
    call puts
    add  esp, 4

    pop  edi
    pop  esi
    pop  ebx
    mov  esp, ebp
    pop  ebp
    ret

; ------------------------------------------------------------
; struct multi* getmulti()
; reads hex line, strips '\n', pads odd length by prepending '0'
; allocates struct and fills num[] little-endian
; ------------------------------------------------------------
getmulti:
    push ebp
    mov  ebp, esp

    push ebx
    push esi
    push edi
    sub  esp, 8                ; [ebp-4]=len

    ; fgets(input_buf, 600, stdin)
    push dword [stdin]
    push dword 600
    push dword input_buf
    call fgets
    add  esp, 12

    ; len = strlen(input_buf)
    push dword input_buf
    call strlen
    add  esp, 4
    mov  edx, eax

    ; strip trailing '\n'
    test edx, edx
    jz   .len_done
    mov  al, [input_buf + edx - 1]
    cmp  al, 10
    jne  .len_done
    dec  edx
 .len_done:   
    ; strip trailing '\r' (CRLF safety)
    test edx, edx
    jz   .len_done_cr
    mov  al, [input_buf + edx - 1]
    cmp  al, 13
    jne  .len_done_cr
    dec  edx
.len_done_cr:
    mov  byte [input_buf + edx], 0
    mov  [ebp-4], edx

    ; if len odd, prepend '0'
    test edx, 1
    jz   .even_len
    mov  ecx, edx
.shift_right:
    mov  al, [input_buf + ecx]         ; includes '\0'
    mov  [input_buf + ecx + 1], al
    dec  ecx
    jns  .shift_right
    mov  byte [input_buf], '0'
    inc  edx
    mov  [ebp-4], edx
.even_len:

    ; size = len/2
    mov  eax, edx
    shr  eax, 1
    mov  ebx, eax

    ; malloc(4 + size)
    mov  eax, ebx
    add  eax, 4
    push eax
    call malloc
    add  esp, 4
    mov  edi, eax

    mov  [edi], ebx

    ; fill bytes little-endian from end
    mov  edx, [ebp-4]
    dec  edx                   ; j = len-1
    xor  ecx, ecx              ; i = 0

.fill_loop:
    cmp  ecx, ebx
    jge  .fill_done

    mov  al, [input_buf + edx]
    call hex_nibble
    mov  ah, al
    dec  edx

    mov  al, [input_buf + edx]
    call hex_nibble
    shl  al, 4
    or   al, ah

    mov  [edi + 4 + ecx], al
    dec  edx
    inc  ecx
    jmp  .fill_loop

.fill_done:
    mov  eax, edi

    add  esp, 8
    pop  edi
    pop  esi
    pop  ebx
    mov  esp, ebp
    pop  ebp
    ret

; ------------------------------------------------------------
; maxmin (NOT cdecl): eax=p, ebx=q -> eax=max, ebx=min
; ------------------------------------------------------------
maxmin:
    mov  ecx, [eax]
    mov  edx, [ebx]
    cmp  ecx, edx
    jge  .mm_done
    xchg eax, ebx
.mm_done:
    ret

; ------------------------------------------------------------
; struct multi* add_multi(struct multi *p, struct multi *q)
; ------------------------------------------------------------
add_multi:
    push ebp
    mov  ebp, esp

    push ebx
    push esi
    push edi
    sub  esp, 16               ; locals: min_len,max_len,res_ptr,carry

    mov  eax, [ebp+8]
    mov  ebx, [ebp+12]
    call maxmin                ; eax=max, ebx=min

    mov  esi, eax              ; max struct
    mov  edi, ebx              ; min struct

    mov  edx, [esi]            ; max_len
    mov  ecx, [edi]            ; min_len
    mov  [ebp-8], edx
    mov  [ebp-4], ecx

    ; malloc(4 + (max_len+1))
    mov  eax, edx
    inc  eax
    add  eax, 4
    push eax
    call malloc
    add  esp, 4
    mov  [ebp-12], eax

    ; res->size = max_len+1
    mov  ebx, [ebp-8]
    inc  ebx
    mov  [eax], ebx

    lea  esi, [esi+4]          ; max->num
    lea  edi, [edi+4]          ; min->num
    mov  ebx, [ebp-12]
    lea  ebx, [ebx+4]          ; res->num

    xor  ecx, ecx
    mov  dword [ebp-16], 0

.loop_min:
    cmp  ecx, [ebp-4]
    jge  .after_min
    movzx eax, byte [esi+ecx]
    movzx edx, byte [edi+ecx]
    add  eax, edx
    add  eax, [ebp-16]
    mov  [ebx+ecx], al
    shr  eax, 8
    mov  [ebp-16], eax
    inc  ecx
    jmp  .loop_min

.after_min:
.loop_max:
    mov  edx, [ebp-8]
    cmp  ecx, edx
    jge  .finish
    movzx eax, byte [esi+ecx]
    add  eax, [ebp-16]
    mov  [ebx+ecx], al
    shr  eax, 8
    mov  [ebp-16], eax
    inc  ecx
    jmp  .loop_max

.finish:
    mov  edx, [ebp-8]
    mov  eax, [ebp-16]
    mov  [ebx+edx], al

    mov  eax, [ebp-12]

    add  esp, 16
    pop  edi
    pop  esi
    pop  ebx
    mov  esp, ebp
    pop  ebp
    ret

; ------------------------------------------------------------
; rand_num: 16-bit Fibonacci LFSR (STATE & MASK parity -> MSB)
; returns AX = new state
; ------------------------------------------------------------
rand_num:
    push ebp
    mov  ebp, esp
    push ebx

    mov  bx, [state]
    mov  ax, bx
    and  ax, [mask]

    xor  dl, dl                ; parity accumulator

.parity_loop:
    test ax, ax
    jz   .parity_done
    test ax, 1
    jz   .skip_xor
    xor  dl, 1
.skip_xor:
    shr  ax, 1
    jmp  .parity_loop

.parity_done:
    shr  bx, 1
    movzx dx, dl
    shl  dx, 15
    or   bx, dx

    mov  [state], bx
    mov  ax, bx

    pop  ebx
    mov  esp, ebp
    pop  ebp
    ret

; ------------------------------------------------------------
; struct multi* PRmulti()
; n = random byte (AL), retry if 0
; allocate struct 4+n, fill n bytes with random AL values
; ------------------------------------------------------------
PRmulti:
    push ebp
    mov  ebp, esp

    push ebx
    push esi
    push edi

.get_len:
    call rand_num
    movzx ebx, al
    test ebx, ebx
    jz   .get_len

    mov  eax, ebx
    add  eax, 4
    push eax
    call malloc
    add  esp, 4
    mov  edi, eax

    mov  [edi], ebx            ; size = n
    lea  esi, [edi+4]          ; num base
    xor  ecx, ecx

.fill_rand:
    cmp  ecx, ebx
    jge  .done
    call rand_num
    mov  [esi+ecx], al
    inc  ecx
    jmp  .fill_rand

.done:
    mov  eax, edi

    pop  edi
    pop  esi
    pop  ebx
    mov  esp, ebp
    pop  ebp
    ret

; ------------------------------------------------------------
; main (Part 4):
; default: x_struct, y_struct
; -I: read two numbers using getmulti
; -R: generate two numbers using PRmulti
; prints p, q, and add_multi(p,q)
; ------------------------------------------------------------
main:
    push ebp
    mov  ebp, esp

    push ebx
    push esi
    push edi

    ; argc at [ebp+8], argv at [ebp+12]
    mov  eax, [ebp+8]
    cmp  eax, 1
    jle  .use_default

    ; argv[1]
    mov  eax, [ebp+12]
    mov  eax, [eax+4]

    ; if argv[1] == "-I"
    push dword flag_I
    push eax
    call strcmp
    add  esp, 8
    test eax, eax
    je   .use_input

    ; if argv[1] == "-R"
    mov  eax, [ebp+12]
    mov  eax, [eax+4]
    push dword flag_R
    push eax
    call strcmp
    add  esp, 8
    test eax, eax
    je   .use_rand

.use_default:
    mov  esi, x_struct
    mov  edi, y_struct
    jmp  .have_pq

.use_input:
    call getmulti
    mov  esi, eax
    call getmulti
    mov  edi, eax
    jmp  .have_pq

.use_rand:
    call PRmulti
    mov  esi, eax
    call PRmulti
    mov  edi, eax

.have_pq:
    push esi
    call print_multi
    add  esp, 4

    push edi
    call print_multi
    add  esp, 4

    push edi
    push esi
    call add_multi
    add  esp, 8

    push eax
    call print_multi
    add  esp, 4

    pop  edi
    pop  esi
    pop  ebx

    xor  eax, eax
    mov  esp, ebp
    pop  ebp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
