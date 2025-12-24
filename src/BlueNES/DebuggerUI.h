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
	enum AddressingMode {
		ACC,
		IMP,
		IMM,
		ZP,
		ZPX,
		ZPY,
		ABS,
		ABSX,
		ABSY,
		IND,
		INDX,
		INDY,
		REL,
		NONE
	};

	const char* instSt[256] = {
		//  0      1	  2 	 3      4      5      6	     7      8	   9      A      B      C     D      E      F
		"BRK", "ORA", "DMP", "DMP", "DMP", "ORA", "ASL", "DMP", "PHP", "ORA", "ASL", "DMP", "DMP", "ORA", "ASL", "DMP", // 0
		"BPL", "ORA", "DMP", "DMP", "DMP", "ORA", "ASL", "DMP", "CLC", "ORA", "DMP", "DMP", "DMP", "ORA", "ASL", "DMP", // 1
		"JSR", "AND", "DMP", "DMP", "BIT", "AND", "ROL", "DMP", "PLP", "AND", "ROL", "DMP", "BIT", "AND", "ROL", "DMP", // 2
		"BMI", "AND", "DMP", "DMP", "DMP", "AND", "ROL", "DMP", "SEC", "AND", "DMP", "DMP", "DMP", "AND", "ROL", "DMP", // 3
		"RTI", "EOR", "DMP", "DMP", "DMP", "EOR", "LSR", "DMP", "PHA", "EOR", "LSR", "DMP", "JMP", "EOR", "LSR", "DMP", // 4
		"BVC", "EOR", "DMP", "DMP", "DMP", "EOR", "LSR", "DMP", "CLI", "EOR", "DMP", "DMP", "DMP", "EOR", "LSR", "DMP", // 5
		"RTS", "ADC", "DMP", "DMP", "DMP", "ADC", "ROR", "DMP", "PLA", "ADC", "ROR", "DMP", "JMP", "ADC", "ROR", "DMP", // 6

	};

	AddressingMode instMode[256] = {
		// 0    1	  2     3	  4    5    6     7    8     9      A     B     C     D     E     F
		IMP, INDX, NONE, NONE, NONE,  ZP,  ZP, NONE, IMP,  IMM,   ACC, NONE, NONE,  ABS,  ABS, NONE, // 0
		REL, INDY, NONE, NONE, NONE, ZPX, ZPX, NONE, IMP, ABSY,  NONE, NONE, NONE, ABSX, ABSX, NONE, // 1
		ABS, INDX, NONE, NONE, ZP,    ZP,  ZP, NONE, IMP,  IMM,   ACC, NONE,  ABS,  ABS,  ABS, NONE, // 2
		REL, INDY, NONE, NONE, NONE, ZPX, ZPX, NONE, IMP, ABSY,  NONE, NONE, NONE, ABSX, ABSX, NONE, // 3
		IMP, INDX, NONE, NONE, NONE,  ZP,  ZP, NONE, IMP,  IMM,   ACC, NONE,  ABS,  ABS,  ABS, NONE, // 4
		REL, INDY, NONE, NONE, NONE, ZPX, ZPX, NONE, IMP, ABSY,  NONE, NONE, NONE, ABSX, ABSX, NONE, // 5
		IMP, INDX, NONE, NONE, NONE,  ZP,  ZP, NONE, IMP,  IMM,   ACC, NONE,  IND,  ABS,  ABS, NONE, // 6
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