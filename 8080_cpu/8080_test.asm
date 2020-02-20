            org     1000h
            ld      hl,jmptab
            rla
            and     0feh
            add     a,l
            ld      l,a
            ld      a,h
            adc     a,00h
            ld      h,a
            ld      e,(hl)
            inc     hl
            ld      d,(hl)
            ex      de,hl
            jp      (hl)

jmptab      dw      test001, test002, test003, test004 
            dw      test005, test006, test007

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

test005:    ld      a,01h
            dec     a       ; flags = z, p, pe
            jp      z,$+6
            jp      stop
            jp      nz,stop
            jp      p,$+6
            jp      stop
            jp      m,stop
            jp      pe,$+6
            jp      stop
            jp      po,stop

            inc     a       ; flags = nz, p, po
            jp      nz,$+6
            jp      stop
            jp      z,stop
            jp      p,$+6
            jp      stop
            jp      m,stop
            jp      po,$+6
            jp      stop
            jp      pe,stop

            dec     a
            dec     a       ; flag = nz, m, pe
            jp      nz,$+6
            jp      stop
            jp      z,stop
            jp      m,$+6
            jp      stop
            jp      p,stop
            jp      pe,$+6
            jp      stop
            jp      po,stop

            jp      cleanup
            nop
            nop
            nop

test006:    ld      sp,stack
            dec     a
            ld      bc,1234h
            ld      de,5678h
            ld      hl,9abch
            push    af
            push    bc
            push    de
            push    hl
            inc     a
            ld      bc,0000h
            ld      de,0000h
            ld      hl,0000h
            pop     hl
            pop     de
            pop     bc
            pop     af
            jp      cleanup
            nop
            nop
            nop

test007:    ld      hl,message
test007a:   ld      a,(hl)
            cp      a,00h
            jp      z,test007b
            out     (01h),a
            inc     hl
            jp      test007a
test007b:   halt

message:    text    "Hello, world!"
            db      0ah,0dh,00h

cleanup:    ld      a,0h
            ld      bc,0h
            ld      de,0h
            ld      hl,0h
            ld      sp,0h
stop:       jp      stop
            blk     80h
stack:      equ     $

