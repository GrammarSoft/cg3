/*
* Copyright (C) 2007-2015, GrammarSoft ApS
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

#pragma once
#ifndef c6d28b7452ec699b_UEXTRAS_H
#define c6d28b7452ec699b_UEXTRAS_H

#include "stdafx.hpp"
#include "Strings.hpp"

#ifdef _WIN32
inline const char *basename(const char *path) {
	if (path != NULL) {
		// Find the last position of \ or / in the path name
		const char *pos = std::max(strrchr(path, '\\'), strrchr(path, '/'));

		if (pos != NULL) { // If a \ char was found...
			if (pos + 1 != NULL) // If it is not the last character in the string...
				return pos + 1; // then return a pointer to the first character after \.
			else
				return pos; // else return a pointer to \.
		}
		else { // If a \ char was NOT found
			return path; // return the pointer passed to basename (this is probably non-conformant)
		}

	}
	else { // If path == NULL, return "."
		return ".";
	}
}
#endif

namespace CG3 {

inline int ux_isSetOp(const UChar *it) {
	switch (it[1]) {
	case 0:
		switch (it[0]) {
		case '|':
			return S_OR;
		case '+':
			return S_PLUS;
		case '-':
			return S_MINUS;
		case '^':
			return S_FAILFAST;
		case 8745:
			return S_SET_ISECT_U;
		case 8710:
			return S_SET_SYMDIFF_U;
		default:
			break;
		}
		break;
	case 'R':
	case 'r':
		switch (it[0]) {
		case 'O':
		case 'o':
			switch (it[2]) {
			case 0:
				return S_OR;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return S_IGNORE;
}

inline bool ux_isEmpty(const UChar *text) {
	size_t length = u_strlen(text);
	if (length > 0) {
		for (size_t i = 0; i < length; i++) {
			if (!ISSPACE(text[i])) {
				return false;
			}
		}
	}
	return true;
}

inline bool ux_simplecasecmp(const UChar *a, const UChar *b, const size_t n) {
	for (size_t i = 0; i < n; ++i) {
		if (a[i] != b[i] && a[i] != b[i] + 32) {
			return false;
		}
	}

	return true;
}


template<typename Str>
struct substr_t {
	typedef typename Str::value_type value_type;
	const Str& str;
	size_t offset, count;
	value_type old_value;

	substr_t(const Str& str, size_t offset = 0, size_t count = Str::npos)
	  : str(str)
	  , offset(offset)
	  , count(count)
	  , old_value(0)
	{
		if (count != Str::npos) {
			old_value = str[offset + count];
		}
	}

	~substr_t() {
		if (count != Str::npos) {
			value_type *buf = const_cast<value_type*>(str.c_str() + offset);
			buf[count] = old_value;
		}
	}

	const value_type *c_str() const {
		value_type *buf = const_cast<value_type*>(str.c_str() + offset);
		buf[count] = 0;
		return buf;
	}
};

template<typename Str>
inline substr_t<Str> substr(const Str& str, size_t offset = 0, size_t count = 0) {
	return substr_t<Str>(str, offset, count);
}

inline UChar *ux_bufcpy(UChar *dst, const UChar *src, size_t n) {
	size_t i = 0;
	for (; i < n && src && src[i]; ++i) {
		dst[i] = src[i];
		if (dst[i] == 0x0A || dst[i] == 0x0D) {
			dst[i] += 0x2400;
		}
	}
	dst[i] = 0;
	return dst;
}

std::string ux_dirname(const char *in);
}

#endif
