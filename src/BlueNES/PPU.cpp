//#include <algorithm>
//#include "PPU.h"
//#include <WinUser.h>
//#include "Bus.h"
//#include "Core.h"
//#include "CPU.h"
//
//HWND m_hwnd;
//
//// Construct
//PPU::PPU() {
//    // zero memory
//    memset(m_vram.data(), 0, m_vram.size());
//    //memset(OAM.data(), 0xFF, OAM.size());
//    //framebuffer.fill(0);
//    framebuffer.fill(0xFF000000); // Initialize to opaque black
//	OAM.fill(0xFF);
//	m_ppuCtrl = 0;
//	oamAddr = 0;
//}
//
//void PPU::set_hwnd(HWND hwnd) {
//	m_hwnd = hwnd;
//}
//
//void PPU::write_register(uint16_t addr, uint8_t value)
//{
//	// addr is in the range 0x2000 to 0x2007
//	// It is the CPU that writes to these registers
//	// addr is mirrored every 8 bytes up to 0x3FFF so we mask it
//	switch (addr) {
//		case PPUCTRL:
//			m_ppuCtrl = value;
//			// Set nametable bits in register t
//			t = (t & ~INTERNAL_NAMETABLE) | ((m_ppuCtrl & 0x03) << 10);
//			break;
//		case PPUMASK: // PPUMASK
//			m_ppuMask = value;
//			break;
//		case PPUSTATUS: // PPUSTATUS (read-only)
//			// Ignore writes to PPUSTATUS
//			break;
//		case OAMADDR: // OAMADDR
//			oamAddr = value;
//			break;
//		case OAMDATA: // OAMDATA
//			OAM[oamAddr++] = value;
//			break;
//		case PPUSCROLL: // PPUSCROLL
//			// Handle PPUSCROLL write here
//			if (!w)
//			{
//				// First write sets horizontal scroll
//				t = (t & ~INTERNAL_COARSE_X) | (value >> 3);
//				x = value & 0x07;
//				//tempVramAddr = (tempVramAddr & 0x00FF) | ((value & 0x3F) << 8); // First write (high byte)
//			}
//			else
//			{
//				// Second write sets vertical scroll
//				// Fine Y
//				t = (t & ~INTERNAL_FINE_Y) | ((value & 0x07) << 12);
//				// Coarse Y
//				t = (t & ~INTERNAL_COARSE_Y) | ((value & 0x1F) << 5);
//			}
//			// Note that writeToggle is shared with PPUADDR
//			// I retain to mimic hardware behavior
//			w = !w;
//			break;
//		case PPUADDR: // PPUADDR
//			// PPUADDR corrupts the t register.
//			// Games need to update PPUSCROLL and PPUCTRL after writing to PPUADDR
//			if (!w)
//			{
//				t = (t & ~0b11111100000000) | ((value & 0b111111) << 8);
//				// Zero this bit for reasons unknown
//				t = (t & ~0b100000000000000);
//
//				// The PPU address space is 14 bits (0x0000 to 0x3FFF), so we mask accordingly
//				//tempVramAddr = (tempVramAddr & 0x00FF) | ((value & 0x3F) << 8); // First write (high byte)
//				// vramAddr = (vramAddr & 0x00FF) | (value & 0x3F) << 8; // First write (high byte)
//			}
//			else
//			{
//				t = (t & ~0b11111111) | ((value & 0b11111111));
//				//tempVramAddr = (tempVramAddr & 0x7F00) | value; // Second write (low byte)
//				v = t;
//			}
//			// If the program doesn't do two writes in a row, the behavior is undefined.
//			// It's their fault if their code is broken.
//			w = !w;
//			break;
//		case PPUDATA: // PPUDATA
//			uint16_t vramAddr = v & 0b11111111111111;
//			write_vram(vramAddr, value);
//			// Increment VRAM address based on PPUCTRL setting (TODO: not implemented yet, default to 1)
//			if (vramAddr >= 0x3F00) {
//				// Palette data always increments by 1
//				v += 1;
//				return;
//			}
//			v += m_ppuCtrl & 0x04 ? 32 : 1;
//			break;
//	}
//}
//
//void PPU::write_vram(uint16_t addr, uint8_t value)
//{
//	addr &= 0x3FFF; // Mask to 14 bits
//	if (addr < 0x2000) {
//		// Write to CHR-RAM (if enabled)
//		bus->cart->WriteCHR(addr, value);
//		// Else ignore write (CHR-ROM is typically read-only)
//		return;
//	}
//	else if (addr < 0x3F00) {
//		// Name tables and attribute tables
//		addr &= 0x2FFF; // Mirror nametables every 4KB
//		addr = bus->cart->MirrorNametable(addr);
//		m_vram[addr] = value;
//		return;
//	}
//	else if (addr < 0x4000) {
//		// Palette RAM (mirrored every 32 bytes)
//		// 3F00 = 0011 1111 0000 0000
//		// 3F1F = 0011 1111 0001 1111
//		uint8_t paletteAddr = addr & 0x1F; // 0001 1111
//		if (paletteAddr % 4 == 0) {
//			// Handle mirroring of the background color by setting the address to 0x3F00
//			paletteAddr = 0;
//		}
//		paletteTable[paletteAddr] = value;
//		InvalidateRect(core->m_hwndPalette, NULL, FALSE); // Update palette window if open
//		return;
//	}
//}
//
//uint8_t PPU::read_register(uint16_t addr)
//{
//	switch (addr)
//	{
//		case PPUCTRL:
//		{
//			return m_ppuCtrl;
//		}
//		case PPUMASK:
//		{
//			// not typically readable, return 0
//			return 0;
//		}
//		case PPUSTATUS:
//		{
//			w = false; // Reset write toggle on reading PPUSTATUS
//			// Return PPU status register value and clear VBlank flag
//			uint8_t status = ppustatus;
//            ppustatus &= ~PPUSTATUS_VBLANK;
//			return status;
//		}
//		case OAMADDR:
//		{
//			return oamAddr;
//		}
//		case OAMDATA:
//		{
//			// Return OAM data at current OAMADDR
//			return OAM[oamAddr];
//		}
//		case PPUSCROLL:
//		{
//			// PPUSCROLL is write-only, return 0
//			return 0;
//		}
//		case PPUADDR:
//		{
//			// PPUADDR is write-only, return 0
//			return 0;
//		}
//		case PPUDATA:
//		{
//			uint16_t vramAddr = v & 0b11111111111111;
//			// Read from VRAM at current vramAddr
//			uint8_t value = ReadVRAM(vramAddr);
//			// Increment VRAM address based on PPUCTRL setting
//			if (vramAddr >= 0x3F00) {
//				// Palette data always increments by 1
//				v += 1;
//				return value;
//			}
//			v += m_ppuCtrl & 0x04 ? 32 : 1;
//			return value;
//		}
//	}
//
//	return 0;
//}
//
//uint8_t PPU::ReadVRAM(uint16_t addr)
//{
//	uint8_t value = 0;
//	if (addr < 0x2000) {
//		// Reading from CHR-ROM/RAM
//		value = bus->cart->ReadCHR(addr);
//	}
//	else if (addr < 0x3F00) {
//		// Reading from nametables and attribute tables
//		uint16_t mirroredAddr = addr & 0x2FFF; // Mirror nametables every 4KB
//		mirroredAddr = bus->cart->MirrorNametable(mirroredAddr);
//		value = ppuDataBuffer;
//		ppuDataBuffer = m_vram[mirroredAddr];
//	}
//	else if (addr < 0x4000) {
//		// Reading from palette RAM (mirrored every 32 bytes)
//		uint8_t paletteAddr = addr & 0x1F;
//		if (paletteAddr % 4 == 0) {
//			// Handle mirroring of the background color by setting the address to 0x3F00
//			paletteAddr = 0;
//		}
//		value = paletteTable[paletteAddr];
//		// But buffer is filled with the underlying nametable byte
//		uint16_t mirroredAddr = addr & 0x2FFF; // Mirror nametables every 4KB
//		mirroredAddr = bus->cart->MirrorNametable(mirroredAddr);
//		ppuDataBuffer = m_vram[mirroredAddr];
//	}
//	
//	return value;
//}
//
//
//
//// ---------------- Loopy helpers ----------------
//void PPU::incrementX() {
//    // if coarse X == 31: coarse X = 0, switch horizontal nametable
//    if ((v & 0x001F) == 31) {
//        v &= ~0x001F;
//        v ^= 0x0400;
//    }
//    else {
//        v += 1;
//    }
//}
//
//void PPU::incrementY() {
//    // if fine Y < 7 -> fine Y++
//    if ((v & 0x7000) != 0x7000) {
//        v += 0x1000; // fine Y++
//    }
//    else {
//        // fine Y = 0
//        v &= ~0x7000;
//        uint16_t y = (v & 0x03E0) >> 5; // coarse Y
//        if (y == 29) {
//            y = 0;
//            v ^= 0x0800; // switch vertical nametable
//        }
//        else if (y == 31) {
//            y = 0; // overflow to 0, no nametable switch
//        }
//        else {
//            y++;
//        }
//        v = (v & ~0x03E0) | (y << 5);
//    }
//}
//
//void PPU::copyHorizontalBitsFromTtoV() {
//    v = (v & ~0x041F) | (t & 0x041F);
//}
//
//void PPU::copyVerticalBitsFromTtoV() {
//    v = (v & ~0x7BE0) | (t & 0x7BE0);
//}
//
//// ---------------- Background shifters & load ----------------
//void PPU::loadBackgroundShifters() {
//    // load next tile pattern bytes into high 8-bits of shift registers
//    bg_shift_pattern_low = (bg_shift_pattern_low & 0xFF00) | next_pattern_low;
//    bg_shift_pattern_high = (bg_shift_pattern_high & 0xFF00) | next_pattern_high;
//
//    // attribute latches: if attribute bit is set, load 0xFF into low byte so that shifting yields correct palette bits
//    uint16_t low_fill = (next_attrib_byte & 0x01) ? 0xFF : 0x00;
//    uint16_t high_fill = (next_attrib_byte & 0x02) ? 0xFF : 0x00;
//
//    bg_shift_attrib_low = (bg_shift_attrib_low & 0xFF00) | low_fill;
//    bg_shift_attrib_high = (bg_shift_attrib_high & 0xFF00) | high_fill;
//}
//
//void PPU::shiftBackgroundRegisters() {
//    bg_shift_pattern_low <<= 1;
//    bg_shift_pattern_high <<= 1;
//    bg_shift_attrib_low <<= 1;
//    bg_shift_attrib_high <<= 1;
//}
//
//// ---------------- Background fetch sequence (8-cycle) ----------------
//// This follows the cycle offsets: dot % 8 == 1 -> NT, 3 -> AT, 5 -> PT low, 7 -> PT high, 0 -> load & incX
//void PPU::fetchBackgroundCycle() {
//    int cycleInTile = dot & 7; // dot % 8  (since dot increments 0..340)
//    switch (cycleInTile) {
//    case 1: {
//        // Nametable byte
//        uint16_t addr = 0x2000 | (v & 0x0FFF);
//        next_nt_byte = ReadVRAM(addr);
//        break;
//    }
//    case 3: {
//        // Attribute byte (derive using coarse coo3f24rds)
//        uint16_t addr = 0x23C0
//            | (v & 0x0C00)
//            | ((v >> 4) & 0x38)
//            | ((v >> 2) & 0x07);
//        uint8_t at = ReadVRAM(addr);
//        // select which two bits in attribute byte apply to this tile
//        // shift = (coarseY & 0x02) * 2 + (coarseX & 0x02)
//        int shift = ((v >> 4) & 4) | ((v >> 2) & 2); // compute as per loopy spec
//        // but easier: derive coarseX/coarseY:
//        int coarseX = v & 0x1F;
//        int coarseY = (v >> 5) & 0x1F;
//        int quadrant = ((coarseY & 2) ? 2 : 0) | ((coarseX & 2) ? 1 : 0);
//        // extract palette bits (2 bits)
//        next_attrib_byte = (at >> (quadrant * 2)) & 0x03;
//        break;
//    }
//    case 5: {
//        // Pattern low byte
//        uint16_t patternBase = ((ppuctrl & 0x10) ? 0x1000 : 0x0000); // background pattern table select ($2000 bit 4)
//        uint16_t tile = next_nt_byte;
//        uint16_t addr = patternBase + (tile * 16) + fineY();
//        next_pattern_low = ReadVRAM(addr);
//        break;
//    }
//    case 7: {
//        // Pattern high byte
//        uint16_t patternBase = ((ppuctrl & 0x10) ? 0x1000 : 0x0000);
//        uint16_t tile = next_nt_byte;
//        uint16_t addr = patternBase + (tile * 16) + fineY() + 8;
//        next_pattern_high = ReadVRAM(addr);
//        break;
//    }
//    case 0: {
//        // load shifters and increment coarse X
//        loadBackgroundShifters();
//        incrementX();
//        break;
//    }
//    default:
//        break;
//    }
//}
//
//// ---------------- Sprite evaluation (simplified) ----------------
//// Accurate sprite evaluation consumes cycles 257..320; for simplicity we evaluate here at 257
//void PPU::evaluateSpritesForNextScanline() {
//    // Clear secondary OAM
//    for (int i = 0; i < 8; ++i) {
//        secondaryOAM[i] = { 0xFF, 0xFF, 0xFF, 0xFF };
//    }
//    int found = 0;
//    uint8_t spriteHeight = (ppuctrl & 0x20) ? 16 : 8;
//
//    for (int i = 0; i < 64 && found < 8; ++i) {
//        uint8_t sy = OAM[i * 4 + 0];
//        int row = scanline - sy;
//        if (row >= 0 && row < spriteHeight) {
//            // copy this sprite into secondary OAM
//            secondaryOAM[found].y = OAM[i * 4 + 0];
//            secondaryOAM[found].tile = OAM[i * 4 + 1];
//            secondaryOAM[found].attr = OAM[i * 4 + 2];
//            secondaryOAM[found].x = OAM[i * 4 + 3];
//            if (i == 0) {
//                // sprite 0 is present on this line somewhere; sprite0Hit detection happens during pixel render
//            }
//            found++;
//        }
//    }
//    if (found > 8) spriteOverflow = true; // simplified; hardware has quirks on overflow set timing
//}
//
////void NesPPU::EvaluateSprites(int screenY, std::array<Sprite, 8>& newOam)
////{
////    // Evaluate sprites for this scanline
////    int spriteCount = 0;
////    for (int i = 0; i < 64; ++i) {
////        int spriteY = oam[i * 4]; // Y position of the sprite
////        if (spriteY > 0xF0) {
////            continue; // Empty sprite slot
////        }
////        int spriteHeight = 8; // For now, assume 8x8 sprites. TODO: Support 8x16 sprites.
////        if (screenY >= spriteY && screenY < (spriteY + spriteHeight)) {
////            if (m_scanline == 0) {
////                int test = 0;
////            }
////            if (spriteCount < 8) {
////                // Copy sprite data to new OAM
////                newOam[spriteCount].y = oam[i * 4];
////                newOam[spriteCount].tileIndex = oam[i * 4 + 1];
////                newOam[spriteCount].attributes = oam[i * 4 + 2];
////                newOam[spriteCount].x = oam[i * 4 + 3];
////                newOam[spriteCount].isSprite0 = (i == 0); // Mark if this is sprite 0
////                spriteCount++;
////            }
////            else {
////                // Sprite overflow - more than 8 sprites on this scanline
////                if (!hasOverflowBeenSet) {
////                    // Set sprite overflow flag only once per frame
////                    hasOverflowBeenSet = true;
////                    m_ppuStatus |= PPUSTATUS_SPRITE_OVERFLOW;
////                }
////                break;
////            }
////        }
////    }
////}
//
//// ---------------- Pixel rendering ----------------
//void PPU::renderPixel() {
//    int x = dot - 1; // visible pixel x [0..255]
//    int y = scanline; // pixel y [0..239]
//
//    // Default palette = universal background color
//    uint8_t bgPaletteIndex = 0;
//    bool bgOpaque = false;
//
//    if (bgEnabled()) {
//        // Get top bits from shifters using fine X
//        uint16_t bitMux = 0x8000 >> x; // NOT correct if x > 15; instead use shift registers' MSB
//        // Correct approach: use MSB of shift registers (we shift them every dot)
//        uint8_t bit0 = (bg_shift_pattern_low & 0x8000) ? 1 : 0;
//        uint8_t bit1 = (bg_shift_pattern_high & 0x8000) ? 1 : 0;
//        uint8_t palLow = (bg_shift_attrib_low & 0x8000) ? 1 : 0;
//        uint8_t palHigh = (bg_shift_attrib_high & 0x8000) ? 1 : 0;
//
//        uint8_t paletteIndex = (palHigh << 3) | (palLow << 2) | (bit1 << 1) | bit0;
//        bgOpaque = (bit0 | bit1) != 0;
//        bgPaletteIndex = paletteIndex & 0x0F;
//    }
//
//    // Sprite pixel (simplified): we check secondaryOAM sprites for an opaque pixel at current x
//    uint8_t spritePaletteIndex = 0;
//    bool spriteOpaque = false;
//    bool spritePriorityBehind = false;
//    bool spriteIsZero = false;
//
//    if (spriteEnabled()) {
//        for (int i = 0; i < 8; ++i) {
//            auto& s = secondaryOAM[i];
//            if (s.x == 0xFF) continue; // empty
//            int sx = s.x;
//            int sy = s.y;
//            int px = x - sx;
//            if (px < 0 || px >= 8) continue;
//
//            // read tile pattern (basic, ignoring flipping / height)
//            uint16_t patternBase = ((ppuctrl & 0x08) ? 0x1000 : 0x0000); // sprite table select is $2000 bit 3? (actually $2000 bit 3 is unused for sprites; typical mapper uses $2000 bit 5 for sprite height - be cautious)
//            // For simplicity, assume 8x8 and no flip; compute bit pattern for px
//            uint8_t tile = s.tile;
//            uint8_t fineY = (scanline - s.y) & 7;
//            uint16_t addr = patternBase + (tile * 16) + fineY;
//            uint8_t low = ReadVRAM(addr);
//            uint8_t high = ReadVRAM(addr + 8);
//            // Extract pixel bit (msb = leftmost)
//            int bit0 = (low >> (7 - px)) & 1;
//            int bit1 = (high >> (7 - px)) & 1;
//            uint8_t palIndex = ((s.attr & 3) << 2) | (bit1 << 1) | bit0;
//            if ((bit0 | bit1) != 0) {
//                spriteOpaque = true;
//                spritePaletteIndex = palIndex;
//                spritePriorityBehind = (s.attr & 0x20) != 0;
//                spriteIsZero = (&s == &secondaryOAM[0]); // if the first copied sprite is sprite0
//                break; // first non-transparent sprite in X wins
//            }
//        }
//    }
//
//    // Sprite0 hit: If sprite0 overlaps non-transparent background and both BG and sprites are enabled,
//    // and x != 255, and sprite0 hasn't been flagged yet.
//    if (!sprite0Hit && spriteIsZero && bgOpaque && spriteOpaque && bgEnabled() && spriteEnabled() && x != 255) {
//        // Determine if sprite belonged to primary OAM index 0 on this scanline -- we simplified above
//        // In a full implementation you should check whether that opaque sprite corresponds to original OAM slot 0.
//        sprite0Hit = true;
//        ppustatus |= 0x40; // set Sprite 0 Hit
//    }
//
//    // Final pixel selection: sprite over bg depending on priority
//    uint8_t finalPalette = 0;
//    if (spriteOpaque && (!bgOpaque || !spritePriorityBehind)) {
//        // sprite wins
//        finalPalette = spritePaletteIndex;
//    }
//    else {
//        finalPalette = bgPaletteIndex;
//    }
//
//    // Convert palette index to color (here we just read a byte from paletteTable and write to framebuffer)
//    if (finalPalette != 0) {
//        int i = 0;
//    }
//    uint8_t color = paletteTable[finalPalette & 0x1F];
//    // For demonstration store color value as ARGB-like 32-bit; user must map palette to RGB
//    //uint32_t pixel32 = 0xFF000000 | (color * 0x010101); // grayish mapping; replace with actual palette RGB
//    uint32_t pixel32 = m_nesPalette[color];
//    if (x >= 0 && x < 256 && y >= 0 && y < 240) {
//        framebuffer[y * 256 + x] = pixel32;
//    }
//}
//
//// ---------------- VBlank / NMI ----------------
//void PPU::setVBlank() {
//    ppustatus |= 0x80;
//    // NMI if enabled
//    if (ppuctrl & 0x80) {
//        if (!nmi_line) {
//            nmi_line = true;
//            nmi_occurred = true;
//            // signal CPU; CPU must sample/handle NMI at instruction boundaries and push PC/P status
//            bus->cpu->NMI();
//        }
//    }
//}
//
//void PPU::clearVBlank() {
//    ppustatus &= ~0x80;
//    nmi_line = false;
//    nmi_occurred = false;
//}
//
//// ---------------- Main tick() ----------------
//void PPU::tick() {
//    // Advance to next dot
//    dot++;
//    if (dot >= DOTS_PER_SCANLINE) {
//        dot = 0;
//        scanline++;
//        if (scanline >= SCANLINES_PER_FRAME) {
//            scanline = 0;
//        }
//    }
//
//    bool rendering = renderingEnabled();
//    bool visibleScanline = (scanline >= 0 && scanline <= 239);
//    bool preRenderLine = (scanline == 261);
//
//    // Pre-render start: clear vblank & sprite0 hit at dot 1
//    if (preRenderLine && dot == 1) {
//        clearVBlank();
//        m_frameComplete = false;
//        ppustatus &= 0x1F; // Clear VBlank, sprite 0 hit, and sprite overflow
//        sprite0Hit = false;
//        spriteOverflow = false;
//        // reset shifters? some emulators do here
//    }
//
//    // Background fetch and pipeline activity occurs on:
//    // visible scanlines (0..239) and pre-render (261) during certain dots
//    if (rendering) {
//        if ((visibleScanline || preRenderLine) &&
//            ((dot >= 1 && dot <= 256) || (dot >= 321 && dot <= 336))) {
//
//            // On each of these dots we run the fetch cycle (cycle % 8)
//            fetchBackgroundCycle();
//        }
//    }
//
//    // Pixel rendering (visible)
//    if (visibleScanline && dot >= 1 && dot <= 256) {
//        // shift shifters BEFORE rendering pixel (hardware shifts each cycle)
//        shiftBackgroundRegisters();
//        renderPixel();
//    }
//    else if ((preRenderLine || visibleScanline) && dot >= 321 && dot <= 336) {
//        // While in 321..336 we still load shifters and such for the next scanline,
//        // but don't render pixels (this is part of the fetch window).
//        // shift each dot as well to keep pipeline in sync.
//        shiftBackgroundRegisters();
//    }
//    else {
//        // On other dots we may still need to shift if rendering enabled:
//        if (rendering && dot >= 1 && dot <= 256) {
//            shiftBackgroundRegisters();
//        }
//    }
//
//    // On dot 256: increment Y
//    if (rendering && dot == 256 && (visibleScanline)) {
//        incrementY();
//    }
//
//    // On dot 257: copy horizontal bits from t to v and start sprite evaluation
//    if (rendering && dot == 257) {
//        copyHorizontalBitsFromTtoV();
//        // evaluate sprites (we do simple eager eval; full timing consumes 64 cycles 257..320)
//        evaluateSpritesForNextScanline();
//    }
//
//    // Pre-render only: dots 280..304 copy vertical bits from t to v
//    if (preRenderLine && dot >= 280 && dot <= 304 && rendering) {
//        copyVerticalBitsFromTtoV();
//    }
//
//    // VBlank set at scanline 241 dot 1
//    if (scanline == 241 && dot == 1) {
//        setVBlank();
//        m_frameComplete = true;
//    }
//
//    // Note: CPU sampling of /NMI must be handled in your CPU: when CPU finishes an instruction and sees the NMI line, it enters NMI sequence.
//    // We assert NMI via cpu->signalNMI() above; your CPU must handle it at instruction boundary timing.
//}