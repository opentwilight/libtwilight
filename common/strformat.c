#include <twilight_common.h>

#define FMT_BUFFER 512
#define NUMERIC_BUFFER 128

#define DOUBLE_AUTO     0
#define DOUBLE_DECIMAL  1
#define DOUBLE_EXPONENT 2

#define SHIFT_DIGITS_RIGHT(amount, to10th) \
	for (int i = firstDigit; i < endDigit; i++) { \
		int digits = ((int)buf[i*4 + src] * to10th) >> amount; \
		int carry = 0; \
		int j = i - amount; \
		while ((digits > 0 || carry > 0) && j < 1384) { \
			int d = digits % 10; \
			digits /= 10; \
			char *dst = &buf[j*4 + (src^1)]; \
			int a = (int)*dst + d + carry; \
			*dst = (char)(a % 10); \
			carry = a / 10; \
			j++; \
		} \
	} \
	firstDigit -= amount;

#define SHIFT_DIGITS_LEFT(amount) \
	int mCarry = 0; \
	int aCarry = 0; \
	int i = firstDigit; \
	for ( ; i < endDigit; i++) { \
		int dM = (((int)buf[i*4 + src]) << amount) + mCarry; \
		mCarry = dM / 10; \
		char *dst = &buf[i*4 + (src^1)]; \
		int dA = (int)*dst + (dM % 10) + aCarry; \
		aCarry = dA / 10; \
		*dst = (char)(dA % 10); \
	} \
	while (mCarry > 0 && i < 1384) { \
		int dM = mCarry; \
		mCarry = dM / 10; \
		buf[i*4 + (src^1)] = (char)(dM % 10); \
		i++; \
		endDigit++; \
	} \
	firstDigit += amount; \

#define ADD_MANTISSA_MULTIPLE(dst, src, amount) \
	int mCarry = 0; \
	int aCarry = 0; \
	int j = firstDigit; \
	for ( ; j < endDigit; j++) { \
		int dM = (((int)buf[j*4 + src]) << amount) + mCarry; \
		mCarry = dM / 10; \
		char *p = &buf[j*4 + dst]; \
		int dA = (int)*p + (dM % 10) + aCarry; \
		aCarry = dA / 10; \
		char out = (char)(dA % 10); \
		*p = out; \
	} \
	char prev = 1; \
	while ((prev != 0 || mCarry > 0 || aCarry > 0) && j < 1384) { \
		int dM = mCarry; \
		mCarry = dM / 10; \
		char *p = &buf[j*4 + dst]; \
		int dA = (int)*p + (dM % 10) + aCarry; \
		aCarry = dA / 10; \
		*p = (char)(dA % 10); \
		prev = *p; \
		j++; \
		endDigit++; \
	}

// Anyone is welcome to write a more efficient version of this
int TW_WriteDouble(char *outBuf, int maxSize, int minWidth, int precision, int mode, double value) {
	unsigned wBuf[1384];
	int outPos = 0;

	unsigned long long asU64 = 0;
	*(double*)&asU64 = value;
	int exponent = (asU64 >> 52) & 0x7ff;
	unsigned long long mantissa = asU64 & 0x000FffffFFFFffffULL;

	if (asU64 >> 63) {
		outBuf[outPos++] = '-';
	}
	if (exponent == 0) {
		// TODO: if mantissa, handle subnormals
		if (minWidth < 1)
			minWidth = 1;
		if (precision < 1)
			precision = 1;

		for (int i = 0; i < minWidth && outPos < maxSize; i++)
			outBuf[outPos++] = '0';
		if (outPos < maxSize)
			outBuf[outPos++] = '.';
		for (int i = 0; i < precision && outPos < maxSize; i++)
			outBuf[outPos++] = '0';

		return outPos;
	}
	if (exponent == 0x7ff) {
		if (mantissa) {
			if (outPos < maxSize) outBuf[outPos++] = 'n';
			if (outPos < maxSize) outBuf[outPos++] = 'a';
			if (outPos < maxSize) outBuf[outPos++] = 'n';
		}
		else {
			if (outPos < maxSize) outBuf[outPos++] = 'i';
			if (outPos < maxSize) outBuf[outPos++] = 'n';
			if (outPos < maxSize) outBuf[outPos++] = 'f';
		}
		return outPos;
	}

	for (int i = 0; i < 1384; i++)
		wBuf[i] = 0;

	// the mantissa takes up 52 bits.
	// 2^-52, without the leading zeroes, reversed.
	char *buf = (char*)wBuf;
	for (int i = 0; i < 37; i++)
		buf[(1022+i)*4] = (char)("5260461816333627480803130529406440222"[i] - '0');

	int firstDigit = 1022;
	int endDigit = firstDigit + 37;
	int src = 0;
	int intervalShift = exponent - 1023;

	if (intervalShift < 0) {
		// shift right
		int toShift = -intervalShift;
		// the trick here is to provide enough zeroes to perform a right shift in a 32-bit integer without losing fractional bits.
		// this means we can only shift 8 at a time, instead of 27.
		while (toShift >= 8) {
			SHIFT_DIGITS_RIGHT(8, 100000000)
			src ^= 1;
			toShift -= 8;
		}
		if (toShift > 0) {
			int power = 1;
			for (int i = 0; i < toShift; i++)
				power *= 10;
			SHIFT_DIGITS_RIGHT(toShift, power)
			src ^= 1;
		}
	}
	else if (intervalShift > 0) {
		// shift left
		int toShift = intervalShift;
		while (toShift >= 27) {
			SHIFT_DIGITS_LEFT(27)
			src ^= 1;
			toShift -= 27;
		}
		if (toShift > 0) {
			SHIFT_DIGITS_LEFT(toShift)
			src ^= 1;
		}
	}

	// the first 27 mantissa bits
	for (int i = 0; i < 27; i++) {
		if ((mantissa >> i) & 1ull) {
			ADD_MANTISSA_MULTIPLE(2, src, i)
		}
	}

	// shift the interval up by 27 bits (so we can still use 32-bit numbers, which speeds up division by 10)
	{
		int mCarry = 0;
		int aCarry = 0;
		int i = firstDigit;
		for ( ; i < endDigit; i++) {
			int dM = (((int)buf[i*4 + src]) << 27) + mCarry;
			mCarry = dM / 10;
			buf[i*4 + (src^1)] = (char)(dM % 10);
		}
		while (mCarry > 0 && i < 1384) {
			int dM = mCarry;
			mCarry = dM / 10;
			buf[i*4 + (src^1)] = (char)(dM % 10);
			i++;
			endDigit++;
		}
		src ^= 1;
		firstDigit += 27;
	}

	// the next 25 mantissa bits, plus one
	for (int i = 0; i < 26; i++) {
		// the mantissa encoding excludes the top bit, so the "i == 25 ||" adds it back in
		if (i == 25 || ((mantissa >> (27+i)) & 1ull)) {
			ADD_MANTISSA_MULTIPLE(2, src, i)
		}
	}

	firstDigit -= 27;

	endDigit--;
	while (buf[endDigit * 4 + 2] == 0 && endDigit > 1074)
		endDigit--;

	int inPos = endDigit;
	if (minWidth < 1)
		minWidth = 1;

	for (int i = 0; i < minWidth && inPos < 1074 + minWidth - i && outPos < maxSize; i++)
		outBuf[outPos++] = '0';

	while (outPos < maxSize && inPos >= 1074 && inPos >= firstDigit) {
		outBuf[outPos++] = '0' + buf[inPos * 4 + 2];
		inPos--;
	}

	if (firstDigit > 1074) {
		for (int i = 0; i < firstDigit - 1074 && outPos < maxSize; i++)
			outBuf[outPos++] = '0';
		if (outPos < maxSize)
			outBuf[outPos++] = '.';
		for (int i = 0; i < precision && outPos < maxSize; i++)
			outBuf[outPos++] = '0';
	}
	else {
		if (outPos < maxSize)
			outBuf[outPos++] = '.';
		int i;
		for (i = 0; i < precision && outPos < maxSize && inPos >= firstDigit; i++) {
			outBuf[outPos++] = '0' + buf[(inPos-i) * 4 + 2];
			inPos--;
		}
		for (; i < precision && outPos < maxSize && inPos - i >= 1074; i++)
			outBuf[outPos++] = '0';
	}

	return outPos;
}

int TW_WriteInteger(char *outBuf, int maxSize, int minWidth, unsigned bits, unsigned base, unsigned flags, unsigned long long value) {
	if (value == 0 || base <= 1) {
		outBuf[0] = '0';
		return 1;
	}

	char buf[64];
	int isNeg = 0;
	if (bits % 2 == 1) {
		if (value >> bits) {
			value = ~value + 1;
			isNeg = 1;
		}
		bits++;
	}

	value &= ((1ull << bits) - 1ull);

	char isUpper = (flags & 1) != 0;
	char padCh = (flags & 2) ? '0' : ' ';
	char sign = (flags & 4) != 0;
	char isLeft = (flags & 8) != 0;
	char isAlt = (flags & 16) != 0;

	char inc[2];
	inc[0] = '0';
	inc[1] = (char)(0x57 - (isUpper << 5));

	int pos = 0;
	while (value && pos < 64) {
		unsigned rem = TW_DivideU64(&value, base);
		buf[pos++] = inc[rem >= 10] + (char)rem;
	}

	int space = pos + (isNeg || sign) + (isAlt && (base == 16 || base == 8)) + (isAlt && base == 16);
	int outPos = 0;

	if (space < minWidth && padCh == ' ' && !isLeft) {
		for (int i = 0; i < minWidth - space && outPos < maxSize; i++)
			outBuf[outPos++] = padCh;
	}

	if ((isNeg || sign) && outPos < maxSize)
		outBuf[outPos++] = isNeg ? '-' : '+';

	if (isAlt) {
		if ((base == 16 || base == 8) && outPos < maxSize)
			outBuf[outPos++] = '0';
		if (base == 16 && outPos < maxSize)
			outBuf[outPos++] = isUpper ? 'X' : 'x';
	}

	if (padCh == '0' && !isLeft) {
		for (int i = 0; i < minWidth - space && outPos < maxSize; i++)
			outBuf[outPos++] = padCh;
	}

	for (int i = 0; i < pos && outPos < maxSize; i++) {
		outBuf[outPos++] = buf[pos-i-1];
	}

	if (space < minWidth && isLeft) {
		for (int i = 0; i < minWidth - space && outPos < maxSize; i++)
			outBuf[outPos++] = ' ';
	}

	return outPos;
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
				if (fmt_min_width == -1) {
					fmt_min_width = va_arg(args, int);
				}
				if (fmt_precision == -1) {
					fmt_precision = va_arg(args, int);
				}

				int numericBytesWritten = 0;
				switch (c) {
					case 'a':
					case 'A':
					{
						// hex float
						// we don't support hex float, so this just prints the float in regular decimal notation
						double value = va_arg(args, double);
						numericBytesWritten = TW_WriteDouble(numeric_buf, NUMERIC_BUFFER, fmt_min_width, fmt_precision, DOUBLE_DECIMAL, value);
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

						// isLeft, sign, isZeroPad, isUpper (always false)
						unsigned flags = ((format_flags & 1) << 3) | ((format_flags & 2) << 1) | ((format_flags & 16) >> 3);
						numericBytesWritten = TW_WriteInteger(numeric_buf, NUMERIC_BUFFER, fmt_min_width, format_bits - 1, 10, flags, value);
						break;
					}
					case 'e':
					case 'E':
					{
						// decimal float
						double value = va_arg(args, double);
						numericBytesWritten = TW_WriteDouble(numeric_buf, NUMERIC_BUFFER, fmt_min_width, fmt_precision, DOUBLE_EXPONENT, value);
						break;
					}
					case 'f':
					case 'F':
					{
						// float
						double value = va_arg(args, double);
						if (fmt_precision == 0)
							fmt_precision = 6;
						numericBytesWritten = TW_WriteDouble(numeric_buf, NUMERIC_BUFFER, fmt_min_width, fmt_precision, DOUBLE_DECIMAL, value);
						break;
					}
					case 'g':
					case 'G':
					{
						// automatic float
						double value = va_arg(args, double);
						numericBytesWritten = TW_WriteDouble(numeric_buf, NUMERIC_BUFFER, fmt_min_width, fmt_precision, DOUBLE_AUTO, value);
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

						// isLeft, sign, isZeroPad, isUpper
						unsigned flags = ((format_flags & 8) << 1) | ((format_flags & 1) << 3) | ((format_flags & 2) << 1) | ((format_flags & 16) >> 3) | (c == 'X');
						numericBytesWritten += TW_WriteInteger(&numeric_buf[numericBytesWritten], NUMERIC_BUFFER, fmt_min_width, format_bits, base, flags, value);
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
							numericBytesWritten = 2 + TW_WriteInteger(&numeric_buf[2], NUMERIC_BUFFER, fmt_min_width, 32, 16, 0, (unsigned)value);
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
						TW_CopyBytes(&buf[pos], numeric_buf, numericBytesWritten);
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

	if (pos + 1 > FMT_BUFFER) {
		sink->transfer(sink, buf, pos);
		flushes++;
		pos = 0;
	}
	buf[pos++] = 0;

	int bytesWritten = flushes * FMT_BUFFER + pos;
	sink->transfer(sink, buf, pos);

	va_end(args);
	return bytesWritten - 1; // don't include null terminator
}
