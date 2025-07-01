#include <twilight_ppc.h>

#define ALIGNED_IOS_BUFFER(idx) (unsigned*)((((unsigned)&_ios_buffers[0]) + ((idx)*0x40) + 0x3f) & ~0x3f)

extern int TW_RebootIosSync(unsigned *iosBufAligned64Bytes);
extern void TW_SubmitIosRequest(unsigned *iosBufAligned64Bytes);
extern void TW_HandleIosInterrupt(void);

static const char *_default_ios_devices[] = {
	"/dev/aes",
	"/dev/boot2",
	"/dev/di",
	"/dev/es",
	"/dev/flash",
	"/dev/fs",
	"/dev/hmac",
	"/dev/listen",
	"/dev/net",
	"/dev/net/ip/bottom",
	"/dev/net/ip/top",
	"/dev/net/ip/top/Progress",
	"/dev/net/kd",
	"/dev/net/kd/request",
	"/dev/net/kd/time",
	"/dev/net/ncd/manage",
	"/dev/net/ssl",
	"/dev/net/ssl/code",
	"/dev/net/usbeth",
	"/dev/net/usbeth/top",
	"/dev/net/wd",
	"/dev/net/wd/command",
	"/dev/net/wd/top",
	"/dev/printserver",
	"/dev/sdio",
	"/dev/sdio/slot0",
	"/dev/sha",
	"/dev/stm",
	"/dev/stm/eventhook",
	"/dev/stm/immediate",
	"/dev/usb",
	"/dev/usb/ehc",
	"/dev/usb/hid",
	"/dev/usb/hub",
	"/dev/usb/kbd",
	"/dev/usb/msc",
	"/dev/usb/oh0",
	"/dev/usb/oh1",
	"/dev/usb/shared",
	"/dev/usb/usb",
	"/dev/usb/ven",
	"/dev/usb/wfssrv",
	"/dev/wfsi",
	"/dev/wl0",
};

static unsigned _ios_buffers[528];

static TwIoCompletionContext _ios_completion_handlers[32];

static char padding[32];
static unsigned _ios_completion_write_head = 0;

void TW_SetupIos(void) {
	TW_SetExternalInterruptHandler(TW_INTERRUPT_BIT_IPC_BROADWAY, TW_HandleIosInterrupt);
	POKE_U32(0xcd000004, 0x36); // initialise IPC and enable IPC interrupts
}

int TW_ListIosFolder(unsigned flags, const char *path, int pathLen, TwStream *output) {
	if (!path || pathLen <= 0)
		return 0;

	int nDevices = sizeof(_default_ios_devices) / sizeof(const char*);
	return TW_WriteMatchingPaths(_default_ios_devices, nDevices, path, pathLen, output);
}

void TW_InvokeMatchingIosCompletionHandler(unsigned *iob) {
	unsigned *base_iob = ALIGNED_IOS_BUFFER(0);
	int idx = (iob - base_iob) >> 6;
	TwIoCompletionContext *ch = &_ios_completion_handlers[idx];
	if (idx >= 0 && idx < 32 && ch->handler)
		ch->handler(ch->file, ch->userData, ch->method, iob[1]);
}

int TW_IoctlvRebootIos(int fd, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs) {
	unsigned ios_buffer_idx = TW_AddAtomic(&_ios_completion_write_head, 1) & 0x1f;
	unsigned *iob = ALIGNED_IOS_BUFFER(ios_buffer_idx);
	iob[0] = TW_IOS_CMD_IOCTLV;
	iob[1] = 0;
	iob[2] = fd;
	iob[3] = method;
	iob[4] = (unsigned)nInputs;
	iob[5] = (unsigned)nOutputs;
	iob[6] = (unsigned)inputsAndOutputs;

	for (int i = 0; i < nInputs + nOutputs; i++) {
		if (i < nInputs && inputsAndOutputs[i].data && inputsAndOutputs[i].size > 0)
			TW_SyncAfterWrite(inputsAndOutputs[i].data, inputsAndOutputs[i].size);
		inputsAndOutputs[i].data = GET_PHYSICAL_POINTER(inputsAndOutputs[i].data);
	}

	TW_RebootIosSync(iob);

	for (int i = nInputs; i < nInputs + nOutputs; i++) {
		if (inputsAndOutputs[i].data && inputsAndOutputs[i].size > 0)
			TW_SyncBeforeRead((void*)(0x80000000 | (unsigned)inputsAndOutputs[i].data), inputsAndOutputs[i].size);
	}

	return (int)iob[1];
}

TwFileProperties _get_ios_file_properties(struct tw_file *file) {
	TwFileProperties props = (TwFileProperties) {
		.totalSize = 0,
	};
	return props;
}

#define RUN_TWO_ARG_IOS_METHOD(idx, fd, cmd_type, arg0, arg1) \
{ \
	unsigned *iob = ALIGNED_IOS_BUFFER(idx); \
	iob[0] = cmd_type; \
	iob[1] = 0; \
	iob[2] = fd; \
	iob[3] = (unsigned)arg0; \
	iob[4] = (unsigned)arg1; \
	TW_SubmitIosRequest(iob); \
}

void _read_ios_file(struct tw_file *file, void *userData, void *data, int size, TwIoCompletion completionHandler) {
	unsigned ios_buffer_idx = TW_AddAtomic(&_ios_completion_write_head, 1) & 0x1f;
	TwIoCompletionContext *ch = &_ios_completion_handlers[ios_buffer_idx];
	ch->handler = completionHandler;
	ch->file = file;
	ch->userData = userData;
	ch->method = TW_FILE_METHOD_READ;

	void *physPtr = GET_PHYSICAL_POINTER(data);
	RUN_TWO_ARG_IOS_METHOD(ios_buffer_idx, file->params[0], TW_IOS_CMD_READ, physPtr, size)
}

void _write_ios_file(struct tw_file *file, void *userData, void *data, int size, TwIoCompletion completionHandler) {
	unsigned ios_buffer_idx = TW_AddAtomic(&_ios_completion_write_head, 1) & 0x1f;
	TwIoCompletionContext *ch = &_ios_completion_handlers[ios_buffer_idx];
	ch->handler = completionHandler;
	ch->file = file;
	ch->userData = userData;
	ch->method = TW_FILE_METHOD_WRITE;

	TW_SyncAfterWrite(data, size);
	void *physPtr = GET_PHYSICAL_POINTER(data);
	RUN_TWO_ARG_IOS_METHOD(ios_buffer_idx, file->params[0], TW_IOS_CMD_WRITE, physPtr, size)
}

void _seek_ios_file(struct tw_file *file, void *userData, long long seekAmount, int whence, TwIoCompletion completionHandler) {
	unsigned ios_buffer_idx = TW_AddAtomic(&_ios_completion_write_head, 1) & 0x1f;
	TwIoCompletionContext *ch = &_ios_completion_handlers[ios_buffer_idx];
	ch->handler = completionHandler;
	ch->file = file;
	ch->userData = userData;
	ch->method = TW_FILE_METHOD_SEEK;

	int delta = (int)seekAmount;
	RUN_TWO_ARG_IOS_METHOD(ios_buffer_idx, file->params[0], TW_IOS_CMD_SEEK, seekAmount, whence)
}

void _ioctl_ios_file(struct tw_file *file, void *userData, unsigned method, void *input, int inputSize, void *output, int outputSize, TwIoCompletion completionHandler) {
	unsigned ios_buffer_idx = TW_AddAtomic(&_ios_completion_write_head, 1) & 0x1f;
	TwIoCompletionContext *ch = &_ios_completion_handlers[ios_buffer_idx];
	ch->handler = completionHandler;
	ch->file = file;
	ch->userData = userData;
	ch->method = TW_FILE_METHOD_IOCTL;

	unsigned *iob = ALIGNED_IOS_BUFFER(ios_buffer_idx);
	iob[0] = TW_IOS_CMD_IOCTL;
	iob[1] = 0;
	iob[2] = file->params[0];
	iob[3] = method;
	iob[4] = (unsigned)GET_PHYSICAL_POINTER(input);
	iob[5] = (unsigned)inputSize;
	iob[6] = (unsigned)GET_PHYSICAL_POINTER(output);
	iob[7] = (unsigned)outputSize;

	if (input && inputSize > 0)
		TW_SyncAfterWrite(input, inputSize);

	TW_SubmitIosRequest(iob);
}

void _ioctlv_ios_file(struct tw_file *file, void *userData, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs, TwIoCompletion completionHandler) {
	unsigned ios_buffer_idx = TW_AddAtomic(&_ios_completion_write_head, 1) & 0x1f;
	TwIoCompletionContext *ch = &_ios_completion_handlers[ios_buffer_idx];
	ch->handler = completionHandler;
	ch->file = file;
	ch->userData = userData;
	ch->method = TW_FILE_METHOD_IOCTLV;

	unsigned *iob = ALIGNED_IOS_BUFFER(ios_buffer_idx);
	iob[0] = TW_IOS_CMD_IOCTLV;
	iob[1] = 0;
	iob[2] = file->params[0];
	iob[3] = method;
	iob[4] = (unsigned)nInputs;
	iob[5] = (unsigned)nOutputs;
	iob[6] = (unsigned)inputsAndOutputs;

	for (int i = 0; i < nInputs + nOutputs; i++) {
		if (i < nInputs && inputsAndOutputs[i].data && inputsAndOutputs[i].size > 0)
			TW_SyncAfterWrite(inputsAndOutputs[i].data, inputsAndOutputs[i].size);
		inputsAndOutputs[i].data = GET_PHYSICAL_POINTER(inputsAndOutputs[i].data);
	}

	TW_SubmitIosRequest(iob);
}

void _close_ios_file(struct tw_file *file, void *userData, TwIoCompletion completionHandler) {
	unsigned ios_buffer_idx = TW_AddAtomic(&_ios_completion_write_head, 1) & 0x1f;
	TwIoCompletionContext *ch = &_ios_completion_handlers[ios_buffer_idx];
	ch->handler = completionHandler;
	ch->file = file;
	ch->userData = userData;
	ch->method = TW_FILE_METHOD_CLOSE;

	unsigned *iob = ALIGNED_IOS_BUFFER(ios_buffer_idx);
	iob[0] = TW_IOS_CMD_CLOSE;
	iob[1] = 0;
	iob[2] = file->params[0];
	TW_SubmitIosRequest(iob);
}

static void _tw_await_ios_open(struct tw_file *file, void *userData, int method, long long result) {
	TW_ReachFuture((TwFuture)userData, result, 0);
}

TwFile TW_OpenIosDevice(unsigned flags, const char *path, int pathLen) {
	char ios_path_unaligned[256];
	TwFile file = {};
	if (!path || pathLen <= 0)
		return file;

	char *ios_path = (char*)((((unsigned)&ios_path_unaligned[0]) + 0x1f) & ~0x1f);
	if (pathLen > 223)
		pathLen = 223;
	for (int i = 0; i < pathLen; i++)
		ios_path[i] = path[i];
	ios_path[pathLen] = 0;

	TwFuture fd_future = TW_CreateFuture(0, 0);

	unsigned ios_buffer_idx = TW_AddAtomic(&_ios_completion_write_head, 1) & 0x1f;
	TwIoCompletionContext *ch = &_ios_completion_handlers[ios_buffer_idx];
	ch->handler = _tw_await_ios_open;
	ch->file = (void*)0;
	ch->userData = fd_future;
	ch->method = TW_FILE_METHOD_OPEN;

	int mode = flags;
	RUN_TWO_ARG_IOS_METHOD(ios_buffer_idx, 0, TW_IOS_CMD_OPEN, path, mode)

	long long result = TW_AwaitFuture(fd_future);
	TW_DestroyFuture(fd_future);

	int fd = (int)(result & 0xFFFFffffLL);
	if (fd <= 0)
		return file;

	file.type = TW_FILE_TYPE_IOS;
	file.params[0] = (unsigned)fd;
	file.getProperties = _get_ios_file_properties;
	file.read = _read_ios_file;
	file.write = _write_ios_file;
	file.seek = _seek_ios_file;
	file.ioctl = _ioctl_ios_file;
	file.ioctlv = _ioctlv_ios_file;
	file.close = _close_ios_file;
	return file;
}

static int _list_ios_fs_folder(TwFilesystem *fs, unsigned flags, const char *path, int pathLen, TwStream *stream) {
	return TW_ListIosFolder(flags, path, pathLen, stream);
}

static TwFile _open_ios_fs_device(TwFilesystem *fs, unsigned flags, const char *path, int pathLen) {
	return TW_OpenIosDevice(flags, path, pathLen);
}

TwFilesystem TW_MakeIosFilesystem(void) {
	TwFilesystem fs = (TwFilesystem) {
		.listDirectory = _list_ios_fs_folder,
		.openFile = _open_ios_fs_device,
	};
	return fs;
}
