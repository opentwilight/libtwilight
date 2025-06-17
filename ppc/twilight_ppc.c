#include <twilight_ppc.h>

static int _initialized = 0;

static TwHeapAllocator _g_tw_heap;
#ifdef TW_WII
static TwHeapAllocator _g_tw_heap_ex;
#endif

static TwTerminal _g_default_terminal = {
	.fore = 0xff80ff80,
	.back = 0x00800080,
};

static int _tw_stdin_read(TwFile *file, char *buf, int size) {
	return 0;
}

static int _tw_stdout_file_write(TwFile *file, char *buf, int size) {
	return TW_PrintTerminal(&_g_default_terminal, (void*)0, TW_GetDefaultVideo(), buf, size);
}

static int _tw_stdout_stream_write(TwStream *stream, char *buf, int size) {
	return TW_PrintTerminal(&_g_default_terminal, (void*)0, TW_GetDefaultVideo(), buf, size);
}

static void init_twilight() {
	int fb_size;
	unsigned framebuffer = TW_GetFramebufferAddress(&fb_size);
	_g_tw_heap = TW_MakeHeapAllocator((void*)(framebuffer + fb_size), (void*)TW_MEM1_END);
	_g_tw_heap.mutex = (void*)1; // use interrupt toggle as the lock, ie. global
#ifdef TW_WII
	_g_tw_heap_ex = TW_MakeHeapAllocator((void*)TW_MEM2_START, (void*)TW_IOS_MEM_START);
	_g_tw_heap_ex.mutex = (void*)1;
	_g_tw_heap.next = &_g_tw_heap_ex;
	TwFilesystem ios_fs = TW_MakeIosFilesystem();
	TW_MountFilesystem(&ios_fs, "/ios", 4);
#endif
	TW_SetFile(0, TW_MakeStdin(_tw_stdin_read));
	TW_SetFile(1, TW_MakeStdout(_tw_stdout_file_write));
	TW_SetFile(2, TW_MakeStdout(_tw_stdout_file_write));
	TW_RegisterMbrHandler();
	TW_RegisterFat32Handler();
}

void TW_InitTwilight(void) {
	if (!_initialized) {
		init_twilight();
		_initialized = 1;
	}
}

unsigned TW_GetFramebufferAddress(int *outSize) {
	unsigned stack_top = (unsigned)TW_GetMainStackStart();
	unsigned framebuffer = (stack_top + 0x1ff) & ~0x1ff;
	int fb_size = 2 * 640 * 576;
	if (outSize)
		*outSize = fb_size;
	return framebuffer;
}

TwHeapAllocator *TW_GetGlobalAllocator(void) {
	return &_g_tw_heap;
}

int TW_Printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	TwStream sink = (TwStream) { .transfer = _tw_stdout_stream_write };
	int written = TW_FormatStringV(&sink, 0, fmt, args);

	va_end(args);
	return written;
}

void _tw_print_regs(void) {
	unsigned *regs = TW_GetStoredRegistersAddress();
	for (int i = 0; i < 16; i++)
		TW_Printf("\x1b[%d;2Hr%d: %08X", 4 + i, i, regs[i]);
	for (int i = 0; i < 16; i++)
		TW_Printf("\x1b[%d;18Hr%d: %08X", 4 + i, 16+i, regs[16+i]);
}

void _tw_default_dsi_handler(void) {
	TW_ClearVideoScreen(TW_GetDefaultVideo(), 0x00800080);
	TW_Printf("\x1b[2;2HData access error");
	_tw_print_regs();
	while (1);
}

void _tw_default_isi_handler(void) {
	TW_ClearVideoScreen(TW_GetDefaultVideo(), 0x00800080);
	TW_Printf("\x1b[2;2HInstruction access error");
	_tw_print_regs();
	while (1);
}

void TW_SetDefaultCrashHandlers(void) {
	TW_SetCpuInterruptHandler(TW_INTERRUPT_TYPE_DSI, _tw_default_dsi_handler);
	TW_SetCpuInterruptHandler(TW_INTERRUPT_TYPE_ISI, _tw_default_isi_handler);
}
