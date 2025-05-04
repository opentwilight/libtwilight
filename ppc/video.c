#include "twilight_ppc.h"
#include "font.h"

#define TERMINAL_COLS 80
#define TERMINAL_ROWS 30

static int _initialized = 0;

static void init_video(TwVideo *params) {
	unsigned width = 320;
	unsigned height;
	unsigned fb_addr = TW_GetFramebufferAddress((void*)0);

	unsigned short vtr = PEEK_U16(VI_REG_BASE + 2);
	params->format = (vtr >> 8) & 3;
	params->is_progressive = (vtr >> 2) & 1;

	unsigned short dcr = 1 | ((params->format == 1) << 8);
	POKE_U16(VI_REG_BASE + 2, dcr);

	if (params->format == 1) {
		POKE_U32(VI_REG_BASE + 4, 0x4B6A01B0);
		POKE_U32(VI_REG_BASE + 8, 0x02F85640);
		POKE_U32(VI_REG_BASE + 0xc, 0x00010023);
		POKE_U32(VI_REG_BASE + 0x10, 0x00000024);
		POKE_U32(VI_REG_BASE + 0x14, 0x4D2B4D6D);
		POKE_U32(VI_REG_BASE + 0x18, 0x4D8A4D4C);
		POKE_U32(VI_REG_BASE + 0x30, 0x113901B1);
		POKE_U16(VI_REG_BASE + 0x2c, 0x013C);
		POKE_U16(VI_REG_BASE + 0x2e, 0x0144);
	} else {
		POKE_U32(VI_REG_BASE + 4, 0x476901AD);
		POKE_U32(VI_REG_BASE + 8, 0x02EA5140);
		POKE_U32(VI_REG_BASE + 0xc, 0x00030018);
		POKE_U32(VI_REG_BASE + 0x10, 0x00020019);
		POKE_U32(VI_REG_BASE + 0x14, 0x410C410C);
		POKE_U32(VI_REG_BASE + 0x18, 0x40ED40ED);
		POKE_U32(VI_REG_BASE + 0x30, 0x110701AE);
		if (params->is_progressive) {
			POKE_U16(VI_REG_BASE + 0x2c, 0x0005);
			POKE_U16(VI_REG_BASE + 0x2e, 0x0176);
		}
		else {
			POKE_U16(VI_REG_BASE + 0x2c, 0x0000);
			POKE_U16(VI_REG_BASE + 0x2e, 0x0000);
		}
	}

	POKE_U32(VI_REG_BASE + 0x1c, (fb_addr & 0xfffe00));
	POKE_U32(VI_REG_BASE + 0x20, 0x00000000);
	POKE_U32(VI_REG_BASE + 0x24, (fb_addr & 0xfffe00));
	POKE_U32(VI_REG_BASE + 0x28, 0x00000000);
	POKE_U32(VI_REG_BASE + 0x34, 0x10010001);
	POKE_U32(VI_REG_BASE + 0x38, 0x10010001);
	POKE_U32(VI_REG_BASE + 0x3c, 0x10010001);
	POKE_U32(VI_REG_BASE + 0x40, 0x00000000);
	POKE_U32(VI_REG_BASE + 0x44, 0x00000000);
	POKE_U16(VI_REG_BASE + 0x48, 0x2850);
	POKE_U16(VI_REG_BASE + 0x4a, 0x0100);
	POKE_U32(VI_REG_BASE + 0x4c, 0x1AE771F0);
	POKE_U32(VI_REG_BASE + 0x50, 0x0DB4A574);
	POKE_U32(VI_REG_BASE + 0x54, 0x00C1188E);
	POKE_U32(VI_REG_BASE + 0x58, 0xC4C0CBE2);
	POKE_U32(VI_REG_BASE + 0x5C, 0xFCECDECF);
	POKE_U32(VI_REG_BASE + 0x60, 0x13130F08);
	POKE_U32(VI_REG_BASE + 0x64, 0x00080C0F);
	POKE_U32(VI_REG_BASE + 0x68, 0x00FF0000);
	POKE_U16(VI_REG_BASE + 0x6c, params->is_progressive);

	POKE_U16(VI_REG_BASE + 0x70, 0x0280);
	POKE_U16(VI_REG_BASE + 0x72, 0x0000);
	POKE_U16(VI_REG_BASE + 0x74, 0x0000);

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

void TW_WriteTerminalAscii(TwTerminal *params, TwVideo *video, const char *chars, int len) {
	int text_offsets[TERMINAL_ROWS];
	for (int i = 0; i < TERMINAL_ROWS; i++)
		text_offsets[i] = 0;

	int line_origin, line = params->row;
	int col_origin, col = params->column;
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

	unsigned blended = ((params->back >> 1) & 0x7f7f7f7f) + ((params->fore >> 1) & 0x7f7f7f7f);
	unsigned colors[4];
	colors[0] = params->back;
	colors[1] = blended;
	colors[2] = blended;
	colors[3] = params->fore;

	for (int i = start; i < limit; i++) {
		int is_newline = chars[i] == '\n' || (!params->disable_wrap && col >= TERMINAL_COLS - 1);
		if (i < limit-1 && !is_newline) {
			col++;
			continue;
		}
		int r = line % TERMINAL_ROWS;
		if (home_col != 0) {
			for (int j = 0; j < _glyph_height; j++) {
				for (int k = 0; k < (home_col * _glyph_width) / 2; k++) {
					video->xfb[((r * _glyph_height) + j) * 320 + k] = params->back;
				}
			}
		}
		// FIXME: this won't work for glyphs of an odd width
		for (int j = 0; j < _glyph_height; j++) {
			for (int k1 = home_col, k2 = home; k1 < col; k1++, k2++) {
				int idx = (chars[k2] >= 0x20 && chars[k2] <= 0x7e) ? (chars[k2] - 0x20) : 0;
				for (int l = 0; l < _glyph_width; l += 2) {
					int bit_pos = idx * _glyph_size * 8 + j * _glyph_width + l;
					int c = (_glyph_data[bit_pos >> 3] >> (6 - (bit_pos & 7))) & 3;
					int offset = ((r * _glyph_height) + j) * 320 + (k1 * _glyph_width + l) / 2;
					//*(volatile unsigned*)(0xff000000 | (unsigned)&video->xfb[offset]) = c;
					video->xfb[offset] = colors[c];
				}
			}
		}
		if (col < TERMINAL_COLS - 1) {
			for (int j = 0; j < _glyph_height; j++) {
				for (int k = (col * _glyph_width) / 2; k < ((TERMINAL_COLS - 1) * _glyph_width) / 2; k++) {
					video->xfb[((r * _glyph_height) + j) * 320 + k] = params->back;
				}
			}
		}
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
