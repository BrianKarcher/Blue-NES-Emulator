This is a simple emulator for the NES. I plan to expand it out to be 100% compatible with:
- 6502 processor.
- PPU.
- MMC1 (including SRAM and saving)
- Savestates

Maybe's if I have time:
- Bank switching

Everything I know is from:
https://www.nesdev.org/wiki/NES_reference_guide

What I do NOT plan to add compatibility for is:
The APU (Audio processing unit). I may add this sometime down the road but it is a lower priority as audio is complicated.

When designing and creating an emulator for NES games, there are several considerations we have to take into account.

The first major one is how the CPU, PPU and memory all work together. The CPU and PPU have no direct communication. There is no system-wide bus. Your code will run on the CPU. You have no direct access whatsoever to the PPU. The PPU is hard-coded and you can change nothing.

The NES contains an 8-bit processor that can access a 16-bit memory address.
The CPU can access a memory range of 65 KB. Of which 2 KB is dedicated to system RAM. ($0000 to $07FF). This then gets mirrored from $0800-$1FFF. $2000 to $2007 are for the PPU registers. $2008 to $3FFF are mirrors of the PPU registers. $4000 to $4017 are APU and I/O registers. $4018 to $401F is APU and I/O functionality that is normally disabled.

$6000-$7FFF is cartridge RAM, when present (This is where your saves are stored. It is also a work area as it is vastly larger than the built-in RAM).
$8000-$FFFF is the cartridge ROM and mapper registers.

Very important interrupt vectors:
$FFFA-$FFFB: NMI vector.
$FFFC-$FFFD: Reset vector.
$FFFE-$FFFF: IRQ vector.

The PPU has its own memory which can be researched at nesdev.org.

Where there be dragons:
The NES is a parallel processing system.
The CPU and PPU have limited direct communications. To send a command to the PPU, the CPU sets a byte in certain locations in memory. The PPU listens to this write and responds accordingly. This should NEVER happen while the PPU is rendering an image. Doing so will corrput the screen at best.

While the two chips are separate, they do both access the CPU's memory directly. If I decide to put the CPU and PPU into separate threads I need to keep this in mind.

The CPU has the ability to instruct the PPU to do an OAM DMA transfer. This transfer is a fast memory copy from the CPU's memory to the PPU's memory. While the instruction is instantiated by the CPU, it is handled by the PPU. During this transfer the CPU is disabled (meaning your code stops running) to prevent it from accessing its RAM. After it is complete the CPU is enabled.

The DMA transfer must always happen during the NMI cyclce.


High-level architecture

The NES does this during the screen drawing:
1. (Starting screen draw). The PPU draws the current frame while the CPU prepares the next frame. Each only accesses their own memory.
..................
..................
..................
..................
2. (Screen draw complete, the PPU trips the NMI vector on the CPU while the ray repositions to the top-left corner of the screen in preparation of the next frame)
2.1 The CPU sends the PPU data for the next frame
2.2 When the television's positioning of the raygun is complete, go back to step 1.

If the CPU is not complete for the next frame before step 2 above, the program must ignore the NMI vector and the PPU redraws the current frame. This is what causes slowdown on the NES.

If the CPU is not done sending data to the PPU by step 2.2 above... this is a very bad case as the CPU must not send data to the PPU while the screen is drawing. The frame gets garbled.

Mimcking how the PPU reacts to this invalid CPU access is hard to replicate. We need to find a way to keep both of them in sync. E.g., if the CPU is on cycle 12,000, the PPU also needs to be on the same cycle.

The "correct" approach is to have the PPU match what the NES hardware would be doing at any given microsecond. If the PPU is rendering a particular tile in the tilemap, we would need to be at the same spot.

This is particulary arduous and difficult. I'll go the easy route and blit to the screen as soon as possible and assume the CPU is acting properly.

==========

https://www.nesdev.org/wiki/CPU
We also want to match the CPU cycles. The NES CPU is ~ 1.79 Mhz. Per frame, the NES CPU can perform 29,780 cycles.

For information on how many cycles each instruction takes, reference https://www.nesdev.org/obelisk-6502-guide/reference.html. This link also includes the opcode for each instruction as well as the bytes which will obviously be very important to us :)

