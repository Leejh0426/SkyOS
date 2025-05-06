#include "InitKernel.h"
#include "Hal.h"
#include "Exception.h"
#include "IDT.h"

void SetInterruptVector()
{
	setvect(0, (void(__cdecl&)(void))kHandleDivideByZero);
	setvect(3, (void(__cdecl&)(void))kHandleBreakPointTrap);
	setvect(6, (void(__cdecl&)(void))kHandleInvalidOpcodeFault);
	
}