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

#include "TextualParser.h"

using namespace CG3;
using namespace CG3::Strings;

TextualParser::TextualParser(UFILE *ux_in, UFILE *ux_out, UFILE *ux_err) {
	ux_stdin = ux_in;
	ux_stdout = ux_out;
	ux_stderr = ux_err;
	filename = 0;
	locale = 0;
	codepage = 0;
	result = 0;
	option_vislcg_compat = false;
	in_before_sections = false;
	in_after_sections = false;
	in_section = false;
}

TextualParser::~TextualParser() {
	filename = 0;
	locale = 0;
	codepage = 0;
	result = 0;
}

inline bool ISNL(const UChar c) {
	return (
	   c == 0x2028L // Unicode Line Seperator
	|| c == 0x2029L // Unicode Paragraph Seperator
	|| c == 0x0085L // EBCDIC NEL
	|| c == 0x000CL // Form Feed
	|| c == 0x000AL // ASCII \n
	);
}

inline bool ISESC(UChar *p) {
	uint32_t a=1;
	while(*(p-a) && *(p-a) == '\\') {
		a++;
	}
	return (a%2==0);
}

inline bool ISCHR(const UChar p, const UChar a, const UChar b) {
	return ((p) && ((p) == (a) || (p) == (b)));
}

inline uint32_t SKIPLN(UChar **p) {
	while (**p && !ISNL(**p)) {
		(*p)++;
	}
	(*p)++;
	return 1;
}

inline uint32_t SKIPWS(UChar **p, UChar a) {
	uint32_t s = 0;
	while (**p && **p != a) {
		if (ISNL(**p)) {
			s++;
		}
		if (**p == '#' && !ISESC(*p)) {
			s += SKIPLN(p);
			(*p)--;
		}
		if (!u_isWhitespace(**p)) {
			break;
		}
		(*p)++;
	}
	return s;
}

inline uint32_t SKIPTOWS(UChar **p) {
	uint32_t s = 0;
	while (**p && !u_isWhitespace(**p)) {
		if (**p == '#' && !ISESC(*p)) {
			s += SKIPLN(p);
			(*p)--;
		}
		if (**p == ';' && !ISESC(*p)) {
			break;
		}
		(*p)++;
	}
	return s;
}

int TextualParser::parseFromUChar(UChar *input) {
	if (!result) {
		u_fprintf(ux_stderr, "Error: No preallocated grammar provided - cannot continue!\n");
		return -1;
	}
	if (!input || !input[0]) {
		u_fprintf(ux_stderr, "Error: Input is empty - cannot continue!\n");
		return -1;
	}

	UChar *p = input;
	result->lines = 1;

	while (*p) {
		if (*p == '#') {
			result->lines += SKIPLN(&p);
		}
		if (*p == 0x000DL) {
			p++;
			continue;
		}
		if (ISNL(*p)) {
			result->lines++;
			p++;
			continue;
		}
		if (u_isWhitespace(*p)) {
			p++;
			continue;
		}
		// DELIMITERS
		if (ISCHR(*p,'D','d') && ISCHR(*(p+9),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
			&& ISCHR(*(p+3),'I','i') && ISCHR(*(p+4),'M','m') && ISCHR(*(p+5),'I','i') && ISCHR(*(p+6),'T','t')
			&& ISCHR(*(p+7),'E','e') && ISCHR(*(p+8),'R','r')) {
			if (result->delimiters) {
				u_fprintf(ux_stderr, "Error: Cannot redefine DELIMITERS on line %u!\n", result->lines);
				return -1;
			}
			result->delimiters = result->allocateSet();
			p += 10;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				return -1;
			}
			p++;
			while (*p && *p != ';') {
				result->lines += SKIPWS(&p, ';');
				if (*p && *p != ';') {
					UChar *n = p;
					result->lines += SKIPTOWS(&n);
					uint32_t l = (uint32_t)(n - p);
					u_strncpy(gbuffers[0], p, l);
					gbuffers[0][l] = 0;
					CompositeTag *ct = result->allocateCompositeTag();
					Tag *t = result->allocateTag(gbuffers[0]);
					result->addTagToCompositeTag(t, ct);
					result->addCompositeTagToSet(result->delimiters, ct);
					p = n;
				}
			}
			result->delimiters->setName(stringbits[S_DELIMITSET]);
			result->addSet(result->delimiters);
			if (result->delimiters->tags.empty()) {
				u_fprintf(ux_stderr, "Error: DELIMITERS declared, but no definitions given, on line %u!\n", result->lines);
				return -1;
			}
		}
		p++;
	}
	
	return 0;
}

int TextualParser::parse_grammar_from_file(const char *fname, const char *loc, const char *cpage) {
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
		u_fprintf(ux_stderr, "Error: Cannot stat %s due to error %d - bailing out!\n", filename, error);
		exit(1);
	} else {
		result->last_modified = _stat.st_mtime;
		result->grammar_size = _stat.st_size;
	}

	result->setName(filename);

	UFILE *grammar = u_fopen(filename, "r", locale, codepage);
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: Error opening %s for reading!\n", filename);
		return -1;
	}

	UChar *data = new UChar[result->grammar_size*4];
	memset(data, result->grammar_size*4, sizeof(UChar));
	uint32_t read = u_file_read(data, result->grammar_size*4, grammar);
	if (read >= result->grammar_size*4-1) {
		u_fprintf(ux_stderr, "Error: Converting from underlying codepage to UTF-16 exceeded factor 4 buffer.\n", filename);
		return -1;
	}
	u_fclose(grammar);

	error = parseFromUChar(data);
	if (error) {
		return error;
	}

	delete[] data;
	return 0;
}

void TextualParser::setResult(CG3::Grammar *res) {
	result = res;
}

void TextualParser::setCompatible(bool f) {
	option_vislcg_compat = f;
}

void TextualParser::addRuleToGrammar(Rule *rule) {
	if (in_section) {
		rule->section = (int32_t)(result->sections.size()-1);
		result->addRule(rule);
	}
	else if (in_before_sections) {
		rule->section = -1;
		result->addRule(rule);
	}
	else if (in_after_sections) {
		rule->section = -2;
		result->addRule(rule);
	}
}
