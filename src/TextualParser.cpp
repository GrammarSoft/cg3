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

int TextualParser::dieIfKeyword(UChar *s) {
	for (uint32_t i=0;i<KEYWORD_COUNT;i++) {
		if (u_strcasecmp(s, keywords[i], U_FOLD_CASE_DEFAULT) == 0) {
			u_fprintf(ux_stderr, "Error: Keyword %S is in an invalid position on line %u!\n", keywords[i], result->lines);
			CG3Quit(1);
		}
	}
	for (uint32_t i=0;i<S_DELIMITSET;i++) {
		if (u_strcasecmp(s, stringbits[i], U_FOLD_CASE_DEFAULT) == 0) {
			u_fprintf(ux_stderr, "Error: Keyword %S is in an invalid position on line %u!\n", stringbits[i], result->lines);
			CG3Quit(1);
		}
	}
	return 0;
}

int TextualParser::parseTagList(Set *s, UChar **p, const bool isinline = false) {
	while (**p && **p != ';' && **p != ')') {
		result->lines += SKIPWS(p, ';', ')');
		if (**p && **p != ';' && **p != ')') {
			if (**p == '(') {
				(*p)++;
				CompositeTag *ct = result->allocateCompositeTag();

				while (**p && **p != ';' && **p != ')') {
					UChar *n = *p;
					result->lines += SKIPTOWS(&n, ')');
					uint32_t c = (uint32_t)(n - *p);
					u_strncpy(gbuffers[0], *p, c);
					gbuffers[0][c] = 0;
					dieIfKeyword(gbuffers[0]);
					Tag *t = result->allocateTag(gbuffers[0]);
					t = result->addTag(t);
					result->addTagToCompositeTag(t, ct);
					*p = n;
					result->lines += SKIPWS(p, ';', ')');
				}
				if (**p == ';') {
					u_fprintf(ux_stderr, "Error: Encountered a ; before the closing ) on line %u!\n", result->lines);
					CG3Quit(1);
				}
				if (**p == ')') {
					(*p)++;
				}

				result->addCompositeTagToSet(s, ct);
			}
			else {
				UChar *n = *p;
				result->lines += SKIPTOWS(&n);
				uint32_t c = (uint32_t)(n - *p);
				u_strncpy(gbuffers[0], *p, c);
				gbuffers[0][c] = 0;
				dieIfKeyword(gbuffers[0]);
				CompositeTag *ct = result->allocateCompositeTag();
				Tag *t = result->allocateTag(gbuffers[0]);
				t = result->addTag(t);
				result->addTagToCompositeTag(t, ct);
				result->addCompositeTagToSet(s, ct);
				*p = n;
			}
		}
	}
	if (isinline) {
		if (**p == ';') {
			u_fprintf(ux_stderr, "Error: Encountered a ; before the closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		if (**p == ')') {
			(*p)++;
		}
	}
	return 0;
}

int TextualParser::parseSetInline(Set *s, UChar **p) {
	bool wantop = false;
	while (**p && **p != ';' && **p != ')') {
		result->lines += SKIPWS(p, ';', ')');
		if (**p && **p != ';' && **p != ')') {
			if (!wantop) {
				if (**p == '(') {
					(*p)++;
					Set *set_c = result->allocateSet();
					set_c->line = result->lines;
					set_c->setName(rand() + hash_sdbm_uchar(gbuffers[0], result->lines + result->sets_all.size()));
					CompositeTag *ct = result->allocateCompositeTag();

					while (**p && **p != ';' && **p != ')') {
						UChar *n = *p;
						result->lines += SKIPTOWS(&n, ')');
						uint32_t c = (uint32_t)(n - *p);
						u_strncpy(gbuffers[0], *p, c);
						gbuffers[0][c] = 0;
						dieIfKeyword(gbuffers[0]);
						Tag *t = result->allocateTag(gbuffers[0]);
						t = result->addTag(t);
						result->addTagToCompositeTag(t, ct);
						*p = n;
						result->lines += SKIPWS(p, ';', ')');
					}
					if (**p == ';') {
						u_fprintf(ux_stderr, "Error: Encountered a ; before the closing ) on line %u!\n", result->lines);
						CG3Quit(1);
					}
					if (**p == ')') {
						(*p)++;
					}

					result->addCompositeTagToSet(set_c, ct);
					result->addSet(set_c);
					s->sets.push_back(set_c->hash);
				}
				else {
					UChar *n = *p;
					result->lines += SKIPTOWS(&n, ')', true);
					uint32_t c = (uint32_t)(n - *p);
					u_strncpy(gbuffers[0], *p, c);
					gbuffers[0][c] = 0;
					dieIfKeyword(gbuffers[0]);
					uint32_t sh = hash_sdbm_uchar(gbuffers[0]);

					if (ux_isSetOp(gbuffers[0]) != S_IGNORE) {
						u_fprintf(ux_stderr, "Error: Found set operator '%S' where set name expected on line %u!\n", gbuffers[0], result->lines);
						CG3Quit(1);
					}

					if (gbuffers[0] && gbuffers[0][0] == '$' && gbuffers[0][1] == '$' && gbuffers[0][2]) {
						const UChar *wname = &(gbuffers[0][2]);
						uint32_t wrap = hash_sdbm_uchar(wname);
						Set *wtmp = result->getSet(wrap);
						if (!wtmp) {
							u_fprintf(ux_stderr, "Error: Attempted to reference undefined set '%S' on line %u!\n", wname, result->lines);
							CG3Quit(1);
						}
						Set *tmp = result->getSet(sh);
						if (!tmp) {
							Set *ns = result->allocateSet();
							ns->line = result->lines;
							ns->setName(gbuffers[0]);
							ns->sets.push_back(wtmp->hash);
							ns->is_unified = true;
							result->addSet(ns);
						}
					}
					if (result->set_alias.find(sh) != result->set_alias.end()) {
						sh = result->set_alias[sh];
					}
					Set *tmp = result->getSet(sh);
					if (!tmp) {
						u_fprintf(ux_stderr, "Error: Attempted to reference undefined set '%S' on line %u!\n", gbuffers[0], result->lines);
						CG3Quit(1);
					}
					sh = tmp->hash;
					s->sets.push_back(sh);
					*p = n;
				}
				wantop = true;
			}
			else {
				UChar *n = *p;
				result->lines += SKIPTOWS(&n, 0, true);
				uint32_t c = (uint32_t)(n - *p);
				u_strncpy(gbuffers[0], *p, c);
				gbuffers[0][c] = 0;
				//dieIfKeyword(gbuffers[0]);
				int sop = ux_isSetOp(gbuffers[0]);
				if (sop != S_IGNORE) {
					s->set_ops.push_back(sop);
					wantop = false;
					*p = n;
				}
				else {
					break;
				}
			}
		}
	}
	return 0;
}

int TextualParser::parseContextualTestList(Rule *rule, std::list<ContextualTest*> *thelist, CG3::ContextualTest *parentTest, UChar **p) {
	ContextualTest *t = 0;
	if (parentTest) {
		t = parentTest->allocateContextualTest();
	}
	else {
		t = rule->allocateContextualTest();
	}
	t->line = result->lines;
	bool negated = false, negative = false;

	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_TEXTNEGATE], u_strlen(stringbits[S_TEXTNEGATE]), U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += u_strlen(stringbits[S_TEXTNEGATE]);
		negated = true;
	}
	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_TEXTNOT], u_strlen(stringbits[S_TEXTNOT]), U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += u_strlen(stringbits[S_TEXTNOT]);
		negative = true;
	}
	result->lines += SKIPWS(p);

	UChar *n = *p;
	result->lines += SKIPTOWS(&n, '(');
	uint32_t c = (uint32_t)(n - *p);
	u_strncpy(gbuffers[0], *p, c);
	gbuffers[0][c] = 0;
	dieIfKeyword(gbuffers[0]);
	t->parsePosition(gbuffers[0], ux_stderr);
	*p = n;
	if (negated) {
		t->pos |= POS_NEGATED;
	}
	if (negative) {
		t->pos |= POS_NEGATIVE;
	}
	if (t->pos & (POS_DEP_CHILD|POS_DEP_PARENT|POS_DEP_SIBLING)) {
		result->has_dep = true;
	}
	result->lines += SKIPWS(p);

	Set *s = result->allocateSet();
	s->line = result->lines;
	s->setName(rand() + hash_sdbm_uchar(gbuffers[0], result->lines + result->sets_all.size()));
	parseSetInline(s, p);
	if (s->sets.size() == 1 && !s->is_unified) {
		Set *tmp = result->getSet(s->sets.back());
		result->destroySet(s);
		s = tmp;
	}
	result->addSet(s);
	t->target = s->hash;

	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_CBARRIER], u_strlen(stringbits[S_CBARRIER]), U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += u_strlen(stringbits[S_CBARRIER]);
		result->lines += SKIPWS(p);
		Set *s = result->allocateSet();
		s->line = result->lines;
		s->setName(rand() + hash_sdbm_uchar(gbuffers[0], result->lines + result->sets_all.size()));
		parseSetInline(s, p);
		if (s->sets.size() == 1 && !s->is_unified) {
			Set *tmp = result->getSet(s->sets.back());
			result->destroySet(s);
			s = tmp;
		}
		result->addSet(s);
		t->cbarrier = s->hash;
	}
	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_BARRIER], u_strlen(stringbits[S_BARRIER]), U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += u_strlen(stringbits[S_BARRIER]);
		result->lines += SKIPWS(p);
		Set *s = result->allocateSet();
		s->line = result->lines;
		s->setName(rand() + hash_sdbm_uchar(gbuffers[0], result->lines + result->sets_all.size()));
		parseSetInline(s, p);
		if (s->sets.size() == 1 && !s->is_unified) {
			Set *tmp = result->getSet(s->sets.back());
			result->destroySet(s);
			s = tmp;
		}
		result->addSet(s);
		t->barrier = s->hash;
	}
	result->lines += SKIPWS(p);

	bool linked = false;
	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_LINK], u_strlen(stringbits[S_LINK]), U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += u_strlen(stringbits[S_LINK]);
		linked = true;
	}
	result->lines += SKIPWS(p);

	if (linked) {
		parseContextualTestList(0, 0, t, p);
	}

	if (parentTest) {
		parentTest->linked = t;
	}
	else {
		rule->addContextualTest(t, thelist);
	}
	return 0;
}

int TextualParser::parseContextualTests(Rule *rule, UChar **p) {
	return parseContextualTestList(rule, &rule->tests, 0, p);
}

int TextualParser::parseContextualDependencyTests(Rule *rule, UChar **p) {
	return parseContextualTestList(rule, &rule->dep_tests, 0, p);
}

int TextualParser::parseRule(KEYWORDS key, UChar **p) {
	Rule *rule = result->allocateRule();
	rule->line = result->lines;
	rule->type = key;

	UChar *lp = *p;
	BACKTONL(&lp);
	result->lines += SKIPWS(&lp);

	if (lp != *p && lp < *p) {
		UChar *n = lp;
		result->lines += SKIPTOWS(&n);
		uint32_t c = (uint32_t)(n - lp);
		u_strncpy(gbuffers[0], lp, c);
		gbuffers[0][c] = 0;
		dieIfKeyword(gbuffers[0]);
		Tag *wform = result->allocateTag(gbuffers[0]);
		rule->wordform = wform->rehash();
		wform = result->addTag(wform);
	}

	(*p) += u_strlen(keywords[key]);
	result->lines += SKIPWS(p);

	if (key == K_SUBSTITUTE) {
		if (**p != '(') {
			u_fprintf(ux_stderr, "Error: Tag list for %S must be in () on line %u!\n", keywords[key], result->lines);
			CG3Quit(1);
		}
		(*p)++;
		result->lines += SKIPWS(p, ';', ')');
		while (**p && **p != ';' && **p != ')') {
			UChar *n = *p;
			result->lines += SKIPTOWS(&n, ')');
			uint32_t c = (uint32_t)(n - *p);
			u_strncpy(gbuffers[0], *p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			Tag *wform = result->allocateTag(gbuffers[0]);
			rule->wordform = wform->rehash();
			wform = result->addTag(wform);
			rule->sublist.push_back(wform->hash);
			*p = n;
			result->lines += SKIPWS(p, ';', ')');
		}
		if (**p == ';') {
			u_fprintf(ux_stderr, "Error: Encountered a ; before the closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		if (**p == ')') {
			(*p)++;
		}
	}
	result->lines += SKIPWS(p);
	if (key == K_MAP || key == K_ADD || key == K_REPLACE || key == K_APPEND || key == K_SUBSTITUTE) {
		if (**p != '(') {
			u_fprintf(ux_stderr, "Error: Tag list for %S must be in () on line %u!\n", keywords[key], result->lines);
			CG3Quit(1);
		}
		(*p)++;
		result->lines += SKIPWS(p, ';', ')');
		while (**p && **p != ';' && **p != ')') {
			UChar *n = *p;
			result->lines += SKIPTOWS(&n, ')');
			uint32_t c = (uint32_t)(n - *p);
			u_strncpy(gbuffers[0], *p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			Tag *wform = result->allocateTag(gbuffers[0]);
			rule->wordform = wform->rehash();
			wform = result->addTag(wform);
			rule->maplist.push_back(wform->hash);
			*p = n;
			result->lines += SKIPWS(p, ';', ')');
		}
		if (**p == ';') {
			u_fprintf(ux_stderr, "Error: Encountered a ; before the closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		if (**p == ')') {
			(*p)++;
		}
	}

	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_TARGET], u_strlen(stringbits[S_TARGET]), U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += u_strlen(stringbits[S_TARGET]);
	}
	result->lines += SKIPWS(p);

	Set *s = result->allocateSet();
	s->line = result->lines;
	s->setName(rand() + hash_sdbm_uchar(gbuffers[0], result->lines + result->sets_all.size()));
	parseSetInline(s, p);
	if (s->sets.size() == 1 && !s->is_unified) {
		Set *tmp = result->getSet(s->sets.back());
		result->destroySet(s);
		s = tmp;
	}
	result->addSet(s);
	rule->target = s->hash;

	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_IF], u_strlen(stringbits[S_IF]), U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += u_strlen(stringbits[S_IF]);
	}
	result->lines += SKIPWS(p);

	while (**p && **p == '(') {
		(*p)++;
		result->lines += SKIPWS(p);
		parseContextualTests(rule, p);
		result->lines += SKIPWS(p);
		if (**p == ';') {
			u_fprintf(ux_stderr, "Error: Encountered a ; before the closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		if (**p == ')') {
			(*p)++;
		}
		result->lines += SKIPWS(p);
	}

	addRuleToGrammar(rule);
	return 0;
}

int TextualParser::parseFromUChar(UChar *input) {
	if (!result) {
		u_fprintf(ux_stderr, "Error: No preallocated grammar provided - cannot continue!\n");
		CG3Quit(1);
	}
	if (!input || !input[0]) {
		u_fprintf(ux_stderr, "Error: Input is empty - cannot continue!\n");
		CG3Quit(1);
	}

	UChar *p = input;
	result->lines = 1;

	while (*p) {
		if (result->lines % 100 == 0) {
			std::cerr << "Parsing line " << result->lines << "          \r" << std::flush;
		}
		while (*p == '#') {
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
				CG3Quit(1);
			}
			result->delimiters = result->allocateSet();
			result->delimiters->line = result->lines;
			result->delimiters->setName(stringbits[S_DELIMITSET]);
			p += 10;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			p++;
			parseTagList(result->delimiters, &p);
			result->addSet(result->delimiters);
			if (result->delimiters->tags.empty() && result->delimiters->single_tags.empty()) {
				u_fprintf(ux_stderr, "Error: DELIMITERS declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(&p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
		}
		// SOFT-DELIMITERS
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+14),'S','s') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'F','f')
			&& ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'-','_')
			&& ISCHR(*(p+5),'D','d') && ISCHR(*(p+6),'E','e') && ISCHR(*(p+7),'L','l')
			&& ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'M','m') && ISCHR(*(p+10),'I','i') && ISCHR(*(p+11),'T','t')
			&& ISCHR(*(p+12),'E','e') && ISCHR(*(p+13),'R','r')) {
			if (result->soft_delimiters) {
				u_fprintf(ux_stderr, "Error: Cannot redefine SOFT-DELIMITERS on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->soft_delimiters = result->allocateSet();
			result->soft_delimiters->line = result->lines;
			result->soft_delimiters->setName(stringbits[S_SOFTDELIMITSET]);
			p += 15;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			p++;
			parseTagList(result->soft_delimiters, &p);
			result->addSet(result->soft_delimiters);
			if (result->soft_delimiters->tags.empty() && result->soft_delimiters->single_tags.empty()) {
				u_fprintf(ux_stderr, "Error: SOFT-DELIMITERS declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(&p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
		}
		// SETS
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+3),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')) {
			p += 4;
		}
		// LIST
		else if (ISCHR(*p,'L','l') && ISCHR(*(p+3),'T','t') && ISCHR(*(p+1),'I','i') && ISCHR(*(p+2),'S','s')) {
			Set *s = result->allocateSet();
			s->line = result->lines;
			p += 4;
			result->lines += SKIPWS(&p);
			UChar *n = p;
			result->lines += SKIPTOWS(&n, 0, true);
			uint32_t c = (uint32_t)(n - p);
			u_strncpy(gbuffers[0], p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			s->setName(gbuffers[0]);
			p = n;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			p++;
			parseTagList(s, &p);
			result->addSet(s);
			if (s->tags.empty() && s->single_tags.empty()) {
				u_fprintf(ux_stderr, "Error: LIST %S declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(&p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
		}
		// SET
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+2),'T','t') && ISCHR(*(p+1),'E','e')
			&& !ISSTRING(p, 2)) {
			Set *s = result->allocateSet();
			s->line = result->lines;
			p += 3;
			result->lines += SKIPWS(&p);
			UChar *n = p;
			result->lines += SKIPTOWS(&n, 0, true);
			uint32_t c = (uint32_t)(n - p);
			u_strncpy(gbuffers[0], p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			s->setName(gbuffers[0]);
			uint32_t sh = hash_sdbm_uchar(gbuffers[0]);
			p = n;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			p++;
			parseSetInline(s, &p);
			if (s->sets.size() == 1 && !s->is_unified) {
				Set *tmp = result->getSet(s->sets.back());
				u_fprintf(ux_stderr, "Warning: Set %S has been aliased to %S.\n", s->name, tmp->name);
				result->set_alias[sh] = tmp->hash;
				result->destroySet(s);
				s = tmp;
			}
			result->addSet(s);
			if (s->sets.empty() && s->tags.empty() && s->single_tags.empty()) {
				u_fprintf(ux_stderr, "Error: SET %S declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(&p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u! Probably caused by missing set operator.\n", *p, result->lines);
				CG3Quit(1);
			}
		}
		// MAPPINGS
		else if (ISCHR(*p,'M','m') && ISCHR(*(p+7),'S','s') && ISCHR(*(p+1),'A','a') && ISCHR(*(p+2),'P','p')
			&& ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'I','i') && ISCHR(*(p+5),'N','n') && ISCHR(*(p+6),'G','g')) {
			p += 8;
			in_before_sections = true;
			in_section = false;
			in_after_sections = false;
		}
		// CORRECTIONS
		else if (ISCHR(*p,'C','c') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'R','r')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'C','c') && ISCHR(*(p+6),'T','t')
			&& ISCHR(*(p+7),'I','i') && ISCHR(*(p+8),'O','o') && ISCHR(*(p+9),'N','n')) {
			p += 11;
			in_before_sections = true;
			in_section = false;
			in_after_sections = false;
		}
		// BEFORE-SECTIONS
		else if (ISCHR(*p,'B','b') && ISCHR(*(p+14),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'F','f')
			&& ISCHR(*(p+3),'O','o') && ISCHR(*(p+4),'R','r') && ISCHR(*(p+5),'E','e') && ISCHR(*(p+6),'-','_')
			&& ISCHR(*(p+7),'S','s') && ISCHR(*(p+8),'E','e') && ISCHR(*(p+9),'C','c') && ISCHR(*(p+10),'T','t')
			&& ISCHR(*(p+11),'I','i') && ISCHR(*(p+12),'O','o') && ISCHR(*(p+13),'N','n')) {
			p += 15;
			in_before_sections = true;
			in_section = false;
			in_after_sections = false;
		}
		// SECTION
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+6),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'C','c')
			&& ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'I','i') && ISCHR(*(p+5),'O','o')) {
			p += 7;
			result->sections.push_back(result->lines);
			in_before_sections = false;
			in_section = true;
			in_after_sections = false;
		}
		// CONSTRAINTS
		else if (ISCHR(*p,'C','c') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'N','n')
			&& ISCHR(*(p+3),'S','s') && ISCHR(*(p+4),'T','t') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'I','i') && ISCHR(*(p+8),'N','n') && ISCHR(*(p+9),'T','t')) {
			p += 11;
			result->sections.push_back(result->lines);
			in_before_sections = false;
			in_section = true;
			in_after_sections = false;
		}
		// AFTER-SECTIONS
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+13),'S','s') && ISCHR(*(p+1),'F','f') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'R','r') && ISCHR(*(p+5),'-','_')
			&& ISCHR(*(p+6),'S','s') && ISCHR(*(p+7),'E','e') && ISCHR(*(p+8),'C','c') && ISCHR(*(p+9),'T','t')
			&& ISCHR(*(p+10),'I','i') && ISCHR(*(p+11),'O','o') && ISCHR(*(p+12),'N','n')) {
			p += 14;
			in_before_sections = false;
			in_section = false;
			in_after_sections = true;
		}
		// IFF
		else if (ISCHR(*p,'I','i') && ISCHR(*(p+2),'F','f') && ISCHR(*(p+1),'F','f')) {
			parseRule(K_IFF, &p);
		}
		// MAP
		else if (ISCHR(*p,'M','m') && ISCHR(*(p+2),'P','p') && ISCHR(*(p+1),'A','a')) {
			parseRule(K_MAP, &p);
		}
		// ADD
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+2),'D','d') && ISCHR(*(p+1),'D','d')) {
			parseRule(K_ADD, &p);
		}
		// APPEND
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+5),'D','d') && ISCHR(*(p+1),'P','p') && ISCHR(*(p+2),'P','p')
			&& ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'N','n')) {
			parseRule(K_APPEND, &p);
		}
		// SELECT
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+5),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
			&& ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'C','c')) {
			parseRule(K_SELECT, &p);
		}
		// REMOVE
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+5),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'O','o') && ISCHR(*(p+4),'V','v')) {
			parseRule(K_REMOVE, &p);
		}
		// DELIMIT
		else if (ISCHR(*p,'D','d') && ISCHR(*(p+6),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
			&& ISCHR(*(p+3),'I','i') && ISCHR(*(p+4),'M','m') && ISCHR(*(p+4),'I','i')) {
			parseRule(K_DELIMIT, &p);
		}
		// END
		else if (ISCHR(*p,'E','e') && ISCHR(*(p+2),'D','d') && ISCHR(*(p+1),'N','n')) {
			if (ISNL(*(p-1)) || u_isWhitespace(*(p-1))) {
				if (*(p+3) == 0 || ISNL(*(p+3)) || u_isWhitespace(*(p+3))) {
					break;
				}
			}
			p++;
		}
		// No keyword found at this position, skip a character.
		else {
			p++;
		}
	}
	
	return 0;
}

int TextualParser::parse_grammar_from_file(const char *fname, const char *loc, const char *cpage) {
	filename = fname;
	locale = loc;
	codepage = cpage;

	if (!result) {
		u_fprintf(ux_stderr, "Error: Cannot parse into nothing - hint: call setResult() before trying.\n");
		CG3Quit(1);
	}

	struct stat _stat;
	int error = stat(filename, &_stat);

	if (error != 0) {
		u_fprintf(ux_stderr, "Error: Cannot stat %s due to error %d - bailing out!\n", filename, error);
		CG3Quit(1);
	} else {
		result->last_modified = _stat.st_mtime;
		result->grammar_size = _stat.st_size;
	}

	result->setName(filename);

	UFILE *grammar = u_fopen(filename, "r", locale, codepage);
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: Error opening %s for reading!\n", filename);
		CG3Quit(1);
	}

	UChar *data = new UChar[result->grammar_size*4];
	memset(data, 0, result->grammar_size*4*sizeof(UChar));
	uint32_t read = u_file_read((data+4), result->grammar_size*4, grammar);
	if (read >= result->grammar_size*4-1) {
		u_fprintf(ux_stderr, "Error: Converting from underlying codepage to UTF-16 exceeded factor 4 buffer.\n", filename);
		CG3Quit(1);
	}
	u_fclose(grammar);

	error = parseFromUChar((data+4));
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
	else {
		u_fprintf(ux_stderr, "Error: Rule definition attempted outside a section on line %u!\n", result->lines);
		CG3Quit(1);
	}
}
