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

#include "stdafx.h"
#include <unicode/uchar.h>
#include "Strings.h"
#include "uextras.h"

using namespace CG3::Strings;

bool ux_isNewline(const UChar32 current, const UChar32 previous) {
	return (current == 0x0D0A // ASCII \r\n
	|| current == 0x2028 // Unicode Line Seperator
	|| current == 0x2029 // Unicode Paragraph Seperator
	|| current == 0x0085 // EBCDIC NEL
	|| current == 0x000C // Form Feed
	|| current == 0x000A // ASCII \n
	|| previous == 0x000D); // ASCII \r
}

// ToDo: Make this faster by trimming all whitespace in a single iteration.
bool ux_trimUChar(UChar *totrim) {
	bool retval = false;
	int length = u_strlen(totrim);
	if (totrim && length) {
		while(u_isWhitespace(totrim[length-1])) {
			totrim[length-1] = 0;
			length--;
			retval = true;
		}
		while(u_isWhitespace(totrim[0])) {
			for (int i=0;i<length;i++) {
				if (totrim[i]) {
					totrim[i] = totrim[i+1];
					retval = true;
				} else {
					break;
				}
			}
		}
	}
	return retval;
}

bool ux_cutComments(UChar *line, const UChar comment) {
	bool retval = false;
	UChar *offset_hash = line;
	while(offset_hash = u_strchr(offset_hash, comment)) {
		if (offset_hash == line || offset_hash[1] == 0 || ux_isNewline(offset_hash[1], offset_hash[0])) {
			offset_hash[0] = 0;
			retval = true;
			break;
		}
		else if (u_isgraph(offset_hash[-1])) {
			offset_hash++;
			continue;
		}
		else {
			offset_hash[0] = 0;
			retval = true;
			break;
		}
	}
	return retval;
}

int ux_isSetOp(const UChar *it) {
	int retval = S_IGNORE;
	if (u_strcmp(it, stringbits[S_OR]) == 0) {
		retval = S_OR;
	} else if (u_strcmp(it, stringbits[S_PLUS]) == 0) {
		retval = S_PLUS;
	} else if (u_strcmp(it, stringbits[S_MINUS]) == 0) {
		retval = S_MINUS;
	} else if (u_strcmp(it, stringbits[S_MULTIPLY]) == 0) {
		retval = S_MULTIPLY;
	} else if (u_strcmp(it, stringbits[S_DENY]) == 0) {
		retval = S_DENY;
	} else if (u_strcmp(it, stringbits[S_NOT]) == 0) {
		retval = S_NOT;
	}
	return retval;
}
