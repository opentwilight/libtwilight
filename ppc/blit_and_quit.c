extern void TW_Exit(void);

void BlitAndQuit() {
	const unsigned color = 0x4C544CFF;
	unsigned *fb = (unsigned*)0x80160000;
	for (int i = 0; i < 320 * 480; i += 8) {
		fb[i] = color;
		fb[i+1] = color;
		fb[i+2] = color;
		fb[i+3] = color;
		fb[i+4] = color;
		fb[i+5] = color;
		fb[i+6] = color;
		fb[i+7] = color;
		asm("dcbf 0,%0" : : "b"(&fb[i]));
	}

	unsigned vtrdcr = *(volatile unsigned*)(0xCC002000);
	vtrdcr &= 0xFFFFF;
	vtrdcr |= 0x0F000000;

	unsigned vimode = (vtrdcr>>8)&3;
	unsigned vto = 0x30018;
	unsigned vte = 0x20019;

	if(vtrdcr & 4) { // progressive
		vto = 0x60030;
		vte = 0x60030;
		vtrdcr += 0x0F000000;
	} else if(vimode == 1) {
		vto = 0x10023;
		vte = 0x24;
		vtrdcr += 0x02F00000;
	}

	*(volatile unsigned*)(0xCC002000) = vtrdcr;
	*(volatile unsigned*)(0xCC00200c) = vto;
	*(volatile unsigned*)(0xCC002010) = vte;

	*(volatile unsigned*)(0xCC00201c) = (((unsigned)fb) >> 5) | 0x10000000;
	*(volatile unsigned*)(0xCC002024) = (((unsigned)fb) >> 5) | 0x10000000;

	TW_Exit();
}
