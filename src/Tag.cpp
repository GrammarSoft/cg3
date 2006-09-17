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
#include <unicode/ustring.h>
#include "Strings.h"
#include "Tag.h"
#include "uextras.h"

using namespace CG3;

Tag::Tag() {
	features = 0;
	type = 0;
	comparison_key = 0;
	comparison_op = OP_NOP;
	comparison_val = 0;
	tag = 0;
}

Tag::~Tag() {
	if (tag) {
		delete tag;
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
				features |= F_NEGATIVE;
				tmp++;
			}
			if (tmp[0] == '^') {
				features |= F_FAILFAST;
				tmp++;
			}
		}
		uint32_t length = u_strlen(tmp);
		while (tmp[0] && (tmp[0] == '"' || tmp[0] == '<') && (tmp[length-1] == 'i' || tmp[length-1] == 'w' || tmp[length-1] == 'r')) {
			if (tmp[length-1] == 'r') {
				features |= F_REGEXP;
				length--;
			}
			if (tmp[length-1] == 'i') {
				features |= F_CASE_INSENSITIVE;
				length--;
			}
			if (tmp[length-1] == 'w') {
				features |= F_WILDCARD;
				length--;
			}
		}

		if (tmp[0] == '"' && tmp[length-1] == '"') {
			if (tmp[1] == '<' && tmp[length-2] == '>') {
				type |= T_WORDFORM;
			}
			else {
				type |= T_BASEFORM;
			}
		}
		
		tag = new UChar[length+1];
		tag[length] = 0;
		u_strncpy(tag, tmp, length);
		UChar *utag = new UChar[u_strlen(tag)+3];
		ux_unEscape(utag, tag);
		delete tag;
		tag = utag;
		utag = 0;

		if (u_strcmp(tag, stringbits[S_ASTERIK]) == 0) {
			type |= T_ANY;
		}
		if (tag[0] == '@') {
			type |= T_MAPPING;
		}
	}
}

void Tag::print(UFILE *to) {
	if (features & F_NEGATIVE) {
		u_fprintf(to, "!");
	}
	if (features & F_FAILFAST) {
		u_fprintf(to, "^");
	}
	if (type & T_META) {
		u_fprintf(to, "META:");
	}
	if (type & T_VARIABLE) {
		u_fprintf(to, "VAR:");
	}

	UChar *tmp = new UChar[u_strlen(tag)*2+3];
	ux_escape(tmp, tag);
	u_fprintf(to, "%S", tmp);
	delete tmp;

	if (features & F_CASE_INSENSITIVE) {
		u_fprintf(to, "i");
	}
	if (features & F_REGEXP) {
		u_fprintf(to, "r");
	}
	if (features & F_WILDCARD) {
		u_fprintf(to, "w");
	}
}

uint32_t Tag::rehash() {
	hash = 0;
	if (features & F_NEGATIVE) {
		hash = hash_sdbm_char("!", hash);
	}
	if (features & F_FAILFAST) {
		hash = hash_sdbm_char("^", hash);
	}
	if (type & T_META) {
		hash = hash_sdbm_char("META:", hash);
	}
	if (type & T_VARIABLE) {
		hash = hash_sdbm_char("VAR:", hash);
	}

	UChar *tmp = new UChar[u_strlen(tag)*2+3];
	ux_escape(tmp, tag);
	hash = hash_sdbm_uchar(tmp, hash);
	delete tmp;

	if (features & F_CASE_INSENSITIVE) {
		hash = hash_sdbm_char("i", hash);
	}
	if (features & F_REGEXP) {
		hash = hash_sdbm_char("r", hash);
	}
	if (features & F_WILDCARD) {
		hash = hash_sdbm_char("w", hash);
	}
	return hash;
}

void Tag::duplicateTag(const Tag *from) {
	features = from->features;
	type = from->type;
	hash = from->hash;
	comparison_op = from->comparison_op;
	comparison_val = from->comparison_val;

	if (from->comparison_key) {
		comparison_key = new UChar[u_strlen(from->comparison_key)+1];
		u_strcpy(comparison_key, from->comparison_key);
	}

	if (from->tag) {
		tag = new UChar[u_strlen(from->tag)+1];
		u_strcpy(tag, from->tag);
	}
}
