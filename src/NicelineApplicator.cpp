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

#include "NicelineApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

NicelineApplicator::NicelineApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
  , did_warn_statictags(false)
  , did_warn_subreadings(false)
{
}

void NicelineApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
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

	std::vector<UChar> line(1024, 0);
	std::vector<UChar> cleaned(line.size(), 0);
	bool ignoreinput = false;
	bool did_soft_lookback = false;

	index();

	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);
	uint32_t lines = 0;

	SingleWindow* cSWindow = 0;
	Cohort* cCohort = 0;
	Reading* cReading = 0;

	SingleWindow* lSWindow = 0;
	Cohort* lCohort = 0;

	gWindow->window_span = num_windows;

	ux_stripBOM(input);

	while (!input.eof()) {
		++lines;
		size_t offset = 0, packoff = 0;
		// Read as much of the next line as will fit in the current buffer
		while (u_fgets(&line[offset], line.size() - offset - 1, input)) {
			// Copy the segment just read to cleaned
			for (size_t i = offset; i < line.size(); ++i) {
				// Only copy one space character, regardless of how many are in input
				if (ISSPACE(line[i]) && !ISNL(line[i])) {
					cleaned[packoff++] = (line[i] == '\t' ? '\t' : ' ');
					while (ISSPACE(line[i]) && !ISNL(line[i])) {
						if (line[i] == '\t') {
							cleaned[packoff - 1] = line[i];
						}
						++i;
					}
				}
				// Break if there is a newline
				if (ISNL(line[i])) {
					cleaned[packoff + 1] = cleaned[packoff] = 0;
					goto gotaline; // Oh how I wish C++ had break 2;
				}
				if (line[i] == 0) {
					cleaned[packoff + 1] = cleaned[packoff] = 0;
					break;
				}
				cleaned[packoff++] = line[i];
			}
			// If we reached this, buffer wasn't big enough. Double the size of the buffer and try again.
			offset = line.size() - 2;
			line.resize(line.size() * 2, 0);
			cleaned.resize(line.size() + 2, 0);
		}

	gotaline:
		// Trim trailing whitespace
		while (cleaned[0] && ISSPACE(cleaned[packoff - 1])) {
			cleaned[packoff - 1] = 0;
			--packoff;
		}
		if (!ignoreinput && cleaned[0] && cleaned[0] != '<') {
			UChar* space = &cleaned[0];
			SKIPTO_NOSPAN(space, '\t');

			if (space[0] && space[0] != '\t') {
				u_fprintf(ux_stderr, "Warning: %S on line %u looked like a cohort but wasn't - treated as text.\n", &cleaned[0], numLines);
				u_fflush(ux_stderr);
				goto istext;
			}
			if (!space[0]) {
				space[1] = 0;
			}
			space[0] = 0;

			if (cSWindow && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && !did_soft_lookback) {
				did_soft_lookback = true;
				reverse_foreach (iter, cSWindow->cohorts) {
					if (doesSetMatchCohortNormal(**iter, grammar->soft_delimiters->number)) {
						did_soft_lookback = false;
						Cohort* cohort = delimitAt(*cSWindow, *iter);
						cSWindow = cohort->parent->next;
						if (cCohort) {
							cCohort->parent = cSWindow;
						}
						if (verbosity_level > 0) {
							u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter in buffer.\n", soft_limit, numLines);
							u_fflush(ux_stderr);
						}
						break;
					}
				}
			}
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesSetMatchCohortNormal(*cCohort, grammar->soft_delimiters->number)) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter.\n", soft_limit, numLines);
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
				did_soft_lookback = false;
			}
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (!dep_delimit && grammar->delimiters && doesSetMatchCohortNormal(*cCohort, grammar->delimiters->number)))) {
				if (!is_conv && cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at line %u - forcing break.\n", hard_limit, numLines);
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
				did_soft_lookback = false;
			}
			if (!cSWindow) {
				// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = gWindow->allocAppendSingleWindow();
				initEmptySingleWindow(cSWindow);

				lSWindow = cSWindow;
				cCohort = 0;
				numWindows++;
				did_soft_lookback = false;
			}
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
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
					u_fflush(ux_stderr);
				}
			}

			UString tag;
			tag += '"';
			tag += '<';
			tag += &cleaned[0];
			tag += '>';
			tag += '"';

			cCohort = alloc_cohort(cSWindow);
			cCohort->global_number = gWindow->cohort_counter++;
			cCohort->wordform = addTag(tag);
			lCohort = cCohort;
			numCohorts++;

			++space;
			while (space && space[0]) {
				cReading = alloc_reading(cCohort);
				insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);

				UChar* base = space;
				if (*space == '"') {
					++space;
					SKIPTO_NOSPAN(space, '"');
				}
				if (*space == '[') {
					SKIPTO_NOSPAN(space, ']');
				}

				TagList mappings;

				UChar* tab = u_strchr(space, '\t');
				if (tab) {
					tab[0] = 0;
				}

				while (space && *space && (space = u_strchr(space, ' ')) != 0) {
					space[0] = 0;
					if (base && base[0]) {
						if (base[0] == '[' && space[-1] == ']') {
							base[0] = space[-1] = '"';
						}
						Tag* tag = addTag(base);
						if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
							mappings.push_back(tag);
						}
						else {
							addTagToReading(*cReading, tag);
						}
					}
					base = ++space;
					if (*space == '"') {
						++space;
						SKIPTO_NOSPAN(space, '"');
					}
					if (*space == '[') {
						SKIPTO_NOSPAN(space, ']');
					}
				}
				if (base && base[0]) {
					if (base[0] == '[' && space[-1] == ']') {
						base[0] = space[-1] = '"';
					}
					Tag* tag = addTag(base);
					if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
						mappings.push_back(tag);
					}
					else {
						addTagToReading(*cReading, tag);
					}
				}
				if (!cReading->baseform) {
					cReading->baseform = cReading->parent->wordform->hash;
					u_fprintf(ux_stderr, "Warning: Line %u had no valid baseform.\n", numLines);
					u_fflush(ux_stderr);
				}
				if (!mappings.empty()) {
					splitMappings(mappings, *cCohort, *cReading, true);
				}
				cCohort->appendReading(cReading);
				numReadings++;

				if (tab) {
					space = ++tab;
				}
			}
			if (cCohort->readings.empty()) {
				initEmptyCohort(*cCohort);
			}
		}
		else {
		istext:
			if (cleaned[0] && line[0]) {
				if (lCohort) {
					lCohort->text += &line[0];
				}
				else if (lSWindow) {
					lSWindow->text += &line[0];
				}
				else {
					u_fprintf(output, "%S", &line[0]);
				}
			}
		}
		numLines++;
		line[0] = cleaned[0] = 0;
	}

	if (cCohort && cSWindow) {
		cSWindow->appendCohort(cCohort);
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

	u_fflush(output);
}

void NicelineApplicator::printReading(const Reading* reading, std::ostream& output) {
	if (reading->noprint) {
		return;
	}
	if (reading->deleted) {
		return;
	}
	u_fputc('\t', output);
	if (reading->baseform) {
		u_fprintf(output, "[%.*S]", single_tags.find(reading->baseform)->second->tag.size() - 2, single_tags.find(reading->baseform)->second->tag.c_str() + 1);
	}

	uint32SortedVector unique;
	for (auto tter : reading->tags_list) {
		if ((!show_end_tags && tter == endtag) || tter == begintag) {
			continue;
		}
		if (tter == reading->baseform || tter == reading->parent->wordform->hash) {
			continue;
		}
		if (unique_tags) {
			if (unique.find(tter) != unique.end()) {
				continue;
			}
			unique.insert(tter);
		}
		const Tag* tag = single_tags[tter];
		if (tag->type & T_DEPENDENCY && has_dep && !dep_original) {
			continue;
		}
		if (tag->type & T_RELATION && has_relations) {
			continue;
		}
		u_fprintf(output, " %S", tag->tag.c_str());
	}

	if (has_dep && !(reading->parent->type & CT_REMOVED)) {
		if (!reading->parent->dep_self) {
			reading->parent->dep_self = reading->parent->global_number;
		}
		const Cohort* pr = 0;
		pr = reading->parent;
		if (reading->parent->dep_parent != DEP_NO_PARENT) {
			if (reading->parent->dep_parent == 0) {
				pr = reading->parent->parent->cohorts[0];
			}
			else if (reading->parent->parent->parent->cohort_map.find(reading->parent->dep_parent) != reading->parent->parent->parent->cohort_map.end()) {
				pr = reading->parent->parent->parent->cohort_map[reading->parent->dep_parent];
			}
		}

		constexpr UChar local_utf_pattern[] = { ' ', '#', '%', 'u', L'\u2192', '%', 'u', 0 };
		constexpr UChar local_latin_pattern[] = { ' ', '#', '%', 'u', '-', '>', '%', 'u', 0 };
		const UChar* pattern = local_latin_pattern;
		if (unicode_tags) {
			pattern = local_utf_pattern;
		}
		if (!dep_has_spanned) {
			u_fprintf_u(output, pattern,
			  reading->parent->local_number,
			  pr->local_number);
		}
		else {
			if (reading->parent->dep_parent == DEP_NO_PARENT) {
				u_fprintf_u(output, pattern,
				  reading->parent->dep_self,
				  reading->parent->dep_self);
			}
			else {
				u_fprintf_u(output, pattern,
				  reading->parent->dep_self,
				  reading->parent->dep_parent);
			}
		}
	}

	if (reading->parent->type & CT_RELATED) {
		u_fprintf(output, " ID:%u", reading->parent->global_number);
		if (!reading->parent->relations.empty()) {
			for (auto miter : reading->parent->relations) {
				for (auto siter : miter.second) {
					u_fprintf(output, " R:%S:%u", grammar->single_tags.find(miter.first)->second->tag.c_str(), siter);
				}
			}
		}
	}

	if (trace) {
		for (auto iter_hb : reading->hit_by) {
			u_fputc(' ', output);
			printTrace(output, iter_hb);
		}
	}

	if (reading->next && !did_warn_subreadings) {
		u_fprintf(ux_stderr, "Warning: Niceline CG format cannot output sub-readings! You are losing information!\n");
		u_fflush(ux_stderr);
		did_warn_subreadings = true;
	}
}

void NicelineApplicator::printCohort(Cohort* cohort, std::ostream& output) {
	constexpr UChar ws[] = { ' ', '\t', 0 };

	if (cohort->local_number == 0) {
		goto removed;
	}
	if (cohort->type & CT_REMOVED) {
		goto removed;
	}

	u_fprintf(output, "%.*S", cohort->wordform->tag.size() - 4, cohort->wordform->tag.c_str() + 2);
	if (cohort->wread && !did_warn_statictags) {
		u_fprintf(ux_stderr, "Warning: Niceline CG format cannot output static tags! You are losing information!\n");
		u_fflush(ux_stderr);
		did_warn_statictags = true;
	}

	if (!split_mappings) {
		mergeMappings(*cohort);
	}

	if (cohort->readings.empty()) {
		u_fputc('\t', output);
	}
	for (auto rter : cohort->readings) {
		printReading(rter, output);
	}

removed:
	u_fputc('\n', output);
	if (!cohort->text.empty() && cohort->text.find_first_not_of(ws) != UString::npos) {
		u_fprintf(output, "%S", cohort->text.c_str());
		if (!ISNL(cohort->text[cohort->text.length() - 1])) {
			u_fputc('\n', output);
		}
	}
}

void NicelineApplicator::printSingleWindow(SingleWindow* window, std::ostream& output) {
	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.c_str());
		if (!ISNL(window->text[window->text.length() - 1])) {
			u_fputc('\n', output);
		}
	}

	uint32_t cs = (uint32_t)window->cohorts.size();
	for (uint32_t c = 0; c < cs; c++) {
		Cohort* cohort = window->cohorts[c];
		printCohort(cohort, output);
	}
	u_fputc('\n', output);
	u_fflush(output);
}
}
