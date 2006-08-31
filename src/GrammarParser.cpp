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

using namespace CG3::Strings;

namespace CG3 {
	namespace GrammarParser {
		int parseSingleLine(const int key, const UChar *line, CG3::Grammar *result) {
			if (!line || !u_strlen(line)) {
				u_fprintf(ux_stderr, "Warning: Line %u is empty - skipping.\n", result->curline);
				return -1;
			}
			if (key <= K_IGNORE || key >= KEYWORD_COUNT) {
				u_fprintf(ux_stderr, "Warning: Invalid keyword %u on line %u - skipping.\n", key, result->curline);
				return -1;
			}

			UErrorCode status = U_ZERO_ERROR;
			int length = u_strlen(line);
			UChar *local = new UChar[length+1];
			//memset(local, 0, length);

			status = U_ZERO_ERROR;
			uregex_setText(regexps[R_CLEANSTRING], line, length, &status);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_setText returned %s - cannot continue!\n", u_errorName(status));
				return -1;
			}
			status = U_ZERO_ERROR;
			uregex_replaceAll(regexps[R_CLEANSTRING], stringbits[S_SPACE], u_strlen(stringbits[S_SPACE]), local, length+1, &status);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_replaceAll returned %s - cannot continue!\n", u_errorName(status));
				return -1;
			}

			length = u_strlen(local);
			UChar *newlocal = new UChar[length+1];

			status = U_ZERO_ERROR;
			uregex_setText(regexps[R_ANDLINK], local, length, &status);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_setText returned %s - cannot continue!\n", u_errorName(status));
				return -1;
			}
			status = U_ZERO_ERROR;
			uregex_replaceAll(regexps[R_ANDLINK], stringbits[S_LINKZ], u_strlen(stringbits[S_LINKZ]), newlocal, length+1, &status);
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_replaceAll returned %s - cannot continue!\n", u_errorName(status));
				return -1;
			}

			delete local;
			local = newlocal;
			ux_packWhitespace(local);

			switch(key) {
				case K_LIST:
					parseList(local, result);
					break;
				case K_SET:
					parseSet(local, result);
					break;
				case K_SELECT:
				case K_REMOVE:
				case K_IFF:
				case K_DELIMIT:
					parseSelectRemoveIffDelimit(local, key, result);
					break;
				case K_DELIMITERS:
					parseDelimiters(local, result);
					break;
				case K_PREFERRED_TARGETS:
					parsePreferredTargets(local, result);
					break;
				default:
					break;
			}

			delete local;
			return 0;
		}
		
		int parse_grammar_from_ufile(UFILE *input, CG3::Grammar *result) {
			u_frewind(input);
			if (u_feof(input)) {
				u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
				return -1;
			}
			if (!result) {
				u_fprintf(ux_stderr, "Error: No preallocated grammar provided - cannot continue!\n");
				return -1;
			}
			
			int error = init_keywords();
			if (error) {
				u_fprintf(ux_stderr, "Error: init_keywords returned %u!\n", error);
				return error;
			}

			error = init_regexps();
			if (error) {
				u_fprintf(ux_stderr, "Error: init_regexps returned %u!\n", error);
				return error;
			}

			error = init_strings();
			if (error) {
				u_fprintf(ux_stderr, "Error: init_strings returned %u!\n", error);
				return error;
			}

			// ToDo: Make this dynamic.
			std::map<uint32_t, UChar*> lines;
			std::map<uint32_t, uint32_t> keys;
			uint32_t lastcmd = 0;
			result->lines = 1;

			while (!u_feof(input)) {
				#define BUFFER_SIZE (65536)
				UChar *line = new UChar[BUFFER_SIZE];
				//memset(line, 0, sizeof(UChar)*BUFFER_SIZE);
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
					ux_trim(line);
					int keyword = 0;
					for (int i=1;i<KEYWORD_COUNT;i++) {
						UChar *pos = 0;
						int length = 0;
						if (pos = u_strstr(line, keywords[i])) {
							length = u_strlen(keywords[i]);
							if (
								((pos == line) || (pos > line && u_isWhitespace(pos[-1])))
								&& (pos[length] == 0 || pos[length] == ':' || u_isWhitespace(pos[length]))
								) {
								lastcmd = result->lines;
								keyword = i;
								break;
							}
						}
					}
					if (keyword || !lines[lastcmd]) {
						while (!lines.empty()) {
							result->curline = (*lines.begin()).first;
							UChar *line = (*lines.begin()).second;
							if (keys[result->curline]) {
								parseSingleLine(keys[result->curline], line, result);
							}
							delete line;
							keys.erase(result->curline);
							lines.erase(lines.begin());
						}
						lines[result->lines] = line;
						keys[result->lines] = keyword;
					} else {
						length = u_strlen(lines[lastcmd]);
						lines[lastcmd][length] = ' ';
						lines[lastcmd][length+1] = 0;
						u_strcat(lines[lastcmd], line);
						delete line;
					}
				} else {
					delete line;
				}
				result->lines++;
			}

			while (!lines.empty()) {
				result->curline = (*lines.begin()).first;
				UChar *line = (*lines.begin()).second;
				if (keys[result->curline]) {
					parseSingleLine(keys[result->curline], line, result);
				}
				delete line;
				keys.erase(result->curline);
				lines.erase(lines.begin());
			}

			free_keywords();
			free_regexps();
			free_strings();
			lines.clear();
			keys.clear();

			return 0; // No errors.
		}
		
		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage, CG3::Grammar *result) {
			UFILE *grammar = u_fopen(filename, "r", locale, codepage);
			if (!grammar) {
				return -1; // File could not be opened for reading.
			}
			
			int error = parse_grammar_from_ufile(grammar, result);
			if (error) {
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
