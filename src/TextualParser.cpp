/*
* Copyright (C) 2007-2011, GrammarSoft ApS
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
#include "Strings.h"
#include "Grammar.h"
#include "ContextualTest.h"

namespace CG3 {

TextualParser::TextualParser(Grammar& res, UFILE *ux_err) {
	ux_stderr = ux_err;
	result = &res;
	filename = 0;
	locale = 0;
	codepage = 0;
	option_vislcg_compat = false;
	in_before_sections = false;
	in_after_sections = false;
	in_null_section = false;
	in_section = false;
	verbosity_level = 0;
	seen_mapping_prefix = 0;
	sets_counter = 100;
}

int TextualParser::parseTagList(UChar *& p, Set *s, const bool isinline) {
	if (isinline) {
		if (*p != '(') {
			u_fprintf(ux_stderr, "Error: Missing opening ( on line %u!\n", result->lines);
			CG3Quit(1);
		}
		++p;
	}
	while (*p && *p != ';' && *p != ')') {
		result->lines += SKIPWS(p, ';', ')');
		if (*p && *p != ';' && *p != ')') {
			if (*p == '(') {
				++p;
				TagVector tags;

				while (*p && *p != ';' && *p != ')') {
					UChar *n = p;
					if (*n == '"') {
						n++;
						result->lines += SKIPTO_NOSPAN(n, '"');
						if (*n != '"') {
							u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
							CG3Quit(1);
						}
					}
					result->lines += SKIPTOWS(n, ')', true);
					ptrdiff_t c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					Tag *t = result->allocateTag(&gbuffers[0][0]);
					tags.push_back(t);
					p = n;
					result->lines += SKIPWS(p, ';', ')');
				}
				if (*p != ')') {
					u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
					CG3Quit(1);
				}
				++p;

				if (tags.size() == 1) {
					result->addTagToSet(tags.back(), s);
				}
				else {
					CompositeTag *ct = result->allocateCompositeTag();
					foreach (TagVector, tags, tvi, tvi_end) {
						result->addTagToCompositeTag(*tvi, ct);
					}
					result->addCompositeTagToSet(s, ct);
				}
			}
			else {
				UChar *n = p;
				if (*n == '"') {
					n++;
					result->lines += SKIPTO_NOSPAN(n, '"');
					if (*n != '"') {
						u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
						CG3Quit(1);
					}
				}
				if (isinline) {
					result->lines += SKIPTOWS(n, ')', true);
				}
				else {
					result->lines += SKIPTOWS(n, 0, true);
				}
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				Tag *t = result->allocateTag(&gbuffers[0][0]);
				result->addTagToSet(t, s);
				p = n;
			}
		}
	}
	if (isinline) {
		if (*p != ')') {
			u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		++p;
	}
	return 0;
}

Set *TextualParser::parseSetInline(UChar *& p, Set *s) {
	uint32Vector set_ops;
	uint32Vector sets;

	bool wantop = false;
	while (*p && *p != ';' && *p != ')') {
		result->lines += SKIPWS(p, ';', ')');
		if (*p && *p != ';' && *p != ')') {
			if (!wantop) {
				if (*p == '(') {
					++p;
					Set *set_c = result->allocateSet();
					set_c->line = result->lines;
					set_c->setName(sets_counter++);
					TagVector tags;

					while (*p && *p != ';' && *p != ')') {
						result->lines += SKIPWS(p, ';', ')');
						UChar *n = p;
						if (*n == '"') {
							n++;
							result->lines += SKIPTO_NOSPAN(n, '"');
							if (*n != '"') {
								u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
								CG3Quit(1);
							}
						}
						result->lines += SKIPTOWS(n, ')', true);
						ptrdiff_t c = n - p;
						u_strncpy(&gbuffers[0][0], p, c);
						gbuffers[0][c] = 0;
						Tag *t = result->allocateTag(&gbuffers[0][0]);
						tags.push_back(t);
						p = n;
						result->lines += SKIPWS(p, ';', ')');
					}
					if (*p != ')') {
						u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
						CG3Quit(1);
					}
					++p;

					if (tags.size() == 1) {
						result->addTagToSet(tags.back(), set_c);
					}
					else {
						CompositeTag *ct = result->allocateCompositeTag();
						foreach (TagVector, tags, tvi, tvi_end) {
							result->addTagToCompositeTag(*tvi, ct);
						}
						result->addCompositeTagToSet(set_c, ct);
					}
					result->addSet(set_c);
					sets.push_back(set_c->hash);
				}
				else {
					UChar *n = p;
					result->lines += SKIPTOWS(n, ')', true);
					while (n[-1] == ',' || n[-1] == ']') {
						--n;
					}
					ptrdiff_t c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					Set *tmp = result->parseSet(&gbuffers[0][0]);
					uint32_t sh = tmp->hash;
					sets.push_back(sh);
					p = n;
				}

				if (!set_ops.empty() && (set_ops.back() == S_SET_ISECT_U || set_ops.back() == S_SET_SYMDIFF_U)) {
					const AnyTagSet a = result->getSet(sets.at(sets.size()-1))->getTagList(*result);
					const AnyTagSet b = result->getSet(sets.at(sets.size()-2))->getTagList(*result);

					AnyTagList r;
					if (set_ops.back() == S_SET_ISECT_U) {
						std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(r));
					}
					else if (set_ops.back() == S_SET_SYMDIFF_U) {
						std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(r));
					}

					set_ops.pop_back();
					sets.pop_back();
					sets.pop_back();

					Set *set_c = result->allocateSet();
					set_c->line = result->lines;
					set_c->setName(sets_counter++);
					foreach (AnyTagList, r, iter, iter_end) {
						if (iter->which == ANYTAG_TAG) {
							Tag *t = iter->getTag();
							result->addTagToSet(t, set_c);
						}
						else {
							CompositeTag *t = iter->getCompositeTag();
							result->addCompositeTagToSet(set_c, t);
						}
					}
					result->addSet(set_c);
					sets.push_back(set_c->hash);
				}

				wantop = true;
			}
			else {
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				//dieIfKeyword(&gbuffers[0][0]);
				int sop = ux_isSetOp(&gbuffers[0][0]);
				if (sop != S_IGNORE) {
					set_ops.push_back(sop);
					wantop = false;
					p = n;
				}
				else {
					break;
				}
			}
		}
	}

	if (s) {
		s->sets = sets;
		s->set_ops = set_ops;
	}
	else if (sets.size() == 1) {
		s = result->getSet(sets.back());
	}
	else {
		s = result->allocateSet();
		s->sets = sets;
		s->set_ops = set_ops;
	}
	return s;
}

Set *TextualParser::parseSetInlineWrapper(UChar *& p) {
	uint32_t tmplines = result->lines;
	Set *s = parseSetInline(p);
	if (!s->line) {
		s->line = tmplines;
	}
	if (s->name.empty()) {
		s->setName(sets_counter++);
	}
	result->addSet(s);
	return s;
}

int TextualParser::parseContextualTestPosition(UChar *& p, ContextualTest& t) {
	bool negative = false;
	bool had_digits = false;

	size_t tries;
	for (tries=0 ; *p != ' ' && *p != '(' && tries < 100 ; ++tries) {
		if (*p == '*' && *(p+1) == '*') {
			t.pos |= POS_SCANALL;
			p += 2;
		}
		if (*p == '*') {
			t.pos |= POS_SCANFIRST;
			++p;
		}
		if (*p == 'C') {
			t.pos |= POS_CAREFUL;
			++p;
		}
		if (*p == 'c') {
			t.pos |= POS_DEP_CHILD;
			++p;
		}
		if (*p == 'p') {
			t.pos |= POS_DEP_PARENT;
			++p;
		}
		if (*p == 's') {
			t.pos |= POS_DEP_SIBLING;
			++p;
		}
		if (*p == 'S') {
			t.pos |= POS_SELF;
			++p;
		}
		if (*p == '<') {
			t.pos |= POS_SPAN_LEFT;
			++p;
		}
		if (*p == '>') {
			t.pos |= POS_SPAN_RIGHT;
			++p;
		}
		if (*p == 'W') {
			t.pos |= POS_SPAN_BOTH;
			++p;
		}
		if (*p == '@') {
			t.pos |= POS_ABSOLUTE;
			++p;
		}
		if (*p == 'O') {
			t.pos |= POS_NO_PASS_ORIGIN;
			++p;
		}
		if (*p == 'o') {
			t.pos |= POS_PASS_ORIGIN;
			++p;
		}
		if (*p == 'L') {
			t.pos |= POS_LEFT_PAR;
			++p;
		}
		if (*p == 'R') {
			t.pos |= POS_RIGHT_PAR;
			++p;
		}
		if (*p == 'X') {
			t.pos |= POS_MARK_SET;
			++p;
		}
		if (*p == 'x') {
			t.pos |= POS_MARK_JUMP;
			++p;
		}
		if (*p == 'D') {
			t.pos |= POS_LOOK_DELETED;
			++p;
		}
		if (*p == 'd') {
			t.pos |= POS_LOOK_DELAYED;
			++p;
		}
		if (*p == 'A') {
			t.pos |= POS_ATTACH_TO;
			++p;
		}
		if (*p == '?') {
			t.pos |= POS_UNKNOWN;
			++p;
		}
		if (*p == '-') {
			negative = true;
			++p;
		}
		if (u_isdigit(*p)) {
			had_digits = true;
			while (*p >= '0' && *p <= '9') {
				t.offset = (t.offset*10) + (*p - '0');
				++p;
			}
		}
		if (*p == 'r' && *(p+1) == ':') {
			t.pos |= POS_RELATION;
			p += 2;
			UChar *n = p;
			SKIPTOWS(n, '(');
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			Tag *tag = result->allocateTag(&gbuffers[0][0], true);
			t.relation = tag->hash;
			p = n;
		}
	}

	if (negative) {
		t.offset = (-1) * abs(t.offset);
	}

	if ((t.pos & (POS_DEP_CHILD|POS_DEP_SIBLING)) && (t.pos & (POS_SCANFIRST|POS_SCANALL))) {
		t.pos &= ~POS_SCANFIRST;
		t.pos &= ~POS_SCANALL;
		t.pos |= POS_DEP_DEEP;
	}
	if ((t.pos & (POS_DEP_CHILD|POS_DEP_SIBLING)) && (t.pos & POS_CAREFUL)) {
		u_fprintf(ux_stderr, "Warning: Deprecated conversion from C to ALL on line %u.\n", result->lines);
		t.pos &= ~POS_CAREFUL;
		t.pos |= POS_ALL;
	}
	if ((t.pos & (POS_DEP_CHILD|POS_DEP_SIBLING)) && (t.pos & POS_NOT)) {
		u_fprintf(ux_stderr, "Warning: Deprecated conversion from NOT to NONE on line %u.\n", result->lines);
		t.pos &= ~POS_NOT;
		t.pos |= POS_NONE;
	}
	if ((t.pos & POS_RELATION) && (t.pos & POS_CAREFUL)) {
		u_fprintf(ux_stderr, "Warning: Deprecated conversion from C to ALL on line %u.\n", result->lines);
		t.pos &= ~POS_CAREFUL;
		t.pos |= POS_ALL;
	}
	if ((t.pos & POS_RELATION) && (t.pos & POS_NOT)) {
		u_fprintf(ux_stderr, "Warning: Deprecated conversion from NOT to NONE on line %u.\n", result->lines);
		t.pos &= ~POS_NOT;
		t.pos |= POS_NONE;
	}

	if (tries >= 5) {
		u_fprintf(ux_stderr, "Warning: Position on line %u took many loops.\n", result->lines);
	}
	if (tries >= 100) {
		u_fprintf(ux_stderr, "Error: Invalid position on line %u - caused endless loop!\n", result->lines);
		CG3Quit(1);
	}
	if (had_digits) {
		if (t.pos & (POS_DEP_CHILD|POS_DEP_SIBLING|POS_DEP_PARENT)) {
			u_fprintf(ux_stderr, "Error: Invalid position on line %u - cannot combine offsets with dependency!\n", result->lines);
			CG3Quit(1);
		}
		if (t.pos & (POS_LEFT_PAR|POS_RIGHT_PAR)) {
			u_fprintf(ux_stderr, "Error: Invalid position on line %u - cannot combine offsets with enclosures!\n", result->lines);
			CG3Quit(1);
		}
		if (t.pos & POS_RELATION) {
			u_fprintf(ux_stderr, "Error: Invalid position on line %u - cannot combine offsets with relations!\n", result->lines);
			CG3Quit(1);
		}
	}
	if ((t.pos & (POS_LEFT_PAR|POS_RIGHT_PAR)) && (t.pos & (POS_SCANFIRST|POS_SCANALL))) {
		u_fprintf(ux_stderr, "Error: Invalid position on line %u - cannot have both enclosure and scan!\n", result->lines);
		CG3Quit(1);
	}
	if ((t.pos & POS_PASS_ORIGIN) && (t.pos & POS_NO_PASS_ORIGIN)) {
		u_fprintf(ux_stderr, "Error: Invalid position on line %u - cannot have both O and o!\n", result->lines);
		CG3Quit(1);
	}
	if ((t.pos & POS_LEFT_PAR) && (t.pos & POS_RIGHT_PAR)) {
		u_fprintf(ux_stderr, "Error: Invalid position on line %u - cannot have both L and R!\n", result->lines);
		CG3Quit(1);
	}
	if ((t.pos & POS_ALL) && (t.pos & POS_NONE)) {
		u_fprintf(ux_stderr, "Error: Invalid position on line %u - cannot have both NONE and ALL!\n", result->lines);
		CG3Quit(1);
	}
	if ((t.pos & POS_UNKNOWN) && (t.pos != POS_UNKNOWN || had_digits)) {
		u_fprintf(ux_stderr, "Error: Invalid position on line %u - '?' cannot be combined with anything else!\n", result->lines);
		CG3Quit(1);
	}

	return 0;
}

int TextualParser::parseContextualTestList(UChar *& p, Rule *rule, ContextualTest **head, CG3::ContextualTest *parentTest, CG3::ContextualTest *self) {
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
	bool negated = false, negative = false, all = false, none = false;

	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_TEXTNEGATE].getTerminatedBuffer(), stringbits[S_TEXTNEGATE].length())) {
		p += stringbits[S_TEXTNEGATE].length();
		negated = true;
	}
	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_ALL].getTerminatedBuffer(), stringbits[S_ALL].length())) {
		p += stringbits[S_ALL].length();
		all = true;
	}
	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_NONE].getTerminatedBuffer(), stringbits[S_NONE].length())) {
		p += stringbits[S_NONE].length();
		none = true;
	}
	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_TEXTNOT].getTerminatedBuffer(), stringbits[S_TEXTNOT].length())) {
		p += stringbits[S_TEXTNOT].length();
		negative = true;
	}
	result->lines += SKIPWS(p);

	UChar *n = p;
	result->lines += SKIPTOWS(n, '(');
	ptrdiff_t c = n - p;
	u_strncpy(&gbuffers[0][0], p, c);
	gbuffers[0][c] = 0;
	if (ux_isEmpty(&gbuffers[0][0])) {
		p = n;
		for (;;) {
			ContextualTest *ored = t->allocateContextualTest();
			if (*p != '(') {
				u_fprintf(ux_stderr, "Error: Expected '(' but found '%C' on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			parseContextualTestList(p, rule, 0, 0, ored);
			++p;
			t->ors.push_back(ored);
			result->lines += SKIPWS(p);
			if (ux_simplecasecmp(p, stringbits[S_OR].getTerminatedBuffer(), stringbits[S_OR].length())) {
				p += stringbits[S_OR].length();
			}
			else {
				break;
			}
			result->lines += SKIPWS(p);
		}
	}
	else if (gbuffers[0][0] == '[') {
		++p;
		result->lines += SKIPWS(p);
		Set *s = parseSetInlineWrapper(p);
		t->offset = 1;
		t->target = s->hash;
		result->lines += SKIPWS(p);
		while (*p == ',') {
			++p;
			result->lines += SKIPWS(p);
			ContextualTest *lnk = t->allocateContextualTest();
			Set *s = parseSetInlineWrapper(p);
			lnk->offset = 1;
			lnk->target = s->hash;
			t->linked = lnk;
			t = lnk;
			result->lines += SKIPWS(p);
		}
		if (*p != ']') {
			u_fprintf(ux_stderr, "Error: Expected ']' but found '%C' on line %u!\n", *p, result->lines);
			CG3Quit(1);
		}
		++p;
	}
	else if (gbuffers[0][0] == 'T' && gbuffers[0][1] == ':') {
		goto label_parseTemplateRef;
	}
	else {
		if (all) {
			t->pos |= POS_ALL;
		}
		if (none) {
			t->pos |= POS_NONE;
		}
		if (negated) {
			t->pos |= POS_NEGATE;
		}
		if (negative) {
			t->pos |= POS_NOT;
		}
		parseContextualTestPosition(p, *t);
		p = n;
		if (t->pos & (POS_DEP_CHILD|POS_DEP_PARENT|POS_DEP_SIBLING)) {
			result->has_dep = true;
		}
		result->lines += SKIPWS(p);

		if (p[0] == 'T' && p[1] == ':') {
			t->pos |= POS_TMPL_OVERRIDE;
label_parseTemplateRef:
			p += 2;
			n = p;
			result->lines += SKIPTOWS(n, ')');
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			uint32_t cn = hash_sdbm_uchar(&gbuffers[0][0]);
			if (result->templates.find(cn) == result->templates.end()) {
				u_fprintf(ux_stderr, "Error: Unknown template '%S' referenced on line %u!\n", &gbuffers[0][0], result->lines);
				CG3Quit(1);
			}
			t->tmpl = result->templates.find(cn)->second;
			p = n;
			result->lines += SKIPWS(p);
		}
		else {
			Set *s = parseSetInlineWrapper(p);
			t->target = s->hash;
		}

		result->lines += SKIPWS(p);
		if (ux_simplecasecmp(p, stringbits[S_CBARRIER].getTerminatedBuffer(), stringbits[S_CBARRIER].length())) {
			p += stringbits[S_CBARRIER].length();
			result->lines += SKIPWS(p);
			Set *s = parseSetInlineWrapper(p);
			t->cbarrier = s->hash;
		}
		result->lines += SKIPWS(p);
		if (ux_simplecasecmp(p, stringbits[S_BARRIER].getTerminatedBuffer(), stringbits[S_BARRIER].length())) {
			p += stringbits[S_BARRIER].length();
			result->lines += SKIPWS(p);
			Set *s = parseSetInlineWrapper(p);
			t->barrier = s->hash;
		}
		result->lines += SKIPWS(p);
	}

	bool linked = false;
	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_AND].getTerminatedBuffer(), stringbits[S_AND].length())) {
		u_fprintf(ux_stderr, "Error: 'AND' is deprecated; use 'LINK 0' or operator '+' instead. Found on line %u!\n", result->lines);
		CG3Quit(1);
	}
	if (ux_simplecasecmp(p, stringbits[S_LINK].getTerminatedBuffer(), stringbits[S_LINK].length())) {
		p += stringbits[S_LINK].length();
		linked = true;
	}
	result->lines += SKIPWS(p);

	if (linked) {
		if (t->pos & POS_NONE) {
			u_fprintf(ux_stderr, "Error: It does not make sense to LINK from a NONE test; perhaps you meant NOT or NEGATE on line %u?\n", result->lines);
			CG3Quit(1);
		}
		parseContextualTestList(p, rule, 0, t);
	}

	if (self) {
		// nothing...
	}
	else if (parentTest) {
		parentTest->linked = t;
	}
	else {
		if (option_vislcg_compat && t->pos & POS_NOT) {
			t->pos &= ~POS_NOT;
			t->pos |= POS_NEGATE;
		}
		rule->addContextualTest(t, head);
	}
	if (rule) {
		if (rule->flags & RF_LOOKDELETED) {
			t->pos |= POS_LOOK_DELETED;
		}
		if (rule->flags & RF_LOOKDELAYED) {
			t->pos |= POS_LOOK_DELAYED;
		}
	}
	t->rehash();
	return 0;
}

int TextualParser::parseContextualTests(UChar *& p, Rule *rule) {
	return parseContextualTestList(p, rule, &rule->test_head, 0);
}

int TextualParser::parseContextualDependencyTests(UChar *& p, Rule *rule) {
	return parseContextualTestList(p, rule, &rule->dep_test_head, 0);
}

int TextualParser::parseRule(UChar *& p, KEYWORDS key) {
	Rule *rule = result->allocateRule();
	rule->line = result->lines;
	rule->type = key;

	UChar *lp = p;
	BACKTONL(lp);
	result->lines += SKIPWS(lp);

	if (lp != p && lp < p) {
		UChar *n = lp;
		if (*n == '"') {
			n++;
			result->lines += SKIPTO_NOSPAN(n, '"');
			if (*n != '"') {
				u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		result->lines += SKIPTOWS(n, 0, true);
		ptrdiff_t c = n - lp;
		u_strncpy(&gbuffers[0][0], lp, c);
		gbuffers[0][c] = 0;
		Tag *wform = result->allocateTag(&gbuffers[0][0]);
		rule->wordform = wform->hash;
	}

	p += keywords[key].length();
	result->lines += SKIPWS(p);

	if (*p == ':') {
		++p;
		UChar *n = p;
		result->lines += SKIPTOWS(n, '(');
		ptrdiff_t c = n - p;
		u_strncpy(&gbuffers[0][0], p, c);
		gbuffers[0][c] = 0;
		rule->setName(&gbuffers[0][0]);
		p = n;
	}
	result->lines += SKIPWS(p);

	bool setflag = true;
	while (setflag) {
		setflag = false;
		for (uint32_t i=0 ; i<FLAGS_COUNT ; i++) {
			if (ux_simplecasecmp(p, flags[i].getTerminatedBuffer(), flags[i].length())) {
				p += flags[i].length();
				rule->flags |= (1 << i);
				setflag = true;
			}
			result->lines += SKIPWS(p);
		}
	}
	// ToDo: ENCL_* are exclusive...detect multiple of them better.
	if (rule->flags & RF_ENCL_OUTER && rule->flags & RF_ENCL_INNER) {
		u_fprintf(ux_stderr, "Error: Line %u: ENCL_OUTER and ENCL_INNER are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}
	if (rule->flags & RF_KEEPORDER && rule->flags & RF_VARYORDER) {
		u_fprintf(ux_stderr, "Error: Line %u: KEEPORDER and VARYORDER are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}
	if (rule->flags & RF_REMEMBERX && rule->flags & RF_RESETX) {
		u_fprintf(ux_stderr, "Error: Line %u: REMEMBERX and RESETX are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}
	if (rule->flags & RF_NEAREST && rule->flags & RF_ALLOWLOOP) {
		u_fprintf(ux_stderr, "Error: Line %u: NEAREST and ALLOWLOOP are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}
	if (rule->flags & RF_UNSAFE && rule->flags & RF_SAFE) {
		u_fprintf(ux_stderr, "Error: Line %u: SAFE and UNSAFE are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}
	if (rule->flags & RF_UNMAPLAST && rule->flags & RF_SAFE) {
		u_fprintf(ux_stderr, "Error: Line %u: SAFE and UNMAPLAST are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}
	if (rule->flags & RF_DELAYED && rule->flags & RF_IMMEDIATE) {
		u_fprintf(ux_stderr, "Error: Line %u: IMMEDIATE and DELAYED are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}
	if (rule->flags & RF_WITHCHILD && rule->flags & RF_NOCHILD) {
		u_fprintf(ux_stderr, "Error: Line %u: WITHCHILD and NOCHILD are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}
	if (rule->flags & RF_ITERATE && rule->flags & RF_NOITERATE) {
		u_fprintf(ux_stderr, "Error: Line %u: ITERATE and NOITERATE are mutually exclusive!\n", result->lines);
		CG3Quit(1);
	}

	if (!(rule->flags & (RF_ITERATE|RF_NOITERATE))) {
		if (key != K_SELECT && key != K_REMOVE && key != K_IFF
			&& key != K_DELIMIT && key != K_REMCOHORT
			&& key != K_MOVE && key != K_SWITCH) {
			rule->flags |= RF_NOITERATE;
		}
	}
	if (key == K_UNMAP && !(rule->flags & (RF_SAFE|RF_UNSAFE))) {
		rule->flags |= RF_SAFE;
	}
	if (rule->flags & RF_UNMAPLAST) {
		rule->flags |= RF_UNSAFE;
	}
	if (rule->flags & RF_ENCL_FINAL) {
		result->has_encl_final = true;
	}
	result->lines += SKIPWS(p);

	if (rule->flags & RF_WITHCHILD) {
		result->has_dep = true;
		Set *s = parseSetInlineWrapper(p);
		rule->childset1 = s->hash;
		result->lines += SKIPWS(p);
	}
	else if (rule->flags & RF_NOCHILD) {
		rule->childset1 = 0;
	}

	if (key == K_JUMP || key == K_EXECUTE) {
		UChar *n = p;
		result->lines += SKIPTOWS(n, '(');
		if (!u_isalnum(*p)) {
			u_fprintf(ux_stderr, "Error: Anchor name for %S must be alphanumeric on line %u!\n", keywords[key].getTerminatedBuffer(), result->lines);
			CG3Quit(1);
		}
		ptrdiff_t c = n - p;
		u_strncpy(&gbuffers[0][0], p, c);
		gbuffers[0][c] = 0;
		uint32_t sh = hash_sdbm_uchar(&gbuffers[0][0]);
		rule->jumpstart = sh;
		p = n;
	}
	result->lines += SKIPWS(p);

	if (key == K_EXECUTE) {
		UChar *n = p;
		result->lines += SKIPTOWS(n, '(');
		if (!u_isalnum(*p)) {
			u_fprintf(ux_stderr, "Error: Anchor name for %S must be alphanumeric on line %u!\n", keywords[key].getTerminatedBuffer(), result->lines);
			CG3Quit(1);
		}
		ptrdiff_t c = n - p;
		u_strncpy(&gbuffers[0][0], p, c);
		gbuffers[0][c] = 0;
		uint32_t sh = hash_sdbm_uchar(&gbuffers[0][0]);
		rule->jumpend = sh;
		p = n;
	}
	result->lines += SKIPWS(p);

	if (key == K_SUBSTITUTE) {
		Set *s = parseSetInlineWrapper(p);
		s->reindex(*result);
		rule->sublist = s;
		if (s->empty()) {
			u_fprintf(ux_stderr, "Error: Empty substitute set on line %u!\n", result->lines);
			CG3Quit(1);
		}
		if (s->tags_list.empty() && !(s->type & (ST_TAG_UNIFY|ST_SET_UNIFY|ST_CHILD_UNIFY))) {
			u_fprintf(ux_stderr, "Error: Substitute set on line %u was neither unified nor of LIST type!\n", result->lines);
			CG3Quit(1);
		}
	}

	result->lines += SKIPWS(p);
	if (key == K_MAP || key == K_ADD || key == K_REPLACE || key == K_APPEND || key == K_SUBSTITUTE || key == K_COPY
	|| key == K_ADDRELATIONS || key == K_ADDRELATION
	|| key == K_SETRELATIONS || key == K_SETRELATION
	|| key == K_REMRELATIONS || key == K_REMRELATION) {
		Set *s = parseSetInlineWrapper(p);
		s->reindex(*result);
		rule->maplist = s;
		if (s->empty()) {
			u_fprintf(ux_stderr, "Error: Empty mapping set on line %u!\n", result->lines);
			CG3Quit(1);
		}
		if (s->tags_list.empty() && !(s->type & (ST_TAG_UNIFY|ST_SET_UNIFY|ST_CHILD_UNIFY))) {
			u_fprintf(ux_stderr, "Error: Mapping set on line %u was neither unified nor of LIST type!\n", result->lines);
			CG3Quit(1);
		}
	}

	result->lines += SKIPWS(p);
	if (key == K_ADDRELATIONS || key == K_SETRELATIONS || key == K_REMRELATIONS) {
		Set *s = parseSetInlineWrapper(p);
		s->reindex(*result);
		rule->sublist = s;
		if (s->empty()) {
			u_fprintf(ux_stderr, "Error: Empty relation set on line %u!\n", result->lines);
			CG3Quit(1);
		}
		if (s->tags_list.empty() && !(s->type & (ST_TAG_UNIFY|ST_SET_UNIFY|ST_CHILD_UNIFY))) {
			u_fprintf(ux_stderr, "Error: Relation set on line %u was neither unified nor of LIST type!\n", result->lines);
			CG3Quit(1);
		}
	}

	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_TARGET].getTerminatedBuffer(), stringbits[S_TARGET].length())) {
		p += stringbits[S_TARGET].length();
	}
	result->lines += SKIPWS(p);

	Set *s = parseSetInlineWrapper(p);
	rule->target = s->hash;

	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_IF].getTerminatedBuffer(), stringbits[S_IF].length())) {
		p += stringbits[S_IF].length();
	}
	result->lines += SKIPWS(p);

	while (*p && *p == '(') {
		++p;
		result->lines += SKIPWS(p);
		parseContextualTests(p, rule);
		result->lines += SKIPWS(p);
		if (*p != ')') {
			u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
			CG3Quit(1);
		}
		++p;
		result->lines += SKIPWS(p);
	}

	if (key == K_SETPARENT || key == K_SETCHILD
	|| key == K_ADDRELATIONS || key == K_ADDRELATION
	|| key == K_SETRELATIONS || key == K_SETRELATION
	|| key == K_REMRELATIONS || key == K_REMRELATION
	|| key == K_MOVE || key == K_SWITCH) {
		result->lines += SKIPWS(p);
		if (key == K_MOVE) {
			if (ux_simplecasecmp(p, stringbits[S_AFTER].getTerminatedBuffer(), stringbits[S_AFTER].length())) {
				p += stringbits[S_AFTER].length();
				rule->type = K_MOVE_AFTER;
			}
			else if (ux_simplecasecmp(p, stringbits[S_BEFORE].getTerminatedBuffer(), stringbits[S_BEFORE].length())) {
				p += stringbits[S_BEFORE].length();
				rule->type = K_MOVE_BEFORE;
			}
			else {
				u_fprintf(ux_stderr, "Error: Missing movement keyword AFTER or BEFORE on line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		else if (key == K_SWITCH) {
			if (ux_simplecasecmp(p, stringbits[S_WITH].getTerminatedBuffer(), stringbits[S_WITH].length())) {
				p += stringbits[S_WITH].length();
			}
			else {
				u_fprintf(ux_stderr, "Error: Missing movement keyword WITH on line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		else {
			if (ux_simplecasecmp(p, stringbits[S_TO].getTerminatedBuffer(), stringbits[S_TO].length())) {
				p += stringbits[S_TO].length();
			}
			else {
				u_fprintf(ux_stderr, "Error: Missing dependency keyword TO on line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		result->lines += SKIPWS(p);

		if (key == K_MOVE) {
			if (ux_simplecasecmp(p, flags[FL_WITHCHILD].getTerminatedBuffer(), flags[FL_WITHCHILD].length())) {
				p += flags[FL_WITHCHILD].length();
				result->has_dep = true;
				Set *s = parseSetInlineWrapper(p);
				rule->childset2 = s->hash;
				result->lines += SKIPWS(p);
			}
			else if (ux_simplecasecmp(p, flags[FL_NOCHILD].getTerminatedBuffer(), flags[FL_NOCHILD].length())) {
				p += flags[FL_NOCHILD].length();
				rule->childset2 = 0;
				result->lines += SKIPWS(p);
			}
		}

		while (*p && *p == '(') {
			++p;
			result->lines += SKIPWS(p);
			parseContextualDependencyTests(p, rule);
			result->lines += SKIPWS(p);
			if (*p != ')') {
				u_fprintf(ux_stderr, "Error: Missing closing ) on line %u!\n", result->lines);
				CG3Quit(1);
			}
			++p;
			result->lines += SKIPWS(p);
		}
		if (!rule->dep_test_head) {
			u_fprintf(ux_stderr, "Error: Missing dependency target on line %u!\n", result->lines);
			CG3Quit(1);
		}
		rule->dep_target = rule->dep_test_head;
		while (rule->dep_target->next) {
			rule->dep_target = rule->dep_target->next;
		}
		rule->dep_target->detach();
		if (rule->dep_target == rule->dep_test_head) {
			rule->dep_test_head = 0;
		}
	}
	if (key == K_SETPARENT || key == K_SETCHILD) {
		result->has_dep = true;
	}

	rule->reverseContextualTests();
	addRuleToGrammar(rule);
	return 0;
}

int TextualParser::parseFromUChar(UChar *input, const char *fname) {
	if (!input || !input[0]) {
		u_fprintf(ux_stderr, "Error: Input is empty - cannot continue!\n");
		CG3Quit(1);
	}

	UChar *p = input;
	result->lines = 1;

	while (*p) {
		if (verbosity_level > 0 && result->lines % 500 == 0) {
			std::cerr << "Parsing line " << result->lines << "          \r" << std::flush;
		}
		result->lines += SKIPWS(p);
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
			result->delimiters->setName(stringbits[S_DELIMITSET].getTerminatedBuffer());
			p += 10;
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			parseTagList(p, result->delimiters);
			result->addSet(result->delimiters);
			if (result->delimiters->tags.empty() && result->delimiters->single_tags.empty()) {
				u_fprintf(ux_stderr, "Error: DELIMITERS declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
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
			result->soft_delimiters->setName(stringbits[S_SOFTDELIMITSET].getTerminatedBuffer());
			p += 15;
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			parseTagList(p, result->soft_delimiters);
			result->addSet(result->soft_delimiters);
			if (result->soft_delimiters->tags.empty() && result->soft_delimiters->single_tags.empty()) {
				u_fprintf(ux_stderr, "Error: SOFT-DELIMITERS declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
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
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			result->lines += SKIPWS(p);

			UChar *n = p;
			result->lines += SKIPTOWS(n, ';');
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			p = n;

			result->mapping_prefix = gbuffers[0][0];

			if (!result->mapping_prefix) {
				u_fprintf(ux_stderr, "Error: MAPPING-PREFIX declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
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
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			result->lines += SKIPWS(p);

			while (*p && *p != ';') {
				UChar *n = p;
				if (*n == '"') {
					n++;
					result->lines += SKIPTO_NOSPAN(n, '"');
					if (*n != '"') {
						u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
						CG3Quit(1);
					}
				}
				result->lines += SKIPTOWS(n, ';', true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				Tag *t = result->allocateTag(&gbuffers[0][0]);
				result->preferred_targets.push_back(t->hash);
				p = n;
				result->lines += SKIPWS(p);
			}

			if (result->preferred_targets.empty()) {
				u_fprintf(ux_stderr, "Error: PREFERRED-TARGETS declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		// STATIC-SETS
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'T','t') && ISCHR(*(p+2),'A','a')
			&& ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'I','i') && ISCHR(*(p+5),'C','c') && ISCHR(*(p+6),'-','-')
			&& ISCHR(*(p+7),'S','s') && ISCHR(*(p+8),'E','e') && ISCHR(*(p+9),'T','t')
			&& !ISSTRING(p, 10)) {
			p += 11;
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			result->lines += SKIPWS(p);

			while (*p && *p != ';') {
				UChar *n = p;
				result->lines += SKIPTOWS(n, ';', true);
				const UString s(p, n);
				result->static_sets.push_back(s);
				p = n;
				result->lines += SKIPWS(p);
			}

			if (result->static_sets.empty()) {
				u_fprintf(ux_stderr, "Error: STATIC-SETS declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		// ADDRELATIONS
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+11),'S','s') && ISCHR(*(p+1),'D','d') && ISCHR(*(p+2),'D','d')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o') && ISCHR(*(p+10),'N','n')
			&& !ISSTRING(p, 11)) {
			parseRule(p, K_ADDRELATIONS);
		}
		// SETRELATIONS
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+11),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o') && ISCHR(*(p+10),'N','n')
			&& !ISSTRING(p, 11)) {
			parseRule(p, K_SETRELATIONS);
		}
		// REMRELATIONS
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+11),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o') && ISCHR(*(p+10),'N','n')
			&& !ISSTRING(p, 11)) {
			parseRule(p, K_REMRELATIONS);
		}
		// ADDRELATION
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+10),'N','n') && ISCHR(*(p+1),'D','d') && ISCHR(*(p+2),'D','d')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o')
			&& !ISSTRING(p, 10)) {
			parseRule(p, K_ADDRELATION);
		}
		// SETRELATION
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+10),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o')
			&& !ISSTRING(p, 10)) {
			parseRule(p, K_SETRELATION);
		}
		// REMRELATION
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+10),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
			&& ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o')
			&& !ISSTRING(p, 10)) {
			parseRule(p, K_REMRELATION);
		}
		// SETPARENT
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+8),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'A','a') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'E','e')
			&& ISCHR(*(p+7),'N','n')
			&& !ISSTRING(p, 8)) {
			parseRule(p, K_SETPARENT);
		}
		// SETCHILD
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+7),'D','d') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
			&& ISCHR(*(p+3),'C','c') && ISCHR(*(p+4),'H','h') && ISCHR(*(p+5),'I','i') && ISCHR(*(p+6),'L','l')
			&& !ISSTRING(p, 7)) {
			parseRule(p, K_SETCHILD);
		}
		// REMCOHORT
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+8),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'C','c') && ISCHR(*(p+4),'O','o') && ISCHR(*(p+5),'H','h') && ISCHR(*(p+6),'O','o')
			&& ISCHR(*(p+7),'R','r')
			&& !ISSTRING(p, 8)) {
			parseRule(p, K_REMCOHORT);
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
			result->lines += SKIPWS(p);
			UChar *n = p;
			result->lines += SKIPTOWS(n, 0, true);
			while (n[-1] == ',' || n[-1] == ']') {
				--n;
			}
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			s->setName(&gbuffers[0][0]);
			p = n;
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			parseTagList(p, s);
			s->rehash();
			Set *tmp = result->getSet(s->hash);
			if (tmp) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: LIST %S was defined twice with the same contents: Lines %u and %u.\n", s->name.c_str(), tmp->line, s->line);
					u_fflush(ux_stderr);
				}
			}
			result->addSet(s);
			if (s->tags.empty() && s->single_tags.empty() && s->sets.empty()) {
				u_fprintf(ux_stderr, "Error: LIST %S declared, but no definitions given, on line %u!\n", s->name.c_str(), result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		// SET
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+2),'T','t') && ISCHR(*(p+1),'E','e')
			&& !ISSTRING(p, 2)) {
			Set *s = result->allocateSet();
			s->line = result->lines;
			p += 3;
			result->lines += SKIPWS(p);
			UChar *n = p;
			result->lines += SKIPTOWS(n, 0, true);
			while (n[-1] == ',' || n[-1] == ']') {
				--n;
			}
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			s->setName(&gbuffers[0][0]);
			uint32_t sh = hash_sdbm_uchar(&gbuffers[0][0]);
			p = n;
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			parseSetInline(p, s);
			s->rehash();
			Set *tmp = result->getSet(s->hash);
			if (tmp) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: SET %S was defined twice with the same contents: Lines %u and %u.\n", s->name.c_str(), tmp->line, s->line);
					u_fflush(ux_stderr);
				}
			}
			else if (s->sets.size() == 1 && !(s->type & ST_TAG_UNIFY)) {
				tmp = result->getSet(s->sets.back());
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Set %S (L:%u) has been aliased to %S (L:%u).\n", s->name.c_str(), s->line, tmp->name.c_str(), tmp->line);
					u_fflush(ux_stderr);
				}
				result->set_alias[sh] = tmp->hash;
				result->destroySet(s);
				s = tmp;
			}
			result->addSet(s);
			if (s->sets.empty() && s->tags.empty() && s->single_tags.empty()) {
				u_fprintf(ux_stderr, "Error: SET %S declared, but no definitions given, on line %u!\n", s->name.c_str(), result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u! Probably caused by missing set operator.\n", result->lines);
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
			in_null_section = false;
			UChar *s = p;
			SKIPLN(s);
			SKIPWS(s);
			result->lines += SKIPWS(p);
			if (p != s) {
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				result->addAnchor(&gbuffers[0][0], result->lines);
				p = n;
			}
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
			in_null_section = false;
			UChar *s = p;
			SKIPLN(s);
			SKIPWS(s);
			result->lines += SKIPWS(p);
			if (p != s) {
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				result->addAnchor(&gbuffers[0][0], result->lines);
				p = n;
			}
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
			in_null_section = false;
			UChar *s = p;
			SKIPLN(s);
			SKIPWS(s);
			result->lines += SKIPWS(p);
			if (p != s) {
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				result->addAnchor(&gbuffers[0][0], result->lines);
				p = n;
			}
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
			in_null_section = false;
			UChar *s = p;
			SKIPLN(s);
			SKIPWS(s);
			result->lines += SKIPWS(p);
			if (p != s) {
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				result->addAnchor(&gbuffers[0][0], result->lines);
				p = n;
			}
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
			in_null_section = false;
			UChar *s = p;
			SKIPLN(s);
			SKIPWS(s);
			result->lines += SKIPWS(p);
			if (p != s) {
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				result->addAnchor(&gbuffers[0][0], result->lines);
				p = n;
			}
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
			in_null_section = false;
			UChar *s = p;
			SKIPLN(s);
			SKIPWS(s);
			result->lines += SKIPWS(p);
			if (p != s) {
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				result->addAnchor(&gbuffers[0][0], result->lines);
				p = n;
			}
		}
		// NULL-SECTION
		else if (ISCHR(*p,'N','n') && ISCHR(*(p+11),'N','n') && ISCHR(*(p+1),'U','u') && ISCHR(*(p+2),'L','l')
			&& ISCHR(*(p+3),'L','l') && ISCHR(*(p+4),'-','-') && ISCHR(*(p+5),'S','s')
			&& ISCHR(*(p+6),'E','e') && ISCHR(*(p+7),'C','c') && ISCHR(*(p+8),'T','t') && ISCHR(*(p+9),'I','i')
			&& ISCHR(*(p+10),'O','o')
			&& !ISSTRING(p, 11)) {
			p += 12;
			in_before_sections = false;
			in_section = false;
			in_after_sections = false;
			in_null_section = true;
			UChar *s = p;
			SKIPLN(s);
			SKIPWS(s);
			result->lines += SKIPWS(p);
			if (p != s) {
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				result->addAnchor(&gbuffers[0][0], result->lines);
				p = n;
			}
		}
		// ANCHOR
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+1),'N','n') && ISCHR(*(p+2),'C','c')
			&& ISCHR(*(p+3),'H','h') && ISCHR(*(p+4),'O','o')
			&& !ISSTRING(p, 5)) {
			p += 6;
			result->lines += SKIPWS(p);
			UChar *n = p;
			result->lines += SKIPTOWS(n, 0, true);
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			result->addAnchor(&gbuffers[0][0], result->lines);
			p = n;
			//*
			// ToDo: Whether ANCHOR should require ; or not...
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
				CG3Quit(1);
			}
			//*/
		}
		// INCLUDE
		else if (ISCHR(*p,'I','i') && ISCHR(*(p+6),'E','e') && ISCHR(*(p+1),'N','n') && ISCHR(*(p+2),'C','c')
			&& ISCHR(*(p+3),'L','l') && ISCHR(*(p+4),'U','u') && ISCHR(*(p+5),'D','d')
			&& !ISSTRING(p, 6)) {
			p += 7;
			result->lines += SKIPWS(p);
			UChar *n = p;
			result->lines += SKIPTOWS(n, 0, true);
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			uint32_t olines = result->lines;
			p = n;
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
				CG3Quit(1);
			}

			UErrorCode err = U_ZERO_ERROR;
			UConverter *conv = ucnv_open("UTF-8", &err);
			ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE-1, &gbuffers[0][0], u_strlen(&gbuffers[0][0]), &err);
			ucnv_close(conv);

			char *absdir = ux_dirname(fname);
			sprintf(&cbuffers[1][0], "%s%s", absdir, &cbuffers[0][0]);
			delete[] absdir;
			char *abspath = new char[strlen(&cbuffers[1][0])+1];
			strcpy(abspath, &cbuffers[1][0]);

			size_t grammar_size = 0;
			struct stat _stat;
			int error = stat(abspath, &_stat);

			if (error != 0) {
				u_fprintf(ux_stderr, "Error: Cannot stat %s due to error %d - bailing out!\n", abspath, error);
				CG3Quit(1);
			}
			else {
				grammar_size = static_cast<size_t>(_stat.st_size);
			}

			UFILE *grammar = u_fopen(abspath, "r", locale, codepage);
			if (!grammar) {
				u_fprintf(ux_stderr, "Error: Error opening %s for reading!\n", abspath);
				CG3Quit(1);
			}

			std::vector<UChar> data(grammar_size*2, 0);
			uint32_t read = u_file_read(&data[4], grammar_size*2, grammar);
			u_fclose(grammar);
			if (read >= grammar_size*2-1) {
				u_fprintf(ux_stderr, "Error: Converting from underlying codepage to UTF-16 exceeded factor 2 buffer.\n");
				CG3Quit(1);
			}
			data.resize(read+4+1);

			parseFromUChar(&data[4], abspath);
			delete[] abspath;

			result->lines = olines;
		}
		// IFF
		else if (ISCHR(*p,'I','i') && ISCHR(*(p+2),'F','f') && ISCHR(*(p+1),'F','f')
			&& !ISSTRING(p, 2)) {
			parseRule(p, K_IFF);
		}
		// MAP
		else if (ISCHR(*p,'M','m') && ISCHR(*(p+2),'P','p') && ISCHR(*(p+1),'A','a')
			&& !ISSTRING(p, 2)) {
			parseRule(p, K_MAP);
		}
		// ADD
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+2),'D','d') && ISCHR(*(p+1),'D','d')
			&& !ISSTRING(p, 2)) {
			parseRule(p, K_ADD);
		}
		// APPEND
		else if (ISCHR(*p,'A','a') && ISCHR(*(p+5),'D','d') && ISCHR(*(p+1),'P','p') && ISCHR(*(p+2),'P','p')
			&& ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'N','n')
			&& !ISSTRING(p, 5)) {
			parseRule(p, K_APPEND);
		}
		// SELECT
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+5),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
			&& ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'C','c')
			&& !ISSTRING(p, 5)) {
			parseRule(p, K_SELECT);
		}
		// REMOVE
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+5),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'O','o') && ISCHR(*(p+4),'V','v')
			&& !ISSTRING(p, 5)) {
			parseRule(p, K_REMOVE);
		}
		// REPLACE
		else if (ISCHR(*p,'R','r') && ISCHR(*(p+6),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'P','p')
			&& ISCHR(*(p+3),'L','l') && ISCHR(*(p+4),'A','a') && ISCHR(*(p+5),'C','c')
			&& !ISSTRING(p, 6)) {
			parseRule(p, K_REPLACE);
		}
		// DELIMIT
		else if (ISCHR(*p,'D','d') && ISCHR(*(p+6),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
			&& ISCHR(*(p+3),'I','i') && ISCHR(*(p+4),'M','m') && ISCHR(*(p+5),'I','i')
			&& !ISSTRING(p, 6)) {
			parseRule(p, K_DELIMIT);
		}
		// SUBSTITUTE
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+9),'E','e') && ISCHR(*(p+1),'U','u') && ISCHR(*(p+2),'B','b')
			&& ISCHR(*(p+3),'S','s') && ISCHR(*(p+4),'T','t') && ISCHR(*(p+5),'I','i') && ISCHR(*(p+6),'T','t')
			&& ISCHR(*(p+7),'U','u') && ISCHR(*(p+8),'T','t')
			&& !ISSTRING(p, 9)) {
			parseRule(p, K_SUBSTITUTE);
		}
		// COPY
		else if (ISCHR(*p,'C','c') && ISCHR(*(p+3),'Y','y') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'P','p')
			&& !ISSTRING(p, 4)) {
			parseRule(p, K_COPY);
		}
		// JUMP
		else if (ISCHR(*p,'J','j') && ISCHR(*(p+3),'P','p') && ISCHR(*(p+1),'U','u') && ISCHR(*(p+2),'M','m')
			&& !ISSTRING(p, 4)) {
			parseRule(p, K_JUMP);
		}
		// MOVE
		else if (ISCHR(*p,'M','m') && ISCHR(*(p+3),'E','e') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'V','v')
			&& !ISSTRING(p, 4)) {
			parseRule(p, K_MOVE);
		}
		// SWITCH
		else if (ISCHR(*p,'S','s') && ISCHR(*(p+5),'H','h') && ISCHR(*(p+1),'W','w') && ISCHR(*(p+2),'I','i')
			&& ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'C','c')
			&& !ISSTRING(p, 5)) {
			parseRule(p, K_SWITCH);
		}
		// EXECUTE
		else if (ISCHR(*p,'E','e') && ISCHR(*(p+6),'E','e') && ISCHR(*(p+1),'X','x') && ISCHR(*(p+2),'E','e')
			&& ISCHR(*(p+3),'C','c') && ISCHR(*(p+4),'U','u') && ISCHR(*(p+5),'T','t')
			&& !ISSTRING(p, 7)) {
			parseRule(p, K_EXECUTE);
		}
		// UNMAP
		else if (ISCHR(*p,'U','u') && ISCHR(*(p+4),'P','p') && ISCHR(*(p+1),'N','n') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'A','a')
			&& !ISSTRING(p, 4)) {
			parseRule(p, K_UNMAP);
		}
		// TEMPLATE
		else if (ISCHR(*p,'T','t') && ISCHR(*(p+7),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
			&& ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'L','l') && ISCHR(*(p+5),'A','a') && ISCHR(*(p+6),'T','t')
			&& !ISSTRING(p, 7)) {
			ContextualTest *t = result->allocateContextualTest();
			t->line = result->lines;
			p += 8;
			result->lines += SKIPWS(p);
			UChar *n = p;
			result->lines += SKIPTOWS(n, 0, true);
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			uint32_t cn = hash_sdbm_uchar(&gbuffers[0][0]);
			t->name = cn;
			result->addContextualTest(t, &gbuffers[0][0]);
			p = n;
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;

			parseContextualTestList(p, 0, 0, 0, t);

			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u! Probably caused by missing set operator.\n", result->lines);
				CG3Quit(1);
			}
		}
		// PARENTHESES
		else if (ISCHR(*p,'P','p') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'A','a') && ISCHR(*(p+2),'R','r')
			&& ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'N','n') && ISCHR(*(p+5),'T','t') && ISCHR(*(p+6),'H','h')
			&& ISCHR(*(p+7),'E','e') && ISCHR(*(p+8),'S','s') && ISCHR(*(p+9),'E','e')
			&& !ISSTRING(p, 10)) {
			p += 11;
			result->lines += SKIPWS(p, '=');
			if (*p != '=') {
				u_fprintf(ux_stderr, "Error: Encountered a %C before the expected = on line %u!\n", *p, result->lines);
				CG3Quit(1);
			}
			++p;
			result->lines += SKIPWS(p);

			while (*p && *p != ';') {
				ptrdiff_t c = 0;
				Tag *left = 0;
				Tag *right = 0;
				UChar *n = p;
				result->lines += SKIPTOWS(n, '(', true);
				if (*n != '(') {
					u_fprintf(ux_stderr, "Error: Encountered %C before the expected ( on line %u!\n", *p, result->lines);
					CG3Quit(1);
				}
				n++;
				result->lines += SKIPWS(n);
				p = n;
				if (*n == '"') {
					n++;
					result->lines += SKIPTO_NOSPAN(n, '"');
					if (*n != '"') {
						u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
						CG3Quit(1);
					}
				}
				result->lines += SKIPTOWS(n, ')', true);
				c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				left = result->allocateTag(&gbuffers[0][0]);
				result->lines += SKIPWS(n);
				p = n;

				if (*p == ')') {
					u_fprintf(ux_stderr, "Error: Encountered ) before the expected Right tag on line %u!\n", result->lines);
					CG3Quit(1);
				}

				if (*n == '"') {
					n++;
					result->lines += SKIPTO_NOSPAN(n, '"');
					if (*n != '"') {
						u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
						CG3Quit(1);
					}
				}
				result->lines += SKIPTOWS(n, ')', true);
				c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				right = result->allocateTag(&gbuffers[0][0]);
				result->lines += SKIPWS(n);
				p = n;

				if (*p != ')') {
					u_fprintf(ux_stderr, "Error: Encountered %C before the expected ) on line %u!\n", *p, result->lines);
					CG3Quit(1);
				}
				++p;
				result->lines += SKIPWS(p);

				if (left && right) {
					result->parentheses[left->hash] = right->hash;
					result->parentheses_reverse[right->hash] = left->hash;
				}
			}

			if (result->parentheses.empty()) {
				u_fprintf(ux_stderr, "Error: PARENTHESES declared, but no definitions given, on line %u!\n", result->lines);
				CG3Quit(1);
			}
			result->lines += SKIPWS(p, ';');
			if (*p != ';') {
				u_fprintf(ux_stderr, "Error: Missing closing ; before line %u!\n", result->lines);
				CG3Quit(1);
			}
		}
		// END
		else if (ISCHR(*p,'E','e') && ISCHR(*(p+2),'D','d') && ISCHR(*(p+1),'N','n')) {
			if (ISNL(*(p-1)) || ISSPACE(*(p-1))) {
				if (*(p+3) == 0 || ISNL(*(p+3)) || ISSPACE(*(p+3))) {
					break;
				}
			}
			++p;
		}
		// No keyword found at this position, skip a character.
		else {
			// For some strange reason, '<' was explicitly allowed to exist without a purpose...
			// I cannot recall why, so removed that since it caused line counting errors.
			if (*p == ';' || *p == '"') {
				if (*p == '"') {
					++p;
					result->lines += SKIPTO_NOSPAN(p, '"');
					if (*p != '"') {
						u_fprintf(ux_stderr, "Error: Missing closing \" on line %u!\n", result->lines);
						CG3Quit(1);
					}
				}
				result->lines += SKIPTOWS(p);
			}
			if (*p && *p != ';' && *p != '"' && !ISNL(*p) && !ISSPACE(*p)) {
				p[16] = 0;
				u_fprintf(ux_stderr, "Error: Garbage data '%S...' encountered on line %u!\n", p, result->lines);
				CG3Quit(1);
			}
			if (ISNL(*p)) {
				result->lines += 1;
			}
			++p;
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
	}
	else {
		result->grammar_size = static_cast<size_t>(_stat.st_size);
	}

	UFILE *grammar = u_fopen(filename, "r", locale, codepage);
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: Error opening %s for reading!\n", filename);
		CG3Quit(1);
	}

	// It reads into the buffer at offset 4 because certain functions may look back, so we need some nulls in front.
	std::vector<UChar> data(result->grammar_size*2, 0);
	uint32_t read = u_file_read(&data[4], result->grammar_size*2, grammar);
	u_fclose(grammar);
	if (read >= result->grammar_size*2-1) {
		u_fprintf(ux_stderr, "Error: Converting from underlying codepage to UTF-16 exceeded factor 2 buffer.\n");
		CG3Quit(1);
	}
	data.resize(read+4+1);

	result->addAnchor(keywords[K_START].getTerminatedBuffer(), result->lines);

	// Allocate the magic * tag
	{
		Tag *tany = result->allocateTag(stringbits[S_ASTERIK].getTerminatedBuffer());
		result->tag_any = tany->hash;
	}
	// Create the magic set _TARGET_ containing the tag _TARGET_
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_TARGET].getTerminatedBuffer());
		Tag *t = result->allocateTag(stringbits[S_UU_TARGET].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _MARK_ containing the tag _MARK_
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_MARK].getTerminatedBuffer());
		Tag *t = result->allocateTag(stringbits[S_UU_MARK].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _ATTACHTO_ containing the tag _ATTACHTO_
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_ATTACHTO].getTerminatedBuffer());
		Tag *t = result->allocateTag(stringbits[S_UU_ATTACHTO].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _LEFT_ containing the tag _LEFT_
	Set *s_left = 0;
	{
		Set *set_c = s_left = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_LEFT].getTerminatedBuffer());
		Tag *t = result->allocateTag(stringbits[S_UU_LEFT].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _RIGHT_ containing the tag _RIGHT_
	Set *s_right = 0;
	{
		Set *set_c = s_right = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_RIGHT].getTerminatedBuffer());
		Tag *t = result->allocateTag(stringbits[S_UU_RIGHT].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _PAREN_ containing (_LEFT_) OR (_RIGHT_)
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_PAREN].getTerminatedBuffer());
		set_c->set_ops.push_back(S_OR);
		set_c->sets.push_back(s_left->hash);
		set_c->sets.push_back(s_right->hash);
		result->addSet(set_c);
	}

	error = parseFromUChar(&data[4], filename);
	if (error) {
		return error;
	}

	result->addAnchor(keywords[K_END].getTerminatedBuffer(), result->lines);

	return 0;
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
	else if (in_null_section) {
		rule->section = -3;
		result->addRule(rule);
	}
	else {
		result->destroyRule(rule);
		u_fprintf(ux_stderr, "Error: Rule definition attempted outside a section on line %u!\n", result->lines);
		CG3Quit(1);
	}
}

}
