/*
* Copyright (C) 2007-2018, GrammarSoft ApS
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

#include "ApertiumApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

ApertiumApplicator::ApertiumApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
{
	nullFlush = false;
	wordform_case = false;
	unique_tags = false;
	print_word_forms = true;
	print_only_first = false;
	runningWithNullFlush = false;
}

void ApertiumApplicator::setNullFlush(bool pNullFlush) {
	nullFlush = pNullFlush;
}

void ApertiumApplicator::runGrammarOnTextWrapperNullFlush(std::istream& input, std::ostream& output) {
	setNullFlush(false);
	runningWithNullFlush = true;
	while (!input.eof()) {
		runGrammarOnText(input, output);
		u_fputc('\0', output);
		u_fflush(output);
	}
	runningWithNullFlush = false;
}

/*
 * Run a constraint grammar on an Apertium input stream
 */

void ApertiumApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
	if (nullFlush) {
		runGrammarOnTextWrapperNullFlush(input, output);
		return;
	}

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

	UChar inchar = 0;        // Current character
	bool superblank = false; // Are we in a superblank ?
	bool incohort = false;   // Are we in a cohort ?
	UString firstblank;      // Blanks before the first window

	index();

	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);

	begintag = addTag(stringbits[S_BEGINTAG].getTerminatedBuffer())->hash; // Beginning of sentence tag
	endtag = addTag(stringbits[S_ENDTAG].getTerminatedBuffer())->hash;     // End of sentence tag

	SingleWindow* cSWindow = 0; // Current single window (Cohort frame)
	Cohort* cCohort = 0;        // Current cohort
	Reading* cReading = 0;      // Current reading

	SingleWindow* lSWindow = 0; // Left hand single window

	gWindow->window_span = num_windows;
	gtimer = getticks();
	ticks timer(gtimer);

	ux_stripBOM(input);

	while ((inchar = u_fgetc(input)) != 0) {
		if (input.eof()) {
			break;
		}

		if (inchar == '[') { // Start of a superblank section
			superblank = true;
		}

		if (inchar == ']') { // End of a superblank section
			superblank = false;
		}

		if (inchar == '\\' && !incohort && !superblank) {
			if (cCohort) {
				cCohort->text += inchar;
				inchar = u_fgetc(input);
				cCohort->text += inchar;
			}
			else if (lSWindow) {
				lSWindow->text += inchar;
				inchar = u_fgetc(input);
				lSWindow->text += inchar;
			}
			else {
				u_fprintf(output, "%C", inchar);
				inchar = u_fgetc(input);
				u_fprintf(output, "%C", inchar);
			}
			continue;
		}

		if (inchar == '^') {
			incohort = true;
		}

		if (superblank == true || inchar == ']' || incohort == false) {
			if (cCohort) {
				cCohort->text += inchar;
			}
			else if (lSWindow) {
				lSWindow->text += inchar;
			}
			else {
				firstblank += inchar; // add to lSWindow when it is created
			}
			continue;
		}

		if (incohort) {
			// Create magic reading
			if (cCohort && cCohort->readings.empty()) {
				initEmptyCohort(*cCohort);
			}
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesSetMatchCohortNormal(*cCohort, grammar->soft_delimiters->number)) {
				// ie. we've read some cohorts
				for (auto iter : cCohort->readings) {
					addTagToReading(*iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				lSWindow = cSWindow;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			} // end >= soft_limit
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (grammar->delimiters && doesSetMatchCohortNormal(*cCohort, grammar->delimiters->number)))) {
				if (!is_conv && cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at cohort %u on line %u - forcing break.\n", hard_limit, numCohorts, numLines);
					u_fflush(ux_stderr);
				}
				for (auto iter : cCohort->readings) {
					addTagToReading(*iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				lSWindow = cSWindow;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			} // end >= hard_limit
			// If we don't have a current window, create one
			if (!cSWindow) {
				cSWindow = gWindow->allocAppendSingleWindow();

				// Create 0th Cohort which serves as the beginning of sentence
				cCohort = alloc_cohort(cSWindow);
				cCohort->global_number = 0;
				cCohort->wordform = tag_begin;

				cReading = alloc_reading(cCohort);
				cReading->baseform = begintag;
				insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
				addTagToReading(*cReading, begintag);

				cCohort->appendReading(cReading);

				cSWindow->appendCohort(cCohort);

				lSWindow = cSWindow;
				lSWindow->text = firstblank;
				firstblank.clear();
				cCohort = 0;
				numWindows++;
			} // created at least one cSWindow by now

			// If the current cohort is looking ok, and we have an available
			// window, add the cohort to the window.
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
			}
			if (gWindow->next.size() > num_windows) {
				while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
					SingleWindow* tmp = gWindow->previous.front();
					printSingleWindow(tmp, output);
					free_swindow(tmp);
					gWindow->previous.erase(gWindow->previous.begin());
				}
				gWindow->shuffleWindowsDown();
				runGrammarOnWindow();
				if (numWindows % resetAfter == 0) {
					resetIndexes();
				}
			}
			cCohort = alloc_cohort(cSWindow);
			cCohort->global_number = gWindow->cohort_counter++;

			// Read in the word form
			UString wordform;

			wordform += '"';
			// We encapsulate wordforms within '"<' and
			// '>"' for internal processing.
			wordform += '<';
			for (;;) {
				inchar = u_fgetc(input);

				if (inchar == '/' || inchar == '<') {
					break;
				}
				else if (inchar == '\\') {
					inchar = u_fgetc(input);
					wordform += inchar;
				}
				else {
					wordform += inchar;
				}
			}
			wordform += '>';
			wordform += '"';

			//u_fprintf(output, "# %S\n", wordform);
			cCohort->wordform = addTag(wordform);
			numCohorts++;

			// We're now at the beginning of the readings
			UString current_reading;
			Reading* cReading = 0;

			// Handle the static reading of ^estació<n><f><sg>/season<n><sg>/station<n><sg>$
			// Gobble up all <tags> until the first / or $ and stuff them in the static reading
			if (inchar == '<') {
				//u_fprintf(ux_stderr, "Static reading\n");
				cCohort->wread = alloc_reading(cCohort);
				UString tag;
				do {
					inchar = u_fgetc(input);
					if (inchar == '\\') {
						inchar = u_fgetc(input);
						tag += inchar;
						continue;
					}
					if (inchar == '<') {
						continue;
					}
					if (inchar == '>') {
						Tag* t = addTag(tag);
						addTagToReading(*cCohort->wread, t);
						//u_fprintf(ux_stderr, "Adding tag %S\n", tag.c_str());
						tag.clear();
						continue;
					}
					if (inchar == '/' || inchar == '$') {
						break;
					}
					tag += inchar;
				} while (inchar != '/' && inchar != '$');
			}

			// Read in the readings
			while (incohort) {
				inchar = u_fgetc(input);

				if (inchar == '\\') {
					// TODO: \< in baseforms -- ^foo\<bars/foo\<bar$ currently outputs ^foo\<bars/foo$
					inchar = u_fgetc(input);
					current_reading += inchar;
					continue;
				}

				if (inchar == '$') {
					// Add the final reading of the cohort
					cReading = alloc_reading(cCohort);

					insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);

					addTagToReading(*cReading, cCohort->wordform);
					processReading(cReading, current_reading);

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

					current_reading.clear();

					incohort = false;
				}

				if (inchar == '/') { // Reached end of reading
					Reading* cReading = 0;
					cReading = alloc_reading(cCohort);

					addTagToReading(*cReading, cCohort->wordform);

					processReading(cReading, current_reading);

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

					current_reading.clear();
					continue; // while not $
				}

				current_reading += inchar;
			} // end while not $

			if (!cReading->baseform) {
				u_fprintf(ux_stderr, "Warning: Cohort %u on line %u had no valid baseform.\n", numCohorts, numLines);
				u_fflush(ux_stderr);
			}
		} // end reading
	}     // end input loop

	if (!firstblank.empty()) {
		u_fprintf(output, "%S", firstblank.c_str());
		firstblank.clear();
	}

	if (cCohort && cSWindow) {
		cSWindow->appendCohort(cCohort);
		// Create magic reading
		if (cCohort->readings.empty()) {
			initEmptyCohort(*cCohort);
		}
		for (auto iter : cCohort->readings) {
			addTagToReading(*iter, endtag);
		}
		cReading = 0;
		cCohort = 0;
		cSWindow = 0;
	}

	// Run the grammar & print results
	while (!gWindow->next.empty()) {
		while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
			SingleWindow* tmp = gWindow->previous.front();
			printSingleWindow(tmp, output);
			free_swindow(tmp);
			gWindow->previous.erase(gWindow->previous.begin());
		}
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

	if ((inchar) && inchar != 0xffff) {
		u_fprintf(output, "%C", inchar); // eg. final newline
	}

	u_fflush(output);

	ticks tmp = getticks();
	grammar->total_time = elapsed(tmp, timer);
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
void ApertiumApplicator::processReading(Reading* cReading, const UChar* reading_string) {
	const UChar* m = reading_string;
	const UChar* c = reading_string;
	UString tmptag;
	UString base;
	UString suf;
	bool tags = false;
	bool unknown = false;
	bool multi = false;
	UChar join_idx = '0'; // Set the join index to the number '0' in ASCII/UTF-8

	insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);

	// Look through the reading for the '#' symbol signifying that
	// this is a multiword.
	while (*m != '\0') {
		if (*m == '\0') {
			break;
		}

		if (*m == '<') {
			tags = true;
		}

		if (*m == '#' && tags == true) { // We only want to shift the lemq if we have seen tags
			multi = true;
		}

		if (*m == '+' && multi == true) { // If we see a '+' and we are in a multiword queue, we want to stop appending
			multi = false;
		}

		if (multi) {
			suf += *m;
		}

		++m;
	}

	// We encapsulate baseforms within '"' for internal processing.
	base += '"';
	while (*c != '\0') {
		if (*c == '*') { // Initial asterisk means word is unknown, and
			             // should just be copied in the output.
			unknown = true;
		}
		// Mark the reading as deleted if initial character is the Not Sign
		if (base[1] == 0 && *c == not_sign) {
			cReading->deleted = true;
			++c;
			continue;
		}
		if (*c == '<' || *c == '\0') {
			break;
		}
		base += *c;
		++c;
	}

	if (!suf.empty()) { // Append the multiword suffix to the baseform
		// (this is normally done in pretransfer)
		base += suf;
	}
	base += '"';

	//	u_fprintf(ux_stderr, ">> b: %S s: %S\n", base.c_str(), suf.c_str());

	TagVector taglist;

	Tag* tag = addTag(base);

	if (unknown) {
		cReading->baseform = tag->hash;
		addTagToReading(*cReading, tag);
		return;
	}

	taglist.push_back(tag);

	bool joiner = false;
	bool intag = false;

	// Now read in the tags
	while (*c != '\0') {
		if (*c == '\0') {
			return;
		}

		if (*c == '+') {
			multi = false;
			joiner = true;
			++join_idx;
		}
		if (*c == '#' && intag == false) { // If we're outside a tag, and we see #, don't append
			multi = true;
		}

		if (*c == '<') {
			multi = false;
			if (intag == true) {
				u_fprintf(ux_stderr, "Error: The Apertium stream format does not allow '<' in tag names.\n");
				++c;
				continue;
			}
			intag = true;

			if (joiner == true) {
				UString bf;
				bf += '"';
				if (tmptag[0] == '+') {
					bf.append(tmptag.begin() + 1, tmptag.end());
				}
				else {
					bf += tmptag;
				}
				bf += '"';
				taglist.push_back(addTag(bf));

				tmptag.clear();
				joiner = false;
			}
			++c;
			continue;
		}
		else if (*c == '>') {
			multi = false;
			if (intag == false) {
				u_fprintf(ux_stderr, "Error: The Apertium stream format does not allow '>' outside tag names.\n");
				++c;
				continue;
			}

			intag = false;

			taglist.push_back(addTag(tmptag));

			tmptag.clear();
			joiner = false;
			++c;
			continue;
		}

		if (multi == true) { // Multiword queue is not part of a tag
			++c;
			continue;
		}

		tmptag += *c;
		++c;
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
				}
				// Add tags
				TagList mappings;
				TagVector::iterator iter = riter.base();
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

void ApertiumApplicator::processReading(Reading* cReading, const UString& reading_string) {
	return processReading(cReading, reading_string.c_str());
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
		Reading* reading = alloc_reading(0);
		processReading(reading, text);
		if (grammar->sub_readings_ltr && reading->next) {
			reading = reverse(reading);
		}
		printReading(reading, output);
		u_fprintf(output, "\n");
		delete reading;
	}
}

void ApertiumApplicator::printReading(Reading* reading, std::ostream& output) {
	if (reading->noprint) {
		return;
	}

	if (reading->next) {
		printReading(reading->next, output);
		u_fputc('+', output);
	}

	if (reading->baseform) {
		// Lop off the initial and final '"' characters
		UnicodeString bf(single_tags[reading->baseform]->tag.c_str() + 1, single_tags[reading->baseform]->tag.size() - 2);

		if (wordform_case && !reading->next) {
			// Use surface/wordform case, eg. if lt-proc
			// was called with "-w" option (which puts
			// dictionary case on lemma/basefrom)
			// Lop off the initial and final '"<>"' characters
			// ToDo: A copy does not need to be made here - use pointer offsets
			UnicodeString wf(reading->parent->wordform->tag.c_str() + 2, reading->parent->wordform->tag.size() - 4);

			int first = 0; // first occurrence of a lowercase character in baseform
			for (; first < bf.length(); ++first) {
				if (u_islower(bf[first]) != 0) {
					break;
				}
			}

			// this corresponds to fst_processor.cc in lttoolbox:
			bool firstupper = first < wf.length() && (u_isupper(wf[first]) != 0);
			bool uppercase = firstupper && u_isupper(wf[wf.length() - 1]);

			if (uppercase) {
				bf.toUpper(); // Perform a Unicode case folding to upper case -- Tino Didriksen
			}
			else if (firstupper && first < bf.length()) {
				// static_cast<UChar>(u_toupper(bf[first])) gives strange output
				UnicodeString range(bf, first, 1);
				range.toUpper();
				bf.setCharAt(first, range[0]);
			}
		} // if (wordform_case)

		UString bf_escaped;
		for (int i = 0; i < bf.length(); ++i) {
			if (bf[i] == '^' || bf[i] == '\\' || bf[i] == '/' || bf[i] == '$' || bf[i] == '[' || bf[i] == ']' || bf[i] == '{' || bf[i] == '}' || bf[i] == '<' || bf[i] == '>') {
				bf_escaped += '\\';
			}
			bf_escaped += bf[i];
		}
		u_fprintf(output, "%S", bf_escaped.c_str());

		// Tag::printTagRaw(output, single_tags[reading->baseform]);
	}

	// Reorder: MAPPING tags should appear before the join of multiword tags,
	// turn <vblex><actv><pri><p3><pl>+í<pr><@FMAINV><@FOO>$
	// into <vblex><actv><pri><p3><pl><@FMAINV><@FOO>+í<pr>$
	Reading::tags_list_t tags_list;
	Reading::tags_list_t multitags_list; // everything after a +, until the first MAPPING tag
	Reading::tags_list_t::iterator tter;
	bool multi = false;
	for (tter = reading->tags_list.begin(); tter != reading->tags_list.end(); tter++) {
		const Tag* tag = single_tags[*tter];
		if (tag->tag[0] == '+') {
			multi = true;
		}
		else if (tag->type & T_MAPPING) {
			multi = false;
		}

		if (multi) {
			multitags_list.push_back(*tter);
		}
		else {
			tags_list.push_back(*tter);
		}
	}
	tags_list.insert(tags_list.end(), multitags_list.begin(), multitags_list.end());

	uint32SortedVector used_tags;
	for (tter = tags_list.begin(); tter != tags_list.end(); tter++) {
		if (unique_tags) {
			if (used_tags.find(*tter) != used_tags.end()) {
				continue;
			}
			used_tags.insert(*tter);
		}
		if (*tter == endtag || *tter == begintag) {
			continue;
		}
		const Tag* tag = single_tags[*tter];
		if (!(tag->type & T_BASEFORM) && !(tag->type & T_WORDFORM)) {
			if (tag->tag[0] == '+') {
				u_fprintf(output, "%S", tag->tag.c_str());
			}
			else if (tag->tag[0] == '&') {
				u_fprintf(output, "<%S>", substr(tag->tag, 2).c_str());
			}
			else {
				u_fprintf(output, "<%S>", tag->tag.c_str());
			}
		}
	}

	if (trace) {
		for (auto iter_hb : reading->hit_by) {
			u_fputc('<', output);
			printTrace(output, iter_hb);
			u_fputc('>', output);
		}
	}
}

void ApertiumApplicator::printSingleWindow(SingleWindow* window, std::ostream& output) {
	// Window text comes at the left
	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.c_str());
	}

	for (uint32_t c = 0; c < window->cohorts.size(); c++) {
		if (c == 0) { // Skip magic cohort
			continue;
		}

		Cohort* cohort = window->cohorts[c];

		if (!split_mappings) {
			mergeMappings(*cohort);
		}

		// Start of cohort
		u_fprintf(output, "^");

		if (print_word_forms == true) {
			// Lop off the initial and final '"' characters
			// ToDo: A copy does not need to be made here - use pointer offsets
			UnicodeString wf(cohort->wordform->tag.c_str() + 2, cohort->wordform->tag.size() - 4);
			UString wf_escaped;
			for (int i = 0; i < wf.length(); ++i) {
				if (wf[i] == '^' || wf[i] == '\\' || wf[i] == '/' || wf[i] == '$' || wf[i] == '[' || wf[i] == ']' || wf[i] == '{' || wf[i] == '}' || wf[i] == '<' || wf[i] == '>') {
					wf_escaped += '\\';
				}
				wf_escaped += wf[i];
			}
			u_fprintf(output, "%S", wf_escaped.c_str());

			// Print the static reading tags
			if (cohort->wread) {
				for (auto tter : cohort->wread->tags_list) {
					if (tter == cohort->wordform->hash) {
						continue;
					}
					const Tag* tag = single_tags[tter];
					u_fprintf(output, "<%S>", tag->tag.c_str());
				}
			}
		}

		bool need_slash = print_word_forms;

		//Tag::printTagRaw(output, single_tags[cohort->wordform]);
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

		u_fprintf(output, "$");
		// End of cohort

		if (!cohort->text.empty()) {
			u_fprintf(output, "%S", cohort->text.c_str());
		}

		u_fflush(output);
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

	std::map<uint32_t, ReadingList>::iterator miter;
	for (miter = mlist.begin(); miter != mlist.end(); miter++) {
		ReadingList clist = miter->second;
		// no merging of mapping tags, so just take first reading of the group
		order.push_back(clist.front());

		clist.erase(clist.begin());
		for (auto cit : clist) {
			free_reading(cit);
		}
	}

	std::sort(order.begin(), order.end(), CG3::Reading::cmp_number);
	cohort.readings.insert(cohort.readings.begin(), order.begin(), order.end());
}
}
