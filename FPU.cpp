#include "FPU.h"

extern "C" void __writecr4(unsigned __int64 Data);
extern "C"  unsigned long __readcr4(void);


bool InitFPU() {
	int result = 0;
	unsigned short temp;

	__asm {
		pushad;
		mov eax, cr0; eax = CR0
			and al, ~6;
		mov cr0, eax;
		fninit; FPU;
		mov temp, 0x5A5A;
		fnstsw temp;
		cmp temp, 0;
		jne noFPU;
		fnstcw temp;
		mov ax, temp;
		and ax, 0x103F;
		cmp ax, 0x003F;
		jne noFPU;
		mov result, 1
		noFPU:

		popad
	}
	return result == 1;
}

bool EnableFPU() {
#ifdef _WIN32
	unsigned long regCR4 = __readcr4();
	__asm or regCR4, 0x200
	__writecr4(regCR4);
#else
	//mov eax,cr4;
	// or eax, 0x200;
	// mov cr4, eax
#endif
}
