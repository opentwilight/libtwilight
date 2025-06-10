#include <twilight.h>

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
