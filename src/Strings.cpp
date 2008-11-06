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

#include "Strings.h"

namespace CG3 {
	namespace Strings {
		UChar *keywords[KEYWORD_COUNT];
		uint32_t keyword_lengths[KEYWORD_COUNT];
		UChar *stringbits[STRINGS_COUNT];
		uint32_t stringbit_lengths[STRINGS_COUNT];
		UChar *flags[FLAGS_COUNT];
		uint32_t flag_lengths[FLAGS_COUNT];

		inline int init_keyword_single(const char *keyword, const uint32_t entry) {
			if (entry >= KEYWORD_COUNT) {
				CG3Quit(1); // Out of bounds
			}
			UChar *buffer = gbuffers[0];
			u_memset(buffer, 0, 1024);
			u_uastrcpy(buffer, keyword);
			keywords[entry] = new UChar[u_strlen(buffer)+1];
			u_strcpy(keywords[entry], buffer);
			keyword_lengths[entry] = u_strlen(keywords[entry]);
			return 0;
		}
		
		int init_keywords() {
			free_keywords();
			init_keyword_single("1f283fc29adb937a892e09bbc124b85c this is a dummy keyword to hold position 0", K_IGNORE);
			init_keyword_single("SETS",              K_SETS);
			init_keyword_single("LIST",              K_LIST);
			init_keyword_single("SET",               K_SET);
			init_keyword_single("DELIMITERS",        K_DELIMITERS);
			init_keyword_single("SOFT-DELIMITERS",   K_SOFT_DELIMITERS);
			init_keyword_single("PREFERRED-TARGETS", K_PREFERRED_TARGETS);
			init_keyword_single("MAPPING-PREFIX",    K_MAPPING_PREFIX);
			init_keyword_single("MAPPINGS",          K_MAPPINGS);
			init_keyword_single("CONSTRAINTS",       K_CONSTRAINTS);
			init_keyword_single("CORRECTIONS",       K_CORRECTIONS);
			init_keyword_single("SECTION",           K_SECTION);
			init_keyword_single("BEFORE-SECTIONS",   K_BEFORE_SECTIONS);
			init_keyword_single("AFTER-SECTIONS",    K_AFTER_SECTIONS);
			init_keyword_single("NULL-SECTION",      K_NULL_SECTION);
			init_keyword_single("ADD",               K_ADD);
			init_keyword_single("MAP",               K_MAP);
			init_keyword_single("REPLACE",           K_REPLACE);
			init_keyword_single("SELECT",            K_SELECT);
			init_keyword_single("REMOVE",            K_REMOVE);
			init_keyword_single("IFF",               K_IFF);
			init_keyword_single("APPEND",            K_APPEND);
			init_keyword_single("SUBSTITUTE",        K_SUBSTITUTE);
			init_keyword_single("START",             K_START);
			init_keyword_single("END",               K_END);
			init_keyword_single("ANCHOR",            K_ANCHOR);
			init_keyword_single("EXECUTE",           K_EXECUTE);
			init_keyword_single("JUMP",              K_JUMP);
			init_keyword_single("REMVARIABLE",       K_REMVARIABLE);
			init_keyword_single("SETVARIABLE",       K_SETVARIABLE);
			init_keyword_single("DELIMIT",           K_DELIMIT);
			init_keyword_single("MATCH",             K_MATCH);
			init_keyword_single("SETPARENT",         K_SETPARENT);
			init_keyword_single("SETCHILD",          K_SETCHILD);
			init_keyword_single("SETRELATION",       K_SETRELATION);
			init_keyword_single("REMRELATION",       K_REMRELATION);
			init_keyword_single("SETRELATIONS",      K_SETRELATIONS);
			init_keyword_single("REMRELATIONS",      K_REMRELATIONS);
			init_keyword_single("TEMPLATE",          K_TEMPLATE);
			init_keyword_single("MOVE",              K_MOVE);
			init_keyword_single("MOVE-AFTER",        K_MOVE_AFTER);
			init_keyword_single("MOVE-BEFORE",       K_MOVE_BEFORE);
			init_keyword_single("SWITCH",            K_SWITCH);

			for (unsigned int i=0;i<KEYWORD_COUNT;i++) {
				if (!keywords[i]) {
					return i; // One did not get set properly. Returns i to pinpoint which.
				}
			}
			return 0;
		}

		int free_keywords() {
			for (unsigned int i=0;i<KEYWORD_COUNT;i++) {
				if (keywords[i]) {
					delete[] keywords[i];
				}
				keywords[i] = 0;
			}
			return 0;
		}

		inline int init_string_single(const char *keyword, const uint32_t entry) {
			if (entry >= STRINGS_COUNT) {
				CG3Quit(1); // Out of bounds
			}
			UChar *buffer = gbuffers[0];
			u_memset(buffer, 0, 1024);
			u_uastrcpy(buffer, keyword);
			stringbits[entry] = new UChar[u_strlen(buffer)+1];
			u_strcpy(stringbits[entry], buffer);
			stringbit_lengths[entry] = u_strlen(stringbits[entry]);
			return 0;
		}
		
		int init_strings() {
			free_strings();
			init_string_single("1f283fc29adb937a892e09bbc124b85c this is a dummy string to hold position 0", S_IGNORE);
			init_string_single("|",          S_PIPE);
			init_string_single("TO",         S_TO);
			init_string_single("OR",         S_OR);
			init_string_single("+",          S_PLUS);
			init_string_single("-",          S_MINUS);
			init_string_single("*",          S_MULTIPLY);
			init_string_single("^",          S_FAILFAST);
			init_string_single("\\",         S_BACKSLASH);
			init_string_single("#",          S_HASH);
			init_string_single("!",          S_NOT);
			init_string_single("NOT",        S_TEXTNOT);
			init_string_single("NEGATE",     S_TEXTNEGATE);
			init_string_single(" ",          S_SPACE);
			init_string_single("LINK",       S_LINK);
			init_string_single(" LINK 0 ",   S_LINKZ);
			init_string_single("BARRIER",    S_BARRIER);
			init_string_single("CBARRIER",   S_CBARRIER);
			init_string_single("*",          S_ASTERIK);
			init_string_single("**",         S_ASTERIKTWO);
			init_string_single(">>>",        S_BEGINTAG);
			init_string_single("<<<",        S_ENDTAG);
			init_string_single("_S_DELIMITERS_", S_DELIMITSET);
			init_string_single("_S_SOFT_DELIMITERS_", S_SOFTDELIMITSET);
			init_string_single("CGCMD:FLUSH",  S_CMD_FLUSH);
			init_string_single("CGCMD:EXIT",   S_CMD_EXIT);
			init_string_single("CGCMD:IGNORE", S_CMD_IGNORE);
			init_string_single("CGCMD:RESUME", S_CMD_RESUME);
			init_string_single("TARGET",       S_TARGET);
			init_string_single("AND",          S_AND);
			init_string_single("IF",           S_IF);
			init_string_single("_LEFT_",       S_UU_LEFT);
			init_string_single("_RIGHT_",      S_UU_RIGHT);
			init_string_single("_PAREN_",      S_UU_PAREN);
			init_string_single("<.*>",         S_RXTEXT_ANY);
			init_string_single("\".*\"",       S_RXBASE_ANY);
			init_string_single("\"<.*>\"",     S_RXWORD_ANY);
			init_string_single("AFTER",        S_AFTER);
			init_string_single("BEFORE",       S_BEFORE);
			init_string_single("WITH",         S_WITH);

			for (unsigned int i=0;i<STRINGS_COUNT;i++) {
				if (!stringbits[i]) {
					return i; // One did not get set properly. Returns i to pinpoint which.
				}
			}
			return 0;
		}

		int free_strings() {
			for (unsigned int i=0;i<STRINGS_COUNT;i++) {
				if (stringbits[i]) {
					delete[] stringbits[i];
				}
				stringbits[i] = 0;
			}
			return 0;
		}

		inline int init_flag_single(const char *keyword, const uint32_t entry) {
			if (entry >= FLAGS_COUNT) {
				CG3Quit(1); // Out of bounds
			}
			UChar *buffer = gbuffers[0];
			u_memset(buffer, 0, 1024);
			u_uastrcpy(buffer, keyword);
			flags[entry] = new UChar[u_strlen(buffer)+1];
			u_strcpy(flags[entry], buffer);
			flag_lengths[entry] = u_strlen(flags[entry]);
			return 0;
		}

		int init_flags() {
			free_flags();
			init_flag_single("NEAREST",       FL_NEAREST);
			init_flag_single("ALLOWLOOP",     FL_ALLOWLOOP);
			init_flag_single("DELAYED",       FL_DELAYED);
			init_flag_single("IMMEDIATE",     FL_IMMEDIATE);
			init_flag_single("LOOKDELETED",   FL_LOOKDELETED);
			init_flag_single("LOOKDELAYED",   FL_LOOKDELAYED);
			init_flag_single("UNSAFE",        FL_UNSAFE);
			init_flag_single("SAFE",          FL_SAFE);

			for (unsigned int i=0;i<FLAGS_COUNT;i++) {
				if (!flags[i]) {
					return i; // One did not get set properly. Returns i to pinpoint which.
				}
			}
			return 0;
		}

		int free_flags() {
			for (unsigned int i=0;i<FLAGS_COUNT;i++) {
				if (flags[i]) {
					delete[] flags[i];
				}
				flags[i] = 0;
			}
			return 0;
		}

		UChar *gbuffers[NUM_GBUFFERS];
		char *cbuffers[NUM_CBUFFERS];

		int init_gbuffers() {
			for (uint32_t i=0;i<NUM_GBUFFERS;i++) {
				gbuffers[i] = new UChar[BUFFER_SIZE];
			}
			for (uint32_t i=0;i<NUM_CBUFFERS;i++) {
				cbuffers[i] = new char[BUFFER_SIZE];
			}
			return 0;
		}

		int free_gbuffers() {
			for (uint32_t i=0;i<NUM_GBUFFERS;i++) {
				delete[] gbuffers[i];
				gbuffers[i] = 0;
			}
			for (uint32_t i=0;i<NUM_CBUFFERS;i++) {
				delete[] cbuffers[i];
				cbuffers[i] = 0;
			}
			return 0;
		}
	}
}
