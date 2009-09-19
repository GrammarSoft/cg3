/*
* Copyright (C) 2007, GrammarSoft ApS
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
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif

#include "uextras.h"
#include "Strings.h"
#include "inlines.h"

namespace CG3 {
using namespace CG3::Strings;

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

bool ux_trim(UChar *totrim) {
	bool retval = false;
	unsigned int length = u_strlen(totrim);
	if (totrim && length) {
		while (length >= 1 && ISSPACE(totrim[length-1])) {
			length--;
		}
		if (ISSPACE(totrim[length])) {
			totrim[length] = 0;
			retval = true;
		}
		if (ISSPACE(totrim[0])) {
			retval = true;
			UChar *current = totrim;
			while (ISSPACE(current[0])) {
				current++;
			}
			size_t num_spaces = ((current-totrim)-1);
			for (unsigned int i=0;i<length;i++) {
				totrim[i] = totrim[i+num_spaces+1];
			}
		}
	}
	return retval;
}

bool ux_packWhitespace(UChar *totrim) {
	bool retval = false;
	unsigned int length = u_strlen(totrim);
	if (totrim && length) {
		UChar *space = 0;
		UChar *current = totrim;
		UChar previous = 0;
		uint32_t num_spaces = 0;
		while (current[0]) {
			if (ISSPACE(current[0]) && !ISSPACE(previous)) {
				current[0] = ' ';
				space = current+1;
				num_spaces = 1;
			}
			else if (!ISSPACE(current[0]) && ISSPACE(previous)) {
				if (num_spaces > 1) {
					num_spaces--;
					retval = true;
					length = u_strlen(current);
					for (unsigned int i=0;i<=length;i++) {
						space[i] = current[i];
					}
					current = space;
				}
			}
			else if (ISSPACE(current[0]) && ISSPACE(previous)) {
				num_spaces++;
			}
			previous = current[0];
			current++;
		}
	}
	return retval;
}

int ux_isSetOp(const UChar *it) {
	int retval = S_IGNORE;
	if (u_strcasecmp(it, stringbits[S_OR], 0) == 0 || u_strcmp(it, stringbits[S_PIPE]) == 0) {
		retval = S_OR;
	}
	else if (u_strcmp(it, stringbits[S_PLUS]) == 0) {
		retval = S_PLUS;
	}
	else if (u_strcmp(it, stringbits[S_MINUS]) == 0) {
		retval = S_MINUS;
	}
	else if (u_strcmp(it, stringbits[S_MULTIPLY]) == 0) {
		retval = S_MULTIPLY;
	}
	else if (u_strcmp(it, stringbits[S_FAILFAST]) == 0) {
		retval = S_FAILFAST;
	}
	else if (u_strcmp(it, stringbits[S_NOT]) == 0) {
		retval = S_NOT;
	}
	return retval;
}

bool ux_unEscape(UChar *target, const UChar *source) {
	bool retval = false;
	uint32_t length = u_strlen(source);
	if (length > 0) {
		uint32_t i=0,j=0;
		for (;i<=length;i++,j++) {
			if (source[i] == '\\') {
				retval = true;
				i++;
			}
			target[j] = source[i];
		}
		target[j+1] = 0;
	}
	return retval;
}

bool ux_escape(UChar *target, const UChar *source) {
	bool retval = false;
	uint32_t length = u_strlen(source);
	if (length > 0) {
		uint32_t i=0,j=0;
		for (;i<=length;i++,j++) {
			if (source[i] == '\\' || source[i] == '(' || source[i] == ')' || source[i] == ';' || source[i] == '#') {
				target[j] = '\\';
				j++;
			}
			target[j] = source[i];
		}
		target[j+1] = 0;
	}
	return retval;
}

// ToDo: ux_append is unsafe and slow...must be a better way
UChar *ux_append(UChar *target, const UChar *data) {
	UChar *tmp = 0;
	if (!target) {
		uint32_t length = u_strlen(data)+1;
		tmp = new UChar[length];
		tmp[0] = 0;
		u_strcat(tmp, data);
		target = tmp;
	}
	else {
		uint32_t length = u_strlen(target)+u_strlen(data)+1;
		tmp = new UChar[length];
		tmp[0] = 0;
		u_strcat(tmp, target);
		u_strcat(tmp, data);
		delete[] target;
		target = tmp;
	}
	return tmp;
}


UChar *ux_append(UChar *target, const UChar data) {
	UChar *tmp = 0;
	UChar *char_tmp = new UChar[2];
	char_tmp[0] = data;
	char_tmp[1] = '\0';

	if (!target) {
		uint32_t length = u_strlen(char_tmp)+1;
		tmp = new UChar[length];
		tmp[0] = 0;
		u_strcat(tmp, char_tmp);
		delete[] target;
		target = tmp;
	}
	else {
		uint32_t length = u_strlen(target)+u_strlen(char_tmp)+1;
		tmp = new UChar[length];
		tmp[0] = 0;
		u_strcat(tmp, target);
		u_strcat(tmp, char_tmp);
		delete[] target;
		target = tmp;
	}
	delete[] char_tmp;
	return tmp;
}

UChar *ux_substr(UChar *string, int start, int end) {
	UChar *tmp = 0;
	int i = 0;

	int len = u_strlen(string);
	
	if (end > len) {
		return tmp;
	}

	for (i = start; i < end; i++) {
		tmp = ux_append(tmp, string[i]);
	}

	return tmp;
}

char *ux_dirname(const char *in) {
	char *tmp = new char[32768];
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
	if (tmp[strlen(tmp)-1] != '/' && tmp[strlen(tmp)-1] != '\\') {
		tmp[strlen(tmp)+1] = 0;
		tmp[strlen(tmp)] = '/';
	}
	return tmp;
}

}
