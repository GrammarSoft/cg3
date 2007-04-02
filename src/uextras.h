/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */

#ifndef __UEXTRAS_H
#define __UEXTRAS_H

#include "stdafx.h"
#include "Strings.h"

bool ux_isNewline(const UChar32 current, const UChar32 previous);
bool ux_trim(UChar *totrim);
bool ux_packWhitespace(UChar *totrim);
bool ux_cutComments(UChar *line, const UChar comment, bool ruthless);
int ux_isSetOp(const UChar *it);
bool ux_findMatchingParenthesis(const UChar *structure, int pos, int *result);

bool ux_escape(UChar *target, const UChar *source);
bool ux_unEscape(UChar *target, const UChar *source);

uint32_t ux_fputuchar(FILE *output, UChar *txt);
UChar *ux_append(UChar *target, const UChar *data);

#endif
