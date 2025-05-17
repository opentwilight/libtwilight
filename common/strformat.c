#include "twilight_common.h"

#define FMT_BUFFER 512
#define NUMERIC_BUFFER 64

int TW_WriteHexDouble(char *buf, int maxSize, double value) {
	return 0;
}

int TW_WriteDecimalDouble(char *buf, int maxSize, double value) {
	return 0;
}

int TW_WriteAutoDouble(char *buf, int maxSize, double value) {
	return 0;
}

int TW_WriteDouble(char *buf, int maxSize, double value) {
	return 0;
}

int TW_WriteInteger(char *buf, int maxSize, int bits, int base, int isUpper, int value) {
	return 0;
}

int TW_FormatString(TwStream *sink, int maxOutputSize, const char *str, ...) {
	va_list args;
	va_start(args, str);

	char buf[FMT_BUFFER];
	char numeric_buf[NUMERIC_BUFFER];
	int pos = 0;
	int flushes = 0;
	unsigned format_mode = 0;
	unsigned short format_flags = 0;
	unsigned short format_bits = 0;
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
						format_bits = 16;
						if (prev == 'h') {
							format_bits = 8;
							format_mode = 6;
						}
						break;
					case 'j':
						// intmax_t or uintmax_t
						format_bits = 64;
						format_mode = 6;
						break;
					case 'l':
						// long, or long long if repeated
						format_bits = 32;
						if (prev == 'l') {
							format_bits = 64;
							format_mode = 6;
						}
						break;
					case 'L':
						// long double
						format_bits = 64;
						format_mode = 6;
						break;
					case 't':
						// ptrdiff_t
						format_bits = 32;
						format_mode = 6;
						break;
					case 'z':
						// size_t
						format_bits = 32;
						format_mode = 6;
						break;
					default:
						format_bits = 32;
						format_mode = 6;
						break;
				}
				if (format_mode != 6)
					continue;
			}
			if (format_mode == 6) {
				int numericBytesWritten = 0;
				switch (c) {
					case 'a':
					case 'A':
					{
						// hex float
						double value = va_arg(args, double);
						numericBytesWritten = TW_WriteHexDouble(numeric_buf, NUMERIC_BUFFER, value);
						break;
					}
					case 'c':
					{
						// single ascii character
						char value = va_arg(args, int);
						numeric_buf[0] = value & 0xff;
						numericBytesWritten = 1;
						break;
					}
					case 'd':
					case 'i':
					{
						// integer
						unsigned long long value = 0;
						if (format_bits == 8) {
							int v = va_arg(args, int);
							value = (unsigned long long)v & 0xffULL;
						}
						else if (format_bits == 16) {
							int v = va_arg(args, int);
							value = (unsigned long long)v & 0xffffULL;
						}
						else if (format_bits == 32) {
							int v = va_arg(args, int);
							value = (unsigned long long)v & 0xFFFFffffULL;
						}
						else {
							long long v = va_arg(args, long long);
							value = (unsigned long long)v;
						}
						numericBytesWritten = TW_WriteInteger(numeric_buf, NUMERIC_BUFFER, format_bits - 1, 10, 0, value);
						break;
					}
					case 'e':
					case 'E':
					{
						// decimal float
						double value = va_arg(args, double);
						numericBytesWritten = TW_WriteDecimalDouble(numeric_buf, NUMERIC_BUFFER, value);
						break;
					}
					case 'f':
					case 'F':
					{
						// float
						double value = va_arg(args, double);
						numericBytesWritten = TW_WriteDouble(numeric_buf, NUMERIC_BUFFER, value);
						break;
					}
					case 'g':
					case 'G':
					{
						// automatic float
						double value = va_arg(args, double);
						numericBytesWritten = TW_WriteAutoDouble(numeric_buf, NUMERIC_BUFFER, value);
						break;
					}
					case 'n':
					{
						// write bytes so far to int pointer
						int *value_out = va_arg(args, int*);
						*value_out = flushes * FMT_BUFFER + pos;
						break;
					}
					case 'o':
					case 'u':
					case 'x':
					case 'X':
					{
						// octal/decimal/hex unsigned int
						unsigned long long value = 0;
						if (format_bits == 8) {
							unsigned v = va_arg(args, unsigned) & 0xffU;
							value = (unsigned long long)v;
						}
						else if (format_bits == 16) {
							unsigned v = va_arg(args, unsigned) & 0xffffU;
							value = (unsigned long long)v;
						}
						else if (format_bits == 32) {
							unsigned v = va_arg(args, unsigned);
							value = (unsigned long long)v;
						}
						else {
							value = va_arg(args, unsigned long long);
						}
						int base = 16;
						if (c == 'o') base = 8;
						if (c == 'u') base = 10;
						numericBytesWritten = TW_WriteInteger(numeric_buf, NUMERIC_BUFFER, format_bits, base, c == 'X', value);
						break;
					}
					case 'p':
					{
						// void pointer
						// 0xpointerhex or (null)
						void *value = va_arg(args, void*);
						if (!value) {
							TW_CopyBytes(numeric_buf, "(null)", 6);
							numericBytesWritten = 6;
						}
						else {
							numeric_buf[0] = '0';
							numeric_buf[1] = 'x';
							numericBytesWritten = 2 + TW_WriteInteger(&numeric_buf[2], NUMERIC_BUFFER, 32, 16, 0, (unsigned)value);
						}
						break;
					}
					case 's':
					{
						// char pointer (string)
						char *value = va_arg(args, char*);
						if (!value) {
							TW_CopyBytes(numeric_buf, "(null)", 6);
							numericBytesWritten = 6;
						}
						else {
							for (int i = 0; value[i]; i++) {
								char ch = value[i];
								if (pos + 1 > FMT_BUFFER) {
									sink->transfer(sink, buf, pos);
									flushes++;
									pos = 0;
								}
								if (maxOutputSize > 0 && flushes * FMT_BUFFER + pos >= maxOutputSize)
									break;
								buf[pos++] = ch;
							}
						}
						break;
					}
					default:
						break;
				}
				if (numericBytesWritten > 0) {
					int leftInBuffer = FMT_BUFFER - pos;
					if (leftInBuffer < numericBytesWritten) {
						TW_CopyBytes(&buf[pos], numeric_buf, leftInBuffer);
						sink->transfer(sink, buf, pos);
						flushes++;
						pos = 0;
						TW_CopyBytes(buf, &numeric_buf[leftInBuffer], numericBytesWritten - leftInBuffer);
					}
					else {
						TW_CopyBytes(buf, numeric_buf, numericBytesWritten);
						pos += numericBytesWritten;
					}
				}
				format_mode = 0;
				format_flags = 0;
				fmt_min_width = 0;
				fmt_precision = 0;
			}
		}
		else {
			if (pos + 1 > FMT_BUFFER) {
				sink->transfer(sink, buf, pos);
				flushes++;
				pos = 0;
			}
			if (maxOutputSize > 0 && flushes * FMT_BUFFER + pos >= maxOutputSize)
				break;
			buf[pos++] = c;
		}

		prev = c;
	}

	int bytesWritten = flushes * FMT_BUFFER + pos;
	if (pos > 0)
		sink->transfer(sink, buf, pos);

	va_end(args);
	return bytesWritten;
}
