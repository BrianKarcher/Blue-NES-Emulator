#include "Input.h"

Input::Input(): controller1(0), controller2(0), controller1_stream(0), controller2_stream(0) {

}

void Input::ButtonDown(int key) {
	controller1 |= key;
	// Add cases for controller2 if needed
}

void Input::ButtonUp(int key) {
	controller1 &= ~key;
	// Add cases for controller2 if needed
}

void Input::Poll() {
	controller1_stream = controller1;
	controller2_stream = controller2;
}

uint8_t Input::ReadController1() {
	uint8_t ret = controller1_stream & 1;
	controller1_stream >>= 1;
	return ret;
}

uint8_t Input::ReadController2() {
	uint8_t ret = controller2_stream & 1;
	controller2_stream >>= 1;
	return ret;
}