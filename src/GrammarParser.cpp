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
#include <sys/stat.h>

using namespace CG3;
using namespace CG3::Strings;

GrammarParser::GrammarParser() {
	filename = 0;
	locale = 0;
	codepage = 0;
	result = 0;
	option_vislcg_compat = false;
}

GrammarParser::~GrammarParser() {
	filename = 0;
	locale = 0;
	codepage = 0;
	result = 0;
}

int GrammarParser::parseSingleLine(KEYWORDS key, const UChar *line) {
	u_fflush(ux_stderr);
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
		case K_SECTION:
		case K_CONSTRAINTS:
			parseSection(local);
			break;
		case K_ANCHOR:
			parseAnchor(local);
			break;
		case K_SUBSTITUTE:
			parseSubstitute(local);
			break;
		case K_LIST:
			parseList(local);
			break;
		case K_SET:
			parseSet(local);
			break;
		case K_SELECT:
		case K_REMOVE:
		case K_IFF:
		case K_DELIMIT:
		case K_MATCH:
			parseSelectRemoveIffDelimitMatch(local, key);
			break;
		case K_ADD:
		case K_MAP:
		case K_REPLACE:
		case K_APPEND:
			parseAddMapReplaceAppend(local, key);
			break;
		case K_DELIMITERS:
			parseDelimiters(local);
			break;
		case K_PREFERRED_TARGETS:
			parsePreferredTargets(local);
			break;
		case K_REMVARIABLE:
		case K_SETVARIABLE:
			parseRemSetVariable(local, key);
			break;
		default:
			break;
	}

	delete local;
	return 0;
}

int GrammarParser::parse_grammar_from_ufile(UFILE *input) {
	u_frewind(input);
	if (u_feof(input)) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		return -1;
	}
	if (!result) {
		u_fprintf(ux_stderr, "Error: No preallocated grammar provided - cannot continue!\n");
		return -1;
	}
	
	free_keywords();
	free_regexps();
	free_strings();

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

	// ToDo: Make line length dynamic
	std::map<uint32_t, UChar*> lines;
	std::map<uint32_t, KEYWORDS> keys;
	uint32_t lastcmd = 0;
	result->lines = 1;

	while (!u_feof(input)) {
		if (result->lines % 100 == 0) {
			std::cerr << "Parsing line " << result->lines << "          \r" << std::flush;
		}
		#define BUFFER_SIZE (131072)
		UChar *line = new UChar[BUFFER_SIZE];
		u_fgets(line, BUFFER_SIZE-1, input);

		ux_cutComments(line, '#', false);
//		if (option_vislcg_compat) {
			ux_cutComments(line, ';', true);
//		}

		int length = u_strlen(line);
		bool notnull = false;
		for (int i=0;i<length;i++) {
			if (!u_isWhitespace(line[i])) {
				notnull = true;
				break;
			}
		}
		// ToDo: Maybe support ICase keywords
		if (notnull) {
			ux_trim(line);
			KEYWORDS keyword = K_IGNORE;
			for (uint32_t i=1;i<KEYWORD_COUNT;i++) {
				UChar *pos = 0;
				int length = 0;
				if ((pos = u_strstr(line, keywords[i])) != 0) {
					length = u_strlen(keywords[i]);
					if (
						((pos == line) || (pos > line && u_isWhitespace(pos[-1])))
						&& (pos[length] == 0 || pos[length] == ':' || u_isWhitespace(pos[length]))
						) {
						lastcmd = result->lines;
						keyword = (KEYWORDS)i;
						break;
					}
				}
			}
			if (keyword || !lines[lastcmd]) {
				while (!lines.empty()) {
					result->curline = (*lines.begin()).first;
					UChar *line = (*lines.begin()).second;
					if (keys[result->curline]) {
						parseSingleLine(keys[result->curline], line);
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
			parseSingleLine(keys[result->curline], line);
		}
		delete line;
		keys.erase(result->curline);
		lines.erase(lines.begin());
	}

	free_regexps();
	free_strings();
	lines.clear();
	keys.clear();

	if (!result->rules.empty()) {
		result->sections.push_back((uint32_t)result->rules.size());
	}

	return 0;
}

int GrammarParser::parse_grammar_from_file(const char *fname, const char *loc, const char *cpage) {
	filename = fname;
	locale = loc;
	codepage = cpage;

	if (!result) {
		u_fprintf(ux_stderr, "Error: Cannot parse into nothing - hint: call setResult() before trying.\n");
		return -1;
	}

	struct stat _stat;
	int error = stat(filename, &_stat);

	if (error != 0) {
		u_fprintf(ux_stderr, "Warning: Cannot stat %s due to error %d - setting defaults.\n", filename, error);
		result->last_modified = 12345678;
		result->grammar_size = 12345678;
	} else {
		result->last_modified = (uint32_t)_stat.st_mtime;
		result->grammar_size = (uint32_t)_stat.st_size;
	}

	result->setName(filename);

	UFILE *grammar = u_fopen(filename, "r", locale, codepage);
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: Error opening %s for reading!\n", filename);
		return -1;
	}
	
	error = parse_grammar_from_ufile(grammar);
	if (error) {
		return error;
	}
	return 0;
}

void GrammarParser::setResult(CG3::Grammar *res) {
	result = res;
}
