#include <twilight_ppc.h>

extern void TW_PumpIos(unsigned *iosBufAligned64Bytes);

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

#define ALIGNED_IOS_BUFFER (unsigned*)((((unsigned)&_ios_buffer[0]) + 0x3f) & ~0x3f)

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
	TW_PumpIos(iob); \
	result = (int)iob[1]; \
}

int TW_ReadIos(int fd, char *data, int size) {
	int result;
	RUN_TWO_ARG_IOS_METHOD(TW_IOS_CMD_READ, data, size)
	return result;
}

int TW_WriteIos(int fd, char *data, int size) {
	int result;
	RUN_TWO_ARG_IOS_METHOD(TW_IOS_CMD_WRITE, data, size)
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
	iob[4] = (unsigned)input;
	iob[5] = (unsigned)inputSize;
	iob[6] = (unsigned)output;
	iob[7] = (unsigned)outputSize;
	TW_PumpIos(iob);
	return (int)iob[1];
}

int TW_IoctlvIos(int fd, unsigned method, int nInputs, int nOutputs, void **inputsAndOutputs) {
	unsigned *iob = ALIGNED_IOS_BUFFER;
	iob[0] = TW_IOS_CMD_IOCTLV;
	iob[1] = 0;
	iob[2] = fd;
	iob[3] = method;
	iob[4] = (unsigned)nInputs;
	iob[5] = (unsigned)nOutputs;
	iob[6] = (unsigned)inputsAndOutputs;
	TW_PumpIos(iob);
	return (int)iob[1];
}

int TW_CloseIos(int fd) {
	unsigned *iob = ALIGNED_IOS_BUFFER;
	iob[0] = TW_IOS_CMD_CLOSE;
	iob[1] = 0;
	iob[2] = fd;
	TW_PumpIos(iob);
	return (int)iob[1];
}

TwFileProperties _get_ios_file_properties(struct tw_file *file) {
	TwFileProperties props = (TwFileProperties) {
		.totalSize = 0,
	};
	return props;
}

int _read_ios_file(struct tw_file *file, char *data, int size) {
	return TW_ReadIos((int)file->tag, data, size);
}

int _write_ios_file(struct tw_file *file, char *data, int size) {
	return TW_WriteIos((int)file->tag, data, size);
}

long long _seek_ios_file(struct tw_file *file, long long seekAmount, int type) {
	int delta = (int)seekAmount;
	return (int)TW_SeekIos((int)file->tag, type, delta);
}

int _ioctl_ios_file(struct tw_file *file, unsigned method, void *input, int inputSize, void *output, int outputSize) {
	return TW_IoctlIos((int)file->tag, method, input, inputSize, output, outputSize);
}

int _ioctlv_ios_file(struct tw_file *file, unsigned method, int nInputs, int nOutputs, void **inputsAndOutputs) {
	return TW_IoctlvIos((int)file->tag, method, nInputs, nOutputs, inputsAndOutputs);
}

int _close_ios_file(struct tw_file *file) {
	return TW_CloseIos((int)file->tag);
}

TwFile TW_OpenIosDevice(unsigned flags, char *path, int pathLen) {
	char prev_ch = path[pathLen];
	path[pathLen] = 0;
	int mode = (flags & TW_FLAG_WRITE) != 0 ? 2 : 1;
	int fd = 0;
	int result;
	RUN_TWO_ARG_IOS_METHOD(TW_IOS_CMD_OPEN, path, mode)
	path[pathLen] = prev_ch;

	TwFile file = {};
	if (fd < 0)
		return file;

	file.tag = (unsigned)fd;
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
	return TW_OpenIosDevice(flags, (char*)path, pathLen);
}

TwFilesystem TW_MakeIosFilesystem(void) {
	TwFilesystem fs = (TwFilesystem) {
		.listDirectory = _list_ios_fs_folder,
		.openFile = _open_ios_fs_device,
	};
	return fs;
}
