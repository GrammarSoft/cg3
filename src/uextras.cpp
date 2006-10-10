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

bool ux_trim(UChar *totrim) {
	bool retval = false;
	int length = u_strlen(totrim);
	if (totrim && length) {
		while(u_isWhitespace(totrim[length-1])) {
			length--;
		}
		if (u_isWhitespace(totrim[length])) {
			totrim[length] = 0;
			retval = true;
		}
		if (u_isWhitespace(totrim[0])) {
			retval = true;
			UChar *current = totrim;
			while(u_isWhitespace(current[0])) {
				current++;
			}
			uint32_t num_spaces = (uint32_t)((current-totrim)-1);
			for (int i=0;i<length;i++) {
				totrim[i] = totrim[i+num_spaces+1];
			}
		}
	}
	return retval;
}

bool ux_packWhitespace(UChar *totrim) {
	bool retval = false;
	int length = u_strlen(totrim);
	if (totrim && length) {
		UChar *space = 0;
		UChar *current = totrim;
		UChar previous = 0;
		uint32_t num_spaces = 0;
		while (current[0]) {
			if (u_isWhitespace(current[0]) && !u_isWhitespace(previous)) {
				current[0] = ' ';
				space = current+1;
				num_spaces = 1;
			}
			else if (!u_isWhitespace(current[0]) && u_isWhitespace(previous)) {
				if (num_spaces > 1) {
					num_spaces--;
					retval = true;
					length = u_strlen(current);
					for (int i=0;i<=length;i++) {
						space[i] = current[i];
					}
					current = space;
				}
			}
			else if (u_isWhitespace(current[0]) && u_isWhitespace(previous)) {
				num_spaces++;
			}
			previous = current[0];
			current++;
		}
	}
	return retval;
}

bool ux_cutComments(UChar *line, const UChar comment, bool compatible) {
	bool retval = false;
	UChar *offset_hash = line;
	while((offset_hash = u_strchr(offset_hash, comment)) != 0) {
		if (offset_hash == line || offset_hash[1] == 0 || ux_isNewline(offset_hash[1], offset_hash[0])) {
			offset_hash[0] = 0;
			retval = true;
			break;
		}
		else if (!compatible && u_isgraph(offset_hash[-1])) {
			offset_hash++;
			continue;
		}
		else if (compatible && offset_hash[-1] == '\\') {
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
	if (u_strcmp(it, stringbits[S_OR]) == 0 || u_strcmp(it, stringbits[S_PIPE]) == 0) {
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

bool ux_findMatchingParenthesis(const UChar *structure, int pos, int *result) {
	uint32_t len = (uint32_t)u_strlen(structure);
	uint32_t paren = 0;
	for (uint32_t i=pos;i<len;i++) {
		if (structure[i] == ')' && structure[i-1] != '\\') {
			paren--;
			if (paren == 0) {
				*result = i;
				return true;
			}
		}
		if (structure[i] == '(' && structure[i-1] != '\\') {
			paren++;
		}
	}
	return false;
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
			if (source[i] == '\\' || source[i] == '(' || source[i] == ')') {
				target[j] = '\\';
				j++;
			}
			target[j] = source[i];
		}
		target[j+1] = 0;
	}
	return retval;
}

uint32_t ux_fputuchar(FILE *output, UChar *txt) {
	uint32_t retval = 0;
	uint32_t length = u_strlen(txt);
	for (uint32_t i=0;i<length;i++) {
		UChar c = (UChar)htons(txt[i]);
		fwrite(&c, sizeof(UChar), 1, output);
		retval++;
	}
	return retval;
}

UChar *ux_append(UChar *target, const UChar *data) {
	UChar *tmp = 0;
	if (!target) {
		uint32_t length = u_strlen(data)+1;
		tmp = new UChar[length];
		tmp[0] = 0;
		u_strcat(tmp, data);
		target = tmp;
	} else {
		uint32_t length = u_strlen(target)+u_strlen(data)+1;
		tmp = new UChar[length];
		tmp[0] = 0;
		u_strcat(tmp, target);
		u_strcat(tmp, data);
		delete target;
		target = tmp;
	}
	return tmp;
}
