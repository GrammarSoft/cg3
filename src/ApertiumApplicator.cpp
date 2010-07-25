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

#include "ApertiumApplicator.h"
#include "GrammarApplicator.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"

namespace CG3 {

ApertiumApplicator::ApertiumApplicator(UFILE *ux_err) 
	: GrammarApplicator(ux_err)
{
	nullFlush=false;
	wordform_case = false;
	print_word_forms = true;
	runningWithNullFlush=false;
	fgetc_converter=0;
}


bool ApertiumApplicator::getNullFlush() {
	return nullFlush;
}

void ApertiumApplicator::setNullFlush(bool pNullFlush) {
	nullFlush=pNullFlush;
}

UChar ApertiumApplicator::u_fgetc_wrapper(UFILE *input) {
	if (runningWithNullFlush) {
		if (!fgetc_converter) {
			fgetc_error=U_ZERO_ERROR;
			fgetc_converter = ucnv_open(ucnv_getDefaultName(), &fgetc_error);
			if (U_FAILURE(fgetc_error)) {
					u_fprintf(ux_stderr, "Error in ucnv_open: %d\n", fgetc_error);
				}
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
 
 
int ApertiumApplicator::runGrammarOnTextWrapperNullFlush(UFILE *input, UFILE *output) {
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
 */

int ApertiumApplicator::runGrammarOnText(UFILE *input, UFILE *output) {
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
	
	if (!grammar->delimiters || (grammar->delimiters->sets.empty() && grammar->delimiters->tags_set.empty())) {
		if (!grammar->soft_delimiters || (grammar->soft_delimiters->sets.empty() && grammar->soft_delimiters->tags_set.empty())) {
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
	gtimer = getticks();
	ticks timer(gtimer);

	while ((inchar = u_fgetc_wrapper(input)) != 0) {
		if (u_feof(input)) {
			break;
		}

		if (inchar == '[') { // Start of a superblank section
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

	return 0;
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
void ApertiumApplicator::processReading(Reading *cReading, const UChar *reading_string) {
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

		if(*m == '+' && multi == true) { // If we see a '+' and we are in a multiword queue, we want to stop appending
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
			multi = false;
			joiner = true;
			joined = true;
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
				uint32_t tag = addTag(tmptag)->hash;
				addTagToReading(*cReading, tag); // Add the baseform to the reading

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
			multi = false;
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
			addTagToReading(*cReading, tag); // Add the baseform to the reading

			tmptag.clear();
			joiner = false;
			++c;
			continue;
		}

		if(multi == true) { // Multiword queue is not part of a tag
			++c;
			continue;
		}

		tmptag += *c;
		++c;
	}
}

void ApertiumApplicator::processReading(Reading *cReading, const UString& reading_string) {
	return processReading(cReading, reading_string.c_str());
}

void ApertiumApplicator::printReading(Reading *reading, UFILE *output) {
	if (reading->noprint) {
		return;
	}

	if (reading->baseform) {
		// Lop off the initial and final '"' characters
		UnicodeString bf(single_tags[reading->baseform]->tag.c_str()+1, single_tags[reading->baseform]->tag.length()-2);

		if (wordform_case) {
			// Use surface/wordform case, eg. if lt-proc
			// was called with "-w" option (which puts
			// dictionary case on lemma/basefrom)
			// Lop off the initial and final '"<>"' characters
			UnicodeString wf(single_tags[reading->wordform]->tag.c_str()+2, single_tags[reading->baseform]->tag.length()-4);
			
			int first = 0; // first occurrence of a lowercase character in baseform
			for (; first<bf.length() ; ++first) {
				if (u_islower(bf[first]) != 0) {
					break;
				}
			}
			
			// this corresponds to fst_processor.cc in lttoolbox:
			bool firstupper = (u_isupper(wf[first]) != 0); // simply casting will not silence the warning - Tino Didriksen
			bool uppercase = firstupper && u_isupper(wf[wf.length()-1]);

			if (uppercase) {
				bf.toUpper(); // Perform a Unicode case folding to upper case -- Tino Didriksen
			}
			else if (firstupper) {
				int32_t len = bf.length();
				UChar *buf = bf.getBuffer(len);
				buf[first] = static_cast<UChar>(u_toupper(bf[first]));
				bf.releaseBuffer(len);
			}
		} // if (wordform_case)
		
		u_fprintf(output, "%S", bf.getTerminatedBuffer());
		
		// Tag::printTagRaw(output, single_tags[reading->baseform]);
	}

	// Reorder: MAPPING tags should appear before the join of multiword tags,
	// turn <vblex><actv><pri><p3><pl>+í<pr><@FMAINV><@FOO>$ 
	// into <vblex><actv><pri><p3><pl><@FMAINV><@FOO>+í<pr>$ 
	uint32List tags_list;
	uint32List multitags_list; // everything after a +, until the first MAPPING tag
	uint32List::iterator tter;
	bool multi = false;
	for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
		const Tag *tag = single_tags[*tter];
		if (tag->tag[0] == '+') {
			multi = true;
		} else if (tag->type & T_MAPPING) {
			multi = false;
		}
		
		if (multi) {
			multitags_list.push_back(*tter);
		} else {
			tags_list.push_back(*tter);
		}
	}
	tags_list.insert(tags_list.end(),multitags_list.begin(),multitags_list.end());
		
	uint32HashMap used_tags;
	for (tter = tags_list.begin() ; tter != tags_list.end() ; tter++) {
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
				u_fprintf(output, "%S", tag->tag.c_str());	
 			}
			else if (tag->tag[0] == '&') {
				UChar *buf = ux_substr(tag->tag.c_str(), 2, tag->tag.length());
				u_fprintf(output, "<%S>", buf);
				delete[] buf;
 			}
 			else {
				u_fprintf(output, "<%S>", tag->tag.c_str());	
 			}
 		}
	}
}

void ApertiumApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {

	// Window text comes at the left
	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.c_str());
	}

	for (uint32_t c=0 ; c < window->cohorts.size() ; c++) {
		if (c == 0) { // Skip magic cohort
			continue;
		}

		Cohort *cohort = window->cohorts[c];

		mergeMappings(*cohort);

		// Start of cohort 
		u_fprintf(output, "^");

		if(print_word_forms == true) {
			// Lop off the initial and final '"' characters 
			UString wf(single_tags[cohort->wordform]->tag.c_str()+2, single_tags[cohort->wordform]->tag.length()-4);
			u_fprintf(output, "%S/", wf.c_str());
		}

		//Tag::printTagRaw(output, single_tags[cohort->wordform]);

		ReadingList::iterator rter;
		for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
			printReading(*rter, output);
			if (*rter != cohort->readings.back()) {
				u_fprintf(output, "/");
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

void ApertiumApplicator::mergeMappings(Cohort &cohort) {
	// Only merge readings which are completely equal (including mapping tags)
	// foo<N><Sg><Acc><@←OBJ>/foo<N><Sg><Acc><@←OBJ>
	// => guovvamánnu<N><Sg><Acc><@←OBJ>
	// foo<N><Sg><Acc><@←SUBJ>/foo<N><Sg><Acc><@←OBJ>
	// => foo<N><Sg><Acc><@←SUBJ>/foo<N><Sg><Acc><@←OBJ>
	std::map<uint32_t, ReadingList> mlist;
	foreach (ReadingList, cohort.readings, iter, iter_end) {
		Reading *r = *iter;
		uint32_t hp = r->hash; // instead of hash_plain, which doesn't include mapping tags
		if (trace) {
			foreach (uint32Vector, r->hit_by, iter_hb, iter_hb_end) {
				hp = hash_sdbm_uint32_t(*iter_hb, hp);
			}
		}
		mlist[hp].push_back(r);
	}

	if (mlist.size() == cohort.readings.size()) {
		return;
	}

	cohort.readings.clear();
	std::vector<Reading*> order;

	std::map<uint32_t, ReadingList>::iterator miter;
	for (miter = mlist.begin() ; miter != mlist.end() ; miter++) {
		ReadingList clist = miter->second;
		Reading *nr = new Reading(*(clist.front()));
		// no merging of mapping tags
		order.push_back(nr);
	}

	std::sort(order.begin(), order.end(), CG3::Reading::cmp_number);
	cohort.readings.insert(cohort.readings.begin(), order.begin(), order.end());
}
  
}
