/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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

#include "MatxinApplicator.h"
#include "GrammarApplicator.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"

namespace CG3 {

MatxinApplicator::MatxinApplicator(UFILE *ux_err) 
	: GrammarApplicator(ux_err)
{
	nullFlush=false;
	wordform_case = false;
	print_word_forms = true;
	runningWithNullFlush=false;
	fgetc_converter=0;
	window_alloc=0;
}


bool MatxinApplicator::getNullFlush() {
	return nullFlush;
}

void MatxinApplicator::setNullFlush(bool pNullFlush) {
	nullFlush=pNullFlush;
}

UChar MatxinApplicator::u_fgetc_wrapper(UFILE *input) {
	if (runningWithNullFlush) {
		if (!fgetc_converter) {
			fgetc_converter = ucnv_open(ucnv_getDefaultName(), &fgetc_error);
		}
		int ch;
		int result;
		int inputsize=0;
		
		do {
			ch = fgetc(u_fgetfile(input));
			if (ch==0) {
				return 0;
			}
			else {
				fgetc_inputbuf[inputsize]=static_cast<char>(ch);
				inputsize++;
				fgetc_error=U_ZERO_ERROR;
				result = ucnv_toUChars(fgetc_converter, fgetc_outputbuf, 5, fgetc_inputbuf, inputsize, &fgetc_error);
				if (U_FAILURE(fgetc_error)) {
					u_fprintf(ux_stderr, "Error conversion: %d\n", fgetc_error);
				}
			}
		}
		while (( ((result>=1 && fgetc_outputbuf[0]==0xFFFD))  || result<1 || U_FAILURE(fgetc_error) ) && !u_feof(input) && inputsize<5);

		if (fgetc_outputbuf[0]==0xFFFD && u_feof(input)) {
			return U_EOF;
		}
		return fgetc_outputbuf[0];
	}
	else {
		return u_fgetc(input);
	}
 }
 
 
int MatxinApplicator::runGrammarOnTextWrapperNullFlush(UFILE *input, UFILE *output) {
	setNullFlush(false);
	runningWithNullFlush=true;
	while (!u_feof(input)) {
		runGrammarOnText(input, output);
		u_fputc('\0', output);
		u_fflush(output);
	}
	runningWithNullFlush=false;
	return 0;
}

/* 
 * Run a constraint grammar on an Apertium input stream
 * TODO KBU: if this remains all more or less the same as in ApertiumApplicator, we could probably subclass it
 * (they read the same type of input, just have different output)
 */

int MatxinApplicator::runGrammarOnText(UFILE *input, UFILE *output) {
	if (getNullFlush()) {
		return runGrammarOnTextWrapperNullFlush(input, output);
	}
	
	if (!input) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		CG3Quit(1);
	}
	u_frewind(input);
	if (u_feof(input)) {
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
	
	// TODO: what do we use as delimiters? Currently we only get
	// hard breaks, but this error never shows up since the "old"
	// delimiters are inside superblanks. -KBU
	// ToDo: Relying on hard breaks is bad...if data is available to do so, chunk sententes into their own SingleWindow. -- Tino Didriksen
	if (!grammar->delimiters || grammar->delimiters->empty()) {
		if (!grammar->soft_delimiters || grammar->soft_delimiters->empty()) {
			u_fprintf(ux_stderr, "Warning: No soft or hard delimiters defined in grammar. Hard limit of %u cohorts may break windows in unintended places.\n", hard_limit);
		}
		else {
			u_fprintf(ux_stderr, "Warning: No hard delimiters defined in grammar. Soft limit of %u cohorts may break windows in unintended places.\n", soft_limit);
		}
	}

	UChar inchar = 0; 		// Current character 
	bool superblank = false; 	// Are we in a superblank ?
	bool incohort = false; 		// Are we in a cohort ?
	
	index();

	uint32_t resetAfter = ((num_windows+4)*2+1);
	
	begintag = addTag(stringbits[S_BEGINTAG].getTerminatedBuffer())->hash; // Beginning of sentence tag
	endtag = addTag(stringbits[S_ENDTAG].getTerminatedBuffer())->hash; // End of sentence tag

	SingleWindow *cSWindow = 0; 	// Current single window (Cohort frame)
	Cohort *cCohort = 0; 		// Current cohort
	Reading *cReading = 0; 		// Current reading

	SingleWindow *lSWindow = 0; 	// Left hand single window
	Cohort *lCohort = 0; 		// Left hand cohort
	Reading *lReading = 0; 		// Left hand reading

	gWindow->window_span = num_windows;

	u_fprintf(output, "<?xml version='1.0' encoding='utf-8'?>\n<corpus>\n");

	gtimer = getticks();
	ticks timer(gtimer);

	while ((inchar = u_fgetc_wrapper(input)) != 0) { 
		if (u_feof(input)) {
			break;
		}

		if (inchar == '[') { // Start of a superblank section
			// TODO: Matxin wants formatting in a separate file! -KBU
			superblank = true;	
		}

		if (inchar == ']') { // End of a superblank section
			superblank = false;
		}

		if (inchar == '\\' && !incohort) {
			if (cCohort) {
				cCohort->text += inchar;
				inchar = u_fgetc_wrapper(input);
				cCohort->text += inchar;
			}
			else if (lSWindow) {
				lSWindow->text += inchar;
				inchar = u_fgetc_wrapper(input);
				lSWindow->text += inchar;
			}
			else {
				u_fprintf(output, "%C", inchar);
				inchar = u_fgetc_wrapper(input); 
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
				u_fprintf(output, "%C", inchar);
			}
			continue;
		}
			
		if (incohort) {
			// Create magic reading
			if (cCohort && cCohort->readings.empty()) {
				cReading = initEmptyCohort(*cCohort);
				lReading = cReading;
			}
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesSetMatchCohortNormal(*cCohort, grammar->soft_delimiters->hash)) {
			  // ie. we've read some cohorts
				foreach (ReadingList, cCohort->readings, iter, iter_end) {
					addTagToReading(**iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			} // end >= soft_limit
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (grammar->delimiters && doesSetMatchCohortNormal(*cCohort, grammar->delimiters->hash)))) {
				if (cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at line %u - forcing break.\n", hard_limit, numLines);
					u_fflush(ux_stderr);
				}
				foreach (ReadingList, cCohort->readings, iter, iter_end) {
					addTagToReading(**iter, endtag);
				}
				
				cSWindow->appendCohort(cCohort);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			} // end >= hard_limit
			// If we don't have a current window, create one
			if (!cSWindow) {
				cSWindow = gWindow->allocAppendSingleWindow();
				
				// Create 0th Cohort which serves as the beginning of sentence
				cCohort = new Cohort(cSWindow);
				cCohort->global_number = 0;
				cCohort->wordform = begintag;

				cReading = new Reading(cCohort);
				cReading->baseform = begintag;
				cReading->wordform = begintag;
				if (grammar->sets_any && !grammar->sets_any->empty()) {
					cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
				}
				addTagToReading(*cReading, begintag);

				cCohort->appendReading(cReading);

				cSWindow->appendCohort(cCohort);

				lSWindow = cSWindow;
				lReading = cReading;
				lCohort = cCohort;
				cCohort = 0;
				numWindows++;
			} // created at least one cSWindow by now
			
			// If the current cohort is looking ok, and we have an available
			// window, add the cohort to the window.
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
			} 
			if (gWindow->next.size() > num_windows) {
				while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
					SingleWindow *tmp = gWindow->previous.front();
					printSingleWindow(tmp, output);
					delete tmp;
					gWindow->previous.pop_front();
				}
				gWindow->shuffleWindowsDown();
				runGrammarOnWindow();
				if (numWindows % resetAfter == 0) {
					resetIndexes();
				}
			}
			cCohort = new Cohort(cSWindow);
			cCohort->global_number = gWindow->cohort_counter++;

			// Read in the word form
			UString wordform;

			wordform += '"';
			// We encapsulate wordforms within '"<' and
			// '>"' for internal processing.
			wordform += '<';
			while (inchar != '/') {
				inchar = u_fgetc_wrapper(input); 

				if (inchar != '/') {
					wordform += inchar;
				}
			}
			
			wordform += '>';
			wordform += '"';
			
			//u_fprintf(output, "# %S\n", wordform);
			cCohort->wordform = addTag(wordform)->hash;
			lCohort = cCohort;
			lReading = 0;
			numCohorts++;

			// We're now at the beginning of the readings
			UString current_reading;
			Reading *cReading = 0;

			// Read in the readings	
			while (inchar != '$') {
				inchar = u_fgetc_wrapper(input);

	 			if (inchar == '$') {
					// Add the final reading of the cohort
					cReading = new Reading(cCohort);
					cReading->wordform = cCohort->wordform;

					if (grammar->sets_any && !grammar->sets_any->empty()) {
						cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
					}

					addTagToReading(*cReading, cReading->wordform);
					processReading(cReading, current_reading);

					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;

					current_reading.clear();

					incohort = false;
				}

				if (inchar == '/') { // Reached end of reading
					// Note: MatxinApplicator expects completely disambiguated input (printSingleWindow signals a warning)
					Reading *cReading = 0;
					cReading = new Reading(cCohort);
					cReading->wordform = cCohort->wordform;

					addTagToReading(*cReading, cReading->wordform);

					processReading(cReading, current_reading);

					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
	
					current_reading.clear();
					continue; // while not $
				}
	
				current_reading += inchar;
			} // end while not $

			if (!cReading->baseform) {
				u_fprintf(ux_stderr, "Warning: Line %u had no valid baseform.\n", numLines);
				u_fflush(ux_stderr);
			}
		} // end reading
		numLines++;
		inchar = 0;
	} // end input loop

	if (cCohort && cSWindow) {
		cSWindow->appendCohort(cCohort);
		// Create magic reading
		if (cCohort->readings.empty()) {
			cReading = initEmptyCohort(*cCohort);
		}
		foreach (ReadingList, cCohort->readings, iter, iter_end) {
			addTagToReading(**iter, endtag);
		}
		cReading = 0;
		cCohort = 0;
		cSWindow = 0;
	}
	
	// Run the grammar & print results
	while (!gWindow->next.empty()) {
		while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
			SingleWindow *tmp = gWindow->previous.front();
			printSingleWindow(tmp, output);
			delete tmp;
			gWindow->previous.pop_front();
		}
		gWindow->shuffleWindowsDown();
		runGrammarOnWindow();
	}
	
	gWindow->shuffleWindowsDown();
	while (!gWindow->previous.empty()) {
		SingleWindow *tmp = gWindow->previous.front();
		printSingleWindow(tmp, output);
		delete tmp;
		gWindow->previous.pop_front();
	}
	
	if ((inchar) && inchar != 0xffff) {
		u_fprintf(output, "%C", inchar); // eg. final newline
	}

	u_fflush(output);

	ticks tmp = getticks();
	grammar->total_time = elapsed(tmp, timer);

	u_fprintf(output, "</corpus>\n");
	
	return 0;
} // runGrammarOnText


/*
 * Parse an Apertium reading into a CG Reading
 *
 * Examples:
 *   venir<vblex><imp><p2><sg>
 *   venir<vblex><inf>+lo<prn><enc><p3><nt><sg>
 *   be<vblex><inf># happy
 *   be# happy<vblex><inf> (for chaining cg-proc)
 */
void MatxinApplicator::processReading(Reading *cReading, const UChar *reading_string) {
	const UChar *m = reading_string;
	const UChar *c = reading_string;
	UString tmptag;
	UString base;
	UString suf;
	bool tags = false; 
	bool unknown = false;
	bool multi = false;
	bool joined = false;
	UChar join_idx = '0'; // Set the join index to the number '0' in ASCII/UTF-8

	if (grammar->sets_any && !grammar->sets_any->empty()) {
		cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
	}
	
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

		if (multi) {
			suf += *m;
		}

		m++;
	}

	// We encapsulate baseforms within '"' for internal processing.
	base += '"';
	while (*c != '\0') {
		if (*c == '*') { // Initial asterisk means word is unknown, and 
				// should just be copied in the output.
			unknown = true;
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

	uint32_t tag = addTag(base)->hash;
	cReading->baseform = tag;
	addTagToReading(*cReading, tag);

	if (unknown) {
		return;
	}

	bool joiner = false;
	bool intag = false;
	
	// Now read in the tags
	while (*c != '\0') {
		if (*c == '\0') {
			return;
		}

		if (*c == '+') {
			joiner = true;
			joined = true;
			++join_idx;
		}

		if (*c == '<') {
			if (intag == true) {
				u_fprintf(ux_stderr, "Error: The Apertium stream format does not allow '<' in tag names.\n");
				++c;
				continue;
			}
			intag = true;
			
			if (joiner == true) {
				uint32_t tag = addTag(tmptag)->hash;
				addTagToReading(*cReading, tag); // Add the baseform to the tag

				tmptag.clear();
				joiner = false;
				++c;
				continue;

			}
			else {

				++c;
				continue;
			}

		}
		else if (*c == '>') {
			if (intag == false) {
				u_fprintf(ux_stderr, "Error: The Apertium stream format does not allow '>' in tag names.\n");
				++c;
				continue;
			}
			intag = false;

			uint32_t shufty = addTag(tmptag)->hash; 
			UString newtag;
			if (cReading->tags.find(shufty) != cReading->tags.end()) {
				newtag += '&';
				newtag += join_idx;
				newtag += tmptag;
			}
			else {
				newtag += tmptag;
			}
			uint32_t tag = addTag(newtag)->hash;
			addTagToReading(*cReading, tag); // Add the baseform to the tag

			tmptag.clear();
			joiner = false;
			++c;
			continue;
		}
		tmptag += *c;
		++c;
	}
}

void MatxinApplicator::processReading(Reading *cReading, const UString& reading_string) {
	return processReading(cReading, reading_string.c_str());
}

int MatxinApplicator::printReading(Reading *reading, UFILE *output, int ischunk, int ord, int alloc) {
	if (reading->noprint) {
		return ord;
	}

	// CHUNK was just null's when I had this line in the constructor, why? -KBU
	CHUNK = UNICODE_STRING_SIMPLE("CHUNK").getTerminatedBuffer();

	uint32HashMap used_tags;
	uint32List::iterator tter;
	UString tags;
	UString syntags;
	for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
		if (used_tags.find(*tter) != used_tags.end()) {
			continue;
		}
		if (*tter == endtag || *tter == begintag) {
			continue;
		}
		used_tags[*tter] = *tter;
		const Tag *tag = single_tags[*tter];
		if (!(tag->type & T_BASEFORM) && !(tag->type & T_WORDFORM)) {
			if (tag->tag[0] == '+') {
				tags += tag->tag;
			}
			else if (tag->tag[0] == '&') {
				tags += '[';
				UChar *buf = ux_substr(tag->tag.c_str(), 2, tag->tag.length());
				tags += buf;
				delete[] buf;
				tags += ']';
			}
			else if (tag->type & T_MAPPING) {
				syntags += tag->tag;
				syntags += '.';
			}
			else if (tag->tag[0] != *CHUNK) {
				tags += '[';
				tags += tag->tag;
				tags += ']';
			}
		}
	} // for tags

	if (ischunk) {
		u_fprintf(output, "<CHUNK ord='%d' alloc='%d'", ord, alloc);
		if (!syntags.empty()) {
			UChar *buf = ux_substr(syntags.c_str(), 0, syntags.size()-1);
			u_fprintf(output, " si='%S'", buf);
			delete[] buf;
		}
		u_fprintf(output, ">\n  <NODE");
	} else {
		u_fprintf(output, "\n    <NODE");
	}

	if (!tags.empty()) {
		u_fprintf(output, " mi='%S'", tags.c_str());
	}
	if (!syntags.empty()) {
		UChar *buf = ux_substr(syntags.c_str(), 0, syntags.size()-1);
		u_fprintf(output, " si='%S'", buf);
		delete[] buf;
	}

	// ord: order in source sentence. local_number is x in the #x->y dependency output so we use that
	// alloc: character position in source sentence
	u_fprintf(output, " ord='%u' alloc='%u'", reading->parent->local_number, alloc);

	if (reading->baseform) {
		UChar *bf = 0;
		// Lop off the initial and final '"' characters
		bf = ux_substr(single_tags[reading->baseform]->tag.c_str(), 1, single_tags[reading->baseform]->tag.length()-1);

		if (wordform_case) {
			// Use surface/wordform case, eg. if lt-proc
			// was called with "-w" option (which puts
			// dictionary case on lemma/basefrom)
			UChar *wf = 0;
			// Lop off the initial and final '"<>' characters
			wf = ux_substr(single_tags[reading->wordform]->tag.c_str(), 2, single_tags[reading->wordform]->tag.length()-2);
			
			int first = 0; // first occurrence of a lowercase character in baseform
			for (; first<u_strlen(bf); first++) {
				if (u_islower(bf[first]) != 0) {
					break;
				}
			}
			
			// this corresponds to fst_processor.cc in lttoolbox:
			bool firstupper = (u_isupper(wf[first]) != 0); // simply casting will not silence the warning - Tino Didriksen
			bool uppercase = firstupper && u_isupper(wf[u_strlen(wf)-1]);

			if (uppercase) {
				for (int i=0; i<u_strlen(bf); i++) {
					bf[i] = static_cast<UChar>(u_toupper(bf[i]));
				}
			}
			else {
				if (firstupper) {
					bf[first] = static_cast<UChar>(u_toupper(bf[first]));
				} 
			}
			delete[] wf;
			wf = 0;
		} // if (wordform_case)
		
		if ((*bf) && (*bf == '*')) {
			u_fprintf(output, " lem='%S'", bf+1);
			u_fprintf(output, " unknown='analysis'");
		}
		else {
			u_fprintf(output, " lem='%S'", bf);
		}
						
		delete[] bf;
		bf = 0;
                // Tag::printTagRaw(output, single_tags[reading->baseform]);
	}

	return ord;
}

void MatxinApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {
	// CHUNK was just null's when I had this line in the constructor, why? -KBU
	CHUNK = UNICODE_STRING_SIMPLE("CHUNK").getTerminatedBuffer();

	std::vector<size_t> alloc(window->cohorts.size()+1);
	alloc[1] = window_alloc;
	// Window text comes at the left
	if (!window->text.empty()) {
		// TODO: warn if this (or cohort->text) is non-whitespace? or wrap it in a tag? o/w it's removed by later matxin modules
		u_fprintf(output, "%S", window->text.c_str());
	} 
		
	// alloc of sentence is alloc of first cohort:
	u_fprintf(output, "<SENTENCE ord='%d' alloc='%d'>\n", window->number, alloc[1]);
		
	std::stack<Cohort*> tree;	// dependency tree
 	Cohort *cohort = 0;		// e.g. head of a dependency relation
	std::stack<int> endtagtree;	// i.e. ischunk markers 
	int ischunk;	                // 0 = NODE, 1 = CHUNK
	
        // chunk_ord[X]==ord if cohort with local_number X is a CHUNK, otherwise 0:
	// C++ does not allow variable length arrays (that is a gcc extension); converted to std::vector - Tino
	std::vector<uint32_t> chunk_ord(window->cohorts.size());
	int ord = 1;		        // Relative order of CHUNK cohorts in window
	// Add top nodes to the stack, and store ord of chunks in chunk_ord:
	for (uint32_t c=0 ; c < window->cohorts.size() ; c++) {
		if (c == 0) { // Skip magic cohort
			continue;
		}
		cohort = window->cohorts[c];

		// remove "< and >", +1 for space (trusting the deformatter) + what we've seen:
		alloc[cohort->local_number+1] = single_tags[cohort->wordform]->tag.length()-4 + 1 + alloc[cohort->local_number];
		// TODO: we _could_ use cohort->text instead of 1, but cohort->text is removed by later matxin modules

		ischunk = 0;
		// Add root (#x->0) and independent (#x->x) cohorts to the stack, all as CHUNK:
		if (cohort->dep_parent == std::numeric_limits<uint32_t>::max() || cohort->dep_parent == 0) {
			tree.push(cohort);
			ischunk = 1;
			endtagtree.push(ischunk);
		}
		// If CHUNK, mark as such (we need to look this up using local_number)
		ReadingList::iterator rter;
		rter = cohort->readings.begin();
		Reading *reading = *rter;
		for (uint32List::iterator tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
			const Tag *tag = single_tags[*tter];
			if (tag->tag[0] == *CHUNK) {
				ischunk = 1;
			}
		}
		if (ischunk) {
			chunk_ord[cohort->local_number] = ord;
			ord++;
		} else {
			chunk_ord[cohort->local_number] = 0;
		}
	} // for cohorts

	window_alloc = alloc[window->cohorts.size()];
	
	// Print cohorts, adding their children to the stack
	while (!tree.empty()) {
		cohort = tree.top();
		tree.pop();

		// Check if this syntactic function should be a CHUNK
		ischunk = endtagtree.top();
		if (cohort == 0) { // Print end of cohort-marker
			endtagtree.pop();
			if (ischunk == 1) {
				u_fprintf(output, "</CHUNK>\n");
			} else {
				u_fprintf(output, "  </NODE>\n");
			}
			continue;
		}
		tree.push(0);	// add end of cohort-marker before adding children
		// Alternative method: don't pop current cohort yet; add the children first, then when we see a number we've seen before, pop and print /NODE

		UString syntags; // ToDo: What is this variable used for? -- Tino Didriksen
		ReadingList::iterator rter;
		rter = cohort->readings.begin();
		Reading *reading = *rter;
		for (uint32List::iterator tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
			const Tag *tag = single_tags[*tter];
			if (tag->type & T_MAPPING) {
				syntags += tag->tag;
				syntags += '.';
			}
		}
		printReading(*rter, output, ischunk, chunk_ord[cohort->local_number], alloc[cohort->local_number]); 
		
		if (*rter != cohort->readings.back()) {
			u_fprintf(ux_stderr, "Warning: Ambiguous cohort. The Matxin stream-format expects one reading per cohort, only printing the first reading.\n");
		}
		
		if (print_word_forms == true) {
			// Lop off the initial and final '"' characters
			UChar *wf = ux_substr(single_tags[cohort->wordform]->tag.c_str(), 2, single_tags[cohort->wordform]->tag.length()-2);
			u_fprintf(output, " form='%S'", wf);
			delete[] wf;
		}
				
		u_fprintf(output, ">");
				
		if (!cohort->text.empty()) {
			u_fprintf(output, "%S", cohort->text.c_str());
		}
		
		u_fflush(output);
				
		std::stack<Cohort*> nchildtree; // children of type NODE must be printed first
		const uint32HashSet *deps = &cohort->dep_children;
		const_foreach (uint32HashSet, *deps, dter, dter_end) {
			Cohort *child = window->parent->cohort_map.find(*dter)->second;
			if (chunk_ord[child->local_number]) {
				if (!ischunk) {
					u_fprintf(ux_stderr, "Warning: NODE ord='%d' has CHUNK child.\n", cohort->local_number);
				}
				tree.push(child);
				endtagtree.push(1);
			}
			else {
				nchildtree.push(child);
			}
			child = 0;
		}
		if (ischunk) {
			tree.push(0);
			endtagtree.push(0);
		}
		while (!nchildtree.empty()) {
			tree.push(nchildtree.top());
			endtagtree.push(0);
			nchildtree.pop();
		}		
		deps = 0;
		syntags.clear();
	} // while !tree.empty
	cohort = 0;
		
	u_fprintf(output, "\n</SENTENCE>\n"); 
}

}
