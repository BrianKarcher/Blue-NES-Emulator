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

	};

	AddressingMode instMode[256] = {
		// 0    1	  2     3	  4    5    6     7    8    9    A     B     C    D    E     F
		IMP, INDX, NONE, NONE, NONE,  ZP,  ZP, NONE, IMP, IMM, ACC, NONE, NONE, ABS, ABS, NONE, // 0
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