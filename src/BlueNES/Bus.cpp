#include "Bus.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Input.h"
#include "CPU.h"
#include "APU.h"
#include "MemoryMapper.h"

Bus::Bus(Processor_6502& cpu, PPU& ppu, APU& apu, Input& input, Cartridge& cart)
    : cpu(cpu), ppu(ppu), apu(apu), input(input), cart(cart) {
    // This may not be needed
    ramMapper.cpuRAM.fill(0);
	memoryMap = new MemoryMapper*[0x10000]; // 64KB address space
}

Bus::~Bus() {
	delete[] memoryMap;
}

void Bus::reset() {
	// Probably not necessary
    ramMapper.cpuRAM.fill(0);
}

void Bus::registerAdd(uint16_t start, uint16_t end, MemoryMapper* mapper) {
    for (uint16_t addr = start; addr <= end; addr++) {
        memoryMap[addr] = mapper;
    }
}

void Bus::initialize() {
	// Initialize CPU RAM mapping
	registerAdd(0x0000, 0x1FFF, (MemoryMapper*)&ramMapper);
}

uint8_t Bus::read(uint16_t addr) {
	return memoryMap[addr]->read(addr);

    
    else if (addr >= 0x4000 && addr <= 0x4017)
    {
        // APU and I/O registers
        if (addr == 0x4015)
        {
            // APU Status register (read)
            data = apu.read_register(addr);
        }
        else if (addr == 0x4016) {
			data = input.ReadController1();
        }
        else if (addr == 0x4017)
        {
			data = input.ReadController2();
        }
        else
        {
            // Other APU registers are write-only, return open bus
            data = 0x00;
        }
    }
    else if (addr >= 0x6000 && addr < 0x8000) {
        data = cart.ReadPRGRAM(addr);
    }
    else if (addr >= 0x8000)
    {
        // Cartridge space (PRG ROM/RAM)
        data = cart.ReadPRG(addr);
	}
    else
    {
        // APU, etc. (not implemented yet)
		data = 0x00;
    }

    return data;
}

void Bus::write(uint16_t addr, uint8_t data)
{
	memoryMap[addr]->write(addr, data);
    return;

    else if (addr == 0x4014)
    {
        // === OAM DMA ===
        performDMA(data);
    }
    else if (addr >= 0x4000 && addr <= 0x4017)
    {
        //wchar_t buf[128];
        //swprintf_s(buf, L"BUS WRITE: %04X <- %02X\n", addr, data);
        //OutputDebugStringW(buf);
        // APU and I/O registers
        if (addr >= 0x4000 && addr <= 0x4013)
        {
            // APU sound registers
            apu.write_register(addr, data);
        }
        else if (addr == 0x4015)
        {
            // APU Status/Control register
            apu.write_register(addr, data);
        }
        else if (addr == 0x4016)
        {
            input.Poll();
            // Controller 1 / APU test register (write)
            // TODO: implement controller writes (strobe)
        }
        else if (addr == 0x4017)
        {
            // APU Frame Counter
            apu.write_register(addr, data);
        }
    }
    else if (addr >= 0x6000 && addr < 0x8000) {
        cart.WritePRGRAM(addr, data);
    }
    else if (addr >= 0x8000)
    {
        // Cartridge space (PRG ROM/RAM)
        cart.WritePRG(addr, data);
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
        ppu.oam[ppu.oamAddr++] = data;
    }

    // Timing penalty (513 or 514 CPU cycles)
    int extraCycle = (cpu.GetCycleCount() & 1) ? 1 : 0;
    cpu.AddCycles(513 + extraCycle);
}