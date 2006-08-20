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
#include "uextras.h"

bool ux_isNewline(const UChar32 current, const UChar32 previous) {
	return (current == 0x0D0A // ASCII \r\n
	|| current == 0x2028 // Unicode Line Seperator
	|| current == 0x2029 // Unicode Paragraph Seperator
	|| current == 0x0085 // EBCDIC NEL
	|| current == 0x000C // Form Feed
	|| current == 0x000A // ASCII \n
	|| previous == 0x000D); // ASCII \r
}

bool ux_trimUChar(UChar *totrim) {
	bool retval = false;
	int length = u_strlen(totrim);
	if (totrim && length) {
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
		length = u_strlen(totrim);
		while(u_isWhitespace(totrim[length-1])) {
			totrim[length-1] = 0;
			length--;
			retval = true;
		}
	}
	return retval;
}

bool ux_cutComments(UChar *line, const UChar comment) {
	bool retval = false;
	UChar *offset_hash = line;
	UChar *offset_escape = line;
	while(offset_hash) {
		offset_escape = u_strchr(offset_hash, '\\');
		offset_hash = u_strchr(offset_hash, comment);
		if (offset_hash) {
			if (!offset_escape || offset_escape != offset_hash-1) {
				offset_hash[0] = 0;
				offset_hash = 0;
				retval = true;
			} else {
				offset_hash++;
			}
		}
	}
	return retval;
}