#pragma once
#include <cstdint>

#define BUTTON_A 0x01
#define BUTTON_B 0x02
#define BUTTON_SELECT 0x04
#define BUTTON_START 0x08
#define BUTTON_UP 0x10
#define BUTTON_DOWN 0x20
#define BUTTON_LEFT 0x40
#define BUTTON_RIGHT 0x80

class Input
{
public:
	Input();
	void ButtonDown(int key);
	void ButtonUp(int key);
	void Poll();
	uint8_t ReadController1();
	uint8_t ReadController2();

private:
	uint8_t controller1;
	uint8_t controller1_stream;
	uint8_t controller2;
	uint8_t controller2_stream;
};