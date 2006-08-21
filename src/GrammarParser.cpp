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
#include <unicode/uregex.h>
#include "GrammarParser.h"
#include "Grammar.h"
#include "uextras.h"

namespace CG3 {
	namespace GrammarParser {
		bool findMatchingParenthesis(const UChar *structure, int pos, int *result) {
			int len = u_strlen(structure);
			while (pos < len) {
				pos++;
				if (structure[pos] == ')' && structure[pos-1] != '\\') {
					*result = pos;
					return true;
				}
				if (structure[pos] == '(' && structure[pos-1] != '\\') {
					int tmp = 0;
					findMatchingParenthesis(structure, pos, &tmp);
					pos = tmp;
				}
			}
			return false;
		}

		int parseList(const UChar *line, const unsigned int which, CG3::Grammar *result) {
			UErrorCode *status = new UErrorCode;
			int length = u_strlen(line);

			uregex_setText(Strings::regexps[Strings::R_PACKSPACE], line, length, status);

			UChar *local = new UChar[length+1];
			u_strcpy(local, line);

			delete local;
			delete status;
			return 0;
		}

		int parseSingleLine(const int key, const UChar *line, const unsigned int which, CG3::Grammar *result) {
			if (key <= Strings::K_IGNORE || key >= Strings::KEYWORD_COUNT) {
				std::cerr << "Error: Invalid keyword " << key << " - skipping." << std::endl;
				return -1;
			}

			UErrorCode *status = new UErrorCode;
			int length = u_strlen(line);
			UChar *local = new UChar[int(length*1.5)];

			uregex_reset(Strings::regexps[Strings::R_PACKSPACE], 0, status);
			uregex_setText(Strings::regexps[Strings::R_PACKSPACE], line, length, status);
			uregex_replaceAll(Strings::regexps[Strings::R_PACKSPACE], Strings::stringbits[Strings::S_SPACE], 1, local, int(length*1.5), status);

			switch(key) {
				case Strings::K_LIST:
					parseList(local, which, result);
					break;
				default:
					break;
			}

			delete local;
			delete status;
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
			
			int error = Strings::init_keywords();
			if (error) {
				std::cerr << "init_keywords returned " << error << std::endl;
				return error;
			}

			error = Strings::init_regexps();
			if (error) {
				std::cerr << "init_regexps returned " << error << std::endl;
				return error;
			}

			error = Strings::init_strings();
			if (error) {
				std::cerr << "init_strings returned " << error << std::endl;
				return error;
			}

			// ToDo: Make this dynamic.
			#define BUFFER_SIZE (65536)
			std::map<unsigned int, UChar*> lines;
			std::map<unsigned int, unsigned int> keys;
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
					int keyword = 0;
					for (int i=1;i<Strings::KEYWORD_COUNT;i++) {
						UChar *pos = 0;
						int length = 0;
						if (pos = u_strstr(line, Strings::keywords[i])) {
							length = u_strlen(Strings::keywords[i]);
							if (
								((pos == line) || (pos > line && u_isWhitespace(pos[-1])))
								&& (pos[length] == ':' || u_isWhitespace(pos[length]))
								) {
								lastcmd = result->lines;
								keyword = i;
								break;
							}
						}
					}
					if (keyword || !lines[lastcmd]) {
						while (!lines.empty()) {
							unsigned int which = (*lines.begin()).first;
							UChar *line = (*lines.begin()).second;
							parseSingleLine(keys[which], line, which, result);
							delete line;
							lines.erase(lines.begin());
							keys.erase(which);
						}
						lines[result->lines] = line;
						keys[result->lines] = keyword;
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

			while (!lines.empty()) {
				unsigned int which = (*lines.begin()).first;
				UChar *line = (*lines.begin()).second;
				parseSingleLine(keys[which], line, which, result);
				delete line;
				lines.erase(lines.begin());
				keys.erase(which);
			}

			Strings::free_keywords();
			Strings::free_regexps();
			Strings::free_strings();
			lines.clear();
			keys.clear();

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
