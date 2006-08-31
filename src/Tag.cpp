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
#include <unicode/ustring.h>
#include "Tag.h"

namespace CG3 {

	Tag::Tag() {
		negative = false;
		denied = false;
		case_insensitive = false;
		regexp = false;
		wildcard = false;
		wordform = false;
		baseform = false;
		numerical = false;
		comparison_key = 0;
		comparison_op = OP_NOP;
		comparison_val = 0;
		tag = 0;
		raw = 0;
	}
	
	Tag::~Tag() {
		if (tag) {
			delete tag;
		}
		if (raw) {
			delete raw;
		}
		if (comparison_key) {
			delete comparison_key;
		}
	}

	void Tag::parseTag(const UChar *to) {
		if (to && u_strlen(to)) {
			const UChar *tmp = to;
			while (tmp[0] && (tmp[0] == '!' || tmp[0] == '^')) {
				if (tmp[0] == '!') {
					negative = true;
					tmp++;
				}
				if (tmp[0] == '^') {
					denied = true;
					tmp++;
				}
			}
			uint32_t length = u_strlen(tmp);
			while (tmp[0] && (tmp[0] == '"' || tmp[0] == '<') && (tmp[length-1] == 'i' || tmp[length-1] == 'w' || tmp[length-1] == 'r')) {
				if (tmp[length-1] == 'r') {
					regexp = true;
					length--;
				}
				if (tmp[length-1] == 'i') {
					case_insensitive = true;
					length--;
				}
				if (tmp[length-1] == 'w') {
					wildcard = true;
					length--;
				}
			}

			if (tmp[0] == '"' && tmp[length-1] == '"') {
				if (tmp[1] == '<' && tmp[length-2] == '>') {
					wordform = true;
				}
				else {
					baseform = true;
				}
			}
			
			tag = new UChar[length+1];
			tag[length] = 0;
			u_strncpy(tag, tmp, length);
			raw = new UChar[u_strlen(to)+1];
			u_strcpy(raw, to);
		}
	}

}
