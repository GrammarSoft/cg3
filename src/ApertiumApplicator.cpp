/*
* Copyright (C) 2007-2023, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "ApertiumApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

constexpr UChar esc_lt = '\1';

ApertiumApplicator::ApertiumApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
{
}

void ApertiumApplicator::parseStreamVar(const SingleWindow* cSWindow, UString& cleaned,
					uint32FlatHashMap& variables_set, uint32FlatHashSet& variables_rem, uint32SortedVector& variables_output) {
	size_t packoff = 0;
	if (u_strncmp(&cleaned[0], STR_CMD_SETVAR.data(), SI32(STR_CMD_SETVAR.size())) == 0) {
		// u_fprintf(ux_stderr, "Info: SETVAR encountered on line %u.\n", numLines);
		cleaned[packoff - 1] = 0;
		// line[0] = 0;

		UChar* s = &cleaned[STR_CMD_SETVAR.size()];
		UChar* c = u_strchr(s, ',');
		UChar* d = u_strchr(s, '=');
		if (c == 0 && d == 0) {
			Tag* tag = addTag(s);
			variables_set[tag->hash] = grammar->tag_any;
			variables_rem.erase(tag->hash);
			variables_output.insert(tag->hash);
			if (cSWindow == nullptr) {
				variables[tag->hash] = grammar->tag_any;
			}
		}
		else {
			uint32_t a = 0, b = 0;
			while (c || d) {
				if (d && (d < c || c == nullptr)) {
					d[0] = 0;
					if (!s[0]) {
						u_fprintf(ux_stderr, "Warning: SETVAR on line %u had no identifier before the =! Defaulting to identifier *.\n", numLines);
						a = grammar->tag_any;
					}
					else {
						a = addTag(s)->hash;
					}
					if (c) {
						c[0] = 0;
						s = c + 1;
					}
					if (!d[1]) {
						u_fprintf(ux_stderr, "Warning: SETVAR on line %u had no value after the =! Defaulting to value *.\n", numLines);
						b = grammar->tag_any;
					}
					else {
						b = addTag(d + 1)->hash;
					}
					if (!c) {
						d = nullptr;
						s = nullptr;
					}
					variables_set[a] = b;
					variables_rem.erase(a);
					variables_output.insert(a);
				}
				else if (c && (c < d || d == nullptr)) {
					c[0] = 0;
					if (!s[0]) {
						u_fprintf(ux_stderr, "Warning: SETVAR on line %u had no identifier after the ,! Defaulting to identifier *.\n", numLines);
						a = grammar->tag_any;
					}
					else {
						a = addTag(s)->hash;
					}
					s = c + 1;
					variables_set[a] = grammar->tag_any;
					variables_rem.erase(a);
					variables_output.insert(a);
				}
				if (s) {
					c = u_strchr(s, ',');
					d = u_strchr(s, '=');
					if (c == nullptr && d == nullptr) {
						a = addTag(s)->hash;
						variables_set[a] = grammar->tag_any;
						variables_rem.erase(a);
						variables_output.insert(a);
						s = nullptr;
					}
				}
			}
		}
	}
	else if (u_strncmp(&cleaned[0], STR_CMD_REMVAR.data(), SI32(STR_CMD_REMVAR.size())) == 0) {
		//u_fprintf(ux_stderr, "Info: REMVAR encountered on line %u.\n", numLines);
		cleaned[packoff - 1] = 0;
		// line[0] = 0;

		UChar* s = &cleaned[STR_CMD_REMVAR.size()];
		UChar* c = u_strchr(s, ',');
		uint32_t a = 0;
		while (c && *c) {
			c[0] = 0;
			if (s[0]) {
				a = addTag(s)->hash;
				variables_set.erase(a);
				variables_rem.insert(a);
				variables_output.insert(a);
			}
			s = c + 1;
			c = u_strchr(s, ',');
		}
		if (s && s[0]) {
			a = addTag(s)->hash;
			variables_set.erase(a);
			variables_rem.insert(a);
			variables_output.insert(a);
		}
	}
}

/*
 * Run a constraint grammar on an Apertium input stream
 */
void ApertiumApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
	ux_stdin = &input;
	ux_stdout = &output;

	if (!input.good()) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		CG3Quit(1);
	}
	if (input.eof()) {
		u_fprintf(ux_stderr, "Error: Input is empty - nothing to parse!\n");
		CG3Quit(1);
	}
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		CG3Quit(1);
	}

	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue! Hint: call setGrammar() first.\n");
		CG3Quit(1);
	}

	if (!grammar->delimiters || grammar->delimiters->empty()) {
		if (!grammar->soft_delimiters || grammar->soft_delimiters->empty()) {
			u_fprintf(ux_stderr, "Warning: No soft or hard delimiters defined in grammar. Hard limit of %u cohorts may break windows in unintended places.\n", hard_limit);
		}
		else {
			u_fprintf(ux_stderr, "Warning: No hard delimiters defined in grammar. Soft limit of %u cohorts may break windows in unintended places.\n", soft_limit);
		}
	}

	UChar c = 0;        // Current character
	bool in_blank = false;   // Are we in a superblank ?
	bool in_wblank = false;  // Are we in a word-bound blank ?
	bool in_cohort = false;  // Are we in a cohort ?
	UString blank;           // Blank between tokens, including superblanks
	UString wblank;
	UString token;           // Current token (cohort)

	index();

	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);

	constexpr UStringView wb_start{ u"[[" };
	constexpr UStringView wb_end{ u"]]" };

	begintag = addTag(STR_BEGINTAG)->hash; // Beginning of sentence tag
	endtag = addTag(STR_ENDTAG)->hash;     // End of sentence tag

	SingleWindow* cSWindow = nullptr; // Current single window (Cohort frame)
	Cohort* cCohort = nullptr;        // Current cohort

	SingleWindow* lSWindow = nullptr; // Last seen single window
	Cohort* lCohort = nullptr;        // Last seen cohort

	gWindow->window_span = num_windows;

	uint32FlatHashMap variables_set;
	uint32FlatHashSet variables_rem;
	uint32SortedVector variables_output;

	ux_stripBOM(input);

	auto ensure_endtag = [&]() {
		if (lSWindow && !lSWindow->cohorts.empty() && lSWindow->cohorts.back()->readings.front()->tags.count(endtag) == 0) {
			for (auto iter : lSWindow->cohorts.back()->readings) {
				addTagToReading(*iter, endtag);
			}
		}
	};

	auto flush = [&](bool n = false) {
		ensure_endtag();

		auto backSWindow = n ? gWindow->back() : nullptr;
		if (backSWindow) {
			backSWindow->flush_after = true;
		}

		if (!blank.empty()) {
			if (lCohort) {
				lCohort->text += blank;
			}
			else if (lSWindow && !lSWindow->cohorts.empty()) {
				lSWindow->cohorts.back()->text += blank;
			}
			else if (lSWindow) {
				lSWindow->text += blank;
			}
			else {
				u_fprintf(output, "%S", blank.data());
			}
			blank.clear();
		}

		// Run the grammar & print results
		while (!gWindow->next.empty()) {
			gWindow->shuffleWindowsDown();
			runGrammarOnWindow();
		}

		gWindow->shuffleWindowsDown();
		while (!gWindow->previous.empty()) {
			SingleWindow* tmp = gWindow->previous.front();
			printSingleWindow(tmp, output);
			free_swindow(tmp);
			gWindow->previous.erase(gWindow->previous.begin());
		}

		if (c && c != 0xffff) {
			u_fprintf(output, "%C", c); // eg. final newline
		}

		if (n && !backSWindow) {
			u_fputc('\0', output);
		}
		u_fflush(output);

		in_blank = false;
		in_wblank = false;
		in_cohort = false;
		lSWindow = nullptr;
		lCohort = nullptr;
		cSWindow = nullptr;
		cCohort = nullptr;
		token.clear();
		variables_rem.clear();
		variables_set.clear();
		variables_output.clear();
		variables.clear();
	};

	while ((c = u_fgetc(input)) != U_EOF) {
		if (c == '\n') {
			++numLines;
		}

		if (c == '\\') {
			auto n = u_fgetc(input);
			if (!in_cohort) {
				blank += c;
				blank += n;
			}
			else {
				token += c;
				token += n;
			}
			continue;
		}

		if (c == 0) {
			flush(true);
			continue;
		}

		if (!in_cohort && c == '[') {
			if (in_blank) {
				in_wblank = true;
			}
			in_blank = true;
		}
		else if (!in_blank && c == '^') {
			in_cohort = true;
		}

		if (!in_cohort) {
			blank += c;
		}
		else {
			token += c;
		}

		if (in_wblank && c == ']') {
			in_wblank = false;
		}
		else if (in_blank && c == ']') {
			in_blank = false;
			if (blank.size() > 14 // contains at least "[<STREAMCMD:>]"
			    && blank[1] == '<' && blank[blank.size() - 2] == '>') {
				UString cleaned = blank.substr(1, blank.size() - 3);
				parseStreamVar(cSWindow, cleaned,
					       variables_set, variables_rem, variables_output);
			}
		}
		else if (!in_blank && c == '$') {
			if (!in_cohort) {
				u_fprintf(ux_stderr, "Error: $ found without prior ^ on line %u.\n", numLines);
				CG3Quit(1);
			}
			in_cohort = false;

			if (!blank.empty()) {
				wblank.clear();
				auto b = blank.find(wb_start);
				auto e = blank.find(wb_end, b);
				if (b != UString::npos && e != UString::npos) {
					// Word-bound blanks are always immediately prior to the token they belong to,
					// except closing word blanks which must be treated as normal blanks
					if (e != b + 3 || blank[b + 2] != '/') {
						wblank = blank.substr(b);
						blank.erase(b, UString::npos);
					}
				}
			}
			if (!wblank.empty() && (wblank[wblank.size() - 1] != ']' || wblank[wblank.size() - 2] != ']')) {
				u_fprintf(ux_stderr, "Error: Word-bound blank was not immediately prior to token on line %u\n", numLines);
				CG3Quit(1);
			}

			if (cCohort) {
				cCohort->text += blank;
				blank.clear();
			}
			else if (lCohort) {
				lCohort->text += blank;
				blank.clear();
			}
			else if (lSWindow) {
				lSWindow->text += blank;
				blank.clear();
			}

			// If we don't have a current window, create one
			if (!cSWindow) {
				ensure_endtag();

				cSWindow = gWindow->allocAppendSingleWindow();

				initEmptySingleWindow(cSWindow);

				cSWindow->variables_set = variables_set;
				variables_set.clear();
				cSWindow->variables_rem = variables_rem;
				variables_rem.clear();
				cSWindow->variables_output = variables_output;
				variables_output.clear();

				lSWindow = cSWindow;
				lSWindow->text = blank;
				blank.clear();
				++numWindows;
			} // created at least one cSWindow by now

			lCohort = cCohort = alloc_cohort(cSWindow);
			cCohort->global_number = gWindow->cohort_counter++;
			numCohorts++;

			cCohort->text = blank;
			blank.clear();
			cCohort->wblank = wblank;
			wblank.clear();

			blank.append(u"\"<");
			UChar* p = &token[1];
			for (; *p && *p != '/' && *p != '<'; ++p) {
				if (*p == '\\') {
					++p;
				}
				blank += *p;
			}
			blank.append(u">\"");
			cCohort->wordform = addTag(blank);

			// Handle the static reading of ^estació<n><f><sg>/season<n><sg>/station<n><sg>$
			// Gobble up all <tags> until the first / or $ and stuff them in the static reading
			if (*p == '<') {
				++p;
				blank.clear();
				//u_fprintf(ux_stderr, "Static reading\n");
				cCohort->wread = alloc_reading(cCohort);
				for (; *p && *p != '/' && *p != '$'; ++p) {
					if (*p == '\\') {
						++p;
						blank += *p;
						continue;
					}
					if (*p == '<') {
						continue;
					}
					if (*p == '>') {
						Tag* t = addTag(blank);
						addTagToReading(*cCohort->wread, t);
						//u_fprintf(ux_stderr, "Adding tag %S\n", tag.data());
						blank.clear();
						continue;
					}
					blank += *p;
				}
			}

			// Read in the readings
			if (*p == '/') {
				++p;
				blank.clear();

				for (; *p; ++p) {
					if (*p == '\\') {
						++p;
						if (*p == '<') {
							blank += esc_lt;
						}
						else {
							blank += *p;
						}
						continue;
					}
					if (*p == '/' || *p == '$') {
						auto cReading = alloc_reading(cCohort);
						processReading(cReading, blank, cCohort->wordform);

						if (grammar->sub_readings_ltr && cReading->next) {
							cReading = reverse(cReading);
						}

						if (cReading->deleted) {
							cCohort->deleted.push_back(cReading);
						}
						else {
							cCohort->appendReading(cReading);
						}
						++numReadings;

						if (!cReading->baseform) {
							u_fprintf(ux_stderr, "Warning: Cohort %u on line %u had no valid baseform.\n", numCohorts, numLines);
							u_fflush(ux_stderr);
						}
						blank.clear();
						continue;
					}
					blank += *p;
				}
			}

			// Create magic reading, if needed
			if (cCohort->readings.empty()) {
				initEmptyCohort(*cCohort);
			}
			insert_if_exists(cCohort->possible_sets, grammar->sets_any);
			cSWindow->appendCohort(cCohort);
			if (cCohort->wordform->tag[2] == '@') {
				cCohort->type |= CT_AP_UNKNOWN;
			}

			bool did_delim = false;
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesSetMatchCohortNormal(*cCohort, grammar->soft_delimiters->number)) {
				for (auto iter : cCohort->readings) {
					addTagToReading(*iter, endtag);
				}

				lSWindow = cSWindow;
				cSWindow = nullptr;
				cCohort = nullptr;
				did_delim = true;
			} // end >= soft_limit
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (grammar->delimiters && doesSetMatchCohortNormal(*cCohort, grammar->delimiters->number)))) {
				if (!is_conv && cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at cohort %S (#%u) on line %u - forcing break.\n", hard_limit, cCohort->wordform->tag.data(), numCohorts, numLines);
					u_fflush(ux_stderr);
				}
				for (auto iter : cCohort->readings) {
					addTagToReading(*iter, endtag);
				}

				lSWindow = cSWindow;
				cSWindow = nullptr;
				cCohort = nullptr;
				did_delim = true;
			} // end >= hard_limit

			if (did_delim && gWindow->next.size() > num_windows) {
				gWindow->shuffleWindowsDown();
				runGrammarOnWindow();
				if (numWindows % resetAfter == 0) {
					resetIndexes();
				}
			}
			token.clear();
		}
	}

	flush();
} // runGrammarOnText

/*
 * Parse an Apertium reading into a CG Reading
 *
 * Examples:
 *   venir<vblex><imp><p2><sg>
 *   venir<vblex><inf>+lo<prn><enc><p3><nt><sg>
 *   be<vblex><inf># happy
 *   sellout<vblex><imp><p2><sg># ouzh+indirect<prn><obj><p3><m><sg>
 *   be# happy<vblex><inf> (for chaining cg-proc)
 */
void ApertiumApplicator::processReading(Reading* cReading, UChar* p, Tag* wform) {
	addTagToReading(*cReading, wform);

	TagList taglist;
	UString bf{'"'};
	TagList tags;
	TagList prefix_tags;

	while (*p) {
		auto n = p;
		while (*n && *n != '#' && *n != '+' && *n != '<') {
			if (*n == esc_lt) {
				*n = '<';
			}
			++n;
		}
		// Found a baseform (if something is between start and [#+<], it must be part of the baseform)
		if (n != p) {
			auto c = *n;
			*n = 0;
			bf += p;
			*n = c;
			p = n;
		}
		// Found a tag start, so read until >
		if (*n == '<') {
			p = n + 1;
			while (*n && *n != '>') {
				++n;
			}
			if (*n != '>') {
				u_fprintf(ux_stderr, "Warning: Did not find matching > to close the tag on line %u.\n", numLines);
				continue;
			}
			auto c = *n;
			*n = 0;
			if (bf.size() == 1) {
				prefix_tags.push_back(addTag(p));
			}
			else {
				tags.push_back(addTag(p));
			}
			*n = c;
			p = n + 1;
		}
		// Found a multiword marker, so read until [+<]
		if (*n == '#') {
			p = n;
			while (*n && *n != '<' && *n != '+') {
				++n;
			}
			auto c = *n;
			*n = 0;
			bf += p;
			*n = c;
			p = n;
		}
		// Found a sub-reading delimiter, so append current tags to the list in the correct order
		if (*n == '+') {
			bf += '"';
			taglist.push_back(addTag(bf));
			taglist.insert(taglist.end(), tags.begin(), tags.end());
			taglist.insert(taglist.end(), prefix_tags.begin(), prefix_tags.end());
			bf.resize(1);
			tags.clear();
			prefix_tags.clear();
			p = n + 1;
		}
	}

	if (!bf.empty() || !tags.empty() || !prefix_tags.empty()) {
		bf += '"';
		taglist.push_back(addTag(bf));
		taglist.insert(taglist.end(), tags.begin(), tags.end());
		taglist.insert(taglist.end(), prefix_tags.begin(), prefix_tags.end());
	}

	// Search from the back until we find a baseform, then add all tags from there until the end onto the reading
	while (!taglist.empty()) {
		Reading* reading = cReading;
		reverse_foreach (riter, taglist) {
			if ((*riter)->type & T_BASEFORM) {
				// If current reading already has a baseform, instead create a sub-reading as target
				if (reading->baseform) {
					Reading* nr = reading->allocateReading(reading->parent);
					reading->next = nr;
					reading = nr;
					addTagToReading(*reading, wform);
				}
				// Add tags
				TagList mappings;
				auto iter = riter.base();
				for (--iter; iter != taglist.end(); ++iter) {
					if ((*iter)->type & T_MAPPING || (*iter)->tag[0] == grammar->mapping_prefix) {
						mappings.push_back(*iter);
					}
					else {
						addTagToReading(*reading, *iter);
					}
				}
				if (!mappings.empty()) {
					splitMappings(mappings, *reading->parent, *reading, true);
				}
				// Remove tags from list
				while (!taglist.empty() && !(taglist.back()->type & T_BASEFORM)) {
					taglist.pop_back();
				}
				taglist.pop_back();
			}
		}
	}

	assert(taglist.empty() && "ApertiumApplicator::processReading() did not handle all tags.");
}

void ApertiumApplicator::processReading(Reading* cReading, UString& reading_string, Tag* wform) {
	return processReading(cReading, &reading_string[0], wform);
}

void ApertiumApplicator::testPR(std::ostream& output) {
	std::string texts[] = {
		"venir<vblex><imp><p2><sg>",
		"venir<vblex><inf>+lo<prn><enc><p3><nt><sg>",
		"be<vblex><inf># happy",
		"sellout<vblex><imp><p2><sg># ouzh+indirect<prn><obj><p3><m><sg>",
		"be# happy<vblex><inf>",
		"aux3<tag>+aux2<tag>+aux1<tag>+main<tag>",
	};
	for (size_t i = 0; i < 6; ++i) {
		UString text(texts[i].begin(), texts[i].end());
		Reading* reading = alloc_reading();
		processReading(reading, text, grammar->single_tags[grammar->tag_any]);
		if (grammar->sub_readings_ltr && reading->next) {
			reading = reverse(reading);
		}
		printReading(reading, output);
		u_fprintf(output, "\n");
		free_reading(reading);
	}
}

void ApertiumApplicator::printReading(Reading* reading, std::ostream& output, ApertiumCasing casing, int32_t firstlower) {
	if (reading->next) {
		printReading(reading->next, output, casing, firstlower);
		u_fputc('+', output);
	}

	if (reading->baseform) {
		// Lop off the initial and final '"' characters
		UnicodeString bf(grammar->single_tags[reading->baseform]->tag.data() + 1, SI32(grammar->single_tags[reading->baseform]->tag.size() - 2));

		if (wordform_case) {
			if (casing == ApertiumCasing::Upper) {
				bf.toUpper(); // Perform a Unicode case folding to upper case -- Tino Didriksen
			}
			else if (casing == ApertiumCasing::Title && !reading->next) {
				// static_cast<UChar>(u_toupper(bf[first])) gives strange output
				UnicodeString range(bf, firstlower, 1);
				range.toUpper();
				bf.setCharAt(firstlower, range[0]);
			}
		} // if (wordform_case)

		UString bf_escaped;
		for (int i = 0; i < bf.length(); ++i) {
			if (bf[i] == '^' || bf[i] == '\\' || bf[i] == '/' || bf[i] == '$' || bf[i] == '[' || bf[i] == ']' || bf[i] == '{' || bf[i] == '}' || bf[i] == '<' || bf[i] == '>') {
				bf_escaped += '\\';
			}
			if ((reading->parent->type & CT_AP_UNKNOWN) && bf[i] == '@') {
				bf_escaped += '\\';
			}
			bf_escaped += bf[i];
		}
		if(surface_readings && bf.length() > 0 && bf_escaped[0] == '@') {
			bf_escaped[0] = '#';
		}
		u_fprintf(output, "%S", bf_escaped.data());

		// Tag::printTagRaw(output, grammar->single_tags[reading->baseform]);
	}

	if(surface_readings && !trace) {
		// As with lt-proc -g, when output has tags (meaning
		// ungenerated words), don't print unless tracing (lt-proc -d)
		return;
	}

	// Reorder: MAPPING tags should appear before the join of multiword tags,
	// turn <vblex><actv><pri><p3><pl>+í<pr><@FMAINV><@FOO>$
	// into <vblex><actv><pri><p3><pl><@FMAINV><@FOO>+í<pr>$
	Reading::tags_list_t tags_list;
	Reading::tags_list_t multitags_list; // everything after a +, until the first MAPPING tag
	bool multi = false;
	for (auto tter : reading->tags_list) {
		const Tag* tag = grammar->single_tags[tter];
		if (tag->tag[0] == '+') {
			multi = true;
		}
		else if (tag->type & T_MAPPING) {
			multi = false;
		}

		if (tag->type & T_DEPENDENCY && has_dep && !dep_original) {
			continue;
		}

		if (multi) {
			multitags_list.push_back(tter);
		}
		else {
			tags_list.push_back(tter);
		}
	}
	tags_list.insert(tags_list.end(), multitags_list.begin(), multitags_list.end());

	uint32SortedVector used_tags;
	UStringView escape{ surface_readings ? u"\\" : u"" };
	for (auto tter : tags_list) {
		if (unique_tags) {
			if (used_tags.find(tter) != used_tags.end()) {
				continue;
			}
			used_tags.insert(tter);
		}
		if (tter == endtag || tter == begintag) {
			continue;
		}
		const Tag* tag = grammar->single_tags[tter];
		if (!(tag->type & T_BASEFORM) && !(tag->type & T_WORDFORM)) {
			if (tag->tag[0] == '+') {
				u_fprintf(output, "%S", tag->tag.data());
			}
			else if (tag->tag[0] == '&') {
				u_fprintf(output, "%S<%S%S>", escape.data(), substr(tag->tag, 2).data(), escape.data());
			}
			else {
				u_fprintf(output, "%S<%S%S>", escape.data(), tag->tag.data(), escape.data());
			}
		}
	}

	if (has_dep && !(reading->parent->type & CT_REMOVED) && !reading->next) {
		if (!reading->parent->dep_self) {
			reading->parent->dep_self = reading->parent->global_number;
		}
		const Cohort* pr = nullptr;
		pr = reading->parent;
		if (reading->parent->dep_parent != DEP_NO_PARENT) {
			if (reading->parent->dep_parent == 0) {
				pr = reading->parent->parent->cohorts[0];
			}
			else if (reading->parent->parent->parent->cohort_map.find(reading->parent->dep_parent) != reading->parent->parent->parent->cohort_map.end()) {
				pr = reading->parent->parent->parent->cohort_map[reading->parent->dep_parent];
			}
		}

		constexpr UChar pattern[] = { '<', '#', '%', 'u', u'\u2192', '%', 'u', '>', 0 };
		u_fprintf_u(output, pattern, reading->parent->local_number, pr->local_number);
	}

	if (trace) {
		for (auto iter_hb : reading->hit_by) {
			u_fputc('<', output);
			printTrace(output, iter_hb);
			u_fputc('>', output);
		}
	}
}

void ApertiumApplicator::printReading(Reading* reading, std::ostream& output) {
	if (reading->noprint) {
		return;
	}

	size_t firstlower = 0;
	ApertiumCasing casing = ApertiumCasing::Lower;

	if (wordform_case) {
		// Use surface/wordform case, eg. if lt-proc
		// was called with "-w" option (which puts
		// dictionary case on lemma/basefrom)
		// cf. fst_processor.cc in lttoolbox
		Reading* last = reading;
		while (last->next && last->next->baseform) {
			last = last->next;
		}
		if (last->baseform) {
			// Including the initial and final '"' characters
			UString* bftag = &grammar->single_tags[last->baseform]->tag;
			// Excluding the initial and final '"' characters
			size_t bf_length = bftag->size() - 2;
			UString* wftag = &reading->parent->wordform->tag;
			size_t wf_length = wftag->size() - 4;

			for (; firstlower < bf_length; ++firstlower) {
				if (u_islower(bftag->at(firstlower+1)) != 0) {
					break;
				}
			}

			int uppercaseseen = 0;
			bool allupper = true;
			// 2-2: Skip the initial and final '"<>"' characters
			for (size_t i = 2; i < wftag->size() - 2; ++i) {
				UChar32 c = wftag->at(i);
				if(u_isUAlphabetic(c)) {
					if(!u_isUUppercase(c)) {
						allupper = false;
						break;
					}
					else {
						uppercaseseen++;
					}
				}
			}

			// Require at least 2 characters to call it UPPER:
			if (allupper && uppercaseseen >= 2) {
				casing = ApertiumCasing::Upper;
			}
			else if (firstlower < wf_length
				 && firstlower < bf_length
				 && (u_isupper(wftag->at(firstlower+2)) != 0)) {
				casing = ApertiumCasing::Title;
			}
		}
	} // if (wordform_case)
	printReading(reading, output, casing, SI32(firstlower));
}

void ApertiumApplicator::printCohort(Cohort* cohort, std::ostream& output, bool profiling) {
	if (!profiling) {
		cohort->unignoreAll();

		if (!split_mappings) {
			mergeMappings(*cohort);
		}
	}

	if (!cohort->wblank.empty()) {
		u_fprintf(output, "%S", cohort->wblank.data());
	}

	// Start of cohort
	if (delimit_lexical_units) {
		u_fprintf(output, "^");
	}

	if (print_word_forms == true) {
		// Lop off the initial and final '"' characters
		// ToDo: A copy does not need to be made here - use pointer offsets
		UnicodeString wf(cohort->wordform->tag.data() + 2, SI32(cohort->wordform->tag.size() - 4));
		UString wf_escaped;
		for (int i = 0; i < wf.length(); ++i) {
			if (wf[i] == '^' || wf[i] == '\\' || wf[i] == '/' || wf[i] == '$' || wf[i] == '[' || wf[i] == ']' || wf[i] == '{' || wf[i] == '}' || wf[i] == '<' || wf[i] == '>') {
				wf_escaped += '\\';
			}
			if ((cohort->type & CT_AP_UNKNOWN) && wf[i] == '@') {
				wf_escaped += '\\';
			}
			wf_escaped += wf[i];
		}
		u_fprintf(output, "%S", wf_escaped.data());

		// Print the static reading tags
		if (cohort->wread) {
			for (auto tter : cohort->wread->tags_list) {
				if (tter == cohort->wordform->hash) {
					continue;
				}
				const Tag* tag = grammar->single_tags[tter];
				u_fprintf(output, "<%S>", tag->tag.data());
			}
		}
	}

	bool need_slash = print_word_forms;

	//Tag::printTagRaw(output, grammar->single_tags[cohort->wordform]);
	std::sort(cohort->readings.begin(), cohort->readings.end(), Reading::cmp_number);
	for (auto reading : cohort->readings) {
		if (need_slash) {
			u_fprintf(output, "/");
		}
		need_slash = true;
		if (grammar->sub_readings_ltr && reading->next) {
			reading = reverse(reading);
		}
		printReading(reading, output);
		if (print_only_first == true) {
			break;
		}
	}

	if (trace) {
		std::sort(cohort->delayed.begin(), cohort->delayed.end(), Reading::cmp_number);
		for (auto reading : cohort->delayed) {
			if (need_slash) {
				u_fprintf(output, "/%C", not_sign);
			}
			need_slash = true;
			if (grammar->sub_readings_ltr && reading->next) {
				reading = reverse(reading);
			}
			printReading(reading, output);
		}
		std::sort(cohort->deleted.begin(), cohort->deleted.end(), Reading::cmp_number);
		for (auto reading : cohort->deleted) {
			if (need_slash) {
				u_fprintf(output, "/%C", not_sign);
			}
			need_slash = true;
			if (grammar->sub_readings_ltr && reading->next) {
				reading = reverse(reading);
			}
			printReading(reading, output);
		}
	}

	if (delimit_lexical_units) {
		u_fprintf(output, "$");
	}
	// End of cohort

	if (!cohort->text.empty()) {
		u_fprintf(output, "%S", cohort->text.data());
	}
	for (auto& c : cohort->removed) {
		if (!c->text.empty()) {
			u_fprintf(output, "%S", c->text.data());
		}
	}
}

void ApertiumApplicator::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
	// Window text comes at the left
	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.data());
	}

	for (uint32_t c = 0; c < window->cohorts.size(); c++) {
		Cohort* cohort = window->cohorts[c];

		if (c == 0) { // Skip magic cohort
			for (auto& c : cohort->removed) {
				if (!c->text.empty()) {
					u_fprintf(output, "%S", c->text.data());
				}
			}
			continue;
		}

		printCohort(cohort, output, profiling);
		u_fflush(output);
	}

	if (!window->text_post.empty()) {
		u_fprintf(output, "%S", window->text_post.data());
		u_fflush(output);
	}

	if (window->flush_after) {
		u_fputc('\0', output);
	}
}

void ApertiumApplicator::mergeMappings(Cohort& cohort) {
	// Only merge readings which are completely equal (including mapping tags)
	// foo<N><Sg><Acc><@←OBJ>/foo<N><Sg><Acc><@←OBJ>
	// => guovvamánnu<N><Sg><Acc><@←OBJ>
	// foo<N><Sg><Acc><@←SUBJ>/foo<N><Sg><Acc><@←OBJ>
	// => foo<N><Sg><Acc><@←SUBJ>/foo<N><Sg><Acc><@←OBJ>
	std::map<uint32_t, ReadingList> mlist;
	for (auto iter : cohort.readings) {
		Reading* r = iter;
		uint32_t hp = r->hash; // instead of hash_plain, which doesn't include mapping tags
		if (trace) {
			for (auto iter_hb : r->hit_by) {
				hp = hash_value(iter_hb, hp);
			}
		}
		Reading* sub = r->next;
		while (sub) {
			hp = hash_value(sub->hash, hp);
			if (trace) {
				for (auto iter_hb : sub->hit_by) {
					hp = hash_value(iter_hb, hp);
				}
			}
			sub = sub->next;
		}
		mlist[hp].push_back(r);
	}

	if (mlist.size() == cohort.readings.size()) {
		return;
	}

	cohort.readings.clear();
	std::vector<Reading*> order;

	for (auto& miter : mlist) {
		ReadingList clist = miter.second;
		// no merging of mapping tags, so just take first reading of the group
		order.push_back(clist.front());

		clist.erase(clist.begin());
		for (auto cit : clist) {
			free_reading(cit);
		}
	}

	std::sort(order.begin(), order.end(), Reading::cmp_number);
	cohort.readings.insert(cohort.readings.begin(), order.begin(), order.end());
}
}
