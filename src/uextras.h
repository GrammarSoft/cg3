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

#pragma once
#ifndef __UEXTRAS_H
#define __UEXTRAS_H

#include "stdafx.h"

bool ux_isEmpty(const UChar *text);

bool ux_trim(UChar *totrim);
bool ux_packWhitespace(UChar *totrim);
int ux_isSetOp(const UChar *it);

bool ux_escape(UChar *target, const UChar *source);
bool ux_unEscape(UChar *target, const UChar *source);

UChar *ux_append(UChar *target, const UChar *data);
UChar *ux_append(UChar *target, const UChar data);

UChar *ux_substr(UChar *string, int start, int end);

char *ux_dirname(const char *in);

#endif
