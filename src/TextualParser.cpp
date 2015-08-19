/*
* Copyright (C) 2007-2015, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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

#include "TextualParser.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"
#include "ContextualTest.hpp"
#include "parser_helpers.hpp"
#include <bitset>

namespace CG3 {

TextualParser::TextualParser(Grammar& res, UFILE *ux_err)
  : verbosity_level(0)
  , sets_counter(100)
  , seen_mapping_prefix(0)
  , option_vislcg_compat(false)
  , in_section(false)
  , in_before_sections(true)
  , in_after_sections(false)
  , in_null_section(false)
  , no_isets(false)
  , no_itmpls(false)
  , strict_wforms(false)
  , strict_bforms(false)
  , strict_second(false)
  , filename(0)
  , locale(0)
  , codepage(0)
  , error_counter(0)
{
	ux_stderr = ux_err;
	result = &res;
}

void TextualParser::incErrorCount() {
	u_fflush(ux_stderr);
	++error_counter;
	if (error_counter >= 10) {
		u_fprintf(ux_stderr, "%s: Too many errors - giving up...\n", filebase);
		CG3Quit(1);
	}
	throw error_counter;
}

struct freq_sorter {
	const bc::flat_map<Tag *, size_t>& tag_freq;

	freq_sorter(const bc::flat_map<Tag *, size_t>& tag_freq)
	  : tag_freq(tag_freq)
	{
	}

	bool operator()(Tag *a, Tag *b) const {
		// Sort highest frequency first
		return tag_freq.find(a)->second > tag_freq.find(b)->second;
	}
};

void TextualParser::error(const char *str) {
	u_fprintf(ux_stderr, str, filebase, result->lines);
	incErrorCount();
}

void TextualParser::error(const char *str, UChar c) {
	u_fprintf(ux_stderr, str, filebase, c, result->lines);
	incErrorCount();
}

void TextualParser::error(const char *str, const UChar *p) {
	ux_bufcpy(nearbuf, p, 20);
	u_fprintf(ux_stderr, str, filebase, result->lines, nearbuf);
	incErrorCount();
}

void TextualParser::error(const char *str, UChar c, const UChar *p) {
	ux_bufcpy(nearbuf, p, 20);
	u_fprintf(ux_stderr, str, filebase, c, result->lines, nearbuf);
	incErrorCount();
}

void TextualParser::error(const char *str, const char *s, const UChar *p) {
	ux_bufcpy(nearbuf, p, 20);
	u_fprintf(ux_stderr, str, filebase, s, result->lines, nearbuf);
	incErrorCount();
}

void TextualParser::error(const char *str, const UChar *s, const UChar *p) {
	ux_bufcpy(nearbuf, p, 20);
	u_fprintf(ux_stderr, str, filebase, s, result->lines, nearbuf);
	incErrorCount();
}

void TextualParser::error(const char *str, const char *s, const UChar *S, const UChar *p) {
	ux_bufcpy(nearbuf, p, 20);
	u_fprintf(ux_stderr, str, filebase, s, S, result->lines, nearbuf);
	incErrorCount();
}

Tag *TextualParser::parseTag(const UChar *to, const UChar *p) {
	Tag *tag = ::CG3::parseTag(to, p, *this);
	if (!strict_tags.empty() && !strict_tags.count(tag->plain_hash)) {
		if (tag->type & (T_ANY | T_VARSTRING | T_VSTR | T_META | T_VARIABLE | T_SET | T_PAR_LEFT | T_PAR_RIGHT | T_ENCL | T_TARGET | T_MARK | T_ATTACHTO | T_SAME_BASIC)) {
			// Always allow...
		}
		else if (u_strcmp(tag->tag.c_str(), stringbits[S_BEGINTAG].getTerminatedBuffer()) == 0 || u_strcmp(tag->tag.c_str(), stringbits[S_ENDTAG].getTerminatedBuffer()) == 0) {
			// Always allow >>> and <<<
		}
		else if (tag->type & T_WORDFORM) {
			if (strict_wforms) {
				error("%s: Error: Wordform tag %S not on the strict-tags list, on line %u near `%S`!\n", tag->tag.c_str(), p);
				incErrorCount();
			}
		}
		else if (tag->type & T_BASEFORM) {
			if (strict_bforms) {
				error("%s: Error: Baseform tag %S not on the strict-tags list, on line %u near `%S`!\n", tag->tag.c_str(), p);
				incErrorCount();
			}
		}
		else if (tag->tag[0] == '<' && tag->tag[tag->tag.size() - 1] == '>') {
			if (strict_second) {
				error("%s: Error: Secondary tag %S not on the strict-tags list, on line %u near `%S`!\n", tag->tag.c_str(), p);
				incErrorCount();
			}
		}
		else {
			error("%s: Error: Tag %S not on the strict-tags list, on line %u near `%S`!\n", tag->tag.c_str(), p);
			incErrorCount();
		}
	}
	return tag;
}

Tag *TextualParser::addTag(Tag *tag) {
	return result->addTag(tag);
}

void TextualParser::parseTagList(UChar *& p, Set *s) {
	std::set<TagVector> taglists;
	bc::flat_map<Tag *, size_t> tag_freq;

	while (*p && *p != ';' && *p != ')') {
		result->lines += SKIPWS(p, ';', ')');
		if (*p && *p != ';' && *p != ')') {
			TagVector tags;
			if (*p == '(') {
				++p;
				result->lines += SKIPWS(p, ';', ')');

				while (*p && *p != ';' && *p != ')') {
					UChar *n = p;
					if (*n == '"') {
						n++;
						SKIPTO_NOSPAN(n, '"');
						if (*n != '"') {
							error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
						}
					}
					result->lines += SKIPTOWS(n, ')', true);
					ptrdiff_t c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					Tag *t = parseTag(&gbuffers[0][0], p);
					tags.push_back(t);
					p = n;
					result->lines += SKIPWS(p, ';', ')');
				}
				if (*p != ')') {
					error("%s: Error: Expected closing ) on line %u near `%S`!\n", p);
				}
				++p;
			}
			else {
				UChar *n = p;
				if (*n == '"') {
					n++;
					SKIPTO_NOSPAN(n, '"');
					if (*n != '"') {
						error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
					}
				}
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				Tag *t = parseTag(&gbuffers[0][0], p);
				tags.push_back(t);
				p = n;
			}

			// sort + uniq the tags
			std::sort(tags.begin(), tags.end());
			tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
			// If this particular list of tags hasn't already been seen, then increment their frequency counts
			if (taglists.insert(tags).second) {
				boost_foreach (Tag *t, tags) {
					++tag_freq[t];
				}
			}
		}
	}

	freq_sorter fs(tag_freq);
	boost_foreach (const TagVector& tvc, taglists) {
		if (tvc.size() == 1) {
			result->addTagToSet(tvc[0], s);
			continue;
		}
		TagVector& tv = const_cast<TagVector&>(tvc);
		// Sort tags by frequency, high-to-low
		// Doing this yields a very cheap imperfect form of trie compression, but it's good enough
		std::sort(tv.begin(), tv.end(), fs);
		bool special = false;
		boost_foreach (Tag *tag, tv) {
			if (tag->type & T_SPECIAL) {
				special = true;
				break;
			}
		}
		if (special) {
			trie_insert(s->trie_special, tv);
		}
		else {
			trie_insert(s->trie, tv);
		}
	}
}

Set *TextualParser::parseSet(const UChar *name, const UChar *p) {
	return ::CG3::parseSet(name, p, *this);
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
					if (no_isets && p[1] != '*') {
						error("%s: Error: Inline set spotted on line %u near `%S`!\n", p);
					}
					// No, this can't just reuse parseTagList() because this will only ever parse a single CompositeTag,
					// whereas parseTagList() will handle mixed Tag and CompositeTag
					// Doubly so now that parseTagList() will sort+uniq the tags, which we don't want for MAP/ADD/SUBSTITUTE/etc
					UChar *n = p;
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
							SKIPTO_NOSPAN(n, '"');
							if (*n != '"') {
								error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
							}
						}
						result->lines += SKIPTOWS(n, ')', true);
						ptrdiff_t c = n - p;
						u_strncpy(&gbuffers[0][0], p, c);
						gbuffers[0][c] = 0;
						Tag *t = parseTag(&gbuffers[0][0], p);
						tags.push_back(t);
						p = n;
						result->lines += SKIPWS(p, ';', ')');
					}
					if (*p != ')') {
						error("%s: Error: Expected closing ) on line %u near `%S`!\n", p);
					}
					++p;

					if (tags.size() == 0) {
						error("%s: Error: Empty inline set on line %u near `%S`! Use (*) if you want to replace with nothing.\n", n);
					}
					else if (tags.size() == 1) {
						result->addTagToSet(tags[0], set_c);
					}
					else {
						bool special = false;
						boost_foreach (Tag *tag, tags) {
							if (tag->type & T_SPECIAL) {
								special = true;
								break;
							}
						}
						if (special) {
							trie_insert(set_c->trie_special, tags);
						}
						else {
							trie_insert(set_c->trie, tags);
						}
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
					Set *tmp = parseSet(&gbuffers[0][0], p);
					uint32_t sh = tmp->hash;
					sets.push_back(sh);
					p = n;
				}

				if (!set_ops.empty() && (set_ops.back() == S_SET_ISECT_U || set_ops.back() == S_SET_SYMDIFF_U)) {
					TagVector tv;
					std::set<TagVector> a;
					trie_getTags(result->getSet(sets[sets.size() - 1])->trie, a, tv);
					trie_getTags(result->getSet(sets[sets.size() - 1])->trie_special, a, tv);
					tv.clear();
					std::set<TagVector> b;
					trie_getTags(result->getSet(sets[sets.size() - 2])->trie, b, tv);
					trie_getTags(result->getSet(sets[sets.size() - 2])->trie_special, b, tv);

					std::vector<TagVector> r;
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

					bc::flat_map<Tag *, size_t> tag_freq;
					boost_foreach (const TagVector& tags, r) {
						boost_foreach (Tag *t, tags) {
							++tag_freq[t];
						}
					}

					freq_sorter fs(tag_freq);
					boost_foreach (TagVector& tv, r) {
						if (tv.size() == 1) {
							result->addTagToSet(tv[0], set_c);
							continue;
						}

						// Sort tags by frequency, high-to-low
						// Doing this yields a very cheap imperfect form of trie compression, but it's good enough
						std::sort(tv.begin(), tv.end(), fs);
						bool special = false;
						boost_foreach (Tag *tag, tv) {
							if (tag->type & T_SPECIAL) {
								special = true;
								break;
							}
						}
						if (special) {
							trie_insert(set_c->trie_special, tv);
						}
						else {
							trie_insert(set_c->trie, tv);
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
		else if (!wantop) {
			error("%s: Error: Expected set on line %u near `%S`!\n", p);
		}
	}

	if (!s && sets.size() == 1) {
		s = result->getSet(sets.back());
	}
	else {
		if (!s) {
			s = result->allocateSet();
		}
		s->sets.swap(sets);
		s->set_ops.swap(set_ops);
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

void TextualParser::parseContextualTestPosition(UChar *& p, ContextualTest& t) {
	bool negative = false;
	bool had_digits = false;

	UChar *n = p;

	size_t tries;
	for (tries = 0; *p != ' ' && *p != '(' && *p != '/' && tries < 100; ++tries) {
		if (*p == '*' && *(p + 1) == '*') {
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
		if (*p == 'c' && (t.pos & POS_DEP_CHILD)) {
			t.pos &= ~POS_DEP_CHILD;
			t.pos |= POS_DEP_GLOB;
			++p;
		}
		if (*p == 'p') {
			t.pos |= POS_DEP_PARENT;
			++p;
		}
		if (*p == 'p' && (t.pos & POS_DEP_PARENT)) {
			t.pos |= POS_DEP_GLOB;
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
		if (*p == 'f') {
			t.pos |= POS_NUMERIC_BRANCH;
			++p;
		}
		if (*p == '-') {
			negative = true;
			++p;
		}
		if (u_isdigit(*p)) {
			had_digits = true;
			while (*p >= '0' && *p <= '9') {
				t.offset = (t.offset * 10) + (*p - '0');
				++p;
			}
		}
		if (*p == 'r' && *(p + 1) == ':') {
			t.pos |= POS_RELATION;
			p += 2;
			UChar *n = p;
			SKIPTOWS(n, '(');
			ptrdiff_t c = n - p;
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
			Tag *tag = result->allocateTag(&gbuffers[0][0]);
			t.relation = tag->hash;
			p = n;
		}
		if (*p == 'r') {
			t.pos |= POS_RIGHT;
			++p;
		}
		if (*p == 'r' && (t.pos & POS_RIGHT)) {
			t.pos &= ~static_cast<uint64_t>(POS_RIGHT);
			t.pos |= POS_RIGHTMOST;
			++p;
		}
		if (*p == 'l') {
			t.pos |= POS_LEFT;
			++p;
		}
		if (*p == 'l' && (t.pos & POS_LEFT)) {
			t.pos &= ~static_cast<uint64_t>(POS_LEFT);
			t.pos |= POS_LEFTMOST;
			++p;
		}
	}

	if (negative) {
		t.offset = (-1) * abs(t.offset);
	}

	if (*p == '/') {
		++p;
		bool negative = false;

		size_t tries;
		for (tries = 0; *p != ' ' && *p != '(' && tries < 100; ++tries) {
			if (*p == '*' && *(p + 1) == '*') {
				t.offset_sub = GSR_ANY;
				p += 2;
			}
			if (*p == '*') {
				t.offset_sub = GSR_ANY;
				++p;
			}
			if (*p == '-') {
				negative = true;
				++p;
			}
			if (u_isdigit(*p)) {
				while (*p >= '0' && *p <= '9') {
					t.offset_sub = (t.offset_sub * 10) + (*p - '0');
					++p;
				}
			}
		}

		if (negative) {
			t.offset_sub = (-1) * abs(t.offset_sub);
		}
	}

	if ((t.pos & (POS_DEP_CHILD | POS_DEP_SIBLING)) && (t.pos & (POS_SCANFIRST | POS_SCANALL))) {
		t.pos &= ~POS_SCANFIRST;
		t.pos &= ~POS_SCANALL;
		t.pos |= POS_DEP_DEEP;
	}

	if (tries >= 100) {
		error("%s: Error: Invalid position on line %u near `%S` - caused endless loop!\n", n);
	}
	else if (tries >= 20) {
		ux_bufcpy(nearbuf, n, 20);
		u_fprintf(ux_stderr, "%s: Warning: Position on line %u near `%S` took many loops.\n", filebase, result->lines, nearbuf);
		u_fflush(ux_stderr);
	}
	if (!ISSPACE(*p)) {
		error("%s: Error: Invalid position on line %u near `%S` - garbage data!\n", n);
	}
	if (had_digits) {
		if (t.pos & (POS_DEP_CHILD | POS_DEP_SIBLING | POS_DEP_PARENT)) {
			error("%s: Error: Invalid position on line %u near `%S` - cannot combine offsets with dependency!\n", n);
		}
		if (t.pos & (POS_LEFT_PAR | POS_RIGHT_PAR)) {
			error("%s: Error: Invalid position on line %u near `%S` - cannot combine offsets with enclosures!\n", n);
		}
		if (t.pos & POS_RELATION) {
			error("%s: Error: Invalid position on line %u near `%S` - cannot combine offsets with relations!\n", n);
		}
	}
	if ((t.pos & POS_DEP_PARENT) && !(t.pos & POS_DEP_GLOB)) {
		if (t.pos & (POS_LEFTMOST | POS_RIGHTMOST)) {
			error("%s: Error: Invalid position on line %u near `%S` - leftmost/rightmost requires ancestor, not parent!\n", n);
		}
	}
	/*
	if ((t.pos & (POS_LEFT_PAR|POS_RIGHT_PAR)) && (t.pos & (POS_SCANFIRST|POS_SCANALL))) {
		ux_bufcpy(nearbuf, n, 20);
		error("%s: Error: Invalid position on line %u near `%S` - cannot have both enclosure and scan!\n", filebase);
	}
	//*/
	if ((t.pos & POS_PASS_ORIGIN) && (t.pos & POS_NO_PASS_ORIGIN)) {
		error("%s: Error: Invalid position on line %u near `%S` - cannot have both O and o!\n", n);
	}
	if ((t.pos & POS_LEFT_PAR) && (t.pos & POS_RIGHT_PAR)) {
		error("%s: Error: Invalid position on line %u near `%S` - cannot have both L and R!\n", n);
	}
	if ((t.pos & POS_ALL) && (t.pos & POS_NONE)) {
		error("%s: Error: Invalid position on line %u near `%S` - cannot have both NONE and ALL!\n", n);
	}
	if ((t.pos & POS_UNKNOWN) && (t.pos != POS_UNKNOWN || had_digits)) {
		error("%s: Error: Invalid position on line %u near `%S` - '?' cannot be combined with anything else!\n", n);
	}
	if ((t.pos & POS_SCANALL) && (t.pos & POS_NOT)) {
		ux_bufcpy(nearbuf, n, 20);
		u_fprintf(ux_stderr, "%s: Warning: Line %u near `%S`: We don't think mixing NOT and ** makes sense...\n", filebase, result->lines, nearbuf);
		u_fflush(ux_stderr);
	}

	if (t.pos > POS_64BIT) {
		t.pos |= POS_64BIT;
	}
}

ContextualTest *TextualParser::parseContextualTestList(UChar *& p, Rule *rule) {
	ContextualTest *t = result->allocateContextualTest();
	ContextualTest *ot = t;
	t->line = result->lines;

	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_TEXTNEGATE].getTerminatedBuffer(), stringbits[S_TEXTNEGATE].length())) {
		p += stringbits[S_TEXTNEGATE].length();
		t->pos |= POS_NEGATE;
	}
	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_ALL].getTerminatedBuffer(), stringbits[S_ALL].length())) {
		p += stringbits[S_ALL].length();
		t->pos |= POS_ALL;
	}
	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_NONE].getTerminatedBuffer(), stringbits[S_NONE].length())) {
		p += stringbits[S_NONE].length();
		t->pos |= POS_NONE;
	}
	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_TEXTNOT].getTerminatedBuffer(), stringbits[S_TEXTNOT].length())) {
		p += stringbits[S_TEXTNOT].length();
		t->pos |= POS_NOT;
	}
	result->lines += SKIPWS(p);

	std::pair<size_t, UString> tmpl_data;

	UChar *pos_p = p;
	UChar *n = p;
	result->lines += SKIPTOWS(n, '(');
	ptrdiff_t c = n - p;
	u_strncpy(&gbuffers[0][0], p, c);
	gbuffers[0][c] = 0;
	if (ux_isEmpty(&gbuffers[0][0])) {
		if (no_itmpls) {
			error("%s: Error: Inline template spotted on line %u near `%S`!\n", p);
		}
		p = n;
		pos_p = p;
		for (;;) {
			if (*p != '(') {
				error("%s: Error: Expected '(' but found '%C' on line %u near `%S`!\n", *p, p);
			}
			++p;
			ContextualTest *ored = parseContextualTestList(p, rule);
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
		if (t->ors.size() == 1 && verbosity_level > 0) {
			uncond_swap<UChar> swp(*p, 0);
			if (t->ors.front()->ors.size() < 2) {
				u_fprintf(ux_stderr, "%s: Warning: Inline templates only make sense if you OR them on line %u at %S.\n", filebase, result->lines, pos_p);
			}
			else {
				u_fprintf(ux_stderr, "%s: Warning: Inline templates do not need () around the whole expression on line %u at %S.\n", filebase, result->lines, pos_p);
			}
			u_fflush(ux_stderr);
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
			ContextualTest *lnk = result->allocateContextualTest();
			Set *s = parseSetInlineWrapper(p);
			lnk->offset = 1;
			lnk->target = s->hash;
			t->linked = lnk;
			t = lnk;
			result->lines += SKIPWS(p);
		}
		if (*p != ']') {
			error("%s: Error: Expected ']' but found '%C' on line %u near `%S`!\n", *p, p);
		}
		++p;
	}
	else if (gbuffers[0][0] == 'T' && gbuffers[0][1] == ':') {
		goto label_parseTemplateRef;
	}
	else {
		pos_p = p;
		parseContextualTestPosition(p, *t);
		p = n;
		if (t->pos & (POS_DEP_CHILD | POS_DEP_PARENT | POS_DEP_SIBLING)) {
			result->has_dep = true;
		}
		if (t->pos & POS_RELATION) {
			result->has_relations = true;
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
			uint32_t cn = hash_value(&gbuffers[0][0]);
			t->tmpl = reinterpret_cast<ContextualTest*>(cn);
			tmpl_data = std::make_pair(result->lines, &gbuffers[0][0]);
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

		if ((t->barrier || t->cbarrier) && !(t->pos & MASK_POS_SCAN)) {
			uncond_swap<UChar> swp(*p, 0);
			u_fprintf(ux_stderr, "%s: Warning: Barriers only make sense for scanning tests on line %u at %S.\n", filebase, result->lines, pos_p);
			u_fflush(ux_stderr);
			t->barrier = 0;
			t->cbarrier = 0;
		}
	}

	bool linked = false;
	result->lines += SKIPWS(p);
	if (ux_simplecasecmp(p, stringbits[S_AND].getTerminatedBuffer(), stringbits[S_AND].length())) {
		error("%s: Error: 'AND' is deprecated; use 'LINK 0' or operator '+' instead. Found on line %u near `%S`!\n", p);
	}
	if (ux_simplecasecmp(p, stringbits[S_LINK].getTerminatedBuffer(), stringbits[S_LINK].length())) {
		p += stringbits[S_LINK].length();
		linked = true;
	}
	result->lines += SKIPWS(p);

	if (linked) {
		if (t->pos & POS_NONE) {
			error("%s: Error: It does not make sense to LINK from a NONE test; perhaps you meant NOT or NEGATE on line %u near `%S`?\n", p);
		}
		t->linked = parseContextualTestList(p, rule);
	}

	if (rule) {
		if (rule->flags & RF_LOOKDELETED) {
			t->pos |= POS_LOOK_DELETED;
		}
		if (rule->flags & RF_LOOKDELAYED) {
			t->pos |= POS_LOOK_DELAYED;
		}
	}

	t = result->addContextualTest(ot);
	if (t->tmpl) {
		deferred_tmpls[t] = tmpl_data;
	}
	return t;
}

void TextualParser::parseContextualTests(UChar *& p, Rule *rule) {
	ContextualTest *t = parseContextualTestList(p, rule);
	if (option_vislcg_compat && (t->pos & POS_NOT)) {
		t->pos &= ~POS_NOT;
		t->pos |= POS_NEGATE;
	}
	rule->addContextualTest(t, rule->tests);
}

void TextualParser::parseContextualDependencyTests(UChar *& p, Rule *rule) {
	ContextualTest *t = parseContextualTestList(p, rule);
	if (option_vislcg_compat && (t->pos & POS_NOT)) {
		t->pos &= ~POS_NOT;
		t->pos |= POS_NEGATE;
	}
	rule->addContextualTest(t, rule->dep_tests);
}

void TextualParser::parseRule(UChar *& p, KEYWORDS key) {
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
			SKIPTO_NOSPAN(n, '"');
			if (*n != '"') {
				error("%s: Error: Expected closing \" on line %u near `%S`!\n", lp);
			}
		}
		result->lines += SKIPTOWS(n, 0, true);
		ptrdiff_t c = n - lp;
		u_strncpy(&gbuffers[0][0], lp, c);
		gbuffers[0][c] = 0;
		Tag *wform = parseTag(&gbuffers[0][0], lp);
		rule->wordform = wform;
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
		if (!gbuffers[0][0]) {
			ux_bufcpy(nearbuf, p, 20);
			u_fprintf(ux_stderr, "%s: Warning: Rule on line %u near `%S` had : but no name.\n", filebase, result->lines, nearbuf);
			u_fflush(ux_stderr);
		}
		else {
			rule->setName(&gbuffers[0][0]);
		}
		p = n;
	}
	result->lines += SKIPWS(p);

	if (key == K_EXTERNAL) {
		if (ux_simplecasecmp(p, stringbits[S_ONCE].getTerminatedBuffer(), stringbits[S_ONCE].length())) {
			p += stringbits[S_ONCE].length();
			rule->type = K_EXTERNAL_ONCE;
		}
		else if (ux_simplecasecmp(p, stringbits[S_ALWAYS].getTerminatedBuffer(), stringbits[S_ALWAYS].length())) {
			p += stringbits[S_ALWAYS].length();
			rule->type = K_EXTERNAL_ALWAYS;
		}
		else {
			error("%s: Error: Expected keyword ONCE or ALWAYS on line %u near `%S`!\n", p);
		}

		result->lines += SKIPWS(p);

		UChar *n = p;
		if (*n == '"') {
			++n;
			SKIPTO_NOSPAN(n, '"');
			if (*n != '"') {
				error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
			}
		}
		result->lines += SKIPTOWS(n, 0, true);
		ptrdiff_t c = n - p;
		if (*p == '"') {
			u_strncpy(&gbuffers[0][0], p + 1, c - 1);
			gbuffers[0][c - 2] = 0;
		}
		else {
			u_strncpy(&gbuffers[0][0], p, c);
			gbuffers[0][c] = 0;
		}

		Tag *ext = result->allocateTag(&gbuffers[0][0]);
		rule->varname = ext->hash;
		p = n;
	}

	lp = p;
	bool setflag = true;
	while (setflag) {
		setflag = false;
		for (uint32_t i = 0; i < FLAGS_COUNT; i++) {
			UChar *op = p;
			if (ux_simplecasecmp(p, g_flags[i].getTerminatedBuffer(), g_flags[i].length())) {
				p += g_flags[i].length();
				rule->flags |= (1 << i);
				setflag = true;

				if (i == FL_SUB) {
					if (*p != ':') {
						goto undo_flag;
					}
					++p;
					UChar *n = p;
					result->lines += SKIPTOWS(n, 0, true);
					ptrdiff_t c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					p = n;
					if (gbuffers[0][0] == '*') {
						rule->sub_reading = GSR_ANY;
					}
					else {
						u_sscanf(&gbuffers[0][0], "%d", &rule->sub_reading);
					}
				}

				// Rule flags followed by letters or valid set characters should not be flags.
				if (*p != '(' && !ISSPACE(*p)) {
				undo_flag:
					rule->flags &= ~(1 << i);
					p = op;
					setflag = false;
					break;
				}
			}
			result->lines += SKIPWS(p);
			// If any of these is the next char, there cannot possibly be more rule options...
			if (*p == '(' || *p == 'T' || *p == 't' || *p == ';') {
				setflag = false;
				break;
			}
		}
	}
	if (rule->flags & MASK_ENCL) {
		std::bitset<sizeof(rule->flags) * CHAR_BIT> bits(static_cast<uint64_t>(rule->flags & MASK_ENCL));
		if (bits.count() > 1) {
			error("%s: Error: Line %u near `%S`: ENCL_* are all mutually exclusive!\n", lp);
		}
	}
	if (rule->flags & RF_KEEPORDER && rule->flags & RF_VARYORDER) {
		error("%s: Error: Line %u near `%S`: KEEPORDER and VARYORDER are mutually exclusive!\n", lp);
	}
	if (rule->flags & RF_REMEMBERX && rule->flags & RF_RESETX) {
		error("%s: Error: Line %u near `%S`: REMEMBERX and RESETX are mutually exclusive!\n", lp);
	}
	if (rule->flags & RF_NEAREST && rule->flags & RF_ALLOWLOOP) {
		error("%s: Error: Line %u near `%S`: NEAREST and ALLOWLOOP are mutually exclusive!\n", lp);
	}
	if (rule->flags & RF_UNSAFE && rule->flags & RF_SAFE) {
		error("%s: Error: Line %u near `%S`: SAFE and UNSAFE are mutually exclusive!\n", lp);
	}
	if (rule->flags & RF_UNMAPLAST && rule->flags & RF_SAFE) {
		error("%s: Error: Line %u near `%S`: SAFE and UNMAPLAST are mutually exclusive!\n", lp);
	}
	if (rule->flags & RF_DELAYED && rule->flags & RF_IMMEDIATE) {
		error("%s: Error: Line %u near `%S`: IMMEDIATE and DELAYED are mutually exclusive!\n", lp);
	}
	if (rule->flags & RF_WITHCHILD && rule->flags & RF_NOCHILD) {
		error("%s: Error: Line %u near `%S`: WITHCHILD and NOCHILD are mutually exclusive!\n", lp);
	}
	if (rule->flags & RF_ITERATE && rule->flags & RF_NOITERATE) {
		error("%s: Error: Line %u near `%S`: ITERATE and NOITERATE are mutually exclusive!\n", lp);
	}

	if (!(rule->flags & (RF_ITERATE | RF_NOITERATE))) {
		if (key != K_SELECT && key != K_REMOVE && key != K_IFF && key != K_DELIMIT && key != K_REMCOHORT && key != K_MOVE && key != K_SWITCH) {
			rule->flags |= RF_NOITERATE;
		}
	}
	if (key == K_UNMAP && !(rule->flags & (RF_SAFE | RF_UNSAFE))) {
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

	lp = p;
	if (key == K_SUBSTITUTE || key == K_EXECUTE) {
		swapper_false swp(no_isets, no_isets);
		Set *s = parseSetInlineWrapper(p);
		s->reindex(*result);
		rule->sublist = s;
		if (s->empty()) {
			error("%s: Error: Empty substitute set on line %u near `%S`!\n", lp);
		}
		if (s->trie.empty() && s->trie_special.empty() && !(s->type & (ST_TAG_UNIFY | ST_SET_UNIFY | ST_CHILD_UNIFY))) {
			error("%s: Error: Substitute set on line %u near `%S` was neither unified nor of LIST type!\n", lp);
		}
	}

	result->lines += SKIPWS(p);
	lp = p;
	if (key == K_MAP || key == K_ADD || key == K_REPLACE || key == K_APPEND || key == K_SUBSTITUTE || key == K_COPY || key == K_ADDRELATIONS || key == K_ADDRELATION || key == K_SETRELATIONS || key == K_SETRELATION || key == K_REMRELATIONS || key == K_REMRELATION || key == K_SETVARIABLE || key == K_REMVARIABLE || key == K_ADDCOHORT || key == K_JUMP) {
		swapper_false swp(no_isets, no_isets);
		Set *s = parseSetInlineWrapper(p);
		s->reindex(*result);
		rule->maplist = s;
		if (s->empty()) {
			error("%s: Error: Empty mapping set on line %u near `%S`!\n", lp);
		}
		if (s->trie.empty() && s->trie_special.empty() && !(s->type & (ST_TAG_UNIFY | ST_SET_UNIFY | ST_CHILD_UNIFY))) {
			error("%s: Error: Mapping set on line %u near `%S` was neither unified nor of LIST type!\n", lp);
		}
		if (key == K_APPEND && !s->getNonEmpty().empty()) {
			if (!(s->getNonEmpty().begin()->first->type & T_BASEFORM)) {
				error("%s: Error: There must be a baseform before any other tags in APPEND on line %u near `%S`!\n", lp);
			}
		}
		if (key == K_ADDCOHORT && !s->getNonEmpty().empty()) {
			if (!(s->getNonEmpty().begin()->first->type & T_WORDFORM)) {
				error("%s: Error: There must be a wordform before any other tags in ADDCOHORT on line %u near `%S`!\n", lp);
			}
		}
	}

	bool copy_except = false;
	if (key == K_COPY && ux_simplecasecmp(p, stringbits[S_EXCEPT].getTerminatedBuffer(), stringbits[S_EXCEPT].length())) {
		p += stringbits[S_EXCEPT].length();
		copy_except = true;
	}

	result->lines += SKIPWS(p);
	lp = p;
	if (key == K_ADDRELATIONS || key == K_SETRELATIONS || key == K_REMRELATIONS || key == K_SETVARIABLE || copy_except) {
		swapper_false swp(no_isets, no_isets);
		Set *s = parseSetInlineWrapper(p);
		s->reindex(*result);
		rule->sublist = s;
		if (s->empty()) {
			error("%s: Error: Empty relation set on line %u near `%S`!\n", lp);
		}
		if (s->trie.empty() && s->trie_special.empty() && !(s->type & (ST_TAG_UNIFY | ST_SET_UNIFY | ST_CHILD_UNIFY))) {
			error("%s: Error: Relation/Value set on line %u near `%S` was neither unified nor of LIST type!\n", lp);
		}
	}

	if (key == K_ADDCOHORT) {
		if (ux_simplecasecmp(p, stringbits[S_AFTER].getTerminatedBuffer(), stringbits[S_AFTER].length())) {
			p += stringbits[S_AFTER].length();
			rule->type = K_ADDCOHORT_AFTER;
		}
		else if (ux_simplecasecmp(p, stringbits[S_BEFORE].getTerminatedBuffer(), stringbits[S_BEFORE].length())) {
			p += stringbits[S_BEFORE].length();
			rule->type = K_ADDCOHORT_BEFORE;
		}
		else {
			error("%s: Error: Expected position keyword AFTER or BEFORE on line %u near `%S`!\n", p);
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
			error("%s: Error: Expected closing ) on line %u near `%S`! Probably caused by missing set operator.\n", p);
		}
		++p;
		result->lines += SKIPWS(p);
	}

	if (key == K_SETPARENT || key == K_SETCHILD || key == K_ADDRELATIONS || key == K_ADDRELATION || key == K_SETRELATIONS || key == K_SETRELATION || key == K_REMRELATIONS || key == K_REMRELATION || key == K_MOVE || key == K_SWITCH) {
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
				error("%s: Error: Expected movement keyword AFTER or BEFORE on line %u near `%S`!\n", p);
			}
		}
		else if (key == K_SWITCH) {
			if (ux_simplecasecmp(p, stringbits[S_WITH].getTerminatedBuffer(), stringbits[S_WITH].length())) {
				p += stringbits[S_WITH].length();
			}
			else {
				error("%s: Error: Expected movement keyword WITH on line %u near `%S`!\n", p);
			}
		}
		else {
			if (ux_simplecasecmp(p, stringbits[S_TO].getTerminatedBuffer(), stringbits[S_TO].length())) {
				p += stringbits[S_TO].length();
			}
			else if (ux_simplecasecmp(p, stringbits[S_FROM].getTerminatedBuffer(), stringbits[S_FROM].length())) {
				p += stringbits[S_FROM].length();
				rule->flags |= RF_REVERSE;
			}
			else {
				error("%s: Error: Expected dependency keyword TO or FROM on line %u near `%S`!\n", p);
			}
		}
		result->lines += SKIPWS(p);

		if (key == K_MOVE) {
			if (ux_simplecasecmp(p, g_flags[FL_WITHCHILD].getTerminatedBuffer(), g_flags[FL_WITHCHILD].length())) {
				p += g_flags[FL_WITHCHILD].length();
				result->has_dep = true;
				Set *s = parseSetInlineWrapper(p);
				rule->childset2 = s->hash;
				result->lines += SKIPWS(p);
			}
			else if (ux_simplecasecmp(p, g_flags[FL_NOCHILD].getTerminatedBuffer(), g_flags[FL_NOCHILD].length())) {
				p += g_flags[FL_NOCHILD].length();
				rule->childset2 = 0;
				result->lines += SKIPWS(p);
			}
		}

		lp = p;
		while (*p && *p == '(') {
			++p;
			result->lines += SKIPWS(p);
			parseContextualDependencyTests(p, rule);
			result->lines += SKIPWS(p);
			if (*p != ')') {
				error("%s: Error: Expected closing ) on line %u near `%S`! Probably caused by missing set operator.\n", p);
			}
			++p;
			result->lines += SKIPWS(p);
		}
		if (rule->dep_tests.empty()) {
			error("%s: Error: Expected dependency target on line %u near `%S`!\n", lp);
		}
		rule->dep_target = rule->dep_tests.back();
		rule->dep_tests.pop_back();
	}
	if (key == K_SETPARENT || key == K_SETCHILD) {
		result->has_dep = true;
	}
	if (key == K_SETRELATION || key == K_SETRELATIONS || key == K_ADDRELATION || key == K_ADDRELATIONS || key == K_REMRELATION || key == K_REMRELATIONS) {
		result->has_relations = true;
	}

	if (!(rule->flags & RF_REMEMBERX)) {
		bool found = false;
		if (rule->dep_target && (rule->dep_target->pos & POS_MARK_JUMP)) {
			found = true;
		}
		else {
			foreach (it, rule->tests) {
				if ((*it)->pos & POS_MARK_JUMP) {
					found = true;
					break;
				}
			}
			foreach (it, rule->dep_tests) {
				if ((*it)->pos & POS_MARK_JUMP) {
					found = true;
					break;
				}
			}
		}
		if (found) {
			u_fprintf(ux_stderr, "%s: Warning: Rule on line %u had 'x' in the first part of a contextual test, but no REMEMBERX flag.\n", filebase, result->lines);
			u_fflush(ux_stderr);
		}
	}

	rule->reverseContextualTests();
	addRuleToGrammar(rule);

	result->lines += SKIPWS(p, ';');
	if (*p != ';') {
		u_fprintf(ux_stderr, "%s: Warning: Expected closing ; on line %u after previous rule!\n", filebase, result->lines);
		u_fflush(ux_stderr);
	}
}

void TextualParser::parseAnchorish(UChar *& p) {
	UChar *n = p;
	result->lines += SKIPTOWS(n, 0, true);
	ptrdiff_t c = n - p;
	u_strncpy(&gbuffers[0][0], p, c);
	gbuffers[0][c] = 0;
	result->addAnchor(&gbuffers[0][0], result->rule_by_number.size(), true);
	p = n;
	result->lines += SKIPWS(p, ';');
	if (*p != ';') {
		error("%s: Error: Expected closing ; on line %u near `%S` after anchor/section name!\n", p);
	}
}

int TextualParser::parseFromUChar(UChar *input, const char *fname) {
	if (!input || !input[0]) {
		u_fprintf(ux_stderr, "%s: Error: Input is empty - cannot continue!\n", fname);
		CG3Quit(1);
	}

	UChar *p = input;
	result->lines = 1;
	filebase = basename(const_cast<char*>(fname));

	while (*p) {
		try {
			if (verbosity_level > 0 && result->lines % 500 == 0) {
				std::cerr << "Parsing line " << result->lines << "          \r" << std::flush;
			}
			result->lines += SKIPWS(p);
			// DELIMITERS
			if (ISCHR(*p, 'D', 'd') && ISCHR(*(p + 9), 'S', 's') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'L', 'l') && ISCHR(*(p + 3), 'I', 'i') && ISCHR(*(p + 4), 'M', 'm') && ISCHR(*(p + 5), 'I', 'i') && ISCHR(*(p + 6), 'T', 't') && ISCHR(*(p + 7), 'E', 'e') && ISCHR(*(p + 8), 'R', 'r') && !ISSTRING(p, 9)) {
				if (result->delimiters) {
					error("%s: Error: Cannot redefine DELIMITERS on line %u near `%S`!\n", p);
				}
				result->delimiters = result->allocateSet();
				result->delimiters->line = result->lines;
				result->delimiters->setName(stringbits[S_DELIMITSET].getTerminatedBuffer());
				p += 10;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;
				parseTagList(p, result->delimiters);
				result->addSet(result->delimiters);
				if (result->delimiters->trie.empty() && result->delimiters->trie_special.empty()) {
					error("%s: Error: DELIMITERS declared, but no definitions given, on line %u near `%S`!\n", p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// SOFT-DELIMITERS
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 14), 'S', 's') && ISCHR(*(p + 1), 'O', 'o') && ISCHR(*(p + 2), 'F', 'f') && ISCHR(*(p + 3), 'T', 't') && ISCHR(*(p + 4), '-', '_') && ISCHR(*(p + 5), 'D', 'd') && ISCHR(*(p + 6), 'E', 'e') && ISCHR(*(p + 7), 'L', 'l') && ISCHR(*(p + 8), 'I', 'i') && ISCHR(*(p + 9), 'M', 'm') && ISCHR(*(p + 10), 'I', 'i') && ISCHR(*(p + 11), 'T', 't') && ISCHR(*(p + 12), 'E', 'e') && ISCHR(*(p + 13), 'R', 'r') && !ISSTRING(p, 14)) {
				if (result->soft_delimiters) {
					error("%s: Error: Cannot redefine SOFT-DELIMITERS on line %u near `%S`!\n", p);
				}
				result->soft_delimiters = result->allocateSet();
				result->soft_delimiters->line = result->lines;
				result->soft_delimiters->setName(stringbits[S_SOFTDELIMITSET].getTerminatedBuffer());
				p += 15;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;
				parseTagList(p, result->soft_delimiters);
				result->addSet(result->soft_delimiters);
				if (result->soft_delimiters->trie.empty() && result->soft_delimiters->trie_special.empty()) {
					error("%s: Error: SOFT-DELIMITERS declared, but no definitions given, on line %u near `%S`!\n", p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// MAPPING-PREFIX
			else if (ISCHR(*p, 'M', 'm') && ISCHR(*(p + 13), 'X', 'x') && ISCHR(*(p + 1), 'A', 'a') && ISCHR(*(p + 2), 'P', 'p') && ISCHR(*(p + 3), 'P', 'p') && ISCHR(*(p + 4), 'I', 'i') && ISCHR(*(p + 5), 'N', 'n') && ISCHR(*(p + 6), 'G', 'g') && ISCHR(*(p + 7), '-', '_') && ISCHR(*(p + 8), 'P', 'p') && ISCHR(*(p + 9), 'R', 'r') && ISCHR(*(p + 10), 'E', 'e') && ISCHR(*(p + 11), 'F', 'f') && ISCHR(*(p + 12), 'I', 'i') && !ISSTRING(p, 13)) {
				if (seen_mapping_prefix) {
					u_fprintf(ux_stderr, "%s: Error: MAPPING-PREFIX on line %u cannot change previous prefix set on line %u!\n", filebase, result->lines, seen_mapping_prefix);
					incErrorCount();
				}
				seen_mapping_prefix = result->lines;

				p += 14;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
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
					error("%s: Error: MAPPING-PREFIX declared, but no definitions given, on line %u near `%S`!\n", p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// PREFERRED-TARGETS
			else if (ISCHR(*p, 'P', 'p') && ISCHR(*(p + 16), 'S', 's') && ISCHR(*(p + 1), 'R', 'r') && ISCHR(*(p + 2), 'E', 'e') && ISCHR(*(p + 3), 'F', 'f') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'R', 'r') && ISCHR(*(p + 6), 'R', 'r') && ISCHR(*(p + 7), 'E', 'e') && ISCHR(*(p + 8), 'D', 'd') && ISCHR(*(p + 9), '-', '_') && ISCHR(*(p + 10), 'T', 't') && ISCHR(*(p + 11), 'A', 'a') && ISCHR(*(p + 12), 'R', 'r') && ISCHR(*(p + 13), 'G', 'g') && ISCHR(*(p + 14), 'E', 'e') && ISCHR(*(p + 15), 'T', 't') && !ISSTRING(p, 16)) {
				p += 17;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;
				result->lines += SKIPWS(p);

				while (*p && *p != ';') {
					UChar *n = p;
					if (*n == '"') {
						n++;
						SKIPTO_NOSPAN(n, '"');
						if (*n != '"') {
							error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
						}
					}
					result->lines += SKIPTOWS(n, ';', true);
					ptrdiff_t c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					Tag *t = parseTag(&gbuffers[0][0], p);
					result->preferred_targets.push_back(t->hash);
					p = n;
					result->lines += SKIPWS(p);
				}

				if (result->preferred_targets.empty()) {
					error("%s: Error: PREFERRED-TARGETS declared, but no definitions given, on line %u near `%S`!\n", p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// REOPEN-MAPPINGS
			else if (ISCHR(*p, 'R', 'r') && ISCHR(*(p + 14), 'S', 's') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'O', 'o') && ISCHR(*(p + 3), 'P', 'p') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'N', 'n') && ISCHR(*(p + 6), '-', '_') && ISCHR(*(p + 7), 'M', 'm') && ISCHR(*(p + 8), 'A', 'a') && ISCHR(*(p + 9), 'P', 'p') && ISCHR(*(p + 10), 'P', 'p') && ISCHR(*(p + 11), 'I', 'i') && ISCHR(*(p + 12), 'N', 'n') && ISCHR(*(p + 13), 'G', 'g') && !ISSTRING(p, 14)) {
				p += 15;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;
				result->lines += SKIPWS(p);

				while (*p && *p != ';') {
					UChar *n = p;
					if (*n == '"') {
						n++;
						SKIPTO_NOSPAN(n, '"');
						if (*n != '"') {
							error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
						}
					}
					result->lines += SKIPTOWS(n, ';', true);
					ptrdiff_t c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					Tag *t = parseTag(&gbuffers[0][0], p);
					result->reopen_mappings.insert(t->hash);
					p = n;
					result->lines += SKIPWS(p);
				}

				if (result->reopen_mappings.empty()) {
					error("%s: Error: REOPEN-MAPPINGS declared, but no definitions given, on line %u near `%S`!\n", p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// STATIC-SETS
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 10), 'S', 's') && ISCHR(*(p + 1), 'T', 't') && ISCHR(*(p + 2), 'A', 'a') && ISCHR(*(p + 3), 'T', 't') && ISCHR(*(p + 4), 'I', 'i') && ISCHR(*(p + 5), 'C', 'c') && ISCHR(*(p + 6), '-', '-') && ISCHR(*(p + 7), 'S', 's') && ISCHR(*(p + 8), 'E', 'e') && ISCHR(*(p + 9), 'T', 't') && !ISSTRING(p, 10)) {
				p += 11;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;
				result->lines += SKIPWS(p);

				while (*p && *p != ';') {
					UChar *n = p;
					result->lines += SKIPTOWS(n, ';', true);
					result->static_sets.push_back(UString(p, n));
					p = n;
					result->lines += SKIPWS(p);
				}

				if (result->static_sets.empty()) {
					error("%s: Error: STATIC-SETS declared, but no definitions given, on line %u near `%S`!\n", p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// ADDRELATIONS
			else if (ISCHR(*p, 'A', 'a') && ISCHR(*(p + 11), 'S', 's') && ISCHR(*(p + 1), 'D', 'd') && ISCHR(*(p + 2), 'D', 'd') && ISCHR(*(p + 3), 'R', 'r') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'L', 'l') && ISCHR(*(p + 6), 'A', 'a') && ISCHR(*(p + 7), 'T', 't') && ISCHR(*(p + 8), 'I', 'i') && ISCHR(*(p + 9), 'O', 'o') && ISCHR(*(p + 10), 'N', 'n') && !ISSTRING(p, 11)) {
				parseRule(p, K_ADDRELATIONS);
			}
			// SETRELATIONS
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 11), 'S', 's') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 3), 'R', 'r') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'L', 'l') && ISCHR(*(p + 6), 'A', 'a') && ISCHR(*(p + 7), 'T', 't') && ISCHR(*(p + 8), 'I', 'i') && ISCHR(*(p + 9), 'O', 'o') && ISCHR(*(p + 10), 'N', 'n') && !ISSTRING(p, 11)) {
				parseRule(p, K_SETRELATIONS);
			}
			// REMRELATIONS
			else if (ISCHR(*p, 'R', 'r') && ISCHR(*(p + 11), 'S', 's') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'M', 'm') && ISCHR(*(p + 3), 'R', 'r') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'L', 'l') && ISCHR(*(p + 6), 'A', 'a') && ISCHR(*(p + 7), 'T', 't') && ISCHR(*(p + 8), 'I', 'i') && ISCHR(*(p + 9), 'O', 'o') && ISCHR(*(p + 10), 'N', 'n') && !ISSTRING(p, 11)) {
				parseRule(p, K_REMRELATIONS);
			}
			// ADDRELATION
			else if (ISCHR(*p, 'A', 'a') && ISCHR(*(p + 10), 'N', 'n') && ISCHR(*(p + 1), 'D', 'd') && ISCHR(*(p + 2), 'D', 'd') && ISCHR(*(p + 3), 'R', 'r') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'L', 'l') && ISCHR(*(p + 6), 'A', 'a') && ISCHR(*(p + 7), 'T', 't') && ISCHR(*(p + 8), 'I', 'i') && ISCHR(*(p + 9), 'O', 'o') && !ISSTRING(p, 10)) {
				parseRule(p, K_ADDRELATION);
			}
			// SETRELATION
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 10), 'N', 'n') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 3), 'R', 'r') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'L', 'l') && ISCHR(*(p + 6), 'A', 'a') && ISCHR(*(p + 7), 'T', 't') && ISCHR(*(p + 8), 'I', 'i') && ISCHR(*(p + 9), 'O', 'o') && !ISSTRING(p, 10)) {
				parseRule(p, K_SETRELATION);
			}
			// REMRELATION
			else if (ISCHR(*p, 'R', 'r') && ISCHR(*(p + 10), 'N', 'n') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'M', 'm') && ISCHR(*(p + 3), 'R', 'r') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'L', 'l') && ISCHR(*(p + 6), 'A', 'a') && ISCHR(*(p + 7), 'T', 't') && ISCHR(*(p + 8), 'I', 'i') && ISCHR(*(p + 9), 'O', 'o') && !ISSTRING(p, 10)) {
				parseRule(p, K_REMRELATION);
			}
			// SETVARIABLE
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 10), 'E', 'e') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 3), 'V', 'v') && ISCHR(*(p + 4), 'A', 'a') && ISCHR(*(p + 5), 'R', 'r') && ISCHR(*(p + 6), 'I', 'i') && ISCHR(*(p + 7), 'A', 'a') && ISCHR(*(p + 8), 'B', 'b') && ISCHR(*(p + 9), 'L', 'l') && !ISSTRING(p, 10)) {
				parseRule(p, K_SETVARIABLE);
			}
			// REMVARIABLE
			else if (ISCHR(*p, 'R', 'r') && ISCHR(*(p + 10), 'E', 'e') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'M', 'm') && ISCHR(*(p + 3), 'V', 'v') && ISCHR(*(p + 4), 'A', 'a') && ISCHR(*(p + 5), 'R', 'r') && ISCHR(*(p + 6), 'I', 'i') && ISCHR(*(p + 7), 'A', 'a') && ISCHR(*(p + 8), 'B', 'b') && ISCHR(*(p + 9), 'L', 'l') && !ISSTRING(p, 10)) {
				parseRule(p, K_REMVARIABLE);
			}
			// SETPARENT
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 8), 'T', 't') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 3), 'P', 'p') && ISCHR(*(p + 4), 'A', 'a') && ISCHR(*(p + 5), 'R', 'r') && ISCHR(*(p + 6), 'E', 'e') && ISCHR(*(p + 7), 'N', 'n') && !ISSTRING(p, 8)) {
				parseRule(p, K_SETPARENT);
			}
			// SETCHILD
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 7), 'D', 'd') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 3), 'C', 'c') && ISCHR(*(p + 4), 'H', 'h') && ISCHR(*(p + 5), 'I', 'i') && ISCHR(*(p + 6), 'L', 'l') && !ISSTRING(p, 7)) {
				parseRule(p, K_SETCHILD);
			}
			// EXTERNAL
			else if (ISCHR(*p, 'E', 'e') && ISCHR(*(p + 7), 'L', 'l') && ISCHR(*(p + 1), 'X', 'x') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 3), 'E', 'e') && ISCHR(*(p + 4), 'R', 'r') && ISCHR(*(p + 5), 'N', 'n') && ISCHR(*(p + 6), 'A', 'a') && !ISSTRING(p, 7)) {
				parseRule(p, K_EXTERNAL);
			}
			// REMCOHORT
			else if (ISCHR(*p, 'R', 'r') && ISCHR(*(p + 8), 'T', 't') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'M', 'm') && ISCHR(*(p + 3), 'C', 'c') && ISCHR(*(p + 4), 'O', 'o') && ISCHR(*(p + 5), 'H', 'h') && ISCHR(*(p + 6), 'O', 'o') && ISCHR(*(p + 7), 'R', 'r') && !ISSTRING(p, 8)) {
				parseRule(p, K_REMCOHORT);
			}
			// ADDCOHORT
			else if (ISCHR(*p, 'A', 'a') && ISCHR(*(p + 8), 'T', 't') && ISCHR(*(p + 1), 'D', 'd') && ISCHR(*(p + 2), 'D', 'd') && ISCHR(*(p + 3), 'C', 'c') && ISCHR(*(p + 4), 'O', 'o') && ISCHR(*(p + 5), 'H', 'h') && ISCHR(*(p + 6), 'O', 'o') && ISCHR(*(p + 7), 'R', 'r') && !ISSTRING(p, 8)) {
				parseRule(p, K_ADDCOHORT);
			}
			// SETS
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 3), 'S', 's') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'T', 't') && !ISSTRING(p, 3)) {
				p += 4;
			}
			// LIST
			else if (ISCHR(*p, 'L', 'l') && ISCHR(*(p + 3), 'T', 't') && ISCHR(*(p + 1), 'I', 'i') && ISCHR(*(p + 2), 'S', 's') && !ISSTRING(p, 3)) {
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
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;
				parseTagList(p, s);
				s->rehash();
				Set *tmp = result->getSet(s->hash);
				if (tmp) {
					if (verbosity_level > 0 && tmp->name[0] != '_' && tmp->name[1] != 'G' && tmp->name[2] != '_') {
						u_fprintf(ux_stderr, "%s: Warning: LIST %S was defined twice with the same contents: Lines %u and %u.\n", filebase, s->name.c_str(), tmp->line, s->line);
						u_fflush(ux_stderr);
					}
				}
				result->addSet(s);
				if (s->empty()) {
					error("%s: Error: LIST %S declared, but no definitions given, on line %u near `%S`!\n", s->name.c_str(), p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// SET
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 1), 'E', 'e') && !ISSTRING(p, 2)) {
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
				uint32_t sh = hash_value(&gbuffers[0][0]);
				p = n;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;

				swapper_false swp(no_isets, no_isets);

				parseSetInline(p, s);
				s->rehash();
				Set *tmp = result->getSet(s->hash);
				if (tmp) {
					if (verbosity_level > 0 && tmp->name[0] != '_' && tmp->name[1] != 'G' && tmp->name[2] != '_') {
						u_fprintf(ux_stderr, "%s: Warning: SET %S was defined twice with the same contents: Lines %u and %u.\n", filebase, s->name.c_str(), tmp->line, s->line);
						u_fflush(ux_stderr);
					}
				}
				else if (s->sets.size() == 1 && !(s->type & ST_TAG_UNIFY)) {
					tmp = result->getSet(s->sets.back());
					if (verbosity_level > 0) {
						u_fprintf(ux_stderr, "%s: Warning: Set %S on line %u aliased to %S on line %u.\n", filebase, s->name.c_str(), s->line, tmp->name.c_str(), tmp->line);
						u_fflush(ux_stderr);
					}
					result->maybe_used_sets.insert(tmp);
					result->set_alias[sh] = tmp->hash;
					result->destroySet(s);
					s = tmp;
				}
				result->addSet(s);
				if (s->empty()) {
					error("%s: Error: SET %S declared, but no definitions given, on line %u near `%S`!\n", s->name.c_str(), p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`! Probably caused by missing set operator.\n", p);
				}
			}
			// MAPPINGS
			else if (ISCHR(*p, 'M', 'm') && ISCHR(*(p + 7), 'S', 's') && ISCHR(*(p + 1), 'A', 'a') && ISCHR(*(p + 2), 'P', 'p') && ISCHR(*(p + 3), 'P', 'p') && ISCHR(*(p + 4), 'I', 'i') && ISCHR(*(p + 5), 'N', 'n') && ISCHR(*(p + 6), 'G', 'g') && !ISSTRING(p, 7)) {
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
					parseAnchorish(p);
				}
			}
			// CORRECTIONS
			else if (ISCHR(*p, 'C', 'c') && ISCHR(*(p + 10), 'S', 's') && ISCHR(*(p + 1), 'O', 'o') && ISCHR(*(p + 2), 'R', 'r') && ISCHR(*(p + 3), 'R', 'r') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'C', 'c') && ISCHR(*(p + 6), 'T', 't') && ISCHR(*(p + 7), 'I', 'i') && ISCHR(*(p + 8), 'O', 'o') && ISCHR(*(p + 9), 'N', 'n') && !ISSTRING(p, 10)) {
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
					parseAnchorish(p);
				}
			}
			// BEFORE-SECTIONS
			else if (ISCHR(*p, 'B', 'b') && ISCHR(*(p + 14), 'S', 's') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'F', 'f') && ISCHR(*(p + 3), 'O', 'o') && ISCHR(*(p + 4), 'R', 'r') && ISCHR(*(p + 5), 'E', 'e') && ISCHR(*(p + 6), '-', '_') && ISCHR(*(p + 7), 'S', 's') && ISCHR(*(p + 8), 'E', 'e') && ISCHR(*(p + 9), 'C', 'c') && ISCHR(*(p + 10), 'T', 't') && ISCHR(*(p + 11), 'I', 'i') && ISCHR(*(p + 12), 'O', 'o') && ISCHR(*(p + 13), 'N', 'n') && !ISSTRING(p, 14)) {
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
					parseAnchorish(p);
				}
			}
			// SECTION
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 6), 'N', 'n') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'C', 'c') && ISCHR(*(p + 3), 'T', 't') && ISCHR(*(p + 4), 'I', 'i') && ISCHR(*(p + 5), 'O', 'o') && !ISSTRING(p, 6)) {
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
					parseAnchorish(p);
				}
			}
			// CONSTRAINTS
			else if (ISCHR(*p, 'C', 'c') && ISCHR(*(p + 10), 'S', 's') && ISCHR(*(p + 1), 'O', 'o') && ISCHR(*(p + 2), 'N', 'n') && ISCHR(*(p + 3), 'S', 's') && ISCHR(*(p + 4), 'T', 't') && ISCHR(*(p + 5), 'R', 'r') && ISCHR(*(p + 6), 'A', 'a') && ISCHR(*(p + 7), 'I', 'i') && ISCHR(*(p + 8), 'N', 'n') && ISCHR(*(p + 9), 'T', 't') && !ISSTRING(p, 10)) {
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
					parseAnchorish(p);
				}
			}
			// AFTER-SECTIONS
			else if (ISCHR(*p, 'A', 'a') && ISCHR(*(p + 13), 'S', 's') && ISCHR(*(p + 1), 'F', 'f') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 3), 'E', 'e') && ISCHR(*(p + 4), 'R', 'r') && ISCHR(*(p + 5), '-', '_') && ISCHR(*(p + 6), 'S', 's') && ISCHR(*(p + 7), 'E', 'e') && ISCHR(*(p + 8), 'C', 'c') && ISCHR(*(p + 9), 'T', 't') && ISCHR(*(p + 10), 'I', 'i') && ISCHR(*(p + 11), 'O', 'o') && ISCHR(*(p + 12), 'N', 'n') && !ISSTRING(p, 13)) {
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
					parseAnchorish(p);
				}
			}
			// NULL-SECTION
			else if (ISCHR(*p, 'N', 'n') && ISCHR(*(p + 11), 'N', 'n') && ISCHR(*(p + 1), 'U', 'u') && ISCHR(*(p + 2), 'L', 'l') && ISCHR(*(p + 3), 'L', 'l') && ISCHR(*(p + 4), '-', '-') && ISCHR(*(p + 5), 'S', 's') && ISCHR(*(p + 6), 'E', 'e') && ISCHR(*(p + 7), 'C', 'c') && ISCHR(*(p + 8), 'T', 't') && ISCHR(*(p + 9), 'I', 'i') && ISCHR(*(p + 10), 'O', 'o') && !ISSTRING(p, 11)) {
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
					parseAnchorish(p);
				}
			}
			// SUBREADINGS
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 10), 'S', 's') && ISCHR(*(p + 1), 'U', 'u') && ISCHR(*(p + 2), 'B', 'b') && ISCHR(*(p + 3), 'R', 'r') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'A', 'a') && ISCHR(*(p + 6), 'D', 'd') && ISCHR(*(p + 7), 'I', 'i') && ISCHR(*(p + 8), 'N', 'n') && ISCHR(*(p + 9), 'G', 'g') && !ISSTRING(p, 10)) {
				p += 11;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;
				result->lines += SKIPWS(p);
				if (p[0] == 'L' || p[0] == 'l') {
					result->sub_readings_ltr = true;
				}
				else if (p[0] == 'R' || p[0] == 'r') {
					result->sub_readings_ltr = false;
				}
				else {
					error("%s: Error: Expected RTL or LTR on line %u near `%S`!\n", *p, p);
				}
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				p = n;
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// OPTIONS
			else if (ISCHR(*p, 'O', 'o') && ISCHR(*(p + 6), 'S', 's') && ISCHR(*(p + 1), 'P', 'p') && ISCHR(*(p + 2), 'T', 't') && ISCHR(*(p + 3), 'I', 'i') && ISCHR(*(p + 4), 'O', 'o') && ISCHR(*(p + 5), 'N', 'n') && !ISSTRING(p, 6)) {
				p += 7;
				result->lines += SKIPWS(p, '+');
				if (p[0] != '+' || p[1] != '=') {
					error("%s: Error: Encountered a %C before the expected += on line %u near `%S`!\n", *p, p);
				}
				p += 2;
				result->lines += SKIPWS(p);

				typedef std::pair<size_t, bool*> pairs_t;
				pairs_t pairs[] = {
					std::pair<size_t, bool*>(S_NO_ISETS, &no_isets),
					std::pair<size_t, bool*>(S_NO_ITMPLS, &no_itmpls),
					std::pair<size_t, bool*>(S_STRICT_WFORMS, &strict_wforms),
					std::pair<size_t, bool*>(S_STRICT_BFORMS, &strict_bforms),
					std::pair<size_t, bool*>(S_STRICT_SECOND, &strict_second),
				};

				while (*p != ';') {
					bool found = false;
					boost_foreach (pairs_t& pair, pairs) {
						if (ux_simplecasecmp(p, stringbits[pair.first].getTerminatedBuffer(), stringbits[pair.first].length())) {
							p += stringbits[pair.first].length();
							*pair.second = true;
							result->lines += SKIPWS(p);
							found = true;
						}
					}
					if (!found) {
						error("%s: Error: Invalid option found on line %u near `%S`!\n", p);
					}
				}

				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// STRICT-TAGS
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 10), 'S', 's') && ISCHR(*(p + 1), 'T', 't') && ISCHR(*(p + 2), 'R', 'r') && ISCHR(*(p + 3), 'I', 'i') && ISCHR(*(p + 4), 'C', 'c') && ISCHR(*(p + 5), 'T', 't') && ISCHR(*(p + 6), '-', '-') && ISCHR(*(p + 7), 'T', 't') && ISCHR(*(p + 8), 'A', 'a') && ISCHR(*(p + 9), 'G', 'g') && !ISSTRING(p, 10)) {
				p += 11;
				result->lines += SKIPWS(p, '+');
				if (p[0] != '+' || p[1] != '=') {
					error("%s: Error: Encountered a %C before the expected += on line %u near `%S`!\n", *p, p);
				}
				p += 2;
				result->lines += SKIPWS(p);

				uint32SortedVector tmp;
				strict_tags.swap(tmp);
				while (*p && *p != ';') {
					UChar *n = p;
					if (*n == '"') {
						n++;
						SKIPTO_NOSPAN(n, '"');
						if (*n != '"') {
							error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
						}
					}
					result->lines += SKIPTOWS(n, ';', true);
					ptrdiff_t c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					Tag *t = parseTag(&gbuffers[0][0], p);
					tmp.insert(t->hash);
					p = n;
					result->lines += SKIPWS(p);
				}

				if (tmp.empty()) {
					error("%s: Error: STRICT-TAGS declared, but no definitions given, on line %u near `%S`!\n", p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
				strict_tags.swap(tmp);
			}
			// ANCHOR
			else if (ISCHR(*p, 'A', 'a') && ISCHR(*(p + 5), 'R', 'r') && ISCHR(*(p + 1), 'N', 'n') && ISCHR(*(p + 2), 'C', 'c') && ISCHR(*(p + 3), 'H', 'h') && ISCHR(*(p + 4), 'O', 'o') && !ISSTRING(p, 5)) {
				p += 6;
				result->lines += SKIPWS(p);
				parseAnchorish(p);
			}
			// INCLUDE
			else if (ISCHR(*p, 'I', 'i') && ISCHR(*(p + 6), 'E', 'e') && ISCHR(*(p + 1), 'N', 'n') && ISCHR(*(p + 2), 'C', 'c') && ISCHR(*(p + 3), 'L', 'l') && ISCHR(*(p + 4), 'U', 'u') && ISCHR(*(p + 5), 'D', 'd') && !ISSTRING(p, 6)) {
				p += 7;
				result->lines += SKIPWS(p);
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				p = n;
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}

				UErrorCode err = U_ZERO_ERROR;
				u_strToUTF8(&cbuffers[0][0], CG3_BUFFER_SIZE - 1, 0, &gbuffers[0][0], u_strlen(&gbuffers[0][0]), &err);

				std::string abspath;
				if (cbuffers[0][0] == '/') {
					abspath = &cbuffers[0][0];
				}
				else {
					abspath = ux_dirname(fname);
					abspath += &cbuffers[0][0];
				}

				size_t grammar_size = 0;
				struct stat _stat;
				int error = stat(abspath.c_str(), &_stat);

				if (error != 0) {
					abspath = &cbuffers[0][0];
					error = stat(abspath.c_str(), &_stat);
				}

				if (error != 0) {
					u_fprintf(ux_stderr, "%s: Error: Cannot stat %s due to error %d - bailing out!\n", filebase, abspath.c_str(), error);
					CG3Quit(1);
				}
				else {
					grammar_size = static_cast<size_t>(_stat.st_size);
				}

				UFILE *grammar = u_fopen(abspath.c_str(), "rb", locale, codepage);
				if (!grammar) {
					u_fprintf(ux_stderr, "%s: Error: Error opening %s for reading!\n", filebase, abspath.c_str());
					CG3Quit(1);
				}
				UChar32 bom = u_fgetcx(grammar);
				if (bom != 0xfeff && bom != static_cast<UChar32>(0xffffffff)) {
					u_fungetc(bom, grammar);
				}

				std::vector<UChar> data(grammar_size * 2, 0);
				uint32_t read = u_file_read(&data[4], grammar_size * 2, grammar);
				u_fclose(grammar);
				if (read >= grammar_size * 2 - 1) {
					u_fprintf(ux_stderr, "%s: Error: Converting from underlying codepage to UTF-16 exceeded factor 2 buffer.\n", filebase);
					CG3Quit(1);
				}
				data.resize(read + 4 + 1);

				uint32_t olines = 0;
				swapper<uint32_t> oswap(true, olines, result->lines);
				const char *obase = 0;
				swapper<const char*> bswap(true, obase, filebase);

				parseFromUChar(&data[4], abspath.c_str());
			}
			// IFF
			else if (ISCHR(*p, 'I', 'i') && ISCHR(*(p + 2), 'F', 'f') && ISCHR(*(p + 1), 'F', 'f') && !ISSTRING(p, 2)) {
				parseRule(p, K_IFF);
			}
			// MAP
			else if (ISCHR(*p, 'M', 'm') && ISCHR(*(p + 2), 'P', 'p') && ISCHR(*(p + 1), 'A', 'a') && !ISSTRING(p, 2)) {
				parseRule(p, K_MAP);
			}
			// ADD
			else if (ISCHR(*p, 'A', 'a') && ISCHR(*(p + 2), 'D', 'd') && ISCHR(*(p + 1), 'D', 'd') && !ISSTRING(p, 2)) {
				parseRule(p, K_ADD);
			}
			// APPEND
			else if (ISCHR(*p, 'A', 'a') && ISCHR(*(p + 5), 'D', 'd') && ISCHR(*(p + 1), 'P', 'p') && ISCHR(*(p + 2), 'P', 'p') && ISCHR(*(p + 3), 'E', 'e') && ISCHR(*(p + 4), 'N', 'n') && !ISSTRING(p, 5)) {
				parseRule(p, K_APPEND);
			}
			// SELECT
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 5), 'T', 't') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'L', 'l') && ISCHR(*(p + 3), 'E', 'e') && ISCHR(*(p + 4), 'C', 'c') && !ISSTRING(p, 5)) {
				parseRule(p, K_SELECT);
			}
			// REMOVE
			else if (ISCHR(*p, 'R', 'r') && ISCHR(*(p + 5), 'E', 'e') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'M', 'm') && ISCHR(*(p + 3), 'O', 'o') && ISCHR(*(p + 4), 'V', 'v') && !ISSTRING(p, 5)) {
				parseRule(p, K_REMOVE);
			}
			// REPLACE
			else if (ISCHR(*p, 'R', 'r') && ISCHR(*(p + 6), 'E', 'e') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'P', 'p') && ISCHR(*(p + 3), 'L', 'l') && ISCHR(*(p + 4), 'A', 'a') && ISCHR(*(p + 5), 'C', 'c') && !ISSTRING(p, 6)) {
				parseRule(p, K_REPLACE);
			}
			// DELIMIT
			else if (ISCHR(*p, 'D', 'd') && ISCHR(*(p + 6), 'T', 't') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'L', 'l') && ISCHR(*(p + 3), 'I', 'i') && ISCHR(*(p + 4), 'M', 'm') && ISCHR(*(p + 5), 'I', 'i') && !ISSTRING(p, 6)) {
				parseRule(p, K_DELIMIT);
			}
			// SUBSTITUTE
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 9), 'E', 'e') && ISCHR(*(p + 1), 'U', 'u') && ISCHR(*(p + 2), 'B', 'b') && ISCHR(*(p + 3), 'S', 's') && ISCHR(*(p + 4), 'T', 't') && ISCHR(*(p + 5), 'I', 'i') && ISCHR(*(p + 6), 'T', 't') && ISCHR(*(p + 7), 'U', 'u') && ISCHR(*(p + 8), 'T', 't') && !ISSTRING(p, 9)) {
				parseRule(p, K_SUBSTITUTE);
			}
			// COPY
			else if (ISCHR(*p, 'C', 'c') && ISCHR(*(p + 3), 'Y', 'y') && ISCHR(*(p + 1), 'O', 'o') && ISCHR(*(p + 2), 'P', 'p') && !ISSTRING(p, 3)) {
				parseRule(p, K_COPY);
			}
			// JUMP
			else if (ISCHR(*p, 'J', 'j') && ISCHR(*(p + 3), 'P', 'p') && ISCHR(*(p + 1), 'U', 'u') && ISCHR(*(p + 2), 'M', 'm') && !ISSTRING(p, 3)) {
				parseRule(p, K_JUMP);
			}
			// MOVE
			else if (ISCHR(*p, 'M', 'm') && ISCHR(*(p + 3), 'E', 'e') && ISCHR(*(p + 1), 'O', 'o') && ISCHR(*(p + 2), 'V', 'v') && !ISSTRING(p, 3)) {
				parseRule(p, K_MOVE);
			}
			// SWITCH
			else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 5), 'H', 'h') && ISCHR(*(p + 1), 'W', 'w') && ISCHR(*(p + 2), 'I', 'i') && ISCHR(*(p + 3), 'T', 't') && ISCHR(*(p + 4), 'C', 'c') && !ISSTRING(p, 5)) {
				parseRule(p, K_SWITCH);
			}
			// EXECUTE
			else if (ISCHR(*p, 'E', 'e') && ISCHR(*(p + 6), 'E', 'e') && ISCHR(*(p + 1), 'X', 'x') && ISCHR(*(p + 2), 'E', 'e') && ISCHR(*(p + 3), 'C', 'c') && ISCHR(*(p + 4), 'U', 'u') && ISCHR(*(p + 5), 'T', 't') && !ISSTRING(p, 6)) {
				parseRule(p, K_EXECUTE);
			}
			// UNMAP
			else if (ISCHR(*p, 'U', 'u') && ISCHR(*(p + 4), 'P', 'p') && ISCHR(*(p + 1), 'N', 'n') && ISCHR(*(p + 2), 'M', 'm') && ISCHR(*(p + 3), 'A', 'a') && !ISSTRING(p, 4)) {
				parseRule(p, K_UNMAP);
			}
			// TEMPLATE
			else if (ISCHR(*p, 'T', 't') && ISCHR(*(p + 7), 'E', 'e') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'M', 'm') && ISCHR(*(p + 3), 'P', 'p') && ISCHR(*(p + 4), 'L', 'l') && ISCHR(*(p + 5), 'A', 'a') && ISCHR(*(p + 6), 'T', 't') && !ISSTRING(p, 7)) {
				size_t line = result->lines;
				p += 8;
				result->lines += SKIPWS(p);
				UChar *n = p;
				result->lines += SKIPTOWS(n, 0, true);
				ptrdiff_t c = n - p;
				u_strncpy(&gbuffers[0][0], p, c);
				gbuffers[0][c] = 0;
				UString name(&gbuffers[0][0]);
				p = n;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
				}
				++p;

				swapper_false swp(no_itmpls, no_itmpls);

				ContextualTest *t = parseContextualTestList(p);
				t->line = line;
				result->addTemplate(t, name.c_str());

				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`! Probably caused by missing set operator.\n", p);
				}
			}
			// PARENTHESES
			else if (ISCHR(*p, 'P', 'p') && ISCHR(*(p + 10), 'S', 's') && ISCHR(*(p + 1), 'A', 'a') && ISCHR(*(p + 2), 'R', 'r') && ISCHR(*(p + 3), 'E', 'e') && ISCHR(*(p + 4), 'N', 'n') && ISCHR(*(p + 5), 'T', 't') && ISCHR(*(p + 6), 'H', 'h') && ISCHR(*(p + 7), 'E', 'e') && ISCHR(*(p + 8), 'S', 's') && ISCHR(*(p + 9), 'E', 'e') && !ISSTRING(p, 10)) {
				p += 11;
				result->lines += SKIPWS(p, '=');
				if (*p != '=') {
					error("%s: Error: Encountered a %C before the expected = on line %u near `%S`!\n", *p, p);
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
						error("%s: Error: Encountered %C before the expected ( on line %u near `%S`!\n", *p, p);
					}
					n++;
					result->lines += SKIPWS(n);
					p = n;
					if (*n == '"') {
						n++;
						SKIPTO_NOSPAN(n, '"');
						if (*n != '"') {
							error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
						}
					}
					result->lines += SKIPTOWS(n, ')', true);
					c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					left = parseTag(&gbuffers[0][0], p);
					result->lines += SKIPWS(n);
					p = n;

					if (*p == ')') {
						error("%s: Error: Encountered ) before the expected Right tag on line %u near `%S`!\n", p);
					}

					if (*n == '"') {
						n++;
						SKIPTO_NOSPAN(n, '"');
						if (*n != '"') {
							error("%s: Error: Expected closing \" on line %u near `%S`!\n", p);
						}
					}
					result->lines += SKIPTOWS(n, ')', true);
					c = n - p;
					u_strncpy(&gbuffers[0][0], p, c);
					gbuffers[0][c] = 0;
					right = parseTag(&gbuffers[0][0], p);
					result->lines += SKIPWS(n);
					p = n;

					if (*p != ')') {
						error("%s: Error: Encountered %C before the expected ) on line %u near `%S`!\n", *p, p);
					}
					++p;
					result->lines += SKIPWS(p);

					if (left && right) {
						result->parentheses[left->hash] = right->hash;
						result->parentheses_reverse[right->hash] = left->hash;
					}
				}

				if (result->parentheses.empty()) {
					error("%s: Error: PARENTHESES declared, but no definitions given, on line %u near `%S`!\n", p);
				}
				result->lines += SKIPWS(p, ';');
				if (*p != ';') {
					error("%s: Error: Expected closing ; before line %u near `%S`!\n", p);
				}
			}
			// END
			else if (ISCHR(*p, 'E', 'e') && ISCHR(*(p + 2), 'D', 'd') && ISCHR(*(p + 1), 'N', 'n')) {
				if (ISNL(*(p - 1)) || ISSPACE(*(p - 1))) {
					if (*(p + 3) == 0 || ISNL(*(p + 3)) || ISSPACE(*(p + 3))) {
						break;
					}
				}
				++p;
			}
			// No keyword found at this position, skip a character.
			else {
				UChar *n = p;
				if (*p == ';' || *p == '"') {
					if (*p == '"') {
						++p;
						SKIPTO_NOSPAN(p, '"');
						if (*p != '"') {
							error("%s: Error: Expected closing \" on line %u near `%S`!\n", n);
						}
					}
					result->lines += SKIPTOWS(p);
				}
				if (*p && *p != ';' && *p != '"' && !ISNL(*p) && !ISSPACE(*p)) {
					error("%s: Error: Garbage data encountered on line %u near `%S`!\n", p);
				}
				if (ISNL(*p)) {
					result->lines += 1;
				}
				++p;
			}
		}
		catch (int) {
			result->lines += SKIPLN(p);
		}
	}

	return 0;
}

int TextualParser::parse_grammar_from_file(const char *fname, const char *loc, const char *cpage) {
	filename = fname;
	filebase = basename(const_cast<char*>(fname));
	locale = loc;
	codepage = cpage;

	if (!result) {
		u_fprintf(ux_stderr, "%s: Error: Cannot parse into nothing - hint: call setResult() before trying.\n", filebase);
		CG3Quit(1);
	}

	struct stat _stat;
	int error = stat(filename, &_stat);

	if (error != 0) {
		u_fprintf(ux_stderr, "%s: Error: Cannot stat %s due to error %d - bailing out!\n", filebase, filename, error);
		CG3Quit(1);
	}
	else {
		result->grammar_size = static_cast<size_t>(_stat.st_size);
	}

	UFILE *grammar = u_fopen(filename, "rb", locale, codepage);
	if (!grammar) {
		u_fprintf(ux_stderr, "%s: Error: Error opening %s for reading!\n", filebase, filename);
		CG3Quit(1);
	}
	UChar32 bom = u_fgetcx(grammar);
	if (bom != 0xfeff && bom != static_cast<UChar32>(0xffffffff)) {
		u_fungetc(bom, grammar);
	}

	// It reads into the buffer at offset 4 because certain functions may look back, so we need some nulls in front.
	std::vector<UChar> data(result->grammar_size * 2, 0);
	uint32_t read = u_file_read(&data[4], result->grammar_size * 2, grammar);
	u_fclose(grammar);
	if (read >= result->grammar_size * 2 - 1) {
		u_fprintf(ux_stderr, "%s: Error: Converting from underlying codepage to UTF-16 exceeded factor 2 buffer.\n", filebase);
		CG3Quit(1);
	}
	data.resize(read + 4 + 1);

	result->addAnchor(keywords[K_START].getTerminatedBuffer(), 0, true);

	// Allocate the magic * tag
	{
		Tag *tany = parseTag(stringbits[S_ASTERIK].getTerminatedBuffer());
		result->tag_any = tany->hash;
	}
	// Create the dummy set
	result->allocateDummySet();
	// Create the magic set _TARGET_ containing the tag _TARGET_
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_TARGET].getTerminatedBuffer());
		Tag *t = parseTag(stringbits[S_UU_TARGET].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _MARK_ containing the tag _MARK_
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_MARK].getTerminatedBuffer());
		Tag *t = parseTag(stringbits[S_UU_MARK].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _ATTACHTO_ containing the tag _ATTACHTO_
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_ATTACHTO].getTerminatedBuffer());
		Tag *t = parseTag(stringbits[S_UU_ATTACHTO].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _LEFT_ containing the tag _LEFT_
	Set *s_left = 0;
	{
		Set *set_c = s_left = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_LEFT].getTerminatedBuffer());
		Tag *t = parseTag(stringbits[S_UU_LEFT].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _RIGHT_ containing the tag _RIGHT_
	Set *s_right = 0;
	{
		Set *set_c = s_right = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_RIGHT].getTerminatedBuffer());
		Tag *t = parseTag(stringbits[S_UU_RIGHT].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}
	// Create the magic set _ENCL_ containing the tag _ENCL_
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_ENCL].getTerminatedBuffer());
		Tag *t = parseTag(stringbits[S_UU_ENCL].getTerminatedBuffer());
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
	// Create the magic set _SAME_BASIC_ containing the tag _SAME_BASIC_
	{
		Set *set_c = result->allocateSet();
		set_c->line = 0;
		set_c->setName(stringbits[S_UU_SAME_BASIC].getTerminatedBuffer());
		Tag *t = parseTag(stringbits[S_UU_SAME_BASIC].getTerminatedBuffer());
		result->addTagToSet(t, set_c);
		result->addSet(set_c);
	}

	error = parseFromUChar(&data[4], filename);
	if (error) {
		return error;
	}

	result->addAnchor(keywords[K_END].getTerminatedBuffer(), result->rule_by_number.size() - 1, true);

	foreach (it, result->rule_by_number) {
		if ((*it)->name) {
			result->addAnchor((*it)->name, (*it)->number, false);
		}
	}

	boost_foreach (Tag *tag, result->single_tags_list) {
		if (!(tag->type & T_VARSTRING)) {
			continue;
		}
		UChar *p = &tag->tag[0];
		UChar *n = 0;
		do {
			SKIPTO(p, '{');
			if (*p) {
				n = p;
				SKIPTO(n, '}');
				if (*n) {
					tag->allocateVsSets();
					tag->allocateVsNames();
					++p;
					UString theSet(p, n);
					Set *tmp = parseSet(theSet.c_str(), p);
					tag->vs_sets->push_back(tmp);
					UString old;
					old += '{';
					old += tmp->name;
					old += '}';
					tag->vs_names->push_back(old);
					p = n;
					++p;
				}
			}
		} while (*p);
	}

	foreach (it, deferred_tmpls) {
		uint32_t cn = hash_value(it->second.second);
		if (result->templates.find(cn) == result->templates.end()) {
			u_fprintf(ux_stderr, "%s: Error: Unknown template '%S' referenced on line %u!\n", filebase, it->second.second.c_str(), it->second.first);
			++error_counter;
			continue;
		}
		it->first->tmpl = result->templates.find(cn)->second;
	}

	bc::flat_map<uint32_t, uint32_t> sets;
	for (BOOST_AUTO(cntx, result->contexts.begin()); cntx != result->contexts.end();) {
		if (cntx->second->pos & POS_NUMERIC_BRANCH) {
			ContextualTest *unsafec = cntx->second;
			result->contexts.erase(cntx);

			if (sets.find(unsafec->target) == sets.end()) {
				sets[unsafec->target] = result->removeNumericTags(unsafec->target);
			}
			unsafec->pos &= ~POS_NUMERIC_BRANCH;

			ContextualTest *safec = result->allocateContextualTest();
			copy_cntx(unsafec, safec);

			safec->pos |= POS_CAREFUL;
			safec->target = sets[unsafec->target];

			ContextualTest *tmp = unsafec;
			unsafec = result->addContextualTest(unsafec);
			safec = result->addContextualTest(safec);

			ContextualTest *orc = result->allocateContextualTest();
			orc->ors.push_back(safec);
			orc->ors.push_back(unsafec);
			orc = result->addContextualTest(orc);

			for (BOOST_AUTO(cntx, result->contexts.begin()); cntx != result->contexts.end(); ++cntx) {
				if (cntx->second->linked == tmp) {
					cntx->second->linked = orc;
				}
			}
			for (BOOST_AUTO(it, result->rule_by_number.begin()); it != result->rule_by_number.end(); ++it) {
				if ((*it)->dep_target == tmp) {
					(*it)->dep_target = orc;
				}
				ContextList *cntxs[2] = { &(*it)->tests, &(*it)->dep_tests };
				for (size_t i = 0; i < 2; ++i) {
					boost_foreach (ContextualTest *& test, *cntxs[i]) {
						if (test == tmp) {
							test = orc;
						}
					}
				}
			}
			cntx = result->contexts.begin();
		}
		else {
			++cntx;
		}
	}

	return error_counter;
}

void TextualParser::setCompatible(bool f) {
	option_vislcg_compat = f;
}

void TextualParser::setVerbosity(uint32_t level) {
	verbosity_level = level;
}

void TextualParser::addRuleToGrammar(Rule *rule) {
	if (in_section) {
		rule->section = (int32_t)(result->sections.size() - 1);
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
	else { // in_before_sections
		rule->section = -1;
		result->addRule(rule);
	}
}
}
