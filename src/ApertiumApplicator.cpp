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
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to anything!\n");
		CG3Quit(1);
	}

#undef BUFFER_SIZE
#define BUFFER_SIZE (131072L)
	Recycler *r = Recycler::instance();
	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);

	UChar inchar = 0; 		// Current character 
	bool superblank = false; 	// Are we in a superblank ?

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
//	grammar->total_time = clock();

	while((inchar = u_fgetc(input)) != 0) { 
		if(u_feof(input)) {
			break;
		}

		// If we don't have a current window, create one.
		if (inchar == '^' && !cSWindow) {
			cSWindow = new SingleWindow(cWindow);

			cSWindow->text = 0;

			if (grammar->rules_by_tag.find(grammar->tag_any) != grammar->rules_by_tag.end()) {
				cSWindow->valid_rules.insert(grammar->rules_by_tag.find(grammar->tag_any)->second->begin(), 
								grammar->rules_by_tag.find(grammar->tag_any)->second->end());
			}
			if (grammar->rules_by_tag.find(endtag) != grammar->rules_by_tag.end()) {
				cSWindow->valid_rules.insert(grammar->rules_by_tag.find(endtag)->second->begin(), 
								grammar->rules_by_tag.find(endtag)->second->end());
			}

			// Create 0th Cohort which serves as the beginning of sentence
			cCohort = r->new_Cohort(cSWindow);
			cCohort->global_number = 0;
			cCohort->wordform = begintag;

			cReading = r->new_Reading(cCohort);
			cReading->baseform = begintag;
			cReading->wordform = begintag;

			if (grammar->sets_by_tag.find(grammar->tag_any) != grammar->sets_by_tag.end()) {
				cReading->possible_sets.insert(grammar->sets_by_tag.find(grammar->tag_any)->second->begin(), 
								grammar->sets_by_tag.find(grammar->tag_any)->second->end());
			}

			addTagToReading(cReading, begintag);
			cCohort->appendReading(cReading);
			cSWindow->appendCohort(cCohort);

			lSWindow = cSWindow;
			lReading = cReading;
			lCohort = cCohort;
			cCohort = 0;
			numWindows++;
		}

		// Start of a superblank section
		if(inchar == '[') {
			superblank = true;	
		}

		if(inchar == ']') { // End of a superblank section
			superblank = false;
		}

		if(superblank == true || inchar == ']' || u_isWhitespace(inchar)) {
			if (cCohort) {
				cCohort->text = ux_append(cCohort->text, inchar);
			} else if (lSWindow) {
				lSWindow->text = ux_append(lSWindow->text, inchar);
			} else {
				u_fprintf(output, "%C", inchar);
			}
			continue;
		}


		// Beginning of a new cohort
		// Ex.: ^la/el<det><def><f><sg>/lo<prn><pro><p3><f><sg>$
		if(inchar == '^') {

			// Initialise cohort
			cCohort = r->new_Cohort(cSWindow);
			cCohort->global_number = cWindow->cohort_counter++;
			UChar *wordform = 0;
			UChar *current_reading = 0;
			Reading *cReading = 0;

			// Read in the word form

			wordform = ux_append(wordform, '"'); // We encapsulate wordforms within '"<' and '">' for internal processing.
			wordform = ux_append(wordform, '<');
			while(inchar != '/') {
				inchar = u_fgetc(input); 

				if(inchar != '/') {
					wordform = ux_append(wordform, inchar);
				}
			}
			wordform = ux_append(wordform, '>');
			wordform = ux_append(wordform, '"');
		
			cCohort->wordform = addTag(wordform)->hash;
	
			if (grammar->rules_by_tag.find(cCohort->wordform) != grammar->rules_by_tag.end()) {
				cSWindow->valid_rules.insert(grammar->rules_by_tag.find(cCohort->wordform)->second->begin(), 
								grammar->rules_by_tag.find(cCohort->wordform)->second->end());
			}

			// Read in the readings	
			while(inchar != '$') {
				inchar = u_fgetc(input);

	 			if(inchar == '$') { // Reached the end of the cohort
	
					// Add the final reading of the cohort
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;

					if (grammar->sets_by_tag.find(grammar->tag_any) != grammar->sets_by_tag.end()) {
						cReading->possible_sets.insert(grammar->sets_by_tag.find(grammar->tag_any)->second->begin(), 
										 grammar->sets_by_tag.find(grammar->tag_any)->second->end());
					}

					addTagToReading(cReading, cReading->wordform);
					processReading(cSWindow, cReading, current_reading);

					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;

					delete[] current_reading;
					current_reading = 0;
					wordform = 0;
				}

				if(inchar == '/') { // Reached end of reading
	
					Reading *cReading = 0;
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;

					addTagToReading(cReading, cReading->wordform);

					processReading(cSWindow, cReading, current_reading);

					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
	
					delete[] current_reading;
					current_reading = 0;
					continue;
				}
	
				current_reading = ux_append(current_reading, inchar);
			}


			// If the current cohort is looking ok, and we have an available
			// window, add the cohort to the window.
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
				if (cCohort->readings.empty()) {
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					if (grammar->sets_by_tag.find(grammar->tag_any) != grammar->sets_by_tag.end()) {
						cReading->possible_sets.insert(grammar->sets_by_tag.find(grammar->tag_any)->second->begin(), 
										grammar->sets_by_tag.find(grammar->tag_any)->second->end());
					}
					addTagToReading(cReading, cCohort->wordform);
					cReading->noprint = true;
					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
				}
			}

			lCohort = cCohort;
			lReading = 0;
			numCohorts++;
			continue;
		}
	} // End of input.

	if (cCohort && cSWindow) {
		//cSWindow->appendCohort(cCohort);

		// Create magic reading
		if (cCohort->readings.empty()) {
			cReading = r->new_Reading(cCohort);
			cReading->wordform = cCohort->wordform;
			cReading->baseform = cCohort->wordform;
			if (grammar->sets_by_tag.find(grammar->tag_any) != grammar->sets_by_tag.end()) {
				cReading->possible_sets.insert(grammar->sets_by_tag.find(grammar->tag_any)->second->begin(), 
								grammar->sets_by_tag.find(grammar->tag_any)->second->end());
			}
			addTagToReading(cReading, cCohort->wordform);
			cReading->noprint = true;
			cCohort->appendReading(cReading);
		}
		std::list<Reading*>::iterator iter;
		for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
			addTagToReading(*iter, endtag);
		}
		cWindow->appendSingleWindow(cSWindow);
		cReading = 0;
		cCohort = 0;
		cSWindow = 0;
	}

	// Run the grammar
	while (!cWindow->next.empty()) {
		while (!cWindow->previous.empty() && (cWindow->previous.size() > num_windows)) {
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

	// Print out the result
	cWindow->shuffleWindowsDown();
	while (!cWindow->previous.empty()) {
		SingleWindow *tmp = cWindow->previous.front();
		printSingleWindow(tmp, output);
		delete tmp;
		cWindow->previous.pop_front();
	}

	if((inchar) && inchar != 0xffff) {
		u_fprintf(output, "%C", inchar);
	}
	u_fflush(input);
	u_fflush(ux_stderr);
	u_fflush(output);

	return 0;
}

/*
 * Parse an Apertium reading into a CG Reading
 *
 * Example:
 *   venir<vblex><imp><p2><sg>
 */
void
ApertiumApplicator::processReading(SingleWindow *cSWindow, Reading *cReading, UChar *reading_string)
{
	UChar *c = reading_string;
	UChar *tmptag = 0;
	UChar *base = 0;
	bool unknown = false;

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
	base = ux_append(base, '"');

	uint32_t tag = addTag(base)->hash;
	cReading->baseform = tag;
	addTagToReading(cReading, tag);

	if(unknown) {
		return;
	}

	if (grammar->rules_by_tag.find(tag) != grammar->rules_by_tag.end()) {
		cSWindow->valid_rules.insert(grammar->rules_by_tag.find(tag)->second->begin(), 
						grammar->rules_by_tag.find(tag)->second->end());
	}

	// Now read in the tags
	while(*c != '\0') {
		if(*c == '\0') {
			return;
		}

		if(*c == '<') {
			c++;
			continue;
		} else if(*c == '>') {
			uint32_t tag = addTag(tmptag)->hash;
			addTagToReading(cReading, tag); // Add the baseform to the tag

			if (grammar->rules_by_tag.find(tag) != grammar->rules_by_tag.end()) {
				cSWindow->valid_rules.insert(grammar->rules_by_tag.find(tag)->second->begin(), 
								grammar->rules_by_tag.find(tag)->second->end());
			}

			/*
			if(!cReading->tags_mapped->empty()) {
				cReading->mapped = true;
			}
			//*/

			delete[] tmptag;
			tmptag = 0;
			c++;
			continue;
		}
		tmptag = ux_append(tmptag, *c);
		c++;
	}

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

	stdext::hash_map<uint32_t, uint32_t> used_tags;
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
			u_fprintf(output, "<");
			Tag::printTagRaw(output, tag);
			u_fprintf(output, ">");
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
		if (cohort->text) {
			u_fprintf(output, "%S", cohort->text);
		}
	}
}
