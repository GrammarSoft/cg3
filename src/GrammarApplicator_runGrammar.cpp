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

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

inline bool testStringAgainst(const UString& str, std::vector<URegularExpression*>& rxs) {
	bool rv = false;

	for (size_t i = 0; i < rxs.size(); ++i) {
		UErrorCode status = U_ZERO_ERROR;
		uregex_setText(rxs[i], str.data(), SI32(str.size()), &status);
		if (status != U_ZERO_ERROR) {
			CG3Quit(1);
		}
		status = U_ZERO_ERROR;
		if (uregex_find(rxs[i], -1, &status)) {
			rv = true;
			if (i != 0) {
				// Move regex that matched up front as it'll more likely match first next time
				std::swap(rxs[0], rxs[i]);
			}
			break;
		}
		if (status != U_ZERO_ERROR) {
			CG3Quit(1);
		}
	}

	return rv;
}

void GrammarApplicator::initEmptySingleWindow(SingleWindow* cSWindow) {
	Cohort* cCohort = alloc_cohort(cSWindow);
	cCohort->global_number = gWindow->cohort_counter++;
	cCohort->wordform = tag_begin;

	Reading* cReading = alloc_reading(cCohort);
	cReading->baseform = begintag;
	insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
	addTagToReading(*cReading, begintag);

	cCohort->appendReading(cReading);

	cSWindow->appendCohort(cCohort);
}

Reading* GrammarApplicator::initEmptyCohort(Cohort& cCohort) {
	Reading* cReading = alloc_reading(&cCohort);
	if (allow_magic_readings) {
		cReading->baseform = makeBaseFromWord(cCohort.wordform)->hash;
	}
	else {
		cReading->baseform = cCohort.wordform->hash;
	}
	insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
	addTagToReading(*cReading, cCohort.wordform);
	cReading->noprint = true;
	cCohort.appendReading(cReading);
	numReadings++;
	return cReading;
}

void GrammarApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
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
	UString cleaned(line.size() + 1, 0);
	bool ignoreinput = false;
	bool did_soft_lookback = false;
	bool is_deleted = false;
	ReadingList* readings;

	index();

	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);
	uint32_t lines = 0;

	SingleWindow* cSWindow = nullptr;
	Cohort* cCohort = nullptr;
	Reading* cReading = nullptr;

	SingleWindow* lSWindow = nullptr;
	Cohort* lCohort = nullptr;
	Reading* lReading = nullptr;

	gWindow->window_span = num_windows;

	uint32FlatHashMap variables_set;
	uint32FlatHashSet variables_rem;
	uint32SortedVector variables_output;

	std::vector<std::pair<size_t, Reading*>> indents;
	all_mappings_t all_mappings;

	ux_stripBOM(input);

	while (!input.eof()) {
		++lines;
		auto packoff = get_line_clean(line, cleaned, input);

		// Trim trailing whitespace
		while (cleaned[0] && ISSPACE(cleaned[packoff - 1])) {
			cleaned[packoff - 1] = 0;
			--packoff;
		}
		if (ignoreinput) {
			goto istext;
		}
		if (cleaned[0] == '"' && cleaned[1] == '<') {
			UChar* space = &cleaned[0];
			if (space[0] == '"' && space[1] == '<') {
				++space;
				SKIPTO_NOSPAN(space, '"');
				while (*space && space[-1] != '>') {
					++space;
					SKIPTO_NOSPAN(space, '"');
				}
				SKIPTOWS(space, 0, true, true);
				--space;
			}
			if (space[0] != '"' || space[-1] != '>') {
				u_fprintf(ux_stderr, "Warning: %S on line %u looked like a cohort but wasn't - treated as text.\n", &cleaned[0], numLines);
				u_fflush(ux_stderr);
				goto istext;
			}
			space[1] = 0;

			if (cCohort && cCohort->readings.empty()) {
				initEmptyCohort(*cCohort);
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

				splitAllMappings(all_mappings, *cCohort, true);
				cSWindow->appendCohort(cCohort);
				cCohort->line_number = numLines;
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

				splitAllMappings(all_mappings, *cCohort, true);
				cSWindow->appendCohort(cCohort);
				cCohort->line_number = numLines;
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

				cSWindow->variables_set = variables_set;
				variables_set.clear();
				cSWindow->variables_rem = variables_rem;
				variables_rem.clear();
				cSWindow->variables_output = variables_output;
				variables_output.clear();

				lSWindow = cSWindow;
				cCohort = nullptr;
				numWindows++;
				did_soft_lookback = false;
			}
			if (cCohort && cSWindow) {
				splitAllMappings(all_mappings, *cCohort, true);
				cSWindow->appendCohort(cCohort);
			}
			if (gWindow->next.size() > num_windows + 1) {
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
			cCohort = alloc_cohort(cSWindow);
			cCohort->global_number = gWindow->cohort_counter++;
			cCohort->wordform = addTag(&cleaned[0]);
			lCohort = cCohort;
			lReading = nullptr;
			indents.clear();
			numCohorts++;
			cCohort->line_number = numLines;

			space += 2;
			if (space[0]) {
				cCohort->wread = alloc_reading(cCohort);
				addTagToReading(*cCohort->wread, cCohort->wordform);
				while (space[0]) {
					SKIPWS(space, 0, 0, true);
					auto n = space;
					if (*n == '"') {
						++n;
						SKIPTO_NOSPAN(n, '"');
					}
					SKIPTOWS(n, 0, true, true);
					n[0] = 0;
					Tag* tag = addTag(space);
					addTagToReading(*cCohort->wread, tag);
					space = ++n;
				}
			}
		}
		else if (cleaned[0] == ' ' && cleaned[1] == '"' && cCohort) {
			is_deleted = false;
			readings = &cCohort->readings;

		got_reading:
			// Count current indent level
			size_t indent = 0;
			while (ISSPACE(line[indent])) {
				++indent;
			}
			while (!indents.empty() && indent <= indents.back().first) {
				indents.pop_back();
			}
			if (!indents.empty() && indent > indents.back().first) {
				if (indents.back().second->next) {
					u_fprintf(ux_stderr, "Warning: Sub-reading %S on line %u will be ignored and lost as each reading currently only can have one sub-reading.\n", &cleaned[0], numLines);
					u_fflush(ux_stderr);
					cReading = nullptr;
					continue;
				}
				cReading = indents.back().second->allocateReading(indents.back().second->parent);
				indents.back().second->next = cReading;
			}
			else {
				cReading = alloc_reading(cCohort);
			}
			insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
			addTagToReading(*cReading, cCohort->wordform);

			UChar* space = &cleaned[1];
			UChar* base = space;
			if (*space == '"') {
				++space;
				SKIPTO_NOSPAN(space, '"');
				SKIPTOWS(space, 0, true, true);
				--space;
			}

			// This does not consider wordforms as invalid readings since chained CG-3 may produce such
			if (*space != '"') {
				u_fprintf(ux_stderr, "Warning: %S on line %u looked like a reading but wasn't - treated as text.\n", &cleaned[0], numLines);
				u_fflush(ux_stderr);
				if (!indents.empty() && indents.back().second->next == cReading) {
					indents.back().second->next = nullptr;
				}
				free_reading(cReading);
				if (is_deleted) {
					cleaned.insert(cleaned.begin(), ';');
					line.insert(line.begin(), ';');
				}
				goto istext;
			}

			cReading->deleted = is_deleted;

			while (space && (space = u_strchr(space, ' ')) != 0) {
				space[0] = 0;
				++space;
				if (base && base[0]) {
					Tag* tag = addTag(base);
					if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
						all_mappings[cReading].push_back(tag);
					}
					else {
						addTagToReading(*cReading, tag);
					}
				}
				base = space;
				if (*space == '"') {
					++space;
					SKIPTO_NOSPAN(space, '"');
				}
			}
			if (base && base[0]) {
				Tag* tag = addTag(base);
				if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
					all_mappings[cReading].push_back(tag);
				}
				else {
					addTagToReading(*cReading, tag);
				}
			}
			if (!cReading->baseform) {
				u_fprintf(ux_stderr, "Warning: Line %u had no valid baseform.\n", numLines);
				u_fflush(ux_stderr);
			}
			if (indents.empty() || indent <= indents.back().first) {
				cCohort->appendReading(cReading, *readings);
			}
			else {
				auto iter = all_mappings.find(cReading);
				if (iter != all_mappings.end()) {
					while (iter->second.size() > 1) {
						u_fprintf(ux_stderr, "Warning: Sub-reading mapping %S on line %u will be discarded.\n", iter->second.back()->tag.data(), numLines);
						u_fflush(ux_stderr);
						iter->second.pop_back();
					}
					splitMappings(iter->second, *cCohort, *cReading, true);
					all_mappings.erase(iter);
				}
				readings->back()->rehash();
			}
			indents.push_back(std::make_pair(indent, cReading));
			numReadings++;

			// Check whether the cohort still belongs to the window, as per --dep-delimit
			if (!is_deleted && dep_delimit && dep_highest_seen && (cCohort->dep_self <= dep_highest_seen || cCohort->dep_self - dep_highest_seen > dep_delimit)) {
				reflowDependencyWindow(cCohort->global_number);

				for (auto iter : cSWindow->cohorts.back()->readings) {
					addTagToReading(*iter, endtag);
				}

				cSWindow = gWindow->allocAppendSingleWindow();
				initEmptySingleWindow(cSWindow);

				cSWindow->variables_set = variables_set;
				variables_set.clear();
				cSWindow->variables_rem = variables_rem;
				variables_rem.clear();
				cSWindow->variables_output = variables_output;
				variables_output.clear();

				lSWindow = cSWindow;
				++numWindows;
				did_soft_lookback = false;
				dep_highest_seen = 0;

				if (grammar->has_bag_of_tags) {
					// This is slow and not 100% correct as it doesn't remove the tags from the previous window
					cCohort->parent = cSWindow;
					for (auto rit : cCohort->readings) {
						reflowReading(*rit);
					}
				}
			}
		}
		else if (pipe_deleted && cleaned[0] == ';' && cleaned[1] == ' ' && cleaned[2] == '"' && cCohort) {
			is_deleted = true;
			readings = &cCohort->deleted;
			cleaned.erase(cleaned.begin());
			line.erase(line.begin());
			goto got_reading;
		}
		else {
			if (cleaned[0] == ' ' && cleaned[1] == '"') {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: %S on line %u looked like a reading but there was no containing cohort - treated as plain text.\n", &cleaned[0], numLines);
					u_fflush(ux_stderr);
				}
			}
		istext:
			if (line[0]) {
				bool is_cmd = false;
				if (&cleaned[0] == STR_CMD_FLUSH) {
					if (verbosity_level > 0) {
						u_fprintf(ux_stderr, "Info: FLUSH encountered on line %u. Flushing...\n", numLines);
					}
					is_cmd = true;

					auto backSWindow = gWindow->back();
					if (backSWindow) {
						backSWindow->flush_after = true;
					}

					if (cCohort && cSWindow) {
						splitAllMappings(all_mappings, *cCohort, true);
						cSWindow->appendCohort(cCohort);
						if (cCohort->readings.empty()) {
							initEmptyCohort(*cCohort);
						}
						for (auto iter : cCohort->readings) {
							addTagToReading(*iter, endtag);
						}
						cReading = lReading = nullptr;
						cCohort = lCohort = nullptr;
						cSWindow = lSWindow = nullptr;
					}
					while (!gWindow->next.empty()) {
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
					gWindow->shuffleWindowsDown();
					while (!gWindow->previous.empty()) {
						SingleWindow* tmp = gWindow->previous.front();
						printSingleWindow(tmp, output);
						free_swindow(tmp);
						gWindow->previous.erase(gWindow->previous.begin());
					}

					if (!backSWindow) {
						printStreamCommand(UString(STR_CMD_FLUSH), output);
					}
					line[0] = 0;
					variables.clear();
					u_fflush(output);
					u_fflush(ux_stderr);
					fflush(stdout);
					fflush(stderr);
				}
				else if (&cleaned[0] == STR_CMD_IGNORE) {
					if (verbosity_level > 0) {
						u_fprintf(ux_stderr, "Info: IGNORE encountered on line %u. Passing through all input...\n", numLines);
					}
					is_cmd = true;
					ignoreinput = true;
					printStreamCommand(UString(STR_CMD_IGNORE), output);
					line[0] = 0;
				}
				else if (&cleaned[0] == STR_CMD_RESUME) {
					if (verbosity_level > 0) {
						u_fprintf(ux_stderr, "Info: RESUME encountered on line %u. Resuming CG...\n", numLines);
					}
					is_cmd = true;
					ignoreinput = false;
					printStreamCommand(UString(STR_CMD_RESUME), output);
					line[0] = 0;
				}
				else if (&cleaned[0] == STR_CMD_EXIT) {
					if (verbosity_level > 0) {
						u_fprintf(ux_stderr, "Info: EXIT encountered on line %u. Exiting...\n", numLines);
					}
					is_cmd = true;
					printStreamCommand(UString(STR_CMD_EXIT), output);
					goto CGCMD_EXIT;
				}
				else if (u_strncmp(&cleaned[0], STR_CMD_SETVAR.data(), SI32(STR_CMD_SETVAR.size())) == 0) {
					//u_fprintf(ux_stderr, "Info: SETVAR encountered on line %u.\n", numLines);
					is_cmd = true;
					cleaned[packoff - 1] = 0;
					line[0] = 0;

					UChar* s = &cleaned[STR_CMD_SETVAR.size()];
					UChar* c = u_strchr(s, ',');
					UChar* d = u_strchr(s, '=');
					if (c == 0 && d == 0) {
						Tag* tag = addTag(s);
						variables_set[tag->hash] = grammar->tag_any;
						variables_rem.erase(tag->hash);
						variables_output.insert(tag->hash);
						if (cSWindow == nullptr) {
							variables[tag->hash] = grammar->tag_any;
						}
					}
					else {
						uint32_t a = 0, b = 0;
						while (c || d) {
							if (d && (d < c || c == nullptr)) {
								d[0] = 0;
								if (!s[0]) {
									u_fprintf(ux_stderr, "Warning: SETVAR on line %u had no identifier before the =! Defaulting to identifier *.\n", numLines);
									a = grammar->tag_any;
								}
								else {
									a = addTag(s)->hash;
								}
								if (c) {
									c[0] = 0;
									s = c + 1;
								}
								if (!d[1]) {
									u_fprintf(ux_stderr, "Warning: SETVAR on line %u had no value after the =! Defaulting to value *.\n", numLines);
									b = grammar->tag_any;
								}
								else {
									b = addTag(d + 1)->hash;
								}
								if (!c) {
									d = nullptr;
									s = nullptr;
								}
								variables_set[a] = b;
								variables_rem.erase(a);
								variables_output.insert(a);
							}
							else if (c && (c < d || d == nullptr)) {
								c[0] = 0;
								if (!s[0]) {
									u_fprintf(ux_stderr, "Warning: SETVAR on line %u had no identifier after the ,! Defaulting to identifier *.\n", numLines);
									a = grammar->tag_any;
								}
								else {
									a = addTag(s)->hash;
								}
								s = c + 1;
								variables_set[a] = grammar->tag_any;
								variables_rem.erase(a);
								variables_output.insert(a);
							}
							if (s) {
								c = u_strchr(s, ',');
								d = u_strchr(s, '=');
								if (c == nullptr && d == nullptr) {
									a = addTag(s)->hash;
									variables_set[a] = grammar->tag_any;
									variables_rem.erase(a);
									variables_output.insert(a);
									s = nullptr;
								}
							}
						}
					}
				}
				else if (u_strncmp(&cleaned[0], STR_CMD_REMVAR.data(), SI32(STR_CMD_REMVAR.size())) == 0) {
					//u_fprintf(ux_stderr, "Info: REMVAR encountered on line %u.\n", numLines);
					is_cmd = true;
					cleaned[packoff - 1] = 0;
					line[0] = 0;

					UChar* s = &cleaned[STR_CMD_REMVAR.size()];
					UChar* c = u_strchr(s, ',');
					uint32_t a = 0;
					while (c && *c) {
						c[0] = 0;
						if (s[0]) {
							a = addTag(s)->hash;
							variables_set.erase(a);
							variables_rem.insert(a);
							variables_output.insert(a);
						}
						s = c + 1;
						c = u_strchr(s, ',');
					}
					if (s && s[0]) {
						a = addTag(s)->hash;
						variables_set.erase(a);
						variables_rem.insert(a);
						variables_output.insert(a);
					}
				}

				if (line[0]) {
					if (lSWindow && lCohort && testStringAgainst(line, text_delimiters)) {
						lSWindow->text_post += &line[0];

						for (auto iter : cCohort->readings) {
							addTagToReading(*iter, endtag);
						}

						splitAllMappings(all_mappings, *cCohort, true);
						cSWindow->appendCohort(cCohort);
						cCohort->line_number = numLines;
						lSWindow = cSWindow;
						cSWindow = nullptr;
						cCohort = nullptr;
						numCohorts++;
						did_soft_lookback = false;
					}
					else if (lCohort) {
						lCohort->text += &line[0];
					}
					else if (lSWindow) {
						lSWindow->text += &line[0];
					}
					else if (!is_cmd) {
						printPlainTextLine(&line[0], output);
					}
				}
			}
		}
		++numLines;
		line[0] = cleaned[0] = 0;
	}

	input_eof = true;

	if (cCohort && cSWindow) {
		splitAllMappings(all_mappings, *cCohort, true);
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
		if (verbosity_level > 0) {
			u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
			u_fflush(ux_stderr);
		}
	}

	gWindow->shuffleWindowsDown();
	while (!gWindow->previous.empty()) {
		SingleWindow* tmp = gWindow->previous.front();
		printSingleWindow(tmp, output);
		free_swindow(tmp);
		gWindow->previous.erase(gWindow->previous.begin());
	}

	u_fflush(output);

	for (auto var : variables_output) {
		Tag* key = grammar->single_tags[var];
		auto iter = variables_set.find(var);
		UString cmd_buf;
		if (iter != variables_set.end()) {
			if (iter->second != grammar->tag_any) {
				Tag* value = grammar->single_tags[iter->second];
				cmd_buf.append(STR_CMD_SETVAR).append(key->tag).append(u"=").append(value->tag).append(u">");
			}
			else {
				cmd_buf.append(STR_CMD_SETVAR).append(key->tag).append(u">");
			}
		}
		else {
			cmd_buf.append(STR_CMD_REMVAR).append(key->tag).append(u">");
		}
		printStreamCommand(cmd_buf, output);
	}

CGCMD_EXIT:
	if (verbosity_level > 0) {
		u_fprintf(ux_stderr, "Did %u lines, %u windows, %u cohorts, %u readings.\n", numLines, numWindows, numCohorts, numReadings);
		u_fflush(ux_stderr);
	}
}
}
