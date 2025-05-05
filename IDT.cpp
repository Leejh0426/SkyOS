#include <IDT.h>
#include <string.h>
#include <memory.h>



//���ͷ�Ʈ ��ũ���� ���̺�
static idt_descriptor	_idt[I86_MAX_INTERRUPTS];

//CPU�� IDTR �������͸� �ʱ�ȭ�ϴµ� ������ �ִ� IDTR ����ü
static idtr _idtr;

//IDTR �������Ϳ� IDT�� �ּҰ��� �ִ´�.
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
		; [EBP + 8] < -IRQ ��ȣ

		MOV AL, 20H; EOI ��ȣ�� ������.
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

//�ٷ� �� �ִ� �ڵ鷯�� �������� ���� �� ȣ��Ǵ� �⺻ �ڵ鷯
//����� ��ɾ� CLI
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

//���ͷ�Ʈ �ڵ鷯 ��ġ
bool InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER irq) {

	if (i > I86_MAX_INTERRUPTS)
		return false;

	if (!irq)
		return false;

	//���ͷ�Ʈ�� ���̽� �ּҸ� ���´�.
	uint64_t		uiBase = (uint64_t) & (*irq);

	if ((flags & 0x0500) == 0x0500) {
		_idt[i].selector = sel;
		_idt[i].flags = uint8_t(flags);
	}
	else
	{
		//���˿� �°� ���ͷ�Ʈ �ڵ鷯�� �÷��� ���� ��ũ���Ϳ� �����Ѵ�.
		_idt[i].offsetLow = uint16_t(uiBase & 0xffff);
		_idt[i].offsetHigh = uint16_t((uiBase >> 16) & 0xffff);
		_idt[i].reserved = 0;
		_idt[i].flags = uint8_t(flags);
		_idt[i].selector = sel;
	}

	return	true;
}