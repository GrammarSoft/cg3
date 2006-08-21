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
#include "Strings.h"

namespace CG3 {
	namespace Strings {
		UChar *keywords[KEYWORD_COUNT];
		UChar *stringbits[STRINGS_COUNT];
		URegularExpression *regexps[REGEXP_COUNT];

		inline int init_keyword_single(const char *keyword, const unsigned int entry) {
			if (entry >= KEYWORD_COUNT) {
				return -1; // Out of bounds
			}
			UChar buffer[1024];
			u_memset(buffer, 0, 1024);
			u_uastrcpy(buffer, keyword);
			keywords[entry] = new UChar[u_strlen(buffer)+1];
			u_strcpy(keywords[entry], buffer);
			return 0;
		}
		
		int init_keywords() {
			init_keyword_single("1f283fc29adb937a892e09bbc124b85c this is a dummy keyword to hold position 0", K_IGNORE);
			init_keyword_single("SETS",              K_SETS);
			init_keyword_single("LIST",              K_LIST);
			init_keyword_single("SET",               K_SET);
			init_keyword_single("DELIMITERS",        K_DELIMITERS);
			init_keyword_single("PREFERRED-TARGETS", K_PREFERRED_TARGETS);
			init_keyword_single("MAPPINGS",          K_MAPPINGS);
			init_keyword_single("CONSTRAINTS",       K_CONSTRAINTS);
			init_keyword_single("CORRECTIONS",       K_CORRECTIONS);
			init_keyword_single("SECTION",           K_SECTION);
			init_keyword_single("ADD",               K_ADD);
			init_keyword_single("MAP",               K_MAP);
			init_keyword_single("REPLACE",           K_REPLACE);
			init_keyword_single("SELECT",            K_SELECT);
			init_keyword_single("REMOVE",            K_REMOVE);
			init_keyword_single("IFF",               K_IFF);
			init_keyword_single("APPEND",            K_APPEND);
			init_keyword_single("SUBSTITUTE",        K_SUBSTITUTE);
			init_keyword_single("END",               K_END);
			init_keyword_single("ANCHOR",            K_ANCHOR);
			init_keyword_single("EXECUTE",           K_EXECUTE);
			init_keyword_single("JUMP",              K_JUMP);
			init_keyword_single("DEFINEGLOBAL",      K_DEFINEGLOBAL);
			init_keyword_single("REMGLOBAL",         K_REMGLOBAL);
			init_keyword_single("SETGLOBAL",         K_SETGLOBAL);
			init_keyword_single("MINVALUE",          K_MINVALUE);
			init_keyword_single("MAXVALUE",          K_MAXVALUE);
			init_keyword_single("DELIMIT",           K_DELIMIT);
			init_keyword_single("MATCH",             K_MATCH);

			for (int i=0;i<KEYWORD_COUNT;i++) {
				if (!keywords[i]) {
					return -(i+1); // One did not get set properly. Returns -i to pinpoint which.
				}
			}
			return 0;
		}

		int free_keywords() {
			for (int i=0;i<KEYWORD_COUNT;i++) {
				if (keywords[i]) {
					delete keywords[i];
				}
				keywords[i] = 0;
			}
			return 0;
		}

		inline int init_string_single(const char *keyword, const unsigned int entry) {
			if (entry >= STRINGS_COUNT) {
				return -1; // Out of bounds
			}
			UChar buffer[1024];
			u_memset(buffer, 0, 1024);
			u_uastrcpy(buffer, keyword);
			stringbits[entry] = new UChar[u_strlen(buffer)+1];
			u_strcpy(stringbits[entry], buffer);
			return 0;
		}
		
		int init_strings() {
			init_string_single("ADD",        S_ADD);
			init_string_single("OR",         S_OR);
			init_string_single("+",          S_PLUS);
			init_string_single("-",          S_MINUS);
			init_string_single("*",          S_MULTIPLY);
			init_string_single("^",          S_DENY);
			init_string_single("\\",         S_BACKSLASH);
			init_string_single("#",          S_HASH);
			init_string_single("!",          S_NOT);
			init_string_single(" ",          S_SPACE);
			init_string_single(" LINK 0 ",   S_LINKZ);

			for (int i=0;i<STRINGS_COUNT;i++) {
				if (!stringbits[i]) {
					return -(i+1); // One did not get set properly. Returns -i to pinpoint which.
				}
			}
			return 0;
		}

		int free_strings() {
			for (int i=0;i<STRINGS_COUNT;i++) {
				if (stringbits[i]) {
					delete stringbits[i];
				}
				stringbits[i] = 0;
			}
			return 0;
		}

		int init_regexps() {
			UParseError *pe = new UParseError;
			UErrorCode status = U_ZERO_ERROR;

			memset(pe, 0, sizeof(UParseError));
			status = U_ZERO_ERROR;
			regexps[R_PACKSPACE] = uregex_openC("\\s+\0", 0, pe, &status);

			memset(pe, 0, sizeof(UParseError));
			status = U_ZERO_ERROR;
			regexps[R_CLEANSTRING] = uregex_openC("\\s+(TARGET|IF)\\s+\0", 0, pe, &status);

			memset(pe, 0, sizeof(UParseError));
			status = U_ZERO_ERROR;
			regexps[R_ANDLINK] = uregex_openC("\\s+AND\\s+\0", 0, pe, &status);

			delete pe;
			return 0;
		}

		int free_regexps() {
			for(int i=0;i<REGEXP_COUNT;i++) {
				if (regexps[i]) {
					uregex_close(regexps[i]);
				}
				regexps[i] = 0;
			}
			return 0;
		}
	}
}
