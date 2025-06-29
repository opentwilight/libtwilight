#include <twilight_ppc.h>
#include <font.h>

#define TERMINAL_COLS 80
#define TERMINAL_ROWS 30

#define BOUNDS_CHECK_COLUMN(term) \
	if (term->column < 0) \
		term->column = 0; \
	if (term->column >= TERMINAL_COLS) \
		term->column = TERMINAL_COLS - 1; \

#define BOUNDS_CHECK_ROW(term) \
	if (term->row < 0) \
		term->row = 0; \
	if (term->row >= TERMINAL_ROWS) \
		term->row = TERMINAL_ROWS - 1; \

static const unsigned _terminalColorPalatte[] = {
	0x00800080, 0x207120DE, 0x76497624, 0x8D3E8D88,
	0x0CCD0C76, 0x32BB32E3, 0x8395831A, 0xBF80BF80,
	0x55805580, 0x797079E8, 0xCE47CE22, 0xF236F289,
	0x61CA6177, 0x85B985DE, 0xDA90DA18, 0xFF80FF80,
};

static int _initialized = 0;
static TwTermFont _default_font;

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

TwVideo _tw_default_video = {0};

TwVideo *TW_GetDefaultVideo(void) {
	return &_tw_default_video;
}

static void init_video(TwVideo *params) {
	_default_font = (TwTermFont) {
		.data = &_glyph_data[0],
		.width = _glyph_width,
		.height = _glyph_height,
		.bytesPerGlyph = _glyph_size,
		.count = _glyph_count
	};

	unsigned short dcr = PEEK_U16(TW_VIDEO_REG_BASE + 2);
	params->format = (dcr >> 8) & 3;
	params->isProgressive = (dcr >> 2) & 1;

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
		if (params->isProgressive) {
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
	POKE_U16(TW_VIDEO_REG_BASE + 0x6c, params->isProgressive);

	/*
	POKE_U16(TW_VIDEO_REG_BASE + 0x70, 0x0280);
	POKE_U16(TW_VIDEO_REG_BASE + 0x72, 0x0000);
	POKE_U16(TW_VIDEO_REG_BASE + 0x74, 0x0000);
	*/

	TW_DisableInterrupts();
	_mode_flags = (params->format == TW_VIDEO_PAL50) | (params->isProgressive << 1);
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
		_tw_default_video = *params;
		_initialized = 1;
	}
}

void TW_InvalidateVideoTiming() {
	TW_DisableInterrupts();
	_changed = 1;
	TW_EnableInterrupts();
}

void TW_AwaitVideoVBlank(TwVideo *params) {
	PPC_SYNC();
	unsigned cur_frame = _frame_counter;
	while (PEEK_U32(&_frame_counter) == cur_frame) {
		TW_Sleep(20000);
		PPC_SYNC();
	}
}

// TODO: represent alpha with second Y channel
unsigned TW_RgbaToYuyv(int r, int g, int b, int a) {
	int y = 2126 * r + 7152 * g + 722 * b;
	int u = -999 * r + -3361 * g + 4360 * b;
	int v = 6150 * r + -5586 * g + -564 * b;
	y /= 10000;
	u = (u / 10000) + 128;
	v = (v / 10000) + 128;
	if (y > 255) y = 255;
	if (y < 0) y = 0;
	if (u > 255) u = 255;
	if (u < 0) u = 0;
	if (v > 255) v = 255;
	if (v < 0) v = 0;
	return ((unsigned)y << 24) | ((unsigned)u << 16) | ((unsigned)y << 8) | (unsigned)v;
}

unsigned TW_YuvToRgb(int y, int u, int v) {
	int r = 10000 * y + 0 * u + 12803 * v;
	int g = 10000 * y + -2148 * u + -3806 * v;
	int b = 10000 * y + 21280 * u + 0 * v;
	r /= 10000;
	g /= 10000;
	b /= 10000;
	if (y > 255) y = 255;
	if (y < 0) y = 0;
	if (u > 255) u = 255;
	if (u < 0) u = 0;
	if (v > 255) v = 255;
	if (v < 0) v = 0;
	return ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b;
}

void TW_ClearVideoScreen(TwVideo *params, unsigned color) {
	TW_FillWordsAndFlush(params->xfb, color, 320 * 480);
}

void TW_ClearVideoRectangle(TwVideo *params, unsigned color, int x, int y, int w, int h) {
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (x + w > 320)
		w = 320 - x;
	if (y + h > 480)
		h = 480 - y;

	if (w <= 0 || h <= 0)
		return;

	unsigned *xfb = params->xfb;
	for (int i = 0; i < h; i++)
		TW_FillWordsAndFlush(&xfb[(y+i)*w+x], color, w);
}

// TODO: actually handle fractional x and y rather than just convert to int
// And maybe handle x & y scaling and rotation in a separate code path?
void TW_DrawAsciiSpan(TwVideo *video, TwTermFont *font, unsigned back, unsigned fore, float x, float y, const char *str, int count) {
	if (!font)
		font = &_default_font;

	int halfw = font->width / 2;
	float w = count * halfw;
	float h = (float)font->height;
	float x_off = 0;
	float y_off = 0;
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
	if (x + w > 320.f) {
		w = 320.f - x;
	}
	if (y + h > 480.f) {
		h = 480.f - y;
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
			char ch = str[(j + (int)x_off) / halfw];
			int idx = ch < 0x20 || ch > 0x7f ? 0 : ((int)ch - 0x20);
			int bit_pos = idx * font->bytesPerGlyph * 8 + ((i + (int)y_off) * halfw + ((j + (int)x_off) % halfw)) * 2;
			int c = (font->data[bit_pos >> 3] >> (6 - (bit_pos & 7))) & 3;
			int offset = (i + y) * 320 + j + x;
			video->xfb[offset] = colors[c];
		}
		TW_FlushMemory(&video->xfb[(i + (int)y) * 320 + (int)x], w * 4);
	}
}

void _tw_handle_ansi_esc(TwTerminal *term, TwTermFont *font, TwVideo *video, char *escSeq, int seqLen) {
	int part = 0;
	int curNm = 0;
	int numeric[8];
	for (int i = 0; i < 8; i++)
		numeric[i] = 0;

	// It's assumed that if this function is called, then the first two bytes are the ANSI escape sequence (0x1b 0x5b)
	for (int i = 2; i < seqLen; i++) {
		if (escSeq[i] >= '0' && escSeq[i] <= '9') {
			numeric[curNm] = numeric[curNm] * 10 + escSeq[i] - '0';
			continue;
		}
		switch (escSeq[i]) {
			case ';':
				if (curNm < 7)
					curNm++;
				break;
			case 'H': // Move to row;column
			case 'f':
				term->row = numeric[0];
				term->column = numeric[1];
				BOUNDS_CHECK_ROW(term)
				BOUNDS_CHECK_COLUMN(term)
				return;
			case 'A': // Move up
			case 'F':
				if (numeric[0] <= 0)
					numeric[0] = 1;
				term->row -= numeric[0];
				BOUNDS_CHECK_ROW(term)
				if (escSeq[i] == 'F')
					term->column = 0;
				return;
			case 'B': // Move down
			case 'E':
				if (numeric[0] <= 0)
					numeric[0] = 1;
				term->row += numeric[0];
				BOUNDS_CHECK_ROW(term)
				if (escSeq[i] == 'E')
					term->column = 0;
				return;
			case 'C':  // Move right
				if (numeric[0] <= 0)
					numeric[0] = 1;
				term->column += numeric[0];
				BOUNDS_CHECK_COLUMN(term)
				return;
			case 'D':  // Move left
				if (numeric[0] <= 0)
					numeric[0] = 1;
				term->column -= numeric[0];
				BOUNDS_CHECK_COLUMN(term)
				return;
			case 'G':  // Move to column
				term->column = numeric[0];
				BOUNDS_CHECK_COLUMN(term)
				return;
			case 'J':
			case 'K':
				if (numeric[0] == 0) {
					TW_ClearVideoRectangle(
						video,
						term->back,
						term->column * font->width,
						term->row * font->height,
						320 - term->column * font->width,
						font->height
					);
					if (escSeq[i] == 'J')
						TW_ClearVideoRectangle(video, term->back, 0, 0, 320, term->row * font->height);
				}
				else if (numeric[0] == 1) {
					TW_ClearVideoRectangle(
						video,
						term->back,
						0,
						term->row * font->height,
						term->column * font->width,
						font->height
					);
					if (escSeq[i] == 'J') {
						TW_ClearVideoRectangle(
							video,
							term->back,
							0,
							(term->row + 1) * font->height,
							320,
							480 - (term->row + 1) * font->height
						);
					}
				}
				else if (numeric[0] == 2 || numeric[0] == 3) {
					if (escSeq[i] == 'J')
						TW_ClearVideoScreen(video, term->back);
					else
						TW_ClearVideoRectangle(video, term->back, 0, term->row * font->height, 320, font->height);
				}
				return;
			case 'm':
				if (numeric[0] == 0) {
					term->fore = 0xff80ff80;
					term->back = 0x00800080;
				}
				else if (numeric[0] >= 30 && numeric[0] <= 37) {
					term->fore = _terminalColorPalatte[numeric[0] - 30];
				}
				else if (numeric[0] >= 90 && numeric[0] <= 97) {
					term->fore = _terminalColorPalatte[numeric[0] - 90 + 8];
				}
				else if (numeric[0] >= 40 && numeric[0] <= 47) {
					term->back = _terminalColorPalatte[numeric[0] - 40];
				}
				else if (numeric[0] >= 100 && numeric[0] <= 107) {
					term->back = _terminalColorPalatte[numeric[0] - 100 + 8];
				}
				else if (numeric[0] == 38 || numeric[0] == 48) {
					unsigned *outColor = numeric[0] == 38 ? &term->fore : &term->back;
					if (numeric[1] == 2) {
						numeric[5] = 255 - numeric[5];
						*outColor = TW_RgbaToYuyv(numeric[2], numeric[3], numeric[4], numeric[5]);
					}
					else if (numeric[1] == 5) {
						if (numeric[2] < 16) {
							*outColor = _terminalColorPalatte[numeric[2]];
						}
						else if (numeric[2] < 232) {
							int rgb = (int)numeric[2] - 16;
							int r = rgb / 36;
							int g = (rgb / 6) % 6;
							int b = rgb % 6;
							*outColor = TW_RgbaToYuyv(r*51, g*51, b*51, 255);
						}
						else {
							unsigned lum = (numeric[2] - 232) * 11;
							if (lum >= 85) lum++;
							if (lum >= 170) lum++;
							*outColor = 0x00800080 | (lum << 24) | (lum << 8);
						}
					}
				}
				else if (numeric[0] == 39) {
					term->fore = 0xff80ff80;
				}
				else if (numeric[0] == 49) {
					term->back = 0x00800080;
				}
				return;
		}
	}
}

int _tw_read_ansi_esc_bytes(TwTerminal *term, TwTermFont *font, TwVideo *video, const char *chars, int len) {
	if (len <= 0)
		return 0;

	int offset = 0;
	if (term->ansiEscLen == 0 && chars[0] == 0x1b) {
		term->ansiEscBuf[term->ansiEscLen++] = 0x1b;
		offset++;
	}	
	while (offset < len && term->ansiEscLen > 0 && term->ansiEscLen < TW_MAX_ANSI_ESC) {
		if (term->ansiEscLen >= 2) {
			char c = chars[offset++];
			term->ansiEscBuf[term->ansiEscLen++] = c;
			if (c >= '@' && c <= '~') {
				_tw_handle_ansi_esc(term, font, video, term->ansiEscBuf, term->ansiEscLen);
				term->ansiEscLen = 0;
				break;
			}
		}
		else {
			if (chars[offset] != '[') {
				term->ansiEscLen = 0;
			}
			else {
				term->ansiEscBuf[term->ansiEscLen++] = '[';
				offset++;
			}
		}
	}
	if (term->ansiEscLen >= TW_MAX_ANSI_ESC)
		term->ansiEscLen = 0;

	return offset;
}

int TW_PrintTerminal(TwTerminal *term, TwTermFont *font, TwVideo *video, const char *chars, int len) {
	if (!font)
		font = &_default_font;

	int offset = 0;
	while (offset < len) {
		int isAnsiStart = chars[offset] == 0x1b && term->ansiEscLen == 0;
		while ((isAnsiStart || term->ansiEscLen > 0) && offset < len) {
			isAnsiStart = 0;
			int processed = _tw_read_ansi_esc_bytes(term, font, video, &chars[offset], len - offset);
			if (processed <= 0)
				term->ansiEscLen = 0;
			else
				offset += processed;
		}
		if (offset >= len)
			return len;

		int line = term->row;
		int col = term->column;
		int home = offset;
		int home_col = col;

		int i;
		for (i = offset; i < len; i++) {
			if (i < len-1 && chars[i] != 0x1b && chars[i] != '\r' && chars[i] != '\n' && !((term->flags & TW_DISABLE_WRAP) == 0 && col >= TERMINAL_COLS - 1)) {
				col++;
				continue;
			}
			if (i > home) {
				int r = line % TERMINAL_ROWS;
				TW_DrawAsciiSpan(video, font, term->back, term->fore, home_col * font->width, r * font->height, &chars[home], i - home);
				home = i;
			}
			if (chars[i] == 0x1b)
				break;

			col = 0;
			home_col = 0;
			if (home == i && (chars[i] == '\r' || chars[i] == '\n'))
				home++;
			if (chars[i] != '\r')
				line++;

			if (line >= TERMINAL_ROWS) {
				if (term->flags & TW_DISABLE_SCROLL) {
					term->row = line;
					term->column = col;
					return i;
				}

				int scroll = line - TERMINAL_ROWS - 1;
				int delta = 320 * font->height * scroll;
				for (int i = delta; i < 320 * 480; i++) {
					video->xfb[i-delta] = video->xfb[i];
					video->xfb[i] = term->back;
				}
				line -= scroll;
			}
		}
		offset = i;

		term->row = line;
		term->column = col;
	}

	return offset;
}
