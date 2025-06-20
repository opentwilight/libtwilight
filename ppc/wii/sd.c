#include <twilight_ppc.h>

static TwFileProperties _tw_sdcard_getProperties(struct tw_file *file) {
	TwFileProperties props = {};
	return props;
}

static int _tw_sdcard_read(struct tw_file *file, void *data, int size) {
	return 0;
}

static int _tw_sdcard_write(struct tw_file *file, void *data, int size) {
	return 0;
}

static long long _tw_sdcard_seek(struct tw_file *file, long long seekAmount, int whence) {
	return 0;
}

static int _tw_sdcard_ioctl(struct tw_file *file, unsigned method, void *input, int inputSize, void *output, int outputSize) {
	return 0;
}

static int _tw_sdcard_ioctlv(struct tw_file *file, unsigned method, int nInputs, int nOutputs, TwView *inputsAndOutputs) {
	return 0;
}

static int _tw_sdcard_flush(struct tw_file *file) {
	return 0;
}

static int _tw_sdcard_close(struct tw_file *file) {
	return 0;
}

TwFile *TW_OpenSdCard(void) {
	TwFile *sd = TW_OpenFile(TW_MODE_RDWR, "/ios/sdio/slot0");
	if (sd->type != TW_FILE_TYPE_IOS) {
		sd->close(sd);
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
