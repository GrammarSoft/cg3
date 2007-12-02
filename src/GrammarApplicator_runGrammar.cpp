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

#include "GrammarApplicator.h"

using namespace CG3;
using namespace CG3::Strings;

int GrammarApplicator::runGrammarOnText(UFILE *input, UFILE *output) {
	if (!input) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		return -1;
	}
	u_frewind(input);
	if (u_feof(input)) {
		u_fprintf(ux_stderr, "Error: Input is empty - nothing to parse!\n");
		return -1;
	}
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue! Hint: call setGrammar() first.\n");
		return -1;
	}

	if (!grammar->delimiters || (grammar->delimiters->sets.empty() && grammar->delimiters->tags_set.empty())) {
		if (!grammar->soft_delimiters || (grammar->soft_delimiters->sets.empty() && grammar->soft_delimiters->tags_set.empty())) {
			u_fprintf(ux_stderr, "Warning: No soft or hard delimiters defined in grammar. Hard limit of %u cohorts may break windows in unintended places.\n", hard_limit);
		}
		else {
			u_fprintf(ux_stderr, "Warning: No hard delimiters defined in grammar. Soft limit of %u cohorts may break windows in unintended places.\n", soft_limit);
		}
	}

#define BUFFER_SIZE (131072)
	UChar _line[BUFFER_SIZE];
	UChar *line = _line;
	UChar _cleaned[BUFFER_SIZE];
	UChar *cleaned = _cleaned;
	bool ignoreinput = false;

	Recycler *r = Recycler::instance();
	uint32_t resetAfter = ((num_windows+4)*2+1);

	begintag = addTag(stringbits[S_BEGINTAG]);
	endtag = addTag(stringbits[S_ENDTAG]);

	gWindow = new Window(this);

	Window *cWindow = gWindow;
	SingleWindow *cSWindow = 0;
	Cohort *cCohort = 0;
	Reading *cReading = 0;

	SingleWindow *lSWindow = 0;
	Cohort *lCohort = 0;
	Reading *lReading = 0;

	cWindow->window_span = num_windows;

	while (!u_feof(input)) {
		u_fgets(line, BUFFER_SIZE-1, input);
		u_strcpy(cleaned, line);
		ux_packWhitespace(cleaned);

		if (!ignoreinput && cleaned[0] == '"' && cleaned[1] == '<') {
			ux_trim(cleaned);
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesTagMatchSet(cCohort->wordform, grammar->soft_delimiters->hash)) {
				if (cSWindow->cohorts.size() >= soft_limit) {
					u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter.\n", soft_limit, numLines);
				}
				if (cCohort->readings.empty()) {
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags.insert(cCohort->wordform);
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					reflowReading(cReading);
					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
				}
				std::list<Reading*>::iterator iter;
				for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
					(*iter)->tags_list.push_back(endtag);
					(*iter)->tags.insert(endtag);
					(*iter)->rehash();
					uint32List::iterator iter_tags;
					for (iter_tags = (*iter)->tags_list.begin() ; iter_tags != (*iter)->tags_list.end() ; iter_tags++) {
						if (grammar->rules_by_tag.find(*iter_tags) != grammar->rules_by_tag.end()) {
							cSWindow->valid_rules.insert(grammar->rules_by_tag.find(*iter_tags)->second->begin(), grammar->rules_by_tag.find(*iter_tags)->second->end());
						}
					}
				}

				cSWindow->appendCohort(cCohort);
				if (grammar->rules_by_tag.find(grammar->tag_any) != grammar->rules_by_tag.end()) {
					cSWindow->valid_rules.insert(grammar->rules_by_tag.find(grammar->tag_any)->second->begin(), grammar->rules_by_tag.find(grammar->tag_any)->second->end());
				}
				if (grammar->rules_by_tag.find(begintag) != grammar->rules_by_tag.end()) {
					cSWindow->valid_rules.insert(grammar->rules_by_tag.find(begintag)->second->begin(), grammar->rules_by_tag.find(begintag)->second->end());
				}
				if (grammar->rules_by_tag.find(endtag) != grammar->rules_by_tag.end()) {
					cSWindow->valid_rules.insert(grammar->rules_by_tag.find(endtag)->second->begin(), grammar->rules_by_tag.find(endtag)->second->end());
				}
				cWindow->appendSingleWindow(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			}
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || doesTagMatchSet(cCohort->wordform, grammar->delimiters->hash))) {
				if (cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at line %u - forcing break.\n", hard_limit, numLines);
				}
				if (cCohort->readings.empty()) {
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags.insert(cCohort->wordform);
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					reflowReading(cReading);
					cCohort->appendReading(cReading);
					lReading = cReading;
					numReadings++;
				}
				std::list<Reading*>::iterator iter;
				for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
					(*iter)->tags_list.push_back(endtag);
					(*iter)->tags.insert(endtag);
					(*iter)->rehash();
					uint32List::iterator iter_tags;
					for (iter_tags = (*iter)->tags_list.begin() ; iter_tags != (*iter)->tags_list.end() ; iter_tags++) {
						if (grammar->rules_by_tag.find(*iter_tags) != grammar->rules_by_tag.end()) {
							cSWindow->valid_rules.insert(grammar->rules_by_tag.find(*iter_tags)->second->begin(), grammar->rules_by_tag.find(*iter_tags)->second->end());
						}
					}
				}

				cSWindow->appendCohort(cCohort);
				if (grammar->rules_by_tag.find(grammar->tag_any) != grammar->rules_by_tag.end()) {
					cSWindow->valid_rules.insert(grammar->rules_by_tag.find(grammar->tag_any)->second->begin(), grammar->rules_by_tag.find(grammar->tag_any)->second->end());
				}
				if (grammar->rules_by_tag.find(begintag) != grammar->rules_by_tag.end()) {
					cSWindow->valid_rules.insert(grammar->rules_by_tag.find(begintag)->second->begin(), grammar->rules_by_tag.find(begintag)->second->end());
				}
				if (grammar->rules_by_tag.find(endtag) != grammar->rules_by_tag.end()) {
					cSWindow->valid_rules.insert(grammar->rules_by_tag.find(endtag)->second->begin(), grammar->rules_by_tag.find(endtag)->second->end());
				}
				cWindow->appendSingleWindow(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			}
			if (!cSWindow) {
				// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = new SingleWindow(cWindow);

				cCohort = r->new_Cohort(cSWindow);
				cCohort->global_number = 0;
				cCohort->wordform = begintag;

				cReading = r->new_Reading(cCohort);
				cReading->baseform = begintag;
				cReading->wordform = begintag;
				cReading->tags.insert(begintag);
				cReading->tags_list.push_back(begintag);
				cReading->rehash();

				cCohort->appendReading(cReading);

				cSWindow->appendCohort(cCohort);

				lSWindow = cSWindow;
				lReading = cReading;
				lCohort = cCohort;
				cCohort = 0;
				numWindows++;
			}
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
				if (cCohort->readings.empty()) {
					cReading = r->new_Reading(cCohort);
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags.insert(cCohort->wordform);
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					reflowReading(cReading);
					uint32List::iterator iter_tags;
					for (iter_tags = cReading->tags_list.begin() ; iter_tags != cReading->tags_list.end() ; iter_tags++) {
						if (grammar->rules_by_tag.find(*iter_tags) != grammar->rules_by_tag.end()) {
							cSWindow->valid_rules.insert(grammar->rules_by_tag.find(*iter_tags)->second->begin(), grammar->rules_by_tag.find(*iter_tags)->second->end());
						}
					}
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
				}
				/*
				u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
				u_fflush(ux_stderr);
				//*/
			}
			cCohort = r->new_Cohort(cSWindow);
			cCohort->global_number = cWindow->cohort_counter++;
			cCohort->wordform = addTag(cleaned);
			lCohort = cCohort;
			lReading = 0;
			numCohorts++;
		}
		else if (cleaned[0] == ' ' && cleaned[1] == '"' && cCohort) {
			cReading = r->new_Reading(cCohort);
			cReading->wordform = cCohort->wordform;
			cReading->tags.insert(cReading->wordform);
			cReading->tags_list.push_back(cReading->wordform);
			if (grammar->rules_by_tag.find(cReading->wordform) != grammar->rules_by_tag.end()) {
				cSWindow->valid_rules.insert(grammar->rules_by_tag.find(cReading->wordform)->second->begin(), grammar->rules_by_tag.find(cReading->wordform)->second->end());
			}

			ux_trim(cleaned);
			UChar *space = cleaned;
			UChar *base = space;

			while (space && (space = u_strchr(space, ' ')) != 0) {
				space[0] = 0;
				space++;
				if (u_strlen(base)) {
					uint32_t tag = addTag(base);
					if (!cReading->baseform && single_tags[tag]->type & T_BASEFORM) {
						cReading->baseform = tag;
					}
					cReading->tags_list.push_back(tag);
					if (grammar->rules_by_tag.find(tag) != grammar->rules_by_tag.end()) {
						cSWindow->valid_rules.insert(grammar->rules_by_tag.find(tag)->second->begin(), grammar->rules_by_tag.find(tag)->second->end());
					}
				}
				base = space;
			}
			if (u_strlen(base)) {
				uint32_t tag = addTag(base);
				if (!cReading->baseform && single_tags[tag]->type & T_BASEFORM) {
					cReading->baseform = tag;
				}
				cReading->tags_list.push_back(tag);
				if (grammar->rules_by_tag.find(tag) != grammar->rules_by_tag.end()) {
					cSWindow->valid_rules.insert(grammar->rules_by_tag.find(tag)->second->begin(), grammar->rules_by_tag.find(tag)->second->end());
				}
			}
			reflowReading(cReading);
			if (!cReading->tags_mapped->empty()) {
				cReading->mapped = true;
			}
			if (!cReading->baseform) {
				u_fprintf(ux_stderr, "Warning: Reading on line %u had no valid baseform.\n", numLines);
				u_fflush(ux_stderr);
			}
			cCohort->appendReading(cReading);
			lReading = cReading;
			numReadings++;
		}
		else {
			if (!ignoreinput && cleaned[0] == ' ' && cleaned[1] == '"') {
				u_fprintf(ux_stderr, "Warning: Line %u looked like a reading but there was no containing cohort - treated as plain text.\n", numLines);
			}
			ux_trim(cleaned);
			if (u_strlen(cleaned) > 0) {
				if (u_strcmp(cleaned, stringbits[S_CMD_FLUSH]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:FLUSH encountered on line %u. Flushing...\n", numLines);
					if (cCohort && cSWindow) {
						cSWindow->appendCohort(cCohort);
						if (cCohort->readings.empty()) {
							cReading = r->new_Reading(cCohort);
							cReading->wordform = cCohort->wordform;
							cReading->baseform = cCohort->wordform;
							cReading->tags.insert(cCohort->wordform);
							cReading->tags_list.push_back(cCohort->wordform);
							cReading->noprint = true;
							cReading->rehash();
							cCohort->appendReading(cReading);
						}
						std::list<Reading*>::iterator iter;
						for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
							(*iter)->tags_list.push_back(endtag);
							(*iter)->tags.insert(endtag);
							(*iter)->rehash();
							uint32List::iterator iter_tags;
							for (iter_tags = (*iter)->tags_list.begin() ; iter_tags != (*iter)->tags_list.end() ; iter_tags++) {
								if (grammar->rules_by_tag.find(*iter_tags) != grammar->rules_by_tag.end()) {
									cSWindow->valid_rules.insert(grammar->rules_by_tag.find(*iter_tags)->second->begin(), grammar->rules_by_tag.find(*iter_tags)->second->end());
								}
							}
						}
						if (grammar->rules_by_tag.find(grammar->tag_any) != grammar->rules_by_tag.end()) {
							cSWindow->valid_rules.insert(grammar->rules_by_tag.find(grammar->tag_any)->second->begin(), grammar->rules_by_tag.find(grammar->tag_any)->second->end());
						}
						if (grammar->rules_by_tag.find(begintag) != grammar->rules_by_tag.end()) {
							cSWindow->valid_rules.insert(grammar->rules_by_tag.find(begintag)->second->begin(), grammar->rules_by_tag.find(begintag)->second->end());
						}
						if (grammar->rules_by_tag.find(endtag) != grammar->rules_by_tag.end()) {
							cSWindow->valid_rules.insert(grammar->rules_by_tag.find(endtag)->second->begin(), grammar->rules_by_tag.find(endtag)->second->end());
						}
						cWindow->appendSingleWindow(cSWindow);
						cReading = lReading = 0;
						cCohort = lCohort = 0;
						cSWindow = lSWindow = 0;
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
						if (numWindows % resetAfter == 0) {
							resetIndexes();
						}
						/*
						u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
						u_fflush(ux_stderr);
						//*/
					}
					cWindow->shuffleWindowsDown();
					while (!cWindow->previous.empty()) {
						SingleWindow *tmp = cWindow->previous.front();
						printSingleWindow(tmp, output);
						delete tmp;
						cWindow->previous.pop_front();
					}
					u_fflush(output);
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_IGNORE]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:IGNORE encountered on line %u. Passing through all input...\n", numLines);
					ignoreinput = true;
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_RESUME]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:RESUME encountered on line %u. Resuming CG...\n", numLines);
					ignoreinput = false;
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_EXIT]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:EXIT encountered on line %u. Exiting...\n", numLines);
					u_fprintf(output, "%S", line);
					goto CGCMD_EXIT;
				}
				
				if (lReading) {
					lReading->text = ux_append(lReading->text, line);
				}
				else if (lCohort) {
					lCohort->text = ux_append(lCohort->text, line);
				}
				else if (lSWindow) {
					lSWindow->text = ux_append(lSWindow->text, line);
				}
				else {
					u_fprintf(output, "%S", line);
				}
			}
		}
		numLines++;
	}

	if (cCohort && cSWindow) {
		cSWindow->appendCohort(cCohort);
		if (cCohort->readings.empty()) {
			cReading = r->new_Reading(cCohort);
			cReading->wordform = cCohort->wordform;
			cReading->baseform = cCohort->wordform;
			cReading->tags.insert(cCohort->wordform);
			cReading->tags_list.push_back(cCohort->wordform);
			cReading->noprint = true;
			cReading->rehash();
			cCohort->appendReading(cReading);
		}
		std::list<Reading*>::iterator iter;
		for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
			(*iter)->tags_list.push_back(endtag);
			(*iter)->tags.insert(endtag);
			(*iter)->rehash();
			uint32List::iterator iter_tags;
			for (iter_tags = (*iter)->tags_list.begin() ; iter_tags != (*iter)->tags_list.end() ; iter_tags++) {
				if (grammar->rules_by_tag.find(*iter_tags) != grammar->rules_by_tag.end()) {
					cSWindow->valid_rules.insert(grammar->rules_by_tag.find(*iter_tags)->second->begin(), grammar->rules_by_tag.find(*iter_tags)->second->end());
				}
			}
		}
		if (grammar->rules_by_tag.find(grammar->tag_any) != grammar->rules_by_tag.end()) {
			cSWindow->valid_rules.insert(grammar->rules_by_tag.find(grammar->tag_any)->second->begin(), grammar->rules_by_tag.find(grammar->tag_any)->second->end());
		}
		if (grammar->rules_by_tag.find(begintag) != grammar->rules_by_tag.end()) {
			cSWindow->valid_rules.insert(grammar->rules_by_tag.find(begintag)->second->begin(), grammar->rules_by_tag.find(begintag)->second->end());
		}
		if (grammar->rules_by_tag.find(endtag) != grammar->rules_by_tag.end()) {
			cSWindow->valid_rules.insert(grammar->rules_by_tag.find(endtag)->second->begin(), grammar->rules_by_tag.find(endtag)->second->end());
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
		/*
		u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
		u_fflush(ux_stderr);
		//*/
	}

	cWindow->shuffleWindowsDown();
	while (!cWindow->previous.empty()) {
		SingleWindow *tmp = cWindow->previous.front();
		printSingleWindow(tmp, output);
		delete tmp;
		cWindow->previous.pop_front();
	}

	u_fflush(output);

CGCMD_EXIT:
	//std::cerr << "Skipped " << skipped_rules << " of " << (skipped_rules+cache_hits+cache_miss) <<" rules." << std::endl;
	//std::cerr << "Match " << (match_sub+match_comp+match_single) << " : " << match_sub << " / " << match_comp << " / " << match_single << std::endl;
	u_fprintf(ux_stderr, "Cache %u total, %u hits, %u misses.\n", (cache_hits+cache_miss), cache_hits, cache_miss);
	u_fprintf(ux_stderr, "Did %u lines, %u windows, %u cohorts, %u readings.\n", numLines, numWindows, numCohorts, numReadings);
	u_fflush(ux_stderr);

	return 0;
}
