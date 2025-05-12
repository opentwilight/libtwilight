#include "twilight_ppc.h"
#include "font.h"

#define TERMINAL_COLS 80
#define TERMINAL_ROWS 30

static int _initialized = 0;
static TwTermFont _default_font;
static char _padding[32];
static int _changed = 0;
static unsigned _frame_counter = 0;
static unsigned _mode_flags = 0;

static void vblank_handler() {
	POKE_U32(TW_VIDEO_REG_BASE + 0x30, (PEEK_U32(TW_VIDEO_REG_BASE + 0x30)) & 0x7fffFFFF);
	POKE_U32(TW_VIDEO_REG_BASE + 0x34, (PEEK_U32(TW_VIDEO_REG_BASE + 0x34)) & 0x7fffFFFF);
	POKE_U32(TW_VIDEO_REG_BASE + 0x38, (PEEK_U32(TW_VIDEO_REG_BASE + 0x38)) & 0x7fffFFFF);
	POKE_U32(TW_VIDEO_REG_BASE + 0x3c, (PEEK_U32(TW_VIDEO_REG_BASE + 0x3c)) & 0x7fffFFFF);

	if (_changed) {
		int count = 2;
		int line = (_mode_flags & 1) ? 296 : 246; // start at a later line if PAL50
		line *= (1 + ((_mode_flags & 2) >> 1)); // double the line if progressive
		TW_SetSerialPollInterval(line, count);
		_changed = 0;
	}

	_frame_counter++;
}

static void init_video(TwVideo *params) {
	_default_font = (TwTermFont) {
		.data = &_glyph_data[0],
		.width = _glyph_width,
		.height = _glyph_height,
		.bytes_per_glyph = _glyph_size,
		.count = _glyph_count
	};

	unsigned short dcr = PEEK_U16(TW_VIDEO_REG_BASE + 2);
	params->format = (dcr >> 8) & 3;
	params->is_progressive = (dcr >> 2) & 1;

	unsigned width = 320;
	unsigned height = params->format == TW_VIDEO_PAL50 ? 574 : 480;
	unsigned fb_addr = TW_GetFramebufferAddress((void*)0);

	unsigned short vtr = ((height >> 1) << 4) | (6 - (params->format == TW_VIDEO_PAL50));

	POKE_U32(TW_VIDEO_REG_BASE + 0, ((unsigned int)vtr << 16) | (unsigned int)dcr);

	unsigned fb_reg_value = fb_addr; // 0x10000000 | (fb_addr >> 5);
	POKE_U32(TW_VIDEO_REG_BASE + 0x1c, fb_reg_value);
	POKE_U32(TW_VIDEO_REG_BASE + 0x24, fb_reg_value);

	if (params->format == TW_VIDEO_PAL50) {
		POKE_U32(TW_VIDEO_REG_BASE + 4, 0x4B6A01B0);
		POKE_U32(TW_VIDEO_REG_BASE + 8, 0x02F85640);
		POKE_U32(TW_VIDEO_REG_BASE + 0xc, 0x00010023);
		POKE_U32(TW_VIDEO_REG_BASE + 0x10, 0x00000024);
		POKE_U32(TW_VIDEO_REG_BASE + 0x14, 0x4D2B4D6D);
		POKE_U32(TW_VIDEO_REG_BASE + 0x18, 0x4D8A4D4C);
		POKE_U32(TW_VIDEO_REG_BASE + 0x30, 0x113901B1);
		//POKE_U16(TW_VIDEO_REG_BASE + 0x2c, 0x013C);
		//POKE_U16(TW_VIDEO_REG_BASE + 0x2e, 0x0144);
	} else {
		POKE_U32(TW_VIDEO_REG_BASE + 4, 0x476901AD);
		POKE_U32(TW_VIDEO_REG_BASE + 8, 0x02EA5140);
		POKE_U32(TW_VIDEO_REG_BASE + 0xc, 0x00030018);
		POKE_U32(TW_VIDEO_REG_BASE + 0x10, 0x00020019);
		POKE_U32(TW_VIDEO_REG_BASE + 0x14, 0x410C410C);
		POKE_U32(TW_VIDEO_REG_BASE + 0x18, 0x40ED40ED);
		POKE_U32(TW_VIDEO_REG_BASE + 0x30, 0x110701AE);
		/*
		if (params->is_progressive) {
			POKE_U16(TW_VIDEO_REG_BASE + 0x2c, 0x0005);
			POKE_U16(TW_VIDEO_REG_BASE + 0x2e, 0x0176);
		}
		else {
			POKE_U16(TW_VIDEO_REG_BASE + 0x2c, 0x0000);
			POKE_U16(TW_VIDEO_REG_BASE + 0x2e, 0x0000);
		}
		*/
	}

	/*
	POKE_U32(TW_VIDEO_REG_BASE + 0x28, 0x00000000);
	POKE_U32(TW_VIDEO_REG_BASE + 0x34, 0x10010001);
	POKE_U32(TW_VIDEO_REG_BASE + 0x38, 0x10010001);
	POKE_U32(TW_VIDEO_REG_BASE + 0x3c, 0x10010001);
	POKE_U32(TW_VIDEO_REG_BASE + 0x40, 0x00000000);
	POKE_U32(TW_VIDEO_REG_BASE + 0x44, 0x00000000);
	POKE_U16(TW_VIDEO_REG_BASE + 0x48, 0x2850);
	POKE_U16(TW_VIDEO_REG_BASE + 0x4a, 0x0100);
	POKE_U32(TW_VIDEO_REG_BASE + 0x4c, 0x1AE771F0);
	POKE_U32(TW_VIDEO_REG_BASE + 0x50, 0x0DB4A574);
	POKE_U32(TW_VIDEO_REG_BASE + 0x54, 0x00C1188E);
	POKE_U32(TW_VIDEO_REG_BASE + 0x58, 0xC4C0CBE2);
	POKE_U32(TW_VIDEO_REG_BASE + 0x5C, 0xFCECDECF);
	POKE_U32(TW_VIDEO_REG_BASE + 0x60, 0x13130F08);
	POKE_U32(TW_VIDEO_REG_BASE + 0x64, 0x00080C0F);
	POKE_U32(TW_VIDEO_REG_BASE + 0x68, 0x00FF0000);
	*/
	POKE_U16(TW_VIDEO_REG_BASE + 0x6c, params->is_progressive);

	/*
	POKE_U16(TW_VIDEO_REG_BASE + 0x70, 0x0280);
	POKE_U16(TW_VIDEO_REG_BASE + 0x72, 0x0000);
	POKE_U16(TW_VIDEO_REG_BASE + 0x74, 0x0000);
	*/

	TW_DisableInterrupts();
	_mode_flags = (params->format == TW_VIDEO_PAL50) | (params->is_progressive << 1);
	_frame_counter = 0;
	_changed = 1;
	TW_SetExternalInterruptHandler(TW_INTERRUPT_BIT_VIDEO, vblank_handler);
	TW_EnableInterrupts();

	params->width = width;
	params->height = height;
	params->xfb = (unsigned*)fb_addr;
}

void TW_InitVideo(TwVideo *params) {
	TW_InitTwilight();
	if (!_initialized) {
		init_video(params);
		_initialized = 1;
	}
}

void TW_InvalidateVideoTiming() {
	TW_DisableInterrupts();
	_changed = 1;
	TW_EnableInterrupts();
}

void TW_AwaitVideoVBlank(TwVideo *params) {
	if (TW_MultiThreadingEnabled()) {
		// try to yield if multithreading, and get woken up by the interrupt
		// TODO
	}
	else {
		// spin
		unsigned cur_frame = _frame_counter;
		while (PEEK_U32(&_frame_counter) == cur_frame);
	}
}

void TW_ClearVideoScreen(TwVideo *params, unsigned color) {
	unsigned *xfb = params->xfb;
	TW_FillWordsAndFlush((unsigned*)params->xfb, color, 320 * 480);
}

void TW_DrawAsciiSpan(TwVideo *video, TwTermFont *font, unsigned back, unsigned fore, int x, int y, const char *str, int count) {
	if (!font)
		font = &_default_font;

	int halfw = (font->width / 2);
	int w = count * halfw;
	int h = font->height;
	int x_off = 0;
	int y_off = 0;
	if (x < 0) {
		x_off = -x;
		x = 0;
		w -= x_off;
	}
	if (y < 0) {
		y_off = -y;
		y = 0;
		h -= y_off;
	}
	if (x + w > 320) {
		w = 320 - x;
	}
	if (y + h > 480) {
		h = 480 - y;
	}

	if (w <= 0 || h <= 0) {
		return;
	}

	unsigned blended = ((back >> 1) & 0x7f7f7f7f) + ((fore >> 1) & 0x7f7f7f7f);
	unsigned colors[4];
	colors[0] = back;
	colors[1] = blended;
	colors[2] = blended;
	colors[3] = fore;

	for (int i = 0; i < h; i++) {
		// FIXME: this won't work for glyphs of an odd width
		for (int j = 0; j < w; j++) {
			char ch = str[(j + x_off) / halfw];
			int idx = ch < 0x20 || ch > 0x7e ? 0 : ((int)ch - 0x20);
			int bit_pos = idx * font->bytes_per_glyph * 8 + ((i + y_off) * halfw + ((j + x_off) % halfw)) * 2;
			int c = (font->data[bit_pos >> 3] >> (6 - (bit_pos & 7))) & 3;
			int offset = (i + y) * 320 + j + x;
			video->xfb[offset] = colors[c];
		}
		TW_FlushMemory(&video->xfb[(i + y) * 320 + x], w * 4);
	}
}

void TW_WriteTerminalAscii(TwTerminal *params, TwVideo *video, const char *chars, int len) {
	int text_offsets[TERMINAL_ROWS];
	for (int i = 0; i < TERMINAL_ROWS; i++)
		text_offsets[i] = 0;

	int line_origin = params->row;
	int col_origin = params->column;
	int line = line_origin;
	int col = col_origin;
	int limit = len;

	for (int i = 0; i < len; i++) {
		if (chars[i] == '\n' || (!params->disable_wrap && col >= TERMINAL_COLS - 1)) {
			col = 0;
			if (chars[i] == '\n')
				col--;
			line++;
			if (params->disable_scroll && line >= TERMINAL_ROWS) {
				limit = i;
				break;
			}
			text_offsets[line % TERMINAL_ROWS] = i;
		}
		col++;
	}

	if (!params->disable_scroll) {
		int scroll = line - TERMINAL_ROWS - 1;
		if (scroll > 0) {
			int delta = 320 * _glyph_height * scroll;
			for (int i = delta; i < 320 * 480; i++)
				video->xfb[i-delta] = video->xfb[i];

			line_origin -= scroll;
			if (line_origin < 0) {
				// maybe do something smart here
				line_origin = 0;
			}
		}
	}

	line = line_origin;
	int start = text_offsets[line % TERMINAL_ROWS];
	int home = start;
	int home_col = col_origin;

	//*(volatile unsigned*)(0xff000000 | (line << 12) | limit) = 3;

	//*(volatile unsigned*)(0xff000000 | (unsigned)&_glyph_data[0]) = 2;

	for (int i = start; i < limit; i++) {
		int is_newline = chars[i] == '\n' || (!params->disable_wrap && col >= TERMINAL_COLS - 1);
		if (i < limit-1 && !is_newline) {
			col++;
			continue;
		}
		int r = line % TERMINAL_ROWS;

		TW_DrawAsciiSpan(video, &_default_font, params->back, params->fore, col * _glyph_width, r * _glyph_height, &chars[home], i - home);

		if (chars[i] == '\n') {
			col = 0;
			home = i+1;
		}
		else {
			col = 1;
			home = i;
		}
		home_col = 0;
		line++;
	}
}
