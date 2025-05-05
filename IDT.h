#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>


//인터럽트 핸들러의 최대 개수 : 256
#define I86_MAX_INTERRUPTS		256

#define I86_IDT_DESC_BIT16		0x06	//00000110
#define I86_IDT_DESC_BIT32		0x0E	//00001110
#define I86_IDT_DESC_RING1		0x40	//01000000
#define I86_IDT_DESC_RING2		0x20	//00100000
#define I86_IDT_DESC_RING3		0x60	//01100000
#define I86_IDT_DESC_PRESENT	0x80	//10000000

typedef void(_cdecl* I86_IRQ_HANDLER)(void);

typedef struct tag_idt_descriptor {
	uint16_t offsetLow; 
	uint16_t selector;
	uint8_t reserved;
	uint8_t flags;
	uint16_t offsetHigh;
	
}idt_descriptor;


typedef struct tag_idtr
{
	uint16_t limit;
	uint32_t base;
}idtr;


idt_descriptor* GetInterruptDescriptor(uint32_t i);
bool  InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER);
bool IDTInitialize(uint16_t codeSel);


#endif