            org     1000h
            jp      test001
            jp      test002
            jp      test003
            jp      test004
            nop
            nop
            nop
test001:    ld      hl,1234h
            ld      (test001d),hl
            ld      hl,0000h
            ld      hl,(test001d)
            jp      cleanup
test001d:   db      0000h
            nop
            nop
            nop

test002:    ld      hl,1234h
            ld      de,4567h
            ex      de,hl 
            ex      de,hl
            jp      cleanup
            nop
            nop
            nop

test003:    dec     a
            dec     a
            dec     a
            inc     a
            inc     a
            inc     a
            dec     b
            dec     b
            dec     b
            inc     b
            inc     b
            inc     b 
            dec     c
            dec     c
            dec     c
            inc     c
            inc     c
            inc     c 
            dec     d
            dec     d
            dec     d
            inc     d
            inc     d
            inc     d 
            dec     e
            dec     e
            dec     e
            inc     e
            inc     e
            inc     e 
            dec     h
            dec     h
            dec     h
            inc     h
            inc     h
            inc     h 
            dec     l
            dec     l
            dec     l
            inc     l
            inc     l
            inc     l
            ld      hl,test003d
            dec     (hl)
            ld      a,(hl)
            dec     (hl)
            ld      a,(hl)
            dec     (hl)
            ld      a,(hl)
            inc     (hl)
            ld      a,(hl)
            inc     (hl)
            ld      a,(hl)
            inc     (hl)
            ld      a,(hl)
            jp      cleanup

test003d:   dw      0000h
            nop
            nop
            nop

test004:    dec     bc
            dec     bc
            dec     bc
            inc     bc
            inc     bc
            inc     bc
            dec     de
            dec     de
            dec     de
            inc     de
            inc     de
            inc     de
            dec     hl
            dec     hl
            dec     hl
            inc     hl
            inc     hl
            inc     hl
            dec     sp
            dec     sp
            dec     sp
            inc     sp
            inc     sp
            inc     sp
            jp      cleanup
            nop
            nop
            nop

cleanup:    ld      a,0h
            ld      bc,0h
            ld      de,0h
            ld      hl,0h
            ld      sp,0h
stop:       jp      stop


