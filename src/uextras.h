/*
* Copyright (C) 2007-2013, GrammarSoft ApS
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

#include "stdafx.h"
#include "Strings.h"

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
		for (size_t i=0 ; i<length ; i++) {
			if (!ISSPACE(text[i])) {
				return false;
			}
		}
	}
	return true;
}

inline UChar *ux_substr(const UChar *string, const size_t start, const size_t end) {
	const size_t length = static_cast<size_t>(u_strlen(string));
	assert(length >= end);
	assert(length >= start);
	assert(length >= end-start);

	UChar *tmp = new UChar[end-start+1];
	std::fill(tmp, tmp+(end-start+1), 0);
	u_strncpy(tmp, &string[start], end-start);

	return tmp;
}

inline bool ux_simplecasecmp(const UChar *a, const UChar *b, const size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		if (a[i] != b[i] && a[i] != b[i]+32) {
			return false;
		}
	}

	return true;
}

std::string ux_dirname(const char *in);

}

#endif
