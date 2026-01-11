#include "Bus.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Input.h"
#include "CPU.h"
#include "APU.h"
#include "MemoryMapper.h"
#include "OpenBusMapper.h"
#include "Serializer.h"

Bus::Bus(CPU& cpu, PPU& ppu, APU& apu, Input& input, Cartridge& cart, OpenBusMapper& openBus)
    : cpu(cpu), ppu(ppu), apu(apu), input(input), cart(cart), openBus(openBus) {
    // This may not be needed
    ramMapper.cpuRAM.fill(0);
	memoryMap = new MemoryMapper*[0x10000]; // 64KB address space
	for (int i = 0; i < 0x10000; i++) {
		memoryMap[i] = &openBus;
	}
}

Bus::~Bus() {
	delete[] memoryMap;
}

void Bus::reset() {
}

void Bus::registerAdd(uint16_t start, uint16_t end, MemoryMapper* mapper) {
    for (uint32_t addr = start; addr <= end; addr++) {
        memoryMap[addr] = mapper;
    }
}

void Bus::initialize() {
	// Initialize CPU RAM mapping
	registerAdd(0x0000, 0x1FFF, (MemoryMapper*)&ramMapper);
}

uint8_t Bus::read(uint16_t addr) {
	uint8_t val = memoryMap[addr]->read(addr);
	openBus.setOpenBus(val);
	return val;
}

uint8_t Bus::peek(uint16_t addr) {
	return memoryMap[addr]->peek(addr);
}

void Bus::write(uint16_t addr, uint8_t data) {
	openBus.setOpenBus(data);
	memoryMap[addr]->write(addr, data);
}

bool Bus::IrqPending() {
	return apu.get_irq_flag() || cart.mapper->IrqPending();
}

void Bus::Serialize(Serializer& serializer) {
	InternalMemoryState state;
	for (size_t i = 0; i < 2048; i++) {
		state.internalMemory[i] = ramMapper.cpuRAM[i];
	}
	serializer.Write(state);
}

void Bus::Deserialize(Serializer& serializer) {
	InternalMemoryState state;
	serializer.Read(state);
	for (size_t i = 0; i < 2048; i++) {
		ramMapper.cpuRAM[i] = state.internalMemory[i];
	}
}

// Holding onto these for more load testing/confirmation of various mappers
//uint8_t Bus::read(uint16_t addr)
//{
//    uint8_t data = 0x00;
//
//    if (addr <= 0x1FFF)
//    {
//        // Internal RAM, mirrored every 2KB
//        data = cpuRAM[addr & 0x07FF];
//    }
//    else if (addr <= 0x3FFF)
//    {
//        // PPU registers, mirrored every 8 bytes
//        data = ppu.read_register(0x2000 + (addr & 0x7));
//    }
//    else if (addr >= 0x4000 && addr <= 0x4017)
//    {
//        // APU and I/O registers
//        if (addr == 0x4015)
//        {
//            // APU Status register (read)
//            data = apu.read_register(addr);
//        }
//        else if (addr == 0x4016) {
//            data = input.ReadController1();
//        }
//        else if (addr == 0x4017)
//        {
//            data = input.ReadController2();
//        }
//        else
//        {
//            // Other APU registers are write-only, return open bus
//            data = 0x00;
//        }
//    }
//    else if (addr >= 0x6000 && addr < 0x8000) {
//        data = cart.ReadPRGRAM(addr);
//    }
//    else if (addr >= 0x8000)
//    {
//        // Cartridge space (PRG ROM/RAM)
//        data = cart.ReadPRG(addr);
//    }
//    else
//    {
//        // APU, etc. (not implemented yet)
//        data = 0x00;
//    }
//
//    return data;
//}
//
//void Bus::write(uint16_t addr, uint8_t data)
//{
//    if (addr <= 0x1FFF)
//    {
//        // Check for blown stack
//        if (cpu.GetSP() == 0x05) {
//            //OutputDebugStringW(L"Stack blown!");
//            int i = 0;
//        }
//        if (addr >= 0x0200 && addr <= 0x02FF) {
//            int i = 0;
//        }
//        if (data != 0x00) {
//            //printf("Write to RAM at %04X: %02X\n", addr, data);
//            int i = 0;
//        }
//        cpuRAM[addr & 0x07FF] = data;
//    }
//    else if (addr >= 0x2000 && addr <= 0x3FFF)
//    {
//        // Write to PPU registers
//        ppu.write_register(0x2000 + (addr & 0x7), data);
//    }
//    else if (addr == 0x4014)
//    {
//        // === OAM DMA ===
//        performDMA(data);
//    }
//    else if (addr >= 0x4000 && addr <= 0x4017)
//    {
//        //wchar_t buf[128];
//        //swprintf_s(buf, L"BUS WRITE: %04X <- %02X\n", addr, data);
//        //OutputDebugStringW(buf);
//        // APU and I/O registers
//        if (addr >= 0x4000 && addr <= 0x4013)
//        {
//            // APU sound registers
//            apu.write_register(addr, data);
//        }
//        else if (addr == 0x4015)
//        {
//            // APU Status/Control register
//            apu.write_register(addr, data);
//        }
//        else if (addr == 0x4016)
//        {
//            input.Poll();
//            // Controller 1 / APU test register (write)
//            // TODO: implement controller writes (strobe)
//        }
//        else if (addr == 0x4017)
//        {
//            // APU Frame Counter
//            apu.write_register(addr, data);
//        }
//    }
//    else if (addr >= 0x6000 && addr < 0x8000) {
//        cart.WritePRGRAM(addr, data);
//    }
//    else if (addr >= 0x8000)
//    {
//        // Cartridge space (PRG ROM/RAM)
//        cart.WritePRG(addr, data);
//    }
//    else
//    {
//        // Cartridge or open bus
//    }
//}