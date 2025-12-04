#include "Nes.h"

Nes::Nes() : bus(cpu, ppu, apu, input, cart) {
	cpu.connectBus(&bus);
}