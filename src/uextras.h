/*
* Copyright (C) 2007-2012, GrammarSoft ApS
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

#pragma once
#ifndef c6d28b7452ec699b_UEXTRAS_H
#define c6d28b7452ec699b_UEXTRAS_H

#include "stdafx.h"
#include "Strings.h"

namespace CG3 {

bool ux_isEmpty(const UChar *text);

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

UChar *ux_substr(const UChar *string, const size_t start, const size_t end);

std::string ux_dirname(const char *in);

bool ux_simplecasecmp(const UChar *a, const UChar *b, const size_t n);

}

#endif
