#include <twilight_ppc.h>

#define ALIGNED_IOS_BUFFER (unsigned*)((((unsigned)&_ios_buffer[0]) + 0x3f) & ~0x3f)

extern int TW_PumpIos(unsigned *iosBufAligned64Bytes, int shouldReboot);

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

static unsigned _ios_buffer[32];

int TW_ListIosFolder(unsigned flags, const char *path, int pathLen, TwStream *output) {
	if (!path || pathLen <= 0)
		return 0;

	int nDevices = sizeof(_default_ios_devices) / sizeof(const char*);
	return TW_WriteMatchingPaths(_default_ios_devices, nDevices, path, pathLen, output);
}

#define RUN_TWO_ARG_IOS_METHOD(cmd_type, arg0, arg1) \
{ \
	unsigned *iob = ALIGNED_IOS_BUFFER; \
	iob[0] = cmd_type; \
	iob[1] = 0; \
	iob[2] = fd; \
	iob[3] = (unsigned)arg0; \
	iob[4] = (unsigned)arg1; \
	TW_PumpIos(iob, 0); \
	result = (int)iob[1]; \
}

int TW_ReadAlignedIos(int fd, void *data, int size) {
	int result;
	void *physPtr = GET_PHYSICAL_POINTER(data);
	RUN_TWO_ARG_IOS_METHOD(TW_IOS_CMD_READ, physPtr, size)
	TW_SyncBeforeRead(data, size);
	return result;
}

int TW_WriteAlignedIos(int fd, void *data, int size) {
	int result;
	TW_SyncAfterWrite(data, size);
	void *physPtr = GET_PHYSICAL_POINTER(data);
	RUN_TWO_ARG_IOS_METHOD(TW_IOS_CMD_WRITE, physPtr, size)
	return result;
}

int TW_SeekIos(int fd, int offset, int whence) {
	int result;
	RUN_TWO_ARG_IOS_METHOD(TW_IOS_CMD_SEEK, offset, whence)
	return result;
}

int TW_IoctlIos(int fd, unsigned method, void *input, int inputSize, void *output, int outputSize) {
	unsigned *iob = ALIGNED_IOS_BUFFER;
	iob[0] = TW_IOS_CMD_IOCTL;
	iob[1] = 0;
	iob[2] = fd;
	iob[3] = method;
	iob[4] = (unsigned)GET_PHYSICAL_POINTER(input);
	iob[5] = (unsigned)inputSize;
	iob[6] = (unsigned)GET_PHYSICAL_POINTER(output);
	iob[7] = (unsigned)outputSize;

	if (input && inputSize > 0)
		TW_SyncAfterWrite(input, inputSize);

	TW_PumpIos(iob, 0);

	if (output && outputSize > 0)
		TW_SyncBeforeRead(output, outputSize);

	return (int)iob[1];
}

int TW_IoctlvIos(int fd, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs, int shouldReboot) {
	unsigned *iob = ALIGNED_IOS_BUFFER;
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

	TW_PumpIos(iob, shouldReboot);

	for (int i = nInputs; i < nInputs + nOutputs; i++) {
		if (inputsAndOutputs[i].data && inputsAndOutputs[i].size > 0)
			TW_SyncBeforeRead((void*)(0x80000000 | (unsigned)inputsAndOutputs[i].data), inputsAndOutputs[i].size);
	}

	return (int)iob[1];
}

int TW_IoctlvRebootIos(int fd, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs) {
	return TW_IoctlvIos(fd, method, nInputs, nOutputs, inputsAndOutputs, 1);
}

int TW_CloseIos(int fd) {
	unsigned *iob = ALIGNED_IOS_BUFFER;
	iob[0] = TW_IOS_CMD_CLOSE;
	iob[1] = 0;
	iob[2] = fd;
	TW_PumpIos(iob, 0);
	return (int)iob[1];
}

TwFileProperties _get_ios_file_properties(struct tw_file *file) {
	TwFileProperties props = (TwFileProperties) {
		.totalSize = 0,
	};
	return props;
}

int _read_ios_file(struct tw_file *file, void *data, int size) {
	return TW_ReadAlignedIos((int)file->params[0], data, size);
}

int _write_ios_file(struct tw_file *file, void *data, int size) {
	return TW_WriteAlignedIos((int)file->params[0], data, size);
}

long long _seek_ios_file(struct tw_file *file, long long seekAmount, int type) {
	int delta = (int)seekAmount;
	return (int)TW_SeekIos((int)file->params[0], type, delta);
}

int _ioctl_ios_file(struct tw_file *file, unsigned method, void *input, int inputSize, void *output, int outputSize) {
	return TW_IoctlIos((int)file->params[0], method, input, inputSize, output, outputSize);
}

int _ioctlv_ios_file(struct tw_file *file, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs) {
	return TW_IoctlvIos((int)file->params[0], method, nInputs, nOutputs, inputsAndOutputs, 0);
}

int _close_ios_file(struct tw_file *file) {
	return TW_CloseIos((int)file->params[0]);
}

TwFile TW_OpenIosDevice(unsigned flags, const char *path, int pathLen) {
	char ios_path_unaligned[256];
	TwFile file = {};
	if (!path || pathLen <= 0)
		return file;

	char *ios_path = (char*)(((unsigned)&ios_path_unaligned[0] + 0x1f) & ~0x1f);
	if (pathLen > 223)
		pathLen = 223;
	for (int i = 0; i < pathLen; i++)
		ios_path[i] = path[i];
	ios_path[pathLen] = 0;

	int mode = flags;
	int fd = 0;
	int result;
	RUN_TWO_ARG_IOS_METHOD(TW_IOS_CMD_OPEN, path, mode)
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
