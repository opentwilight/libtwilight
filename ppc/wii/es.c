#include <twilight_ppc.h>

#define TW_ES_IOCTL_ES_LAUNCHTITLE    8
#define TW_ES_IOCTL_ES_GETVIEWCOUNT  18
#define TW_ES_IOCTL_ES_GETVIEWS      19

int TW_LaunchWiiTitle(unsigned long long titleId) {
	// 16 data,size pairs (16*2 -> 0x20)
	// titleId (2 -> 8), viewCount (1 -> 8), ticketViews (0xd8 -> 0xd8)
	// plus padding (0x10)
	unsigned es_buffer_unaligned[0x118];

	TwFile *es = TW_OpenFile(TW_MODE_NONE, "/ios/dev/es");
	if (es->tag != TW_FILE_TAG_IOS) {
		es->close(es);
		return -1;
	}

	unsigned *es_buf = (unsigned*)(((unsigned)&es_buffer_unaligned[0] + 0x3f) & ~0x3f);
	TwView *es_hdrs = (TwView*)es_buf;

	es_hdrs[0].data = &es_buf[0x20];
	es_hdrs[0].size = 8;
	es_hdrs[1].data = &es_buf[0x28];
	es_hdrs[1].size = 4;
	es_buf[0x20] = (unsigned)(titleId >> 32);
	es_buf[0x21] = (unsigned)titleId;
	int res = es->ioctlv(es, TW_ES_IOCTL_ES_GETVIEWCOUNT, 1, 1, es_hdrs);
	if (res != 0) {
		es->close(es);
		return -2;
	}

	int nViews = (int)es_buf[0x28];
	es_hdrs[2].data = &es_buf[0x30];
	es_hdrs[2].size = 4 * nViews;
	res = es->ioctlv(es, TW_ES_IOCTL_ES_GETVIEWS, 2, 1, es_hdrs);
	if (res != 0) {
		es->close(es);
		return -3;
	}

	unsigned *first_ticket = &es_buf[0x30];
	es_hdrs[1].data = first_ticket;
	es_hdrs[1].size = 0xd8;
	res = TW_IoctlvRebootIos(es->params[0], TW_ES_IOCTL_ES_LAUNCHTITLE, 2, 0, es_hdrs);

	es->close(es);
	if (res != 0)
		return -4;
	return 0;
}
