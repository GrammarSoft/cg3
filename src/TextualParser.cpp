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

#include "TextualParser.h"

using namespace CG3;
using namespace CG3::Strings;

TextualParser::TextualParser(UFILE *ux_err) {
	ux_stderr = ux_err;
	filename = 0;
	locale = 0;
	codepage = 0;
	result = 0;
	option_vislcg_compat = false;
	in_before_sections = false;
	in_after_sections = false;
	in_section = false;
	verbosity_level = 0;
	sets_counter = 100;
}

TextualParser::~TextualParser() {
	filename = 0;
	locale = 0;
	codepage = 0;
	result = 0;
}

int TextualParser::dieIfKeyword(UChar *s) {
	s=s;
	/*
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
	//*/
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
					if (*n == '"') {
						n++;
						result->lines += SKIPTO_NOSPAN(&n, '"');
						if (*n != '"') {
							u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
							CG3Quit(1);
						}
					}
					result->lines += SKIPTOWS(&n, ')', true);
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
				if (**p != ')') {
					u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
					CG3Quit(1);
				}
				(*p)++;

				ct = result->addCompositeTagToSet(s, ct);
			}
			else {
				UChar *n = *p;
				if (*n == '"') {
					n++;
					result->lines += SKIPTO_NOSPAN(&n, '"');
					if (*n != '"') {
						u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
						CG3Quit(1);
					}
				}
				result->lines += SKIPTOWS(&n, 0, true);
				uint32_t c = (uint32_t)(n - *p);
				u_strncpy(gbuffers[0], *p, c);
				gbuffers[0][c] = 0;
				dieIfKeyword(gbuffers[0]);
				CompositeTag *ct = result->allocateCompositeTag();
				Tag *t = result->allocateTag(gbuffers[0]);
				t = result->addTag(t);
				result->addTagToCompositeTag(t, ct);
				ct = result->addCompositeTagToSet(s, ct);
				*p = n;
			}
		}
	}
	if (isinline) {
		if (**p != ')') {
			u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		(*p)++;
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
					set_c->setName(sets_counter++);
					CompositeTag *ct = result->allocateCompositeTag();

					while (**p && **p != ';' && **p != ')') {
						result->lines += SKIPWS(p, ';', ')');
						UChar *n = *p;
						if (*n == '"') {
							n++;
							result->lines += SKIPTO_NOSPAN(&n, '"');
							if (*n != '"') {
								u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
								CG3Quit(1);
							}
						}
						result->lines += SKIPTOWS(&n, ')', true);
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
					if (**p != ')') {
						u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
						CG3Quit(1);
					}
					(*p)++;

					ct = result->addCompositeTagToSet(set_c, ct);
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

int TextualParser::parseContextualTestList(Rule *rule, std::list<ContextualTest*> *thelist, CG3::ContextualTest *parentTest, UChar **p, CG3::ContextualTest *self) {
	ContextualTest *t = 0;
	if (self) {
		t = self;
	}
	else if (parentTest) {
		t = parentTest->allocateContextualTest();
	}
	else {
		t = rule->allocateContextualTest();
	}
	t->line = result->lines;
	bool negated = false, negative = false;

	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_TEXTNEGATE], stringbit_lengths[S_TEXTNEGATE], U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += stringbit_lengths[S_TEXTNEGATE];
		negated = true;
	}
	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_TEXTNOT], stringbit_lengths[S_TEXTNOT], U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += stringbit_lengths[S_TEXTNOT];
		negative = true;
	}
	result->lines += SKIPWS(p);

	UChar *n = *p;
	result->lines += SKIPTOWS(&n, '(');
	uint32_t c = (uint32_t)(n - *p);
	u_strncpy(gbuffers[0], *p, c);
	gbuffers[0][c] = 0;
	dieIfKeyword(gbuffers[0]);
	if (ux_isEmpty(gbuffers[0])) {
		*p = n;
		for (;;) {
			ContextualTest *ored = t->allocateContextualTest();
			if (**p != '(') {
				u_fprintf(ux_stderr, "Error: Expected '(' but found '%C' on line %u!\n", **p, result->lines);
				CG3Quit(1);
			}
			(*p)++;
			parseContextualTestList(0, 0, 0, p, ored);
			(*p)++;
			t->ors.push_back(ored);
			result->lines += SKIPWS(p);
			if (u_strncasecmp(*p, stringbits[S_OR], stringbit_lengths[S_OR], U_FOLD_CASE_DEFAULT) == 0) {
				(*p) += stringbit_lengths[S_OR];
			}
			else {
				break;
			}
			result->lines += SKIPWS(p);
		}
	}
	else if (gbuffers[0][0] == 'T' && gbuffers[0][1] == ':') {
		(*p) += 2;
		n = *p;
		result->lines += SKIPTOWS(&n, ')');
		uint32_t c = (uint32_t)(n - *p);
		u_strncpy(gbuffers[0], *p, c);
		gbuffers[0][c] = 0;
		dieIfKeyword(gbuffers[0]);
		uint32_t cn = hash_sdbm_uchar(gbuffers[0]);
		if (result->templates.find(cn) == result->templates.end()) {
			u_fprintf(ux_stderr, "Error: Unknown template '%S' referenced on line %u!\n", gbuffers[0], result->lines);
			CG3Quit(1);
		}
		t->tmpl = result->templates.find(cn)->second;
		*p = n;
		result->lines += SKIPWS(p);
		/*
		if (**p != ')') {
			u_fprintf(ux_stderr, "Error: Expected ')' but found '%C' on line %u!\n", **p, result->lines);
			CG3Quit(1);
		}
		(**p)++;
		//*/
	}
	else {
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
		s->setName(sets_counter++);
		parseSetInline(s, p);
		if (s->sets.size() == 1 && !s->is_unified) {
			Set *tmp = result->getSet(s->sets.back());
			result->destroySet(s);
			s = tmp;
		}
		result->addSet(s);
		t->target = s->hash;

		result->lines += SKIPWS(p);
		if (u_strncasecmp(*p, stringbits[S_CBARRIER], stringbit_lengths[S_CBARRIER], U_FOLD_CASE_DEFAULT) == 0) {
			(*p) += stringbit_lengths[S_CBARRIER];
			result->lines += SKIPWS(p);
			Set *s = result->allocateSet();
			s->line = result->lines;
			s->setName(sets_counter++);
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
		if (u_strncasecmp(*p, stringbits[S_BARRIER], stringbit_lengths[S_BARRIER], U_FOLD_CASE_DEFAULT) == 0) {
			(*p) += stringbit_lengths[S_BARRIER];
			result->lines += SKIPWS(p);
			Set *s = result->allocateSet();
			s->line = result->lines;
			s->setName(sets_counter++);
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
	}

	bool linked = false;
	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_AND], stringbit_lengths[S_AND], U_FOLD_CASE_DEFAULT) == 0) {
		u_fprintf(ux_stderr, "Error: 'AND' is deprecated; use 'LINK 0' instead. Found on line %u!\n", result->lines);
		CG3Quit(1);
	}
	if (u_strncasecmp(*p, stringbits[S_LINK], stringbit_lengths[S_LINK], U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += stringbit_lengths[S_LINK];
		linked = true;
	}
	result->lines += SKIPWS(p);

	/*
	if (linked && (t->tmpl || !t->ors.empty())) {
		u_fprintf(ux_stderr, "Error: Cannot LINK after templates or alternative tests on line %u!\n", result->lines);
		CG3Quit(1);
	}
	//*/

	if (linked) {
		parseContextualTestList(0, 0, t, p);
	}

	if (self) {
		// nothing...
	}
	else if (parentTest) {
		parentTest->linked = t;
	}
	else {
		if (option_vislcg_compat && t->pos & POS_NEGATIVE) {
			t->pos &= ~POS_NEGATIVE;
			t->pos |= POS_NEGATED;
		}
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
		if (*n == '"') {
			n++;
			result->lines += SKIPTO_NOSPAN(&n, '"');
			if (*n != '"') {
				u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		result->lines += SKIPTOWS(&n, 0, true);
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

	if (**p == ':') {
		(*p)++;
		UChar *n = *p;
		result->lines += SKIPTOWS(&n, '(');
		uint32_t c = (uint32_t)(n - *p);
		u_strncpy(gbuffers[0], *p, c);
		gbuffers[0][c] = 0;
		dieIfKeyword(gbuffers[0]);
		rule->setName(gbuffers[0]);
		*p = n;
	}
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
			result->lines += SKIPTOWS(&n, ')', true);
			uint32_t c = (uint32_t)(n - *p);
			u_strncpy(gbuffers[0], *p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			Tag *wform = result->allocateTag(gbuffers[0]);
			wform = result->addTag(wform);
			rule->sublist.push_back(wform->hash);
			*p = n;
			result->lines += SKIPWS(p, ';', ')');
		}
		if (**p != ')') {
			u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		(*p)++;
	}
	result->lines += SKIPWS(p);
	if (key == K_MAP || key == K_ADD || key == K_REPLACE || key == K_APPEND || key == K_SUBSTITUTE
		|| key == K_SETRELATIONS || key == K_SETRELATION || key == K_REMRELATIONS || key == K_REMRELATION) {
		if (**p != '(') {
			u_fprintf(ux_stderr, "Error: Tag list for %S must be in () on line %u!\n", keywords[key], result->lines);
			CG3Quit(1);
		}
		(*p)++;
		result->lines += SKIPWS(p, ';', ')');
		while (**p && **p != ';' && **p != ')') {
			UChar *n = *p;
			result->lines += SKIPTOWS(&n, ')', true);
			uint32_t c = (uint32_t)(n - *p);
			u_strncpy(gbuffers[0], *p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			Tag *wform = result->allocateTag(gbuffers[0]);
			wform = result->addTag(wform);
			rule->maplist.push_back(wform);
			*p = n;
			result->lines += SKIPWS(p, ';', ')');
		}
		if (**p != ')') {
			u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		(*p)++;
	}
	result->lines += SKIPWS(p);
	if (key == K_SETRELATIONS || key == K_REMRELATIONS) {
		if (**p != '(') {
			u_fprintf(ux_stderr, "Error: Tag list for %S must be in () on line %u!\n", keywords[key], result->lines);
			CG3Quit(1);
		}
		(*p)++;
		result->lines += SKIPWS(p, ';', ')');
		while (**p && **p != ';' && **p != ')') {
			UChar *n = *p;
			result->lines += SKIPTOWS(&n, ')', true);
			uint32_t c = (uint32_t)(n - *p);
			u_strncpy(gbuffers[0], *p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			Tag *wform = result->allocateTag(gbuffers[0]);
			wform = result->addTag(wform);
			rule->sublist.push_back(wform->hash);
			*p = n;
			result->lines += SKIPWS(p, ';', ')');
		}
		if (**p != ')') {
			u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		(*p)++;
	}

	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_TARGET], stringbit_lengths[S_TARGET], U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += stringbit_lengths[S_TARGET];
	}
	result->lines += SKIPWS(p);

	Set *s = result->allocateSet();
	s->line = result->lines;
	s->setName(sets_counter++);
	parseSetInline(s, p);
	if (s->sets.size() == 1 && !s->is_unified) {
		Set *tmp = result->getSet(s->sets.back());
		result->destroySet(s);
		s = tmp;
	}
	result->addSet(s);
	rule->target = s->hash;

	result->lines += SKIPWS(p);
	if (u_strncasecmp(*p, stringbits[S_IF], stringbit_lengths[S_IF], U_FOLD_CASE_DEFAULT) == 0) {
		(*p) += stringbit_lengths[S_IF];
	}
	result->lines += SKIPWS(p);

	while (**p && **p == '(') {
		(*p)++;
		result->lines += SKIPWS(p);
		parseContextualTests(rule, p);
		result->lines += SKIPWS(p);
		if (**p != ')') {
			u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		(*p)++;
		result->lines += SKIPWS(p);
	}

	if (key == K_SETPARENT || key == K_SETCHILD || key == K_SETRELATION || key == K_REMRELATION
		|| key == K_SETRELATIONS || key == K_REMRELATIONS) {
		result->lines += SKIPWS(p);
		if (u_strncasecmp(*p, stringbits[S_TO], stringbit_lengths[S_TO], U_FOLD_CASE_DEFAULT) == 0) {
			(*p) += stringbit_lengths[S_TO];
		}
		else {
			u_fprintf(ux_stderr, "Error: Missing dependency keyword TO on line %u!\n", result->lines);
			CG3Quit(1);
		}
		result->lines += SKIPWS(p);
		while (**p && **p == '(') {
			(*p)++;
			result->lines += SKIPWS(p);
			parseContextualDependencyTests(rule, p);
			result->lines += SKIPWS(p);
			if (**p != ')') {
				u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
				CG3Quit(1);
			}
			(*p)++;
			result->lines += SKIPWS(p);
		}
		if (rule->dep_tests.empty()) {
			u_fprintf(ux_stderr, "Error: Missing dependency target on line %u!\n", result->lines);
			CG3Quit(1);
		}
		rule->dep_target = rule->dep_tests.front();
		rule->dep_tests.pop_front();
	}
	if (key == K_SETPARENT || key == K_SETCHILD) {
		result->has_dep = true;
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

	uint32_t seen_mapping_prefix = 0;

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
			&& ISCHR(*(p+7),'E','e') && ISCHR(*(p+8),'R','r')
			&& !ISSTRING(p, 9)) {
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
			&& ISCHR(*(p+12),'E','e') && ISCHR(*(p+13),'R','r')
			&& !ISSTRING(p, 14)) {
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
		// MAPPING-PREFIX
		else if (ISCHR(*p,'M','m') && ISCHR(*(p+13),'X','x') && ISCHR(*(p+1),'A','a') && ISCHR(*(p+2),'P','p')
			&& ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'I','i')
			&& ISCHR(*(p+5),'N','n') && ISCHR(*(p+6),'G','g') && ISCHR(*(p+7),'-','_')
			&& ISCHR(*(p+8),'P','p') && ISCHR(*(p+9),'R','r') && ISCHR(*(p+10),'E','e') && ISCHR(*(p+11),'F','f')
			&& ISCHR(*(p+12),'I','i')
			&& !ISSTRING(p, 13)) {

			if (seen_mapping_prefix) {
				u_fprintf(ux_stderr, "Error: MAPPING-PREFIX on line %u cannot change previous prefix set on line %u!\n", result->lines, seen_mapping_prefix);
				CG3Quit(1);
			}
			seen_mapping_prefix = result->lines;

			p += 14;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			p++;
			result->lines += SKIPWS(&p);

			UChar *n = p;
			result->lines += SKIPTOWS(&n, ';');
			uint32_t c = (uint32_t)(n - p);
			u_strncpy(gbuffers[0], p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			p = n;

			result->mapping_prefix = gbuffers[0][0];

			if (!result->mapping_prefix) {
				u_fprintf(ux_stderr, "Error: MAPPING-PREFIX declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(&p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
		}
		// PREFERRED-TARGETS
		else if (ISCHR(*p,'P','p') && ISCHR(*(p+16),'S','s') && ISCHR(*(p+1),'R','r') && ISCHR(*(p+2),'E','e')
			&& ISCHR(*(p+3),'F','f') && ISCHR(*(p+4),'E','e')
			&& ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'R','r') && ISCHR(*(p+7),'E','e')
			&& ISCHR(*(p+8),'D','d') && ISCHR(*(p+9),'-','_') && ISCHR(*(p+10),'T','t') && ISCHR(*(p+11),'A','a')
			&& ISCHR(*(p+12),'R','r') && ISCHR(*(p+13),'G','g') && ISCHR(*(p+14),'E','e') && ISCHR(*(p+15),'T','t')
			&& !ISSTRING(p, 16)) {
			p += 17;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			p++;
			result->lines += SKIPWS(&p);

			while (*p && *p != ';') {
				UChar *n = p;
				result->lines += SKIPTOWS(&n, ';', true);
				uint32_t c = (uint32_t)(n - p);
				u_strncpy(gbuffers[0], p, c);
				gbuffers[0][c] = 0;
				dieIfKeyword(gbuffers[0]);
				Tag *t = result->allocateTag(gbuffers[0]);
				t = result->addTag(t);
				result->preferred_targets.push_back(t->hash);
				p = n;
				result->lines += SKIPWS(&p);
			}

			if (result->preferred_targets.empty()) {
				u_fprintf(ux_stderr, "Error: PREFERRED-TARGETS declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(&p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
		}
		// SETRELATIONS
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+11),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o') && ISCHR(*(p+10),'N','n')
			&& !ISSTRING(p, 11)) {
			parseRule(K_SETRELATIONS, &p);
		}
		// REMRELATIONS
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+11),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o') && ISCHR(*(p+10),'N','n')
			&& !ISSTRING(p, 11)) {
			parseRule(K_REMRELATIONS, &p);
		}
		// SETRELATION
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+10),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o')
			&& !ISSTRING(p, 10)) {
			parseRule(K_SETRELATION, &p);
		}
		// REMRELATION
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+10),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o')
			&& !ISSTRING(p, 10)) {
			parseRule(K_REMRELATION, &p);
		}
		// SETPARENT
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+8),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'A','a') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'E','e')
			&& ISCHR(*(p+7),'N','n')
			&& !ISSTRING(p, 8)) {
			parseRule(K_SETPARENT, &p);
		}
		// SETCHILD
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+7),'D','d') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'C','c') && ISCHR(*(p+4),'H','h') && ISCHR(*(p+5),'I','i') && ISCHR(*(p+6),'L','l')
			&& !ISSTRING(p, 7)) {
			parseRule(K_SETCHILD, &p);
		}
		// SETS
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+3),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& !ISSTRING(p, 3)) {
			p += 4;
		}
		// LIST
		else if (ISCHR(*p,'L','l') && ISCHR(*(p+3),'T','t') && ISCHR(*(p+1),'I','i') && ISCHR(*(p+2),'S','s')
			&& !ISSTRING(p, 3)) {
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
			uint32_t sh = hash_sdbm_uchar(gbuffers[0]);
			p = n;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			p++;
			parseTagList(s, &p);
			s->rehash();
			Set *tmp = result->getSet(s->hash);
			if (tmp) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: LIST %S was defined twice with the same contents: Lines %u and %u.\n", s->name, tmp->line, s->line);
					u_fflush(ux_stderr);
				}
			}
			else if (tmp) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Set %S (L:%u) has been aliased to %S (L:%u).\n", s->name, s->line, tmp->name, tmp->line);
					u_fflush(ux_stderr);
				}
				result->set_alias[sh] = tmp->hash;
				result->destroySet(s);
				s = tmp;
			}
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
			s->rehash();
			Set *tmp = result->getSet(s->hash);
			if (tmp) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: SET %S was defined twice with the same contents: Lines %u and %u.\n", s->name, tmp->line, s->line);
					u_fflush(ux_stderr);
				}
			}
			else if (s->sets.size() == 1 && !s->is_unified) {
				tmp = result->getSet(s->sets.back());
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Set %S (L:%u) has been aliased to %S (L:%u).\n", s->name, s->line, tmp->name, tmp->line);
					u_fflush(ux_stderr);
				}
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
			&& ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'I','i') && ISCHR(*(p+5),'N','n') && ISCHR(*(p+6),'G','g')
			&& !ISSTRING(p, 7)) {
			p += 8;
			in_before_sections = true;
			in_section = false;
			in_after_sections = false;
		}
		// CORRECTIONS
		else if (ISCHR(*p,'C','c') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'R','r')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'C','c') && ISCHR(*(p+6),'T','t')
			&& ISCHR(*(p+7),'I','i') && ISCHR(*(p+8),'O','o') && ISCHR(*(p+9),'N','n')
			&& !ISSTRING(p, 10)) {
			p += 11;
			in_before_sections = true;
			in_section = false;
			in_after_sections = false;
		}
		// BEFORE-SECTIONS
		else if (ISCHR(*p,'B','b') && ISCHR(*(p+14),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'F','f')
			&& ISCHR(*(p+3),'O','o') && ISCHR(*(p+4),'R','r') && ISCHR(*(p+5),'E','e') && ISCHR(*(p+6),'-','_')
			&& ISCHR(*(p+7),'S','s') && ISCHR(*(p+8),'E','e') && ISCHR(*(p+9),'C','c') && ISCHR(*(p+10),'T','t')
			&& ISCHR(*(p+11),'I','i') && ISCHR(*(p+12),'O','o') && ISCHR(*(p+13),'N','n')
			&& !ISSTRING(p, 14)) {
			p += 15;
			in_before_sections = true;
			in_section = false;
			in_after_sections = false;
		}
		// SECTION
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+6),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'C','c')
			&& ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'I','i') && ISCHR(*(p+5),'O','o')
			&& !ISSTRING(p, 6)) {
			p += 7;
			result->sections.push_back(result->lines);
			in_before_sections = false;
			in_section = true;
			in_after_sections = false;
		}
		// CONSTRAINTS
		else if (ISCHR(*p,'C','c') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'N','n')
			&& ISCHR(*(p+3),'S','s') && ISCHR(*(p+4),'T','t') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'I','i') && ISCHR(*(p+8),'N','n') && ISCHR(*(p+9),'T','t')
			&& !ISSTRING(p, 10)) {
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
			&& ISCHR(*(p+10),'I','i') && ISCHR(*(p+11),'O','o') && ISCHR(*(p+12),'N','n')
			&& !ISSTRING(p, 13)) {
			p += 14;
			in_before_sections = false;
			in_section = false;
			in_after_sections = true;
		}
		// IFF
		else if (ISCHR(*p,'I','i') && ISCHR(*(p+2),'F','f') && ISCHR(*(p+1),'F','f')
			&& !ISSTRING(p, 2)) {
			parseRule(K_IFF, &p);
		}
		// MAP
		else if (ISCHR(*p,'M','m') && ISCHR(*(p+2),'P','p') && ISCHR(*(p+1),'A','a')
			&& !ISSTRING(p, 2)) {
			parseRule(K_MAP, &p);
		}
		// ADD
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+2),'D','d') && ISCHR(*(p+1),'D','d')
			&& !ISSTRING(p, 2)) {
			parseRule(K_ADD, &p);
		}
		// APPEND
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+5),'D','d') && ISCHR(*(p+1),'P','p') && ISCHR(*(p+2),'P','p')
			&& ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'N','n')
			&& !ISSTRING(p, 5)) {
			parseRule(K_APPEND, &p);
		}
		// SELECT
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+5),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
			&& ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'C','c')
			&& !ISSTRING(p, 5)) {
			parseRule(K_SELECT, &p);
		}
		// REMOVE
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+5),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'O','o') && ISCHR(*(p+4),'V','v')
			&& !ISSTRING(p, 5)) {
			parseRule(K_REMOVE, &p);
		}
		// REPLACE
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+6),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'P','p')
			&& ISCHR(*(p+3),'L','l') && ISCHR(*(p+4),'A','a') && ISCHR(*(p+5),'C','c')
			&& !ISSTRING(p, 6)) {
			parseRule(K_REPLACE, &p);
		}
		// DELIMIT
		else if (ISCHR(*p,'D','d') && ISCHR(*(p+6),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
			&& ISCHR(*(p+3),'I','i') && ISCHR(*(p+4),'M','m') && ISCHR(*(p+5),'I','i')
			&& !ISSTRING(p, 6)) {
			parseRule(K_DELIMIT, &p);
		}
		// SUBSTITUTE
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+9),'E','e') && ISCHR(*(p+1),'U','u') && ISCHR(*(p+2),'B','b')
			&& ISCHR(*(p+3),'S','s') && ISCHR(*(p+4),'T','t') && ISCHR(*(p+5),'I','i') && ISCHR(*(p+6),'T','t')
			&& ISCHR(*(p+7),'U','u') && ISCHR(*(p+8),'T','t')
			&& !ISSTRING(p, 9)) {
			parseRule(K_SUBSTITUTE, &p);
		}
		// TEMPLATE
		else if (ISCHR(*p,'T','t') && ISCHR(*(p+7),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'L','l') && ISCHR(*(p+5),'A','a') && ISCHR(*(p+6),'T','t')
			&& !ISSTRING(p, 7)) {
			ContextualTest *t = result->allocateContextualTest();
			t->line = result->lines;
			p += 8;
			result->lines += SKIPWS(&p);
			UChar *n = p;
			result->lines += SKIPTOWS(&n, 0, true);
			uint32_t c = (uint32_t)(n - p);
			u_strncpy(gbuffers[0], p, c);
			gbuffers[0][c] = 0;
			dieIfKeyword(gbuffers[0]);
			uint32_t cn = hash_sdbm_uchar(gbuffers[0]);
			t->name = cn;
			result->addContextualTest(t, gbuffers[0]);
			p = n;
			result->lines += SKIPWS(&p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			p++;

			parseContextualTestList(0, 0, 0, &p, t);

			result->lines += SKIPWS(&p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u! Probably caused by missing set operator.\n", *p, result->lines);
				CG3Quit(1);
			}
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
			if (*p == ';' || *p == '"' || *p == '<') {
				if (*p == '"') {
					p++;
					result->lines += SKIPTO_NOSPAN(&p, '"');
					if (*p != '"') {
						u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
						CG3Quit(1);
					}
				}
				result->lines += SKIPTOWS(&p);
			}
			if (*p && *p != ';' && *p != '"' && *p != '<' && !ISNL(*p) && !u_isWhitespace(*p)) {
				p[16] = 0;
				u_fprintf(ux_stderr, "Error: Garbage data '%S' encountered on line %u!\n", p, result->lines);
				CG3Quit(1);
			}
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

void TextualParser::setVerbosity(uint32_t level) {
	verbosity_level = level;
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
