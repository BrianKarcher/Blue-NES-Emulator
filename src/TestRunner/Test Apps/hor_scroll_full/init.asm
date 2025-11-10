OAM_ADDRESS     = $2003
OAM_DMA         = $4014
PPU_ADDRESS     = $2006
PPU_DATA        = $2007
PPU_SCROLL      = $2005
;sprite_data     = $0200
OAM = $0200
NT0             = $2000 ; Nametable Zero
NT1             = $2400 ; Nametable One


; 2x2 tiles. indexes into the tile table below
board0:
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

board1:
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

board2:
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

board3:
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0

board4:
.byte 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1
.byte 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1
.byte 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1
.byte 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1
.byte 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1
.byte 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1
.byte 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
.byte 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
.byte 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1
.byte 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1
.byte 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1
.byte 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1
.byte 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1
.byte 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1
.byte 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1

board5:
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

board6:
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1

board7:
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

board8:
.byte 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1
.byte 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0

board9:
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

board10:
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0
.byte 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0
.byte 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0

boards:
.word board0, board1, board2, board3, board4, board5, board6, board7, board8, board9, board10

top_left:
.byte $90, $92, $b0, $ff, $05, $80, $02, $05, $11, $05, $80, $00, $15, $15

top_right:
.byte $91, $93, $b1, $ff, $05, $81, $05, $03, $81, $03, $10, $15, $15, $01

bottom_left:
.byte $a0, $a2, $c0, $ff, $15, $80, $80, $01, $15, $01, $12, $81, $ff, $ff

bottom_right:
.byte $a1, $a3, $c1, $ff, $15, $81, $00, $81, $13, $81, $15, $ff, $ff, $80

palette_index:
.byte 1,   2,   0,   0,   3,   3,   3,   3,   3,   3,   3,   2,   2,   2


;COLOR_BLACK     = #$0f
;COLOR_WHITE     = #$20

.segment "ZEROPAGE"
frame_done:     .res 1
scroll_x:       .res 1
zp_temp_1:      .res 1
zp_temp_2:      .res 1
zp_temp_3:      .res 1
current_low:    .res 1 ; Low byte of current address, this is a memory pointer for indirect indexing
current_high:   .res 1 ; High byte of current address
selected_board_index: .res 1 ; debug: requested board index (0-based)
selected_board_offset: .res 1 ; debug: byte offset used into 'boards' (.word table)
ppu_ctrl:       .res 1


.segment "HEADER"

INES_MAPPER = 0 ; 0 = NROM, 1 = MMC 1
INES_MIRROR = 1 ; 0 = horizontal mirroring, 1 = vertical mirroring
INES_SRAM   = 0 ; 1 = battery backed SRAM at $6000-7FFF

.byte 'N', 'E', 'S', $1A ; ID
.byte $02 ; 16k PRG chunk count
.byte $01 ; 8k CHR chunk count
.byte INES_MIRROR | (INES_SRAM << 1) | ((INES_MAPPER & $f) << 4)
.byte (INES_MAPPER & %11110000)
.byte $0, $0, $0, $0, $0, $0, $0, $0 ; padding

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

; Load board index 9 (boards is a .word table - low,high pairs)
; Save debug info: requested index and computed byte offset
lda #$A            ; requested board index (0-based)
sta selected_board_index
asl                 ; multiply by 2 to get byte offset into .word table
tay                 ; Y = offset
sty selected_board_offset
lda boards, y
sta current_low
lda boards+1, y
sta current_high

; Set PPUCTRL to nametable 0 ($2000)
lda #$80            ; Enable NMI, nametable 0
sta ppu_ctrl
sta $2000

lda #>NT0
sta PPU_ADDRESS
lda #<NT0
sta PPU_ADDRESS

JSR board_to_ppu_load

lda #$00
sta PPU_SCROLL
sta PPU_SCROLL

; Load board index 9 (boards is a .word table - low,high pairs)
; Save debug info: requested index and computed byte offset
lda #$9            ; requested board index (0-based)
sta selected_board_index
asl                 ; multiply by 2 to get byte offset into .word table
tay                 ; Y = offset
sty selected_board_offset
lda boards, y
sta current_low
lda boards+1, y
sta current_high

; Switch to nametable 1 before writing to it
lda #$81            ; Enable NMI, nametable 1
sta ppu_ctrl
sta $2000

lda #>NT1
sta PPU_ADDRESS
lda #<NT1
sta PPU_ADDRESS

JSR board_to_ppu_load

; Switch back to nametable 0
lda #$80            ; Enable NMI, nametable 0
sta ppu_ctrl
sta $2000

lda #$20
sta PPU_ADDRESS
lda #$00
sta PPU_ADDRESS

; reset scroll location to top-left of screen
lda #$00
sta PPU_SCROLL
sta PPU_SCROLL

; reset scroll location to top-left of screen
; lda #$00
; sta PPU_SCROLL
; sta PPU_SCROLL

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
; Ensure our copy of PPUCTRL holds the NMI enable bit and initial nametable
; bit7 = NMI enable, bits1-0 = base nametable (0 -> $2000)
sta ppu_ctrl
; Enable NMI in the actual PPUCTRL as well
sta $2000  ; enable NMI
lda #$1e
sta $2001  ; enable rendering
lda #$ff
sta $4010  ; enable DMC IRQs

MainLoop:
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
    inc scroll_x
    bne no_new_page
    ; We're near the right, prepare to switch nametables
    lda ppu_ctrl
    eor #$01           ; Toggle nametable bit
    sta ppu_ctrl
    lda #$00           ; Reset scroll_x when switching nametables
    sta scroll_x
    no_new_page:
    ;inc $00
    ; inc $0203
jmp MainLoop

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

    lda ppu_ctrl
    sta $2000
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


; Loads the entire board from the ROM to the PPU. Must be done with the PPU OFF.
; IN
; current_low/high points to the board array in the ROM
board_to_ppu_load:
    ldx #$0
    ldy #$0
    ; PPU_DATA requires a stream of memory. We are using 2x2 tiles. Act accordingly.

    ldy #$0 ; 15 x 16 = 240, we don't ever need to carry to the high byte.
    lda #$0
    sta zp_temp_1 ; start of row
    lda #$10 ; 16
    sta zp_temp_2 ; end of row
    lda #$0
    sta zp_temp_3 ; row count
    @fory:
        ldy zp_temp_1
        ; Loop through the top row
        @forx:
            ; Transfer the current memory location to the PPU
            ; The board stores indexes into a series of arrays to represent the 2x2 tile
            lda (current_low), y
            tax
            lda top_left, x
            ;jsr tile_convert
            sta PPU_DATA ; PPU memory
            lda top_right, x
            ;jsr tile_convert
            sta PPU_DATA ; PPU memory
            
            ; Increment the address
            iny
            cpy zp_temp_2 ; 16
        bne @forx
        ; Loop through the same row on the board again, this time to do the bottom row
        ldy zp_temp_1
        @forx2:
            ; Transfer the current memory location to the PPU
            ; The board stores indexes into a series of arrays to represent the 2x2 tile
            lda (current_low), y
            tax
            lda bottom_left, x
            sta PPU_DATA ; PPU memory
            lda bottom_right, x
            sta PPU_DATA ; PPU memory
            
            ; Increment the address
            iny
            cpy zp_temp_2 ; 16
        bne @forx2

        lda zp_temp_1
        clc
        adc #$10
        sta zp_temp_1
        lda zp_temp_2
        clc
        adc #$10
        sta zp_temp_2
        inc zp_temp_3
        lda zp_temp_3 ; row count
        cmp #$f ; 15
    bne @fory
rts




.segment "VECTORS"
	.addr nmi, reset, 0

.segment "CHR_BANK_0"
    .incbin "sprites.chr"