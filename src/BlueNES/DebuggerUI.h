#pragma once
#include <Windows.h>
#include <array>
#include <string>

class Core;

class DebuggerUI
{
public:
	DebuggerUI(HINSTANCE hInst, Core& core);
private:
	const char* instSt[256] = {
	//  0      1	  2 	 3      4      5      6	     7      8	   9      A      B      C      D      E      F
		"BRK", "ORA", "DMP", "DMP", "DMP", "ORA", "ASL", "DMP", "PHP", "ORA", "ASL", "DMP", "DMP", "ORA", "ASL", "DMP", // 0
		"BPL", "ORA", "DMP", "DMP", "DMP", "ORA", "ASL", "DMP", "CLC", "ORA", "DMP", "DMP", "DMP", "ORA", "ASL", "DMP", // 1
		"JSR", "AND", "DMP", "DMP", "BIT", "AND", "ROL", "DMP", "PLP", "AND", "ROL", "DMP", "BIT", "AND", "ROL", "DMP", // 2
		"BMI", "AND", "DMP", "DMP", "DMP", "AND", "ROL", "DMP", "SEC", "AND", "DMP", "DMP", "DMP", "AND", "ROL", "DMP", // 3
		"RTI", "EOR", "DMP", "DMP", "DMP", "EOR", "LSR", "DMP", "PHA", "EOR", "LSR", "DMP", "JMP", "EOR", "LSR", "DMP", // 4
		"BVC", "EOR", "DMP", "DMP", "DMP", "EOR", "LSR", "DMP", "CLI", "EOR", "DMP", "DMP", "DMP", "EOR", "LSR", "DMP", // 5
		"RTS", "ADC", "DMP", "DMP", "DMP", "ADC", "ROR", "DMP", "PLA", "ADC", "ROR", "DMP", "JMP", "ADC", "ROR", "DMP", // 6
		"BVS", "ADC", "DMP", "DMP", "DMP", "ADC", "ROR", "DMP", "SEI", "ADC", "DMP", "DMP", "DMP", "ADC", "ROR", "DMP", // 7
		"DMP", "STA", "DMP", "DMP", "STY", "STA", "STX", "DMP", "DEY", "DMP", "TXA", "DMP", "STY", "STA", "STX", "DMP", // 8
		"BCC", "STA", "DMP", "DMP", "STY", "STA", "STX", "DMP", "TYA", "STA", "TXS", "DMP", "DMP", "STA", "DMP", "DMP", // 9
		"LDY", "LDA", "LDX", "DMP", "LDY", "LDA", "LDX", "DMP", "TAY", "LDA", "TAX", "DMP", "LDY", "LDA", "LDX", "DMP", // A
		"BCS", "LDA", "DMP", "DMP", "LDY", "LDA", "LDX", "DMP", "CLV", "LDA", "TSX", "DMP", "LDY", "LDA", "LDX", "DMP", // B
		"CPY", "CMP", "DMP", "DMP", "CPY", "CMP", "DEC", "DMP", "INY", "CMP", "DEX", "DMP", "CPY", "CMP", "DEC", "DMP", // C
		"BNE", "CMP", "DMP", "DMP", "DMP", "CMP", "DEC", "DMP", "CLD", "CMP", "DMP", "DMP", "DMP", "CMP", "DEC", "DMP", // D
		"CPX", "SBC", "DMP", "DMP", "CPX", "SBC", "INC", "DMP", "INX", "SBC", "NOP", "DMP", "CPX", "SBC", "INC", "DMP", // E
		"BEQ", "SBC", "DMP", "DMP", "DMP", "SBC", "INC", "DMP", "SED", "SBC", "DMP", "DMP", "DMP", "SBC", "INC", "DMP", // F

	};

	HINSTANCE hInst;
	HWND hDebuggerWnd = NULL;
	HWND hList = nullptr;
	static LRESULT CALLBACK WndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);
	Core& _core;
};