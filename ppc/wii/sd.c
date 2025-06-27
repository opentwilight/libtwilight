#include <twilight_ppc.h>

static TwFileProperties _tw_sdcard_getProperties(struct tw_file *file) {
	TwFileProperties props = {};
	return props;
}

static void _tw_sdcard_read(struct tw_file *file, void *userData, void *data, int size, TwIoCompletion completionHandler) {
	int res = 0;
	completionHandler(file, userData, TW_FILE_METHOD_READ, res);
}

static void _tw_sdcard_write(struct tw_file *file, void *userData, void *data, int size, TwIoCompletion completionHandler) {
	int res = 0;
	completionHandler(file, userData, TW_FILE_METHOD_WRITE, res);
}

static void _tw_sdcard_seek(struct tw_file *file, void *userData, long long seekAmount, int whence, TwIoCompletion completionHandler) {
	int res = 0;
	completionHandler(file, userData, TW_FILE_METHOD_SEEK, res);
}

static void _tw_sdcard_ioctl(struct tw_file *file, void *userData, unsigned method, void *input, int inputSize, void *output, int outputSize, TwIoCompletion completionHandler) {
	int res = 0;
	completionHandler(file, userData, TW_FILE_METHOD_IOCTL, res);
}

static void _tw_sdcard_ioctlv(struct tw_file *file, void *userData, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs, TwIoCompletion completionHandler) {
	int res = 0;
	completionHandler(file, userData, TW_FILE_METHOD_IOCTLV, res);
}

static void _tw_sdcard_flush(struct tw_file *file, void *userData, TwIoCompletion completionHandler) {
	int res = 0;
	completionHandler(file, userData, TW_FILE_METHOD_FLUSH, res);
}

static void _tw_sdcard_close(struct tw_file *file, void *userData, TwIoCompletion completionHandler) {
	int res = 0;
	completionHandler(file, userData, TW_FILE_METHOD_CLOSE, res);
}

TwFile *TW_OpenSdCard(void) {
	TwFile *sd = TW_OpenFileSync(TW_MODE_RDWR, "/ios/sdio/slot0");
	if (sd->type != TW_FILE_TYPE_IOS) {
		TW_CloseFileSync(sd);
		return (void*)0;
	}

	// Do initialisation here

	TwFile sdDisk = (TwFile) {
		.type = TW_FILE_TYPE_DISK,
		.getProperties = _tw_sdcard_getProperties,
		.read = _tw_sdcard_read,
		.write = _tw_sdcard_write,
		.seek = _tw_sdcard_seek,
		.ioctl = _tw_sdcard_ioctl,
		.ioctlv = _tw_sdcard_ioctlv,
		.flush = _tw_sdcard_flush,
		.close = _tw_sdcard_close,
	};
	sdDisk.params[0] = sd->params[0];

	TwFile *ptr = (void*)0;
	TW_AddFile(sdDisk, &ptr);
	return ptr;
}
