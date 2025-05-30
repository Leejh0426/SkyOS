#include "exception.h"
#include <hal.h>
#include <stdint.h>
#include "SkyConsole.h"
#include "sprintf.h"
#include "string.h"

#pragma warning (disable:4100)

static char* sickpc = " (>_<) SkyOS Error!!\n\n";

void _cdecl kExceptionMessageHeader() {

	char* disclamer = "We apologize, SkyOS has encountered a problem and has been shut down\n\
to prevent damage to your computer. Any unsaved work might be lost.\n\
We are sorry for the inconvenience this might have caused.\n\n\
Please report the following information and restart your computer.\n\
The system has been halted.\n\n";

	SkyConsole::MoveCursor(0, 0);
	SkyConsole::SetColor(ConsoleColor::White, ConsoleColor::Blue, false);
	SkyConsole::Clear();
	SkyConsole::Print(sickpc);
	SkyConsole::Print(disclamer);
}

extern int _divider;
extern int _dividend;

/*
void HandleDivideByZero(registers_t regs)
{
	_divider = 10;
	_dividend = 100;
}*/

void HandleDivideByZero(registers_t regs)
{
	kExceptionMessageHeader();
	SkyConsole::Print("Divide by 0 at Address[0x%x:0x%x]\n", regs.cs, regs.eip);
	SkyConsole::Print("EFLAGS[0x%x]\n", regs.eflags);
	SkyConsole::Print("ss : 0x%x\n", regs.ss);
	for (;;);
}

interrupt void kHandleDivideByZero()
{

	_asm {
		cli
		pushad

		push ds
		push es
		push fs
		push gs
	}

	_asm
	{
		call HandleDivideByZero
	}

	_asm {

		pop gs
		pop fs
		pop es
		pop ds

		popad

		mov al, 0x20
		out 0x20, al
		sti
		iretd
	}
}

interrupt void kHandleSingleStepTrap() {

	kExceptionMessageHeader();
	SkyConsole::Print("Single step\n");
	for (;;);
}

interrupt void kHandleNMITrap() {

	kExceptionMessageHeader();
	SkyConsole::Print("NMI trap\n");
	for (;;);
}

interrupt void kHandleBreakPointTrap() {
	kExceptionMessageHeader();
	SkyConsole::Print("Breakpoint trap\n");
	for (;;);
}

interrupt void kHandleOverflowTrap() {
	kExceptionMessageHeader();
	SkyConsole::Print("Overflow trap");
	for (;;);
}

interrupt void kHandleBoundsCheckFault() {
	kExceptionMessageHeader();
	SkyConsole::Print("Bounds check fault\n");
	for (;;);
}

interrupt void kHandleInvalidOpcodeFault() {
	kExceptionMessageHeader();
	SkyConsole::Print("Invalid opcode");
	for (;;);
}

interrupt void kHandleNoDeviceFault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Device not found\n");
	for (;;);
}

interrupt void kHandleDoubleFaultAbort()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Double fault\n");
	for (;;);
}

interrupt void kHandleInvalidTSSFault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Invalid TSS\n");
	for (;;);
}

interrupt void kHandleSegmentFault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Invalid segment\n");
	for (;;);
}

interrupt void kHandleStackFault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Stack fault\n");
	for (;;);
}

interrupt void kHandleGeneralProtectionFault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("General Protection Fault\n");
	for (;;);
}


interrupt void kHandlePageFault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Page Fault\n");
	for (;;);
}

interrupt void kHandlefpu_fault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("FPU Fault\n");
	for (;;);
}

interrupt void kHandleAlignedCheckFault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Alignment Check\n");
	for (;;);
}

interrupt void kHandleMachineCheckAbort()
{
	kExceptionMessageHeader();
	SkyConsole::Print("Machine Check\n");
	for (;;);
}

interrupt void kHandleSIMDFPUFault()
{
	kExceptionMessageHeader();
	SkyConsole::Print("FPU SIMD fault\n");
	for (;;);
}

void HaltSystem(const char* errMsg)
{
	SkyConsole::MoveCursor(0, 0);
	SkyConsole::SetColor(ConsoleColor::White, ConsoleColor::Blue, false);
	SkyConsole::Clear();
	SkyConsole::Print(sickpc);

	SkyConsole::Print("*** STOP: %s", errMsg);

	__asm
	{

	halt:
		jmp halt;
	}
}

void error(char* s)
{

}