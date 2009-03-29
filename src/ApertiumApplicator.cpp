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

#include "ApertiumApplicator.h"
#include "GrammarApplicator.h"

using namespace CG3;
using namespace CG3::Strings;

ApertiumApplicator::ApertiumApplicator(UFILE *ux_in, UFILE *ux_out, UFILE *ux_err) 
	: GrammarApplicator(ux_in, ux_out, ux_err) 
{

}


/* 
 * Run a constraint grammar on an Apertium input stream
 */

int
ApertiumApplicator::runGrammarOnText(UFILE *input, UFILE *output) 
{
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
	// not sure if I should keep this. TODO
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
	/*
--soft-limit          number of cohorts after which the SOFT-DELIMITERS kick in; defaults to 300

--hard-limit          number of cohorts after which the window is forcefully cut; defaults to 500
	*/
#undef BUFFER_SIZE
#define BUFFER_SIZE (131072L)
	UChar inchar = 0; 		// Current character 
	bool superblank = false; 	// Are we in a superblank ?
	bool incohort = false; 		// Are we in a cohort ?
	bool inreading = false;		// Are we in a reading ?
	
	index();

	Recycler *r = Recycler::instance();
	uint32_t resetAfter = ((num_windows+4)*2+1);
	
	begintag = addTag(stringbits[S_BEGINTAG])->hash; // Beginning of sentence tag
	endtag = addTag(stringbits[S_ENDTAG])->hash; // End of sentence tag

	gWindow = new Window(this); 	// Global window singleton

	Window *cWindow = gWindow; 	// Set the current window to the global window
	SingleWindow *cSWindow = 0; 	// Current single window (Cohort frame)
	Cohort *cCohort = 0; 		// Current cohort
	Reading *cReading = 0; 		// Current reading

	SingleWindow *lSWindow = 0; 	// Left hand single window
	Cohort *lCohort = 0; 		// Left hand cohort
	Reading *lReading = 0; 		// Left hand reading

	cWindow->window_span = num_windows;
	gtimer = getticks();
	ticks timer(gtimer);

	while((inchar = u_fgetc(input)) != 0) { 
		if(u_feof(input)) {
			break;
		}

		if(inchar == '[') { // Start of a superblank section
			superblank = true;	
		}

		if(inchar == ']') { // End of a superblank section
			superblank = false;
		}

		if(inchar == '^') {
			incohort = true;
		}
		
		if(superblank == true || inchar == ']' || incohort == false) {
			if (cCohort) {
				cCohort->text_pre = ux_append(cCohort->text_pre, inchar);
			} else if (lSWindow) {
				lSWindow->text = ux_append(lSWindow->text, inchar);
			} else {
				u_fprintf(output, "%C", inchar);
			}
			continue;
		}
			
		if (incohort) {
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesTagMatchSet(cCohort->wordform, grammar->soft_delimiters)) {
			  // ie we've read some cohorts
				if (cCohort->readings.empty()) {
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					if (grammar->sets_any && !grammar->sets_any->empty()) {
						cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
						cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
					}
					addTagToReading(cReading, cCohort->wordform);
					cReading->noprint = true;
					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
				}
				foreach (std::list<Reading*>, cCohort->readings, iter, iter_end) {
					addTagToReading(*iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				cWindow->appendSingleWindow(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			}
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (grammar->delimiters && doesTagMatchSet(cCohort->wordform, grammar->delimiters)))) {
				if (cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at line %u - forcing break.\n", hard_limit, numLines);
					u_fflush(ux_stderr);
				}
				if (cCohort->readings.empty()) {
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					if (grammar->sets_any && !grammar->sets_any->empty()) {
						cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
						cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
					}
					addTagToReading(cReading, cCohort->wordform);
					cReading->noprint = true;
					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
				} 
				foreach (std::list<Reading*>, cCohort->readings, iter, iter_end) {
					addTagToReading(*iter, endtag);
				}
				
				cSWindow->appendCohort(cCohort);
				cWindow->appendSingleWindow(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			}
			// If we don't have a current window, create one.
			if (!cSWindow) {
				// Tino-ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = new SingleWindow(cWindow);

				cSWindow->text = 0; // necessary?  TODO -KBU

				// Create 0th Cohort which serves as the beginning of sentence
				cCohort = r->new_Cohort(cSWindow);
				cCohort->global_number = 0;
				cCohort->wordform = begintag;

				cReading = r->new_Reading(cCohort);
				cReading->baseform = begintag;
				cReading->wordform = begintag;
				if (grammar->sets_any && !grammar->sets_any->empty()) {
					cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
					cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
				}
				addTagToReading(cReading, begintag);

				cCohort->appendReading(cReading);

				cSWindow->appendCohort(cCohort);

				lSWindow = cSWindow;
				lReading = cReading;
				lCohort = cCohort;
				cCohort = 0;
				numWindows++;
			} // created cSWindow by now
			
			// If the current cohort is looking ok, and we have an available
			// window, add the cohort to the window.
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
				if (cCohort->readings.empty()) {
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					if (grammar->sets_any && !grammar->sets_any->empty()) {
						cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
						cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
					}
					addTagToReading(cReading, cCohort->wordform);
					cReading->noprint = true;
					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
				}
			}
			if (cWindow->next.size() > num_windows) {
				while (!cWindow->previous.empty() && cWindow->previous.size() > num_windows) {
					SingleWindow *tmp = cWindow->previous.front();
					printSingleWindow(tmp, output);
					delete tmp;
					cWindow->previous.pop_front();
				}
				cWindow->shuffleWindowsDown();
				runGrammarOnWindow(cWindow);
				if (numWindows % resetAfter == 0) {
					resetIndexes();
					r->trim();
				}
			}
			cCohort = r->new_Cohort(cSWindow);
			cCohort->global_number = cWindow->cohort_counter++;

			// Read in the word form
			UChar *wordform = 0;

			wordform = ux_append(wordform, '"');
			// We encapsulate wordforms within '"<' and
			// '>"' for internal processing.
			wordform = ux_append(wordform, '<');
			while(inchar != '/') {
				inchar = u_fgetc(input); 

				if(inchar != '/') {
					wordform = ux_append(wordform, inchar);
				}
			}
			inreading = true; //  TODO try without below -KBU
			
			wordform = ux_append(wordform, '>');
			wordform = ux_append(wordform, '"');
			
			//u_fprintf(output, "# %S\n", wordform);
			cCohort->wordform = addTag(wordform)->hash;
			lCohort = cCohort;
			lReading = 0;
			wordform = 0;
			numCohorts++;

			// We're now at the beginning of the readings
			UChar *current_reading = 0;
			Reading *cReading = 0;

			// Read in the readings	
			while(inchar != '$') {
				inchar = u_fgetc(input);

	 			if(inchar == '$') { 
					// Add the final reading of the cohort
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;

					if (grammar->sets_any && !grammar->sets_any->empty()) {
						cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
						cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
					}

					addTagToReading(cReading, cReading->wordform);
					processReading(cReading, current_reading);

					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;

					delete[] current_reading;
					current_reading = 0;

					incohort = false;
				}

				if(inchar == '/') { // Reached end of reading
	
					Reading *cReading = 0;
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;

					addTagToReading(cReading, cReading->wordform);

					processReading(cReading, current_reading);

					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
	
					delete[] current_reading;
					current_reading = 0;
					continue; // while not $
				}
	
				current_reading = ux_append(current_reading, inchar);
			} // end while not $


			if (!cReading->baseform) {
				u_fprintf(ux_stderr, "Warning: Line %u had no valid baseform.\n", numLines);
				u_fflush(ux_stderr);
			}
		} // end reading
		//  this _should_ be taken care of by superblanks, right? -KBU
// 		else {
// 			ux_trim(cleaned);
// 			if (u_strlen(cleaned) > 0) {
// 				if (lReading && lCohort) {
// 					lCohort->text_post = ux_append(lCohort->text_post, line);
// 				}
// 				else if (lCohort) {
// 					lCohort->text_pre = ux_append(lCohort->text_pre, line);
// 				}
// 				else if (lSWindow) {
// 					lSWindow->text = ux_append(lSWindow->text, line);
// 				}
// 				else {
// 					u_fprintf(output, "%S", line);
// 				}
// 			}
// 		}
		numLines++;
		inchar = 0;	// needed? TODO -KBU 
	} // end input loop

	if (cCohort && cSWindow) {
		cSWindow->appendCohort(cCohort);
		if (cCohort->readings.empty()) {
			cReading = r->new_Reading(cCohort);
			cReading->wordform = cCohort->wordform;
			cReading->baseform = cCohort->wordform;
			if (grammar->sets_any && !grammar->sets_any->empty()) {
				cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
				cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
			}
			addTagToReading(cReading, cCohort->wordform);
			cReading->noprint = true;
			cCohort->appendReading(cReading);
		}
		foreach (std::list<Reading*>, cCohort->readings, iter, iter_end) {
			addTagToReading(*iter, endtag);
		}
		cWindow->appendSingleWindow(cSWindow);
		cReading = 0;
		cCohort = 0;
		cSWindow = 0;
	}
	while (!cWindow->next.empty()) {
		while (!cWindow->previous.empty() && cWindow->previous.size() > num_windows) {
			SingleWindow *tmp = cWindow->previous.front();
			printSingleWindow(tmp, output);
			delete tmp;
			cWindow->previous.pop_front();
		}
		cWindow->shuffleWindowsDown();
		runGrammarOnWindow(cWindow);
	}

	cWindow->shuffleWindowsDown();
	while (!cWindow->previous.empty()) {
		SingleWindow *tmp = cWindow->previous.front();
		printSingleWindow(tmp, output);
		delete tmp;
		cWindow->previous.pop_front();
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
 */
void
ApertiumApplicator::processReading(Reading *cReading, UChar *reading_string)
{
	UChar *m = reading_string;
	UChar *c = reading_string;
	UChar *tmptag = 0;
	UChar *base = 0;
	UChar *suf = 0;
	bool unknown = false;
	bool multi = false;
	bool joined = false;
	int join_idx = 48; // Set the join index to the number '0' in ASCII/UTF-8

	if (grammar->sets_any && !grammar->sets_any->empty()) {
		cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
		cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
	}

	// Look through the reading for the '#' symbol signifying that
	// this is a multiword.
	while(*m != '\0') {
		if(*m == '\0') {
			break;
		}

		if(*m == '#') {
			multi = true;
		}

		if(multi) {
			suf = ux_append(suf, *m);
		}

		m++;
	}

	// We encapsulate baseforms within '"' for internal processing.
	base = ux_append(base, '"');
	while(*c != '\0') {
		if(*c == '*') { // Initial asterisk means word is unknown, and 
				// should just be copied in the output.
			unknown = true;
		}
		if(*c == '<' || *c == '\0') {
			break;
		}
		base = ux_append(base, *c);
		c++;
	}

	if(suf != 0) { // Append the multiword suffix to the baseform
		       // (this is normally done in pretransfer)

		base = ux_append(base, suf);
	}
	base = ux_append(base, '"');

	uint32_t tag = addTag(base)->hash;
	cReading->baseform = tag;
	addTagToReading(cReading, tag);

	if(unknown) {
		return;
	}

	bool joiner = false;

	// Now read in the tags
	while(*c != '\0') {
		if(*c == '\0') {
			return;
		}

		if(*c == '+') {
			joiner = true;
			joined = true;
			join_idx++;
		}

		if(*c == '<') {
			if(joiner == true) {
				uint32_t tag = addTag(tmptag)->hash;
				addTagToReading(cReading, tag); // Add the baseform to the tag

				delete[] tmptag;
				tmptag = 0;
				joiner = false;
				c++;
				continue;

			} else {

				c++;
				continue;
			}

		} else if(*c == '>') {
			uint32_t shufty = addTag(tmptag)->hash;
			UChar *newtag = 0;
			if (cReading->tags.find(shufty) != cReading->tags.end()) {
				newtag = ux_append(newtag, '&');	
				newtag = ux_append(newtag, (UChar)join_idx);
				newtag = ux_append(newtag, tmptag);
			} else {
				newtag = ux_append(newtag, tmptag);
			}
			uint32_t tag = addTag(newtag)->hash;
			addTagToReading(cReading, tag); // Add the baseform to the tag

			delete[] tmptag;
			tmptag = 0;
			joiner = false;
			c++;
			continue;
		}
		tmptag = ux_append(tmptag, *c);
		c++;
	}
	
	joined = false;
	join_idx = 48;

	return;
}

void 
ApertiumApplicator::printReading(Reading *reading, UFILE *output) 
{
	if (reading->noprint) {
		return;
	}

	if (reading->baseform) {

		// Lop off the initial and final '"' characters 
		UChar *bf = single_tags[reading->baseform]->tag;
		u_fprintf(output, "%S", ux_substr(bf, 1, u_strlen(bf)-1));

		bf = 0;
		
//		Tag::printTagRaw(output, single_tags[reading->baseform]);
	}

	uint32HashMap used_tags;
	uint32List::iterator tter;
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
			if(tag->tag[0] == '+') {
				Tag::printTagRaw(output, tag);
			} else if(tag->tag[0] == '&') {
				u_fprintf(output, "<");
				u_fprintf(output, "%S", ux_substr(tag->tag, 2, u_strlen(tag->tag)));	
				u_fprintf(output, ">");  
			} else {
				u_fprintf(output, "<");
				Tag::printTagRaw(output, tag);
				u_fprintf(output, ">");
			}
		}

	}
}

void 
ApertiumApplicator::printSingleWindow(SingleWindow *window, UFILE *output) 
{

	// Window text comes at the left
	if (window->text) {
		u_fprintf(output, "%S", window->text);
	}

	for (uint32_t c=0 ; c < window->cohorts.size() ; c++) {
		if (c == 0) { // Skip magic cohort
			continue;
		}

		// Start of cohort 
		u_fprintf(output, "^");

		Cohort *cohort = window->cohorts[c];

		// Lop off the initial and final '"' characters 
		UChar *wf = single_tags[cohort->wordform]->tag;
		u_fprintf(output, "%S/", ux_substr(wf, 2, u_strlen(wf)-2));
		wf = 0;

		//Tag::printTagRaw(output, single_tags[cohort->wordform]);

		std::list<Reading*>::iterator rter;
		for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
			printReading(*rter, output);
			if(*rter != cohort->readings.back()) {
				u_fprintf(output, "/");
			}
		}

		u_fprintf(output, "$");
		// End of cohort

		// Cohort text comes at the right.
		if (cohort->text_pre) {
			u_fprintf(output, "%S", cohort->text_pre);
		}
		
		u_fflush(output); // KBU
	}
}
