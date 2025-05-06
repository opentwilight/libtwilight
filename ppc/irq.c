#include <twilight_ppc.h>

void _tw_irq_handler_impl(unsigned irqIndex) {
	*(volatile unsigned*)(0x7f123400 | irqIndex) = 3;
}
