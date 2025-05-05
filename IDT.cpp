#include <IDT.h>
#include <string.h>
#include <memory.h>



//인터럽트 디스크립터 테이블
static idt_descriptor	_idt[I86_MAX_INTERRUPTS];

//CPU의 IDTR 레지스터를 초기화하는데 도움을 주는 IDTR 구조체
static idtr _idtr;

//IDTR 레지스터에 IDT의 주소값을 넣는다.
static void IDTInstall() {
#ifdef _MSC_VER
	_asm lidt[_idtr]
#endif
}



#define DMA_PICU1       0x0020
#define DMA_PICU2       0x00A0

__declspec(naked) void SendEOI()
{
	_asm
	{
		PUSH EBP
		MOV  EBP, ESP
		PUSH EAX

		; [EBP] < -EBP
		; [EBP + 4] < -RET Addr
		; [EBP + 8] < -IRQ 번호

		MOV AL, 20H; EOI 신호를 보낸다.
		OUT DMA_PICU1, AL

		CMP BYTE PTR[EBP + 8], 7
		JBE END_OF_EOI
		OUT DMA_PICU2, AL; Send to 2 also

		END_OF_EOI :
		POP EAX
			POP EBP
			RET
	}
}

//다룰 수 있는 핸들러가 존재하지 않을 때 호출되는 기본 핸들러
//어셈블리 명령어 CLI
__declspec(naked) void InterruptDefaultHandler() {
	_asm
	{
		PUSHAD
		PUSHFD
		CLI
	}
	SendEOI();
	_asm {
		POPFD
		POPAD
		IRETD
	}
}

bool IDTInitialize(uint16_t codeSel) {

	_idtr.limit = sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1;
	_idtr.base = (uint32_t)&_idt[0];

	memset((void*)&_idt[0], 0, sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1);

	for (int i = 0; i < I86_MAX_INTERRUPTS; i++)
	{
		InstallInterrputHandler(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, codeSel, (I86_IRQ_HANDLER)InterruptDefaultHandler);
	}

	IDTInstall();

	return true;
}

//인터럽트 핸들러 설치
bool InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER irq) {

	if (i > I86_MAX_INTERRUPTS)
		return false;

	if (!irq)
		return false;

	//인터럽트의 베이스 주소를 얻어온다.
	uint64_t		uiBase = (uint64_t) & (*irq);

	if ((flags & 0x0500) == 0x0500) {
		_idt[i].selector = sel;
		_idt[i].flags = uint8_t(flags);
	}
	else
	{
		//포맷에 맞게 인터럽트 핸들러와 플래그 값을 디스크립터에 세팅한다.
		_idt[i].offsetLow = uint16_t(uiBase & 0xffff);
		_idt[i].offsetHigh = uint16_t((uiBase >> 16) & 0xffff);
		_idt[i].reserved = 0;
		_idt[i].flags = uint8_t(flags);
		_idt[i].selector = sel;
	}

	return	true;
}