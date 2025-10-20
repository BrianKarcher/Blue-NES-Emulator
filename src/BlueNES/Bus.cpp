#include "Bus.h"

Bus::Bus() {
	// Constructor implementation (if needed)

}

uint8_t Bus::read(uint16_t addr)
{
    uint8_t data = 0x00;

    if (addr <= 0x1FFF)
    {
        // Internal RAM, mirrored every 2KB
        data = cpuRAM[addr & 0x07FF];
    }
    else if (addr <= 0x3FFF)
    {
        // PPU registers, mirrored every 8 bytes
        data = ppu->read_register(0x2000 + (addr & 0x7));
    }
    else if (addr == 0x4016 || addr == 0x4017)
    {
        // Controller ports (optional for now)
        // TODO: implement controller reads
    }
    else
    {
        // Cartridge, APU, etc. (not implemented yet)
    }

    return data;
}

void Bus::write(uint16_t addr, uint8_t data)
{
    if (addr <= 0x1FFF)
    {
        cpuRAM[addr & 0x07FF] = data;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        // Write to PPU registers
        ppu->write_register(0x2000 + (addr & 0x7), data);
    }
    else if (addr == 0x4014)
    {
        // === OAM DMA ===
        performDMA(data);
    }
    else if (addr >= 0x4000 && addr <= 0x4017)
    {
        // APU or I/O registers
        // TODO: handle APU writes
    }
    else
    {
        // Cartridge or open bus
    }
}

void Bus::performDMA(uint8_t page)
{
    uint16_t baseAddr = page << 8; // high byte from $4014
    for (int i = 0; i < 256; i++)
    {
        uint8_t data = read(baseAddr + i);
        ppu->oam[ppu->oamAddr++] = data;
    }

    // Timing penalty (513 or 514 CPU cycles)
    int extraCycle = (cpu->GetCycleCount() & 1) ? 1 : 0;
    cpu->AddCycles(513 + extraCycle);
}