/*
* Copyright (C) 2007-2017, GrammarSoft ApS
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
