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
#include <unicode/uregex.h>
#include "GrammarParser.h"
#include "Grammar.h"
#include "uextras.h"

namespace CG3 {
	namespace GrammarParser {
		enum KEYWORDS {
			K_IGNORE,
			K_SETS,
			K_LIST,
			K_SET,
			K_DELIMITERS,
			K_PREFERRED_TARGETS,
			K_MAPPINGS,
			K_CONSTRAINTS,
			K_CORRECTIONS,
			K_SECTION,
			K_ADD,
			K_MAP,
			K_REPLACE,
			K_SELECT,
			K_REMOVE,
			K_IFF,
			K_APPEND,
			K_SUBSTITUTE,
			K_END,
			K_ANCHOR,
			K_EXECUTE,
			K_JUMP,
			K_DEFINEGLOBAL,
			K_REMGLOBAL,
			K_SETGLOBAL,
			K_MINVALUE,
			K_MAXVALUE,
			K_DELIMIT,
			K_MATCH,
			KEYWORD_COUNT
		};
		UChar *keywords[KEYWORD_COUNT];

/* These are not really needed.
		U_STRING_DECL(S_ADD, "ADD", 3);      U_STRING_INIT(S_ADD, "ADD", 3);
		U_STRING_DECL(S_OR, "OR", 2);        U_STRING_INIT(S_OR, "OR", 2);
		U_STRING_DECL(S_PLUS, "+", 1);       U_STRING_INIT(S_PLUS, "+", 1);
		U_STRING_DECL(S_MINUS, "-", 1);      U_STRING_INIT(S_MINUS, "-", 1);
		U_STRING_DECL(S_MULTIPLY, "*", 1);   U_STRING_INIT(S_MULTIPLY, "*", 1);
		U_STRING_DECL(S_DENY, "^", 1);       U_STRING_INIT(S_DENY, "^", 1);
		U_STRING_DECL(S_BACKSLASH, "\\", 1); U_STRING_INIT(S_BACKSLASH, "\\", 1);
		U_STRING_DECL(S_HASH, "#", 1);       U_STRING_INIT(S_HASH, "#", 1);
//*/

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
			init_keyword_single("IGNORE",            K_IGNORE);
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

		int parseSingleLine(const UChar *line, CG3::Grammar *result) {
			return 0;
		}
		
		int parse_grammar_from_ufile(UFILE *input, CG3::Grammar *result) {
			u_frewind(input);
			if (u_feof(input)) {
				std::cerr << "Input is null - nothing to parse!" << std::endl;
				return -1;
			}
			if (!result) {
				std::cerr << "No preallocated grammar provided - cannot continue!" << std::endl;
				return -1;
			}
			
			int error = init_keywords();
			if (error) {
				std::cerr << "init_keywords returned " << error << std::endl;
				return error;
			}

			// ToDo: Make this dynamic.
			#define BUFFER_SIZE (6*1024)
			UErrorCode *uerror = new UErrorCode;
			URegularExpression* regex_stripcomments = uregex_openC("a+", 0, NULL, uerror);

			std::map<unsigned int, UChar*> lines;
			unsigned int lastcmd = 0;
			while (!u_feof(input)) {
				result->lines++;
				UChar *line = new UChar[BUFFER_SIZE];
				u_fgets(line, BUFFER_SIZE-1, input);
				ux_cutComments(line, '#');
				ux_cutComments(line, ';');

				int length = u_strlen(line);
				bool notnull = false;
				for (int i=0;i<length;i++) {
					if (!u_isWhitespace(line[i])) {
						notnull = true;
						break;
					}
				}
				if (notnull) {
					ux_trimUChar(line);
					bool keyword = false;
					for (int i=1;i<KEYWORD_COUNT;i++) {
						if (u_strstr(line, keywords[i])) {
							lastcmd = result->lines;
							keyword = true;
							break;
						}
					}
					if (keyword || !lines[lastcmd]) {
						lines[result->lines] = line;
					} else {
						length = u_strlen(lines[lastcmd]);
						lines[lastcmd][length] = ' ';
						lines[lastcmd][length+1] = 0;
						u_strcat(lines[lastcmd], line);
					}
				} else {
					delete line;
				}
			}

			std::map<unsigned int, UChar*>::iterator lines_iter;
			int _min=BUFFER_SIZE, _max=0, _num=0;
			double _avg=0.0;
			for (lines_iter = lines.begin() ; lines_iter != lines.end() ; lines_iter++) {
				unsigned int which = lines_iter->first;
				UChar *line = lines_iter->second;
				int length = u_strlen(line);
				_min = _min < length ? _min : length;
				_max = _max > length ? _max : length;
				_num++;
				_avg += length;
				//std::wcout << line << std::endl;
			}
			_avg = _avg / (double)_num;

			free_keywords();
			return 0; // No errors.
		}
		
		int parse_grammar_from_file(const char *filename, const char *codepage, CG3::Grammar *result) {
			UFILE *grammar = u_fopen(filename, "r", NULL, codepage);
			if (!grammar) {
				return -1; // File could not be opened for reading.
			}
			
			int error = parse_grammar_from_ufile(grammar, result);
			if (error || !result) {
				return error;
			}
			result->name = new UChar[strlen(filename)+1];
			u_uastrcpy(result->name, filename);
			return 0;
		}

		int parse_grammar_from_buffer(const UChar *buffer, const char *codepage, CG3::Grammar *result) {
			return -1; // Not implemented.
		}
	}
}
