            org     0010h
            push    psw
            call    printstr
            pop     psw
            ret

            org     1000h
            lxi     h,jmptab
            ral
            ani     0feh
            add     l
            mov     l,a
            mov     a,h
            aci     00h
            mov     h,a
            mov     e,m
            inx     h
            mov     d,m
            xchg
            lxi     sp,stack
            pchl

jmptab      dw      test001, test002, test003, test004 
            dw      test005, test006, test007, test008
            dw      test009,  test010
            nop
            nop
            nop
test001:    lxi     h,1234h
            shld    test001d
            lxi     h,0000h
            lhld    test001d
            jmp     cleanup
test001d:   db      0000h
            nop
            nop
            nop

test002:    lxi     h,1234h
            lxi     d,4567h
            xchg
            xchg
            jmp     cleanup
            nop
            nop
            nop

test003:    dcr     a
            dcr     a
            dcr     a
            inr     a
            inr     a
            inr     a
            dcr     b
            dcr     b
            dcr     b
            inr     b
            inr     b
            inr     b 
            dcr     c
            dcr     c
            dcr     c
            inr     c
            inr     c
            inr     c 
            dcr     d
            dcr     d
            dcr     d
            inr     d
            inr     d
            inr     d 
            dcr     e
            dcr     e
            dcr     e
            inr     e
            inr     e
            inr     e 
            dcr     h
            dcr     h
            dcr     h
            inr     h
            inr     h
            inr     h 
            dcr     l
            dcr     l
            dcr     l
            inr     l
            inr     l
            inr     l
            lxi     h,test003d
            dcr     m
            mov     a,m
            dcr     m
            mov     a,m
            dcr     m
            mov     a,m
            inr     m
            mov     a,m
            inr     m
            mov     a,m
            inr     m
            mov     a,m
            jmp     cleanup

test003d:   dw      0000h
            nop
            nop
            nop

test004:    dcx     b
            dcx     b
            dcx     b
            inx     b
            inx     b
            inx     b
            dcx     d
            dcx     d
            dcx     d
            inx     d
            inx     d
            inx     d
            dcx     h
            dcx     h
            dcx     h
            inx     h
            inx     h
            inx     h
            dcx     sp
            dcx     sp
            dcx     sp
            inx     sp
            inx     sp
            inx     sp
            jmp     cleanup
            nop
            nop
            nop

test005:    mvi     a,01h
            dcr     a       ; flags = z, p, pe
            jz      $+6
            jmp     stop
            jnz     stop
            jp      $+6
            jmp     stop
            jm      stop
            jpe     $+6
            jmp     stop
            jpo     stop

            inr     a       ; flags = nz, p, po
            jnz     $+6
            jmp     stop
            jz      stop
            jp      $+6
            jmp     stop
            jm      stop
            jpo     $+6
            jmp     stop
            jpe     stop

            dcr     a
            dcr     a       ; flag = nz, m, pe
            jnz     $+6
            jmp     stop
            jz      stop
            jm      $+6
            jmp     stop
            jp      stop
            jpe     $+6
            jmp     stop
            jpo     stop

            jmp     cleanup
            nop
            nop
            nop

test006:    lxi     sp,stack
            dcr     a
            lxi     b,1234h
            lxi     d,5678h
            lxi     h,9abch
            push    psw
            push    b
            push    d
            push    h
            inr     a
            lxi     b,0000h
            lxi     d,0000h
            lxi     h,0000h
            pop     h
            pop     d
            pop     b
            pop     psw
            jmp     cleanup
            nop
            nop
            nop

test007:    lxi     h,message
            call    printstr
            hlt

message:    text    "Hello, world!"
            db      0ah,0dh,00h

char_out:   equ     01h

test008:    lxi     sp,stack
            mvi     a,42h
            call    print_byte
            call    crlf
            hlt

test009:    lxi     sp,stack
            lxi     h,1234h
            call    print_word
            call    crlf 
            hlt

test010:    lxi     sp,stack
            lxi     h,message
            rst     02h
            lxi     h,message2
            rst     02h
            call    crlf
            hlt
message2:   text    "That message (and this one) was brought to you by rst 02h!"
            db      00h
cleanup:    mvi     a,0h
            lxi     b,0h
            lxi     d,0h
            lxi     h,0h
            lxi     sp,0h
stop:       jmp     stop
            blk     80h
stack:      equ     $


;**********************************************
; printstr: Output string point to by h to
;       to screen
;**********************************************
printstr:   mov     a,m         ; Get byte
            cpi     0h          ; Is it 0?
            rz                  ; Return to caller
            out     char_out    ; Output
            inx     h           ; Move to next byte
            jmp     printstr    ; Loop until done

;**********************************************
; print_word: Print one 16 bit hex word in h
; print_byte: Print one byte in a
; print_nyb: Print nybble in lower 4 bits of a
;**********************************************
print_word: mov     a,h          ; Get higher byte in a
            call    print_byte   ; Print byte
            mov     a,l          ; Get lower byte in a
print_byte: push    psw          ; Save a
            ani     0f0h         ; Kill lower four bits
            rar                  ; Rotate high bits into low
            rar
            rar
            rar
            call    print_nyb    ; Print high nybble
            pop     psw          ; Restore a
            ani     0fh          ; Kill higher four bits
print_nyb:  push    psw          ; Store a
            ori     30h          ; Add ascii char '0'
            cpi     3ah          ; Compare ':'
            jm      $+2          ; Jump over if lower
            adi     27h          ; Add to 'a'
            out     char_out     ; Output   
            pop     psw          ; Restore a
            ret                  ; Return from call

;**********************************************
; Print line feed to screen
;**********************************************
crlf:       push    psw          ; Store a
            mvi     a,0ah        ; Load line feed
            out     char_out     ; Output
            mvi     a,0dh        ; Load carriage return
            out     char_out     ; Output
            pop     psw          ; Restore a
            ret                  ; Return


