#pragma once

#include "windef.h"

//Led Numbers
#define ScrollLock (unsigned char)0x01
#define NumLock (unsigned char)0x02
#define CapsLock (unsigned char)0x04


#define KDefault 0x02	//Default Delay-Rate

#define K25Delay 0x00	//Delay values (25 fastest -> 01 Slowest)
#define K50Delay 0x20
#define K75Delay 0x40
#define K01Delay 0x60

#define K0Rate 0x0		//Rate values (0 fastest -> L4 Slowest)
#define K1Rate 0x1
#define K2Rate 0x2
#define K4Rate 0x4
#define K8Rate 0x8
#define KL0Rate 0x0A
#define KL1Rate 0x0D
#define KL2Rate 0x10
#define KL3Rate 0x14
#define KL4Rate 0x1F

typedef struct Func_Key_Tag
{
	bool enabled;
	void(*func)();
}Func_Key;

class KeyboardController
{
private :
	KeyboardController();
	~KeyboardController();

public:
	static void HandleKeyboardInterrupt();
	static char GetInput();
	static void UpdateLeds(unsigned char led);
	static int KeyboardController::SpecialKey(unsigned char key);

	static void FlushBuffers();
	static void SetInterrupts();
	static void SetLEDs(bool scroll, bool num, bool caps);
};