/*
* Copyright (C) 2007-2018, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef _WIN32
	#include <windows.h>
#endif

#include "uextras.hpp"
#include "inlines.hpp"

namespace CG3 {

std::string ux_dirname(const char* in) {
	char tmp[32768] = { 0 };
#ifdef _WIN32
	char *fname = 0;
	GetFullPathNameA(in, 32767, tmp, &fname);
	if (fname) {
		fname[0] = 0;
	}
#else
	strcpy(tmp, in);
	char *dir = dirname(tmp);
	if (dir != tmp) {
		strcpy(tmp, dir);
	}
#endif
	size_t tlen = strlen(tmp);
	if (tmp[tlen - 1] != '/' && tmp[tlen - 1] != '\\') {
		tmp[tlen + 1] = 0;
		tmp[tlen] = '/';
	}
	return tmp;
}
}

// ICU std::istream input wrappers
UChar* u_fgets(UChar* s, int32_t n, std::istream& input) {
	using namespace CG3;

	s[0] = 0;
	int32_t i = 0;
	for (; i < n; ++i) {
		UChar c = u_fgetc(input);
		if (c == U_EOF) {
			break;
		}
		s[i] = c;
		if (ISNL(c)) {
			break;
		}
	}
	if (i < n) {
		s[i + 1] = 0;
	}

	if (i == 0) {
		return nullptr;
	}
	return s;
}

UChar u_fgetc(std::istream& input) {
	struct _cps {
		std::istream* i = 0;
		UChar c = 0;
	};
	static _cps cps[4];

	for (auto& cp : cps) {
		if (cp.i == &input) {
			cp.i = 0;
			return cp.c;
		}
	}

	int c = 0;
	int i = 0;
	char buf[4];
	if ((c = input.get()) != EOF) {
		buf[i++] = static_cast<char>(c);
		if ((c & 0xF0) == 0xF0) {
			if (!input.read(buf + i, 3)) {
				throw std::runtime_error("Could not read 3 expected bytes from stream");
			}
			i += 3;
		}
		else if ((c & 0xE0) == 0xE0) {
			if (!input.read(buf + i, 2)) {
				throw std::runtime_error("Could not read 2 expected bytes from stream");
			}
			i += 2;
		}
		else if ((c & 0xC0) == 0xC0) {
			if (!input.read(buf + i, 1)) {
				throw std::runtime_error("Could not read 1 expected byte from stream");
			}
			i += 1;
		}
	}

	if (i == 0 && c == EOF) {
		return U_EOF;
	}

	if (c == 0) {
		return 0;
	}

	UChar u16[2] = {};
	UErrorCode err = U_ZERO_ERROR;
	u_strFromUTF8(u16, 2, 0, buf, i, &err);
	if (U_FAILURE(err)) {
		throw std::runtime_error("Failed to convert from UTF-8 to UTF-16");
	}

	if (u16[1]) {
		for (auto& cp : cps) {
			if (cp.i == 0) {
				cp.i = &input;
				cp.c = u16[1];
				return u16[0];
			}
		}
		throw std::runtime_error("Not enough space to store UTF-16 high surrogate");
	}

	return u16[0];
}

// ICU std::ostream output wrappers
void u_fflush(std::ostream& output) {
	output.flush();
}

void u_fflush(std::ostream* output) {
	output->flush();
}

inline int32_t _u_vsnprintf(UChar* dst, int32_t count, const UChar* fmt, va_list args) {
	return u_vsnprintf_u(dst, count, fmt, args);
}

inline int32_t _u_vsnprintf(UChar* dst, int32_t count, const char* fmt, va_list args) {
	return u_vsnprintf(dst, count, fmt, args);
}

template<typename Char>
inline int32_t _u_fprintf(std::ostream& output, const Char* fmt, va_list args) {
	using namespace CG3;

	UChar _buf16[500];
	UString _str16;
	UChar* buf16 = &_buf16[0];

	va_list args2;
	va_copy(args2, args);

	int32_t n16 = size(_buf16);
	n16 = _u_vsnprintf(buf16, n16, fmt, args);
	if (n16 < 0) {
		throw std::runtime_error("Critical error in u_fprintf() wrapper");
	}
	if (n16 > static_cast<int32_t>(size(_buf16))) {
		_str16.resize(n16 + 1);
		buf16 = &_str16[0];
		n16 = _u_vsnprintf(buf16, n16, fmt, args2);
	}

	char _buf8[size(_buf16) * 3];
	std::string _str8;
	char* buf8 = &_buf8[0];
	int32_t n8 = size(_buf8);
	int32_t u8 = 0;
	UErrorCode err = U_ZERO_ERROR;
	u_strToUTF8(buf8, n8, &u8, buf16, n16, &err);
	if (u8 > n8) {
		_str8.resize(u8 + 1);
		buf8 = &_str8[0];
		err = U_ZERO_ERROR;
		u_strToUTF8(buf8, u8, 0, buf16, n16, &err);
	}

	output.write(buf8, u8);

	return n16;
}

int32_t u_fprintf(std::ostream& output, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int32_t rv = _u_fprintf(output, fmt, args);
	va_end(args);
	return rv;
}

int32_t u_fprintf(std::unique_ptr<std::ostream>& output, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int32_t rv = _u_fprintf(*output.get(), fmt, args);
	va_end(args);
	return rv;
}

int32_t u_fprintf(std::ostream* output, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int32_t rv = _u_fprintf(*output, fmt, args);
	va_end(args);
	return rv;
}

int32_t u_fprintf_u(std::ostream& output, const UChar* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int32_t rv = _u_fprintf(output, fmt, args);
	va_end(args);
	return rv;
}

UChar32 u_fputc(UChar32 c32, std::ostream& output) {
	using namespace CG3;

	if (c32 <= 0x7F) {
		output.put(static_cast<char>(c32));
	}
	else if (c32 <= 0x7FFF) {
		char buf8[5];
		int32_t n8 = size(buf8);
		int32_t u8 = 0;
		UErrorCode err = U_ZERO_ERROR;
		UChar c16 = static_cast<UChar>(c32);
		u_strToUTF8(buf8, n8, &u8, &c16, 1, &err);

		output.write(buf8, u8);
	}
	else {
		throw std::runtime_error("u_fputc() wrapper can't handle >= 0x7FFF");
	}

	return c32;
}
