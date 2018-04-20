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

#include "FSTApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

FSTApplicator::FSTApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
  , wfactor(1.0)
{
	wtag += 'W';
	sub_delims += '#';
	//sub_delims += '+';
}

void FSTApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
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
	UString wtag_buf;
	Tag* wtag_tag;

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
					UChar space = (line[i] == '\t' ? '\t' : ' ');
					while (ISSPACE(line[i]) && !ISNL(line[i])) {
						if (line[i] == '\t') {
							space = line[i];
						}
						++i;
					}
					cleaned[packoff++] = space;
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
			cleaned.resize(line.size() + 1, 0);
		}

	gotaline:
		// Trim trailing whitespace
		while (cleaned[0] && ISSPACE(cleaned[packoff - 1])) {
			cleaned[packoff - 1] = 0;
			--packoff;
		}
		if (!ignoreinput && cleaned[0]) {
			UChar* space = &cleaned[0];
			SKIPTO_NOSPAN_RAW(space, '\t');

			if (space[0] != '\t') {
				// If this line looks like markup, don't warn about it
				if (cleaned[0] != '<') {
					u_fprintf(ux_stderr, "Warning: %S on line %u looked like a cohort but wasn't - treated as text.\n", &cleaned[0], numLines);
					u_fflush(ux_stderr);
				}
				goto istext;
			}
			space[0] = 0;

			UString tag;
			tag += '"';
			tag += '<';
			tag += &cleaned[0];
			tag += '>';
			tag += '"';

			if (!cCohort) {
				if (!cSWindow) {
					// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
					cSWindow = gWindow->allocAppendSingleWindow();
					initEmptySingleWindow(cSWindow);

					lSWindow = cSWindow;
					++numWindows;
					did_soft_lookback = false;
				}
				cCohort = alloc_cohort(cSWindow);
				cCohort->global_number = gWindow->cohort_counter++;
				cCohort->wordform = addTag(tag);
				lCohort = cCohort;
				++numCohorts;
			}

			++space;
			while (space && *space && (space[0] != '+' || space[1] != '?' || space[2] != 0)) {
				UChar* tab = u_strchr(space, '\t');
				// FSTs sometimes output the input twice for non-matches, which turned into a weight of 0
				if (tab && tab[1] == '+' && tab[2] == '?') {
					break;
				}

				cReading = alloc_reading(cCohort);
				insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
				addTagToReading(*cReading, cCohort->wordform);

				constexpr UChar notag[] = { '_', 0 };
				const UChar* base = space;
				TagList mappings;

				wtag_tag = 0;
				double weight = 0.0;
				if (tab) {
					tab[0] = 0;
					++tab;
					UChar* comma = u_strchr(tab, ',');
					if (comma) {
						comma[0] = '.';
					}
					char buf[32];
					size_t i = 0;
					for (; i < 31 && tab[i]; ++i) {
						buf[i] = static_cast<char>(tab[i]);
					}
					buf[i] = 0;
					if (strcmp(buf, "inf") == 0) {
						i = sprintf(buf, "%f", NUMERIC_MAX);
					}
					else {
						weight = strtof(buf, 0);
						weight *= wfactor;
						i = sprintf(buf, "%f", weight);
					}
					wtag_buf.clear();
					wtag_buf.reserve(wtag.size() + i + 3);
					wtag_buf += '<';
					wtag_buf += wtag;
					wtag_buf += ':';
					std::copy(buf, buf + i, std::back_inserter(wtag_buf));
					wtag_buf += '>';
					wtag_tag = addTag(wtag_buf);
				}

				// Initial baseform, because it may end on +
				UChar* plus = u_strchr(space, '+');
				if (plus) {
					++plus;
					constexpr UChar cplus[] = { '+', 0 };
					int32_t p = u_strspn(plus, cplus);
					space = plus + p;
					--space;
				}

				while (space && *space && (space = u_strchr(space, '+')) != 0) {
					if (base && base[0]) {
						int32_t f = u_strcspn(base, sub_delims.c_str());
						UChar* hash = 0;
						if (f && base + f < space) {
							hash = const_cast<UChar*>(base) + f;
							size_t oh = hash - &cleaned[0];
							size_t ob = base - &cleaned[0];
							cleaned.resize(cleaned.size() + 1, 0);
							hash = &cleaned[oh];
							base = &cleaned[ob];
							std::copy_backward(hash, &cleaned[cleaned.size() - 2], &cleaned[cleaned.size() - 1]);
							hash[0] = 0;
							space = hash;
						}
						space[0] = 0;
						if (cReading->baseform == 0) {
							tag.clear();
							tag += '"';
							tag += base;
							tag += '"';
							base = tag.c_str();
						}
						if (base[0] == 0) {
							base = notag;
							u_fprintf(ux_stderr, "Warning: Line %u had empty tag.\n", numLines);
							u_fflush(ux_stderr);
						}
						Tag* tag = addTag(base);
						if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
							mappings.push_back(tag);
						}
						else {
							addTagToReading(*cReading, tag);
						}
						if (hash && hash[0] == 0) {
							if (wtag_tag) {
								addTagToReading(*cReading, wtag_tag);
							}
							Reading* nr = cReading->allocateReading(cReading->parent);
							nr->next = cReading;
							cReading = nr;
							++space;
						}
					}
					base = ++space;
				}
				if (base && base[0]) {
					if (cReading->baseform == 0) {
						tag.clear();
						tag += '"';
						tag += base;
						tag += '"';
						base = tag.c_str();
					}
					Tag* tag = addTag(base);
					if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
						mappings.push_back(tag);
					}
					else {
						addTagToReading(*cReading, tag);
					}
				}
				if (wtag_tag) {
					addTagToReading(*cReading, wtag_tag);
				}
				if (!cReading->baseform) {
					cReading->baseform = cCohort->wordform->hash;
					u_fprintf(ux_stderr, "Warning: Line %u had no valid baseform.\n", numLines);
					u_fflush(ux_stderr);
				}
				if (single_tags[cReading->baseform]->tag.size() == 2) {
					delTagFromReading(*cReading, cReading->baseform);
					cReading->baseform = makeBaseFromWord(cCohort->wordform->hash)->hash;
				}
				if (!mappings.empty()) {
					splitMappings(mappings, *cCohort, *cReading, true);
				}
				if (grammar->sub_readings_ltr && cReading->next) {
					cReading = reverse(cReading);
				}
				cCohort->appendReading(cReading);
				++numReadings;
			}
		}
		else {
		istext:
			if (cCohort && cCohort->readings.empty()) {
				initEmptyCohort(*cCohort);
			}
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
				lCohort = cCohort;
				cSWindow = 0;
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
				lCohort = cCohort;
				cSWindow = 0;
				did_soft_lookback = false;
			}
			if (!cSWindow) {
				// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = gWindow->allocAppendSingleWindow();
				initEmptySingleWindow(cSWindow);

				lSWindow = cSWindow;
				lCohort = cSWindow->cohorts[0];
				cCohort = 0;
				++numWindows;
				did_soft_lookback = false;
			}
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
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

			cCohort = 0;

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
}
