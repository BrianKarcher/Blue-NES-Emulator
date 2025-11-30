#include "Bus.h"
#include "nes_ppu.h"
#include "Cartridge.h"
#include "Core.h"
#include "Input.h"

Bus::Bus() {
	// Constructor implementation (if needed)
}

void Bus::Initialize(Core* core)
{
    cpuRAM.fill(0); // Clear CPU RAM
    this->core = core;
    this->cpu = &this->core->cpu;
    this->ppu = &this->core->ppu;
    this->cart = &this->core->cart;
	this->apu = &this->core->apu;
	this->input = &this->core->input;
    //this->core->g_bufferSize = cpuRAM.size();
    //this->core->g_buffer = cpuRAM.data();
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
    else if (addr >= 0x4000 && addr <= 0x4017)
    {
        // APU and I/O registers
        if (addr == 0x4015)
        {
            // APU Status register (read)
            data = apu->read_register(addr);
        }
        else if (addr == 0x4016) {
			data = input->ReadController1();
        }
        else if (addr == 0x4017)
        {
			data = input->ReadController2();
        }
        else
        {
            // Other APU registers are write-only, return open bus
            data = 0x00;
        }
    }
    else if (addr >= 0x6000 && addr < 0x8000) {
        data = cart->ReadPRGRAM(addr);
    }
    else if (addr >= 0x8000)
    {
        // Cartridge space (PRG ROM/RAM)
        data = cart->ReadPRG(addr);
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
    if (addr <= 0x1FFF)
    {
        // Check for blown stack
        if (cpu->GetSP() == 0x05) {
            //OutputDebugStringW(L"Stack blown!");
            int i = 0;
        }
        if (addr >= 0x0200 && addr <= 0x02FF) {
            int i = 0;
		}
        if (data != 0x00) {
			//printf("Write to RAM at %04X: %02X\n", addr, data);
            int i = 0;
        }
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
        //wchar_t buf[128];
        //swprintf_s(buf, L"BUS WRITE: %04X <- %02X\n", addr, data);
        //OutputDebugStringW(buf);
        // APU and I/O registers
        if (addr >= 0x4000 && addr <= 0x4013)
        {
            // APU sound registers
            apu->write_register(addr, data);
        }
        else if (addr == 0x4015)
        {
            // APU Status/Control register
            apu->write_register(addr, data);
        }
        else if (addr == 0x4016)
        {
            input->Poll();
            // Controller 1 / APU test register (write)
            // TODO: implement controller writes (strobe)
        }
        else if (addr == 0x4017)
        {
            // APU Frame Counter
            apu->write_register(addr, data);
        }
    }
    else if (addr >= 0x6000 && addr < 0x8000) {
        cart->WritePRGRAM(addr, data);
    }
    else if (addr >= 0x8000)
    {
        // Cartridge space (PRG ROM/RAM)
        cart->WritePRG(addr, data);
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