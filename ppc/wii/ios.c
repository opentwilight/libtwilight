#include <twilight.h>

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
	"/dev/sdio/slot1",
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
	"/dev/wl0"
};

TwStream TW_ListIosFolder(unsigned flags, const char *path, int pathLen) {
	
}

TwFile TW_OpenIosDevice(unsigned flags, const char *path, int pathLen) {
	
}

static TwStream _list_fs_ios_folder(struct _tw_filesystem *fs, unsigned flags, const char *path, int pathLen) {
	return TW_ListIosFolder(flags, path, pathLen);
}

static TwFile _open_fs_ios_device(struct _tw_filesystem *fs, unsigned flags, const char *path, int pathLen) {
	return TW_OpenIosDevice(flags, path, pathLen);
}

TwFilesystem TW_MakeIosFilesystem() {
	TwFilesystem fs = (TwFilesystem) {
		.listDirectory = _list_fs_ios_folder,
		.openFile = _open_fs_ios_device,
	};
	return fs;
}
