/*
* Copyright (C) 2007-2011, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
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

#ifdef _MSC_VER
	#define _SECURE_SCL 0
	#define _CRT_SECURE_NO_DEPRECATE
	#define WIN32_LEAN_AND_MEAN
	#define VC_EXTRALEAN
	#define NOMINMAX
	#include <windows.h>
#endif

#include "uextras.h"
#include "inlines.h"

namespace CG3 {

// ToDo: Make all of ux_* inline to get around possible memory errors.

bool ux_isEmpty(const UChar *text) {
	size_t length = u_strlen(text);
	if (length > 0) {
		for (size_t i=0 ; i<=length ; i++) {
			if (!ISSPACE(text[i])) {
				return false;
			}
		}
	}
	return true;
}

UChar *ux_substr(const UChar *string, const size_t start, const size_t end) {
	assert((size_t)u_strlen(string) >= end);
	assert((size_t)u_strlen(string) >= start);
	assert((size_t)u_strlen(string) >= end-start);

	UChar *tmp = new UChar[end-start+1];
	std::fill(tmp, tmp+(end-start+1), 0);
	u_strncpy(tmp, &string[start], end-start);

	return tmp;
}

std::string ux_dirname(const char *in) {
	char tmp[32768] = {0};
#ifdef _MSC_VER
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
	if (tmp[tlen-1] != '/' && tmp[tlen-1] != '\\') {
		tmp[tlen+1] = 0;
		tmp[tlen] = '/';
	}
	return tmp;
}

bool ux_simplecasecmp(const UChar *a, const UChar *b, const size_t n) {
	for (size_t i = 0 ; i < n ; ++i) {
		if (a[i] != b[i] && a[i] != b[i]+32) {
			return false;
		}
	}

	return true;
}

}
