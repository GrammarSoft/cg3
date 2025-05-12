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
{
}

void NicelineApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
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
				cSWindow = nullptr;
				cCohort = nullptr;
				numCohorts++;
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
				cSWindow = nullptr;
				cCohort = nullptr;
				numCohorts++;
				did_soft_lookback = false;
			}
			if (!cSWindow) {
				// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = gWindow->allocAppendSingleWindow();
				initEmptySingleWindow(cSWindow);

				lSWindow = cSWindow;
				cCohort = nullptr;
				numWindows++;
				did_soft_lookback = false;
			}
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
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

			UString tag;
			tag.append(u"\"<");
			tag += &cleaned[0];
			tag.append(u">\"");

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

void NicelineApplicator::printReading(const Reading* reading, std::ostream& output) {
	if (reading->noprint) {
		return;
	}
	if (reading->deleted) {
		return;
	}
	u_fputc('\t', output);
	if (reading->baseform) {
		u_fprintf(output, "[%.*S]", grammar->single_tags.find(reading->baseform)->second->tag.size() - 2, grammar->single_tags.find(reading->baseform)->second->tag.data() + 1);
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
		u_fprintf(output, " %S", tag->tag.data());
	}

	if (has_dep && !(reading->parent->type & CT_REMOVED)) {
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

		constexpr UChar local_utf_pattern[] = { ' ', '#', '%', 'u', u'\u2192', '%', 'u', 0 };
		constexpr UChar local_latin_pattern[] = { ' ', '#', '%', 'u', '-', '>', '%', 'u', 0 };
		const UChar* pattern = local_latin_pattern;
		if (unicode_tags) {
			pattern = local_utf_pattern;
		}
		if (dep_absolute) {
			u_fprintf_u(output, pattern, reading->parent->global_number, pr->global_number);
		}
		else if (!dep_has_spanned) {
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
			for (const auto& miter : reading->parent->relations) {
				for (auto siter : miter.second) {
					u_fprintf(output, " R:%S:%u", grammar->single_tags.find(miter.first)->second->tag.data(), siter);
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

void NicelineApplicator::printCohort(Cohort* cohort, std::ostream& output, bool profiling) {
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

	u_fprintf(output, "%.*S", cohort->wordform->tag.size() - 4, cohort->wordform->tag.data() + 2);
	if (cohort->wread && !did_warn_statictags) {
		u_fprintf(ux_stderr, "Warning: Niceline CG format cannot output static tags! You are losing information!\n");
		u_fflush(ux_stderr);
		did_warn_statictags = true;
	}

	if (!profiling) {
		cohort->unignoreAll();

		if (!split_mappings) {
			mergeMappings(*cohort);
		}
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
		u_fprintf(output, "%S", cohort->text.data());
		if (!ISNL(cohort->text.back())) {
			u_fputc('\n', output);
		}
	}
}

void NicelineApplicator::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
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
