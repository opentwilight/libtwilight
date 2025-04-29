int main() {
	*(unsigned int*)0x80003000 = 0x3fc00000;
	return 0;
}

void _start() {
	main();
}
