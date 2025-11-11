OAM_ADDRESS     = $2003
OAM_DMA         = $4014
PPU_ADDRESS     = $2006
PPU_DATA        = $2007
PPU_SCROLL      = $2005
;sprite_data     = $0200
OAM = $0200

;COLOR_BLACK     = #$0f
;COLOR_WHITE     = #$20

Message:
.byte "Hello World!", $00

.segment "ZEROPAGE"
frame_done:     .res 1
scroll_x:       .res 1

.segment "HEADER"
	.byte "NES",26, 2,1, 0,0

.segment "STARTUP" ; avoids warning

reset:
    sei        ; ignore IRQs
    cld        ; disable decimal mode
    ldx #$40
    stx $4017  ; disable APU frame IRQ
    ldx #$ff
    txs        ; Set up stack
    inx        ; now X = 0
    stx $2000  ; disable NMI
    stx $2001  ; disable rendering
    stx $4010  ; disable DMC IRQs

    ; Optional (omitted):
    ; Set up mapper and jmp to further init code here.

    ; The vblank flag is in an unknown state after reset,
    ; so it is cleared here to make sure that @vblankwait1
    ; does not exit immediately.
    bit $2002

    ; First of two waits for vertical blank to make sure that the
    ; PPU has stabilized
@vblankwait1:  
    bit $2002
    bpl @vblankwait1

    ; We now have about 30,000 cycles to burn before the PPU stabilizes.
    ; One thing we can do with this time is put RAM in a known state.
    ; Here we fill it with $00, which matches what (say) a C compiler
    ; expects for BSS.  Conveniently, X is still 0.
    txa
@clrmem:
    sta $000,x
    sta $100,x
    sta $200,x
    sta $300,x
    sta $400,x
    sta $500,x
    sta $600,x
    sta $700,x
    inx
    bne @clrmem

    ; Other things you can do between vblank waits are set up audio
    ; or set up other mapper registers.
    jsr clear_nametable
   
@vblankwait2:
    bit $2002
    bpl @vblankwait2

; initialize PPU OAM
;ldx #$00
;stx OAM_ADDRESS ; $00
;lda #$02 ; use page $0200-$02ff
;sta OAM_DMA

jsr load_palette
lda #%00000100 ; vertical address increment
sta $2000  ; disable NMI
lda #$21
sta PPU_ADDRESS
lda #$ca
sta PPU_ADDRESS

Loop:
    LDA Message,X     ; Load the byte at Message + X into A
    BEQ Done          ; If it's the null terminator, jump to Done
    STA PPU_DATA       ; Store the byte in memory at $0200 + Y
    INX               ; Increment X to point to the next character in the string
    ;INY               ; Increment Y to point to the next memory location
    JMP Loop          ; Repeat the loop

Done:

; reset scroll location to top-left of screen
lda #$00
sta PPU_SCROLL
sta PPU_SCROLL

; lda #$c0
; sta $0200
; lda #$43
; sta $0201
; lda #%00000110
; sta $0202
; lda #$05
; sta $0203

; Set up OAMADDR
; lda #<OAM   ; Low byte of sprite_data address
; sta OAM_ADDRESS           ; Store low byte into OAMADDR

; lda #$20            ; High byte of sprite_data address
; sta OAM_ADDRESS           ; Store high byte into OAMADDR

; Trigger OAMDMA transfer
;lda #%00000001      ; Any non-zero value will initiate DMA transfer
;sta $4014           ; Start DMA transfer to OAM (this transfers all sprite data - 256 bytes - from CPU memory to PPU memory)

lda #$80
;lda #%10000000
;inx        ; now X = 1
sta $2000  ; enable NMI
lda #$1e
sta $2001  ; enable rendering
lda #$ff
sta $4010  ; enable DMC IRQs

@loop:
	;ldx #$0
	;@loop2:
		; lda #$cf
		;txa
		;sta $000, x
		;inx
		;cpx #$a
		;bne @loop2
	;inc $00
    jsr wait_frame
    ;inc $00
    ; inc $0203
jmp @loop

;palette = $0000

.proc wait_frame
    inc frame_done          ; Make it non-zero
    @loop:
        lda frame_done
        bne @loop           ; Wait for frame_done to become zero again
        rts
.endproc

.proc load_palette
    lda #$3f
    sta PPU_ADDRESS
    lda #$00
    sta PPU_ADDRESS
    ; load 8 palettes (4 bg, 4 sprite)
    ldx #$08
    palette_loop:
        ;ldx 0
        lda #$0f ; black
        sta PPU_DATA
        lda #$20 ; white
        sta PPU_DATA
        sta PPU_DATA
        sta PPU_DATA
        dex
        bne palette_loop
    rts
.endproc

.proc nmi
    ; Define RGB values for a sprite palette (example)
    ;palette:

	;sta $00, x
	

    ; Load palette into PPU registers
    ;ldx #$0              ; Start with color 0
    ;lda #$55       ; Load first color (RGB values)
    ;sta $3F00,x         ; Store in PPU palette register
    ;inx                 ; Increment index
    ;lda $0000,x       ; Load second color (RGB values)
    ;sta $3F00,x         ; Store in PPU palette register
    ;inx                 ; Increment index
    ;lda $0000,x       ; Load third color (RGB values)
    ;sta $3F00,x         ; Store in PPU palette register
    ;inx                 ; Increment index
    ;lda $0000,x       ; Load fourth color (RGB values)
    ;sta $3F00,x         ; Store in PPU palette register

    ; Example of setting palette for sprites (PPU address $3F10-$3F1F)
    ;ldx #$0              ; Start with color 0
    ;lda $0000,x       ; Load first color (RGB values)
    ;sta $3F10,x         ; Store in PPU palette register (sprite palette)
    ;inx                 ; Increment index
    ;lda $0000,x       ; Load second color (RGB values)
    ;sta $3F10,x         ; Store in PPU palette register (sprite palette)
    ;inx                 ; Increment index
    ;lda $0000,x       ; Load third color (RGB values)
    ;sta $3F10,x         ; Store in PPU palette register (sprite palette)
    ;inx                 ; Increment index
    ;lda $0000,x       ; Load fourth color (RGB values)
    ;sta $3F10,x         ; Store in PPU palette register (sprite palette)

    ; Set up OAMADDR
    ;lda #<OAM   ; Low byte of sprite_data address
    ;sta OAM_ADDRESS           ; Store low byte into OAMADDR

    ;lda #$20            ; High byte of sprite_data address
    ; lda #$0                     ; Start writing at OAM_ADDRESS 0 in the PPU
    ; sta OAM_ADDRESS           ; Store high byte into OAMADDR
    ; lda #>OAM                   ; Store high byte into OAM_DMA
    ; sta OAM_DMA

    lda #$0
    sta frame_done      ; Ding fries are done

    inc scroll_x
    lda scroll_x
    sta PPU_SCROLL ; x scroll
    lda #$0
    sta PPU_SCROLL ; y scroll


    ; Wait for DMA transfer to complete (optional)
    ;wait_dma:
        ;bit $2002       ; Check if DMA transfer is still in progress
        ;bpl wait_dma    ; Wait until DMA transfer completes
	rti
.endproc

clear_nametable:
; Example: Clear nametables
    LDA #$20        ; start at $2000
    STA $2006
    LDA #$00
    STA $2006

    LDA #$00        ; value to clear with
    LDY #$10        ; 4 KB / 256 bytes per page = 16 pages
ClearLoop:
    LDX #$00
PageLoop:
    STA $2007
    INX
    BNE PageLoop
    DEY
    BNE ClearLoop
    RTS


.segment "VECTORS"
	.addr nmi, reset, 0

.segment "CHR_BANK_0"
    .incbin "sprites.chr"