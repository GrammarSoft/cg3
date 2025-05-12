/*
* Copyright (C) 2007-2025, GrammarSoft ApS
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
{
}

void FSTApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
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

	UString line(1024, 0);
	UString cleaned(line.size(), 0);
	bool ignoreinput = false;
	bool did_soft_lookback = false;
	UString wtag_buf;
	Tag* wtag_tag;

	index();

	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);
	uint32_t lines = 0;

	SingleWindow* cSWindow = nullptr;
	Cohort* cCohort = nullptr;
	Reading* cReading = nullptr;

	SingleWindow* lSWindow = nullptr;
	Cohort* lCohort = nullptr;

	gWindow->window_span = num_windows;

	ux_stripBOM(input);

	while (!input.eof()) {
		++lines;
		auto packoff = get_line_clean(line, cleaned, input, true);

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
			tag.append(u"\"<");
			tag += &cleaned[0];
			tag.append(u">\"");

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

				wtag_tag = nullptr;
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
						int32_t f = u_strcspn(base, sub_delims.data());
						UChar* hash = nullptr;
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
							base = tag.data();
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
						base = tag.data();
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
				if (grammar->single_tags[cReading->baseform]->tag.size() == 2) {
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
			if (is_conv) {
				if (cCohort) {
					cCohort->local_number = 1;
					printCohort(cCohort, output);
					free_cohort(cCohort);
					cCohort = nullptr;
				}
				if (cleaned[0] && line[0]) {
					printPlainTextLine(&line[0], output);
				}
				continue;
			}
			if (cSWindow && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && !did_soft_lookback) {
				did_soft_lookback = true;
				for (auto c : reversed(cSWindow->cohorts)) {
					if (doesSetMatchCohortNormal(*c, grammar->soft_delimiters->number)) {
						did_soft_lookback = false;
						Cohort* cohort = delimitAt(*cSWindow, c);
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
				cSWindow = nullptr;
				did_soft_lookback = false;
			}
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (!dep_delimit && grammar->delimiters && doesSetMatchCohortNormal(*cCohort, grammar->delimiters->number)))) {
				if (!is_conv && cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at cohort %S (#%u) on line %u - forcing break.\n", hard_limit, cCohort->wordform->tag.data(), numCohorts, numLines);
					u_fflush(ux_stderr);
				}
				for (auto iter : cCohort->readings) {
					addTagToReading(*iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = nullptr;
				did_soft_lookback = false;
			}
			if (!cSWindow) {
				// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = gWindow->allocAppendSingleWindow();
				initEmptySingleWindow(cSWindow);

				lSWindow = cSWindow;
				lCohort = cSWindow->cohorts[0];
				cCohort = nullptr;
				++numWindows;
				did_soft_lookback = false;
			}
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
			}
			if (gWindow->next.size() > num_windows) {
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

			cCohort = nullptr;

			if (cleaned[0] && line[0]) {
				if (lCohort) {
					lCohort->text += &line[0];
				}
				else if (lSWindow) {
					lSWindow->text += &line[0];
				}
				else {
					printPlainTextLine(&line[0], output);
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
		cReading = nullptr;
		cCohort = nullptr;
		cSWindow = nullptr;
	}
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

	u_fflush(output);
}
void FSTApplicator::printReading(const Reading* reading, std::ostream& output) {
	if (reading->noprint) {
		return;
	}
	if (reading->deleted) {
		return;
	}

	if (reading->next) {
		printReading(reading->next, output);
		u_fprintf(output, "%S", sub_delims.data());
	}

	if (reading->baseform) {
		auto& tag = grammar->single_tags.find(reading->baseform)->second->tag;
		u_fprintf(output, "%.*S", tag.size() - 2, tag.data() + 1);
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
		const Tag* tag = grammar->single_tags[tter];
		if (tag->type & T_DEPENDENCY && has_dep && !dep_original) {
			continue;
		}
		if (tag->type & T_RELATION && has_relations) {
			continue;
		}
		u_fprintf(output, "+%S", tag->tag.data());
	}
}

void FSTApplicator::printCohort(Cohort* cohort, std::ostream& output, bool profiling) {
	if (cohort->local_number == 0) {
		goto removed;
	}
	if (cohort->type & CT_REMOVED) {
		goto removed;
	}

	if (!cohort->wblank.empty()) {
		u_fprintf(output, "%S", cohort->wblank.data());
		if (!ISNL(cohort->wblank.back())) {
			u_fputc('\n', output);
		}
	}

	if (cohort->wread && !did_warn_statictags) {
		u_fprintf(ux_stderr, "Warning: FST CG format cannot output static tags! You are losing information!\n");
		u_fflush(ux_stderr);
		did_warn_statictags = true;
	}

	if (!profiling) {
		cohort->unignoreAll();

		if (!split_mappings) {
			mergeMappings(*cohort);
		}
	}

	{
		auto& wform = cohort->wordform->tag;
		if (cohort->readings.empty() || (cohort->readings.size() == 1 && cohort->readings[0]->noprint)) {
			u_fprintf(output, "%.*S\t+?\n", wform.size() - 4, wform.data() + 2);
		}
		else {
			for (auto rter : cohort->readings) {
				u_fprintf(output, "%.*S\t", wform.size() - 4, wform.data() + 2);
				printReading(rter, output);
				u_fputc('\n', output);
			}
		}
		u_fputc('\n', output);
	}

removed:
	if (!cohort->text.empty() && cohort->text.find_first_not_of(ws) != UString::npos) {
		u_fprintf(output, "%S", cohort->text.data());
		if (!ISNL(cohort->text.back())) {
			u_fputc('\n', output);
		}
	}
}

void FSTApplicator::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.data());
		if (!ISNL(window->text.back())) {
			u_fputc('\n', output);
		}
	}

	for (auto& cohort : window->all_cohorts) {
		printCohort(cohort, output, profiling);
	}

	if (!window->text_post.empty()) {
		u_fprintf(output, "%S", window->text_post.data());
		if (!ISNL(window->text_post.back())) {
			u_fputc('\n', output);
		}
	}

	u_fputc('\n', output);
	u_fflush(output);
}
}
