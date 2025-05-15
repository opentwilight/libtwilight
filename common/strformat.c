#include "twilight_common.h"

#define FMT_BUFFER 256

int TW_FormatString(TwStream *sink, int maxOutputSize, const char *str, ...) {
	char buf[FMT_BUFFER];
	int pos = 0;
	int flushes = 0;
	unsigned format_mode = 0;
	unsigned format_flags = 0;
	int fmt_min_width = 0;
	int fmt_precision = 0;
	char prev = 0;

	for (int i = 0; str[i]; i++) {
		char c = str[i];
		if (c == '%') {
			if (format_mode > 0 || prev == '\\') {
				if (pos + 1 > FMT_BUFFER) {
					sink->transfer(sink, buf, pos);
					flushes++;
					pos = 0;
				}
				if (maxOutputSize > 0 && flushes * FMT_BUFFER + pos >= maxOutputSize)
					break;
				buf[pos++] = '%';
				format_mode = 0;
				format_flags = 0;
				fmt_min_width = 0;
				fmt_precision = 0;
			}
			else {
				format_mode = 1;
			}
		}
		else if (format_mode > 0) {
			if (format_mode == 1) {
				if (c == '-') {
					// left justified
					format_flags |= 1;
				}
				else if (c == '+') {
					// always print sign
					format_flags |= 2;
				}
				else if (c == ' ') {
					// pad with spaces
					format_flags |= 4;
				}
				else if (c == '#') {
					// alternative print mode
					format_flags |= 8;
				}
				else if (c == '0') {
					// pad with zeros
					format_flags |= 16;
				}
				else {
					format_mode = 2;
				}
				if (format_mode != 2)
					continue;
			}
			if (format_mode == 2) {
				if (fmt_min_width >= 0 && c >= '0' && c <= '9') {
					fmt_min_width = fmt_min_width * 10 + (c - '0');
				}
				else if (c == '*') {
					fmt_min_width = -1;
				}
				else {
					format_mode = 3;
				}
				if (format_mode != 3)
					continue;
			}
			if (format_mode == 3) {
				if (c == '.') {
					format_mode = 4;
					continue;
				}
				format_mode = 5;
			}
			if (format_mode == 4) {
				if (fmt_precision >= 0 && c >= '0' && c <= '9') {
					fmt_precision = fmt_precision * 10 + (c - '0');
				}
				else if (c == '*') {
					fmt_precision = -1;
				}
				else {
					format_mode = 5;
				}
				if (format_mode != 5)
					continue;
			}
			if (format_mode == 5) {
				switch (c) {
					case 'h':
						// short, or char if repeated
						break;
					case 'j':
						// intmax_t or uintmax_t
						break;
					case 'l':
						// long, or long long if repeated
						break;
					case 'L':
						// long double
						break;
					case 't':
						// ptrdiff_t
						break;
					case 'z':
						// size_t
						break;
					default:
						format_mode = 6;
						break;
				}
				if (format_mode != 6)
					continue;
			}
			if (format_mode == 6) {
				switch (c) {
					case 'a':
					case 'A':
						// hex float
						break;
					case 'c':
						// char
						break;
					case 'd':
					case 'i':
						// int
						break;
					case 'e':
					case 'E':
						// decimal float
						break;
					case 'f':
					case 'F':
						// float
						break;
					case 'g':
					case 'G':
						// automatic float
						break;
					case 'n':
						// write bytes so far to int pointer
						// flushes * FMT_BUFFER + pos
						break;
					case 'o':
						// octal unsigned int
						break;
					case 'p':
						// void pointer
						// 0xpointerhex or (null)
						break;
					case 's':
						// char pointer (string)
						break;
					case 'u':
						// unsigned decimal
						break;
					case 'x':
					case 'X':
						// unsigned hex
						break;
					default:
						format_mode = 8;
						break;
				}
				if (format_mode == 6)
					format_mode = 7;
			}
			if (format_mode == 7 || format_mode == 8) {
				
			}
		}
		else {
			if (pos + 1 > FMT_BUFFER) {
				sink->transfer(sink, buf, pos);
				flushes++;
				pos = 0;
			}
			buf[pos++] = c;
			if (maxOutputSize > 0 && flushes * FMT_BUFFER + pos >= maxOutputSize)
				break;
		}

		prev = c;
	}

	return flushes * FMT_BUFFER + pos;
}
