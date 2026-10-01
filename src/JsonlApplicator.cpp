/*
* Copyright (C) 2025-2026, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* JSONL I/O developed by Robert Reynolds <reynoldsrjr@gmail.com>
* Based on contributions from GitHub Copilot
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

#include "JsonlApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

#include <sstream>
#include <string>
#include "uextras.hpp"

namespace json = rapidjson;

namespace CG3 {

namespace {

std::string ustring_to_utf8(UStringView ustr) {
	std::string utf8_str;
	UErrorCode status = U_ZERO_ERROR;
	int32_t required_length = 0;
	u_strToUTF8(nullptr, 0, &required_length, ustr.data(), SI32(ustr.size()), &status);

	utf8_str.resize(required_length);
	status = U_ZERO_ERROR;

	u_strToUTF8(&utf8_str[0], required_length, nullptr, ustr.data(), SI32(ustr.size()), &status);

	return utf8_str;
}

UString utf8_to_ustring(const char* str, size_t len) {
	icu::UnicodeString unicode_str = icu::UnicodeString::fromUTF8(icu::StringPiece(str, SI32(len)));
	return UString(unicode_str.getBuffer(), unicode_str.length());
}

json::Value to_json(UStringView str, json::Document::AllocatorType& allocator) {
	auto utf8 = ustring_to_utf8(str);
	return json::Value(utf8.c_str(), json::SizeType(utf8.size()), allocator);
}

void write_json_line(const json::Document& doc, std::ostream& output) {
	json::StringBuffer buffer;
	json::Writer<json::StringBuffer> writer(buffer);
	doc.Accept(writer);
	output << buffer.GetString() << "\n";
}

}

JsonlApplicator::JsonlApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err) {
}

// Add explicit destructor definition to potentially anchor the vtable
JsonlApplicator::~JsonlApplicator() {
	// Empty destructor body
}

// Reads obj[key] into out. Returns false if the key is absent, or warns and returns false if it isn't a string.
bool JsonlApplicator::getJsonString(const json::Value& obj, const char* key, UString& out) {
	auto it = obj.FindMember(key);
	if (it == obj.MemberEnd()) {
		return false;
	}
	if (!it->value.IsString()) {
		u_fprintf(ux_stderr, "Warning: '%s' on line %u is not a string - ignored.\n", key, numLines);
		return false;
	}
	out = utf8_to_ustring(it->value.GetString(), it->value.GetStringLength());
	return true;
}

// Sub-readings and deleted readings can't be split into several readings, so they keep only their first mapping tag
void JsonlApplicator::addSingleMapping(Reading& reading, TagList& mappings) {
	if (mappings.empty()) {
		return;
	}
	for (size_t i = 1; i < mappings.size(); ++i) {
		u_fprintf(ux_stderr, "Warning: Mapping tag %S on line %u will be discarded, as this reading can only have one.\n", mappings[i]->tag.data(), numLines);
	}
	addTagToReading(reading, mappings.front());
	mappings.clear();
}

// Parses a reading and its sub-readings. Mapping tags of the top reading are returned in mappings so the caller can split them.
Reading* JsonlApplicator::parseJsonReading(const json::Value& reading_obj, Cohort* cohort, TagList& mappings) {
	Reading* cReading = alloc_reading(cohort);
	addTagToReading(*cReading, cohort->wordform);

	UString str;
	if (!reading_obj.HasMember("l")) {
		u_fprintf(ux_stderr, "Warning: Reading missing 'l' (baseform) on line %u.\n", numLines);
	}
	else if (getJsonString(reading_obj, "l", str)) {
		if (str.empty()) {
			u_fprintf(ux_stderr, "Warning: Empty 'l' (baseform) in reading on line %u.\n", numLines);
		}
		else {
			UString base_tag;
			base_tag += '"';
			base_tag += str;
			base_tag += '"';
			addTagToReading(*cReading, addTag(base_tag));
		}
	}

	auto ts = reading_obj.FindMember("ts");
	if (ts != reading_obj.MemberEnd()) {
		if (!ts->value.IsArray()) {
			u_fprintf(ux_stderr, "Warning: 'ts' (tags) on line %u is not an array - ignored.\n", numLines);
		}
		else {
			for (auto& tag_val : ts->value.GetArray()) {
				if (!tag_val.IsString()) {
					u_fprintf(ux_stderr, "Warning: Non-string found in 'ts' (tags) array on line %u - skipping.\n", numLines);
					continue;
				}
				str = utf8_to_ustring(tag_val.GetString(), tag_val.GetStringLength());
				if (str.empty()) {
					continue;
				}
				auto tag = addTag(str);
				if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
					tag->type |= T_MAPPING;
					mappings.push_back(tag);
				}
				else {
					addTagToReading(*cReading, tag);
				}
			}
		}
	}

	auto sub = reading_obj.FindMember("s");
	if (sub != reading_obj.MemberEnd()) {
		if (sub->value.IsObject()) {
			TagList sub_mappings;
			cReading->next = parseJsonReading(sub->value, cohort, sub_mappings);
			addSingleMapping(*cReading->next, sub_mappings);
		}
		else {
			u_fprintf(ux_stderr, "Warning: Value for 's' (sub-reading) is not an object on line %u - skipping.\n", numLines);
		}
	}

	if (!cReading->baseform) {
		cReading->baseform = cohort->wordform->hash;
		u_fprintf(ux_stderr, "Warning: Reading on line %u ended up with no baseform. Using wordform.\n", numLines);
	}

	// The hash covers the sub-reading, which was attached after the tags were added
	cReading->rehash();
	return cReading;
}

Cohort* JsonlApplicator::parseJsonCohort(const json::Value& obj, SingleWindow* cSWindow) {
	Cohort* cCohort = alloc_cohort(cSWindow);
	cCohort->global_number = gWindow->cohort_counter++;
	cCohort->line_number = numLines;
	++numCohorts;

	UString str;
	getJsonString(obj, "w", str); // Validated by the caller
	UString wform_tag;
	wform_tag.append(u"\"<");
	wform_tag += str;
	wform_tag.append(u">\"");
	cCohort->wordform = addTag(wform_tag);

	str.clear();
	if (getJsonString(obj, "wb", str)) {
		cCohort->wblank = str;
	}

	// Text following the cohort. The CG reader keeps each line's newline, and the writer drops the last one, so add it back.
	// An empty "z" is a blank line.
	str.clear();
	if (getJsonString(obj, "z", str)) {
		cCohort->text = str;
		cCohort->text += '\n';
	}

	auto sts = obj.FindMember("sts");
	if (sts != obj.MemberEnd()) {
		if (!sts->value.IsArray()) {
			u_fprintf(ux_stderr, "Warning: 'sts' (static tags) on line %u is not an array - ignored.\n", numLines);
		}
		else {
			for (auto& tag_val : sts->value.GetArray()) {
				if (!tag_val.IsString()) {
					u_fprintf(ux_stderr, "Warning: Non-string found in 'sts' (static tags) array on line %u - skipping.\n", numLines);
					continue;
				}
				str = utf8_to_ustring(tag_val.GetString(), tag_val.GetStringLength());
				if (str.empty()) {
					continue;
				}
				if (!cCohort->wread) {
					cCohort->wread = alloc_reading(cCohort);
					addTagToReading(*cCohort->wread, cCohort->wordform);
				}
				addTagToReading(*cCohort->wread, addTag(str));
			}
		}
	}

	auto rs = obj.FindMember("rs");
	if (rs != obj.MemberEnd()) {
		if (!rs->value.IsArray()) {
			u_fprintf(ux_stderr, "Warning: 'rs' (readings) on line %u is not an array - ignored.\n", numLines);
		}
		else {
			all_mappings_t all_mappings;
			for (auto& reading_val : rs->value.GetArray()) {
				if (!reading_val.IsObject()) {
					u_fprintf(ux_stderr, "Warning: Non-object found in 'rs' (readings) array on line %u - skipping.\n", numLines);
					continue;
				}
				TagList mappings;
				auto cReading = parseJsonReading(reading_val, cCohort, mappings);
				cCohort->appendReading(cReading);
				if (!mappings.empty()) {
					all_mappings[cReading] = mappings;
				}
				++numReadings;
			}
			splitAllMappings(all_mappings, *cCohort, true);
		}
	}

	// Before initEmptyCohort(), since setRelated() would make the empty reading printable
	auto id = obj.FindMember("id");
	if (id != obj.MemberEnd()) {
		if (!id->value.IsUint() || id->value.GetUint() == 0) {
			u_fprintf(ux_stderr, "Warning: 'id' on line %u is not a positive integer - ignored.\n", numLines);
		}
		else {
			gWindow->relation_map[id->value.GetUint()] = cCohort->global_number;
		}
	}

	// "rels" marks the cohort as related, even when it is empty
	auto rels = obj.FindMember("rels");
	if (rels != obj.MemberEnd()) {
		if (!rels->value.IsObject()) {
			u_fprintf(ux_stderr, "Warning: 'rels' (relations) on line %u is not an object - ignored.\n", numLines);
		}
		else {
			for (auto& rel : rels->value.GetObject()) {
				if (!rel.value.IsArray()) {
					u_fprintf(ux_stderr, "Warning: Relation targets on line %u are not an array - ignored.\n", numLines);
					continue;
				}
				auto name = addTag(utf8_to_ustring(rel.name.GetString(), rel.name.GetStringLength()));
				for (auto& target : rel.value.GetArray()) {
					if (!target.IsUint()) {
						u_fprintf(ux_stderr, "Warning: Relation target on line %u is not a non-negative integer - skipping.\n", numLines);
						continue;
					}
					cCohort->relations_input[name->hash].insert(target.GetUint());
				}
			}
			has_relations = true;
			cCohort->setRelated();
		}
	}

	if (cCohort->readings.empty()) {
		initEmptyCohort(*cCohort);
	}
	insert_if_exists(cCohort->possible_sets, grammar->sets_any);

	auto drs = obj.FindMember("drs");
	if (drs != obj.MemberEnd()) {
		if (!pipe_deleted) {
			if (verbosity_level > 0) {
				u_fprintf(ux_stderr, "Info: Ignoring 'drs' (deleted readings) on line %u; use --deleted to read them.\n", numLines);
			}
		}
		else if (!drs->value.IsArray()) {
			u_fprintf(ux_stderr, "Warning: 'drs' (deleted readings) on line %u is not an array - ignored.\n", numLines);
		}
		else {
			for (auto& dr_val : drs->value.GetArray()) {
				if (!dr_val.IsObject()) {
					u_fprintf(ux_stderr, "Warning: Non-object found in 'drs' (deleted readings) array on line %u - skipping.\n", numLines);
					continue;
				}
				TagList mappings;
				auto delR = parseJsonReading(dr_val, cCohort, mappings);
				addSingleMapping(*delR, mappings);
				for (auto r = delR; r; r = r->next) {
					r->deleted = true;
				}
				cCohort->appendReading(delR, cCohort->deleted);
				++numReadings;
			}
		}
	}

	auto ds = obj.FindMember("ds");
	auto dp = obj.FindMember("dp");
	if (ds != obj.MemberEnd()) {
		if (!ds->value.IsUint() || ds->value.GetUint() == 0) {
			u_fprintf(ux_stderr, "Warning: 'ds' (dependency self) on line %u is not a positive integer - ignored.\n", numLines);
		}
		else {
			cCohort->dep_self = ds->value.GetUint();
			if (dp != obj.MemberEnd()) {
				if (!dp->value.IsUint()) {
					u_fprintf(ux_stderr, "Warning: 'dp' (dependency parent) on line %u is not a non-negative integer - ignored.\n", numLines);
				}
				else if (dp->value.GetUint() != cCohort->dep_self) {
					cCohort->dep_parent = dp->value.GetUint();
				}
			}
			has_dep = true;
		}
	}
	else if (dp != obj.MemberEnd()) {
		u_fprintf(ux_stderr, "Warning: 'dp' (dependency parent) on line %u without 'ds' (dependency self) - ignored.\n", numLines);
	}

	return cCohort;
}

void JsonlApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
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
			u_fprintf(ux_stderr, "Warning: No soft or hard delimiters defined in grammar. Hard limit of %u cohorts may break windows.\n", hard_limit);
		}
		else {
			u_fprintf(ux_stderr, "Warning: No hard delimiters defined in grammar. Soft limit of %u cohorts may break windows.\n", soft_limit);
		}
	}

	index();

	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);

	bool ignoreinput = false;
	bool did_soft_lookback = false;
	SingleWindow* cSWindow = nullptr; // Window being filled; nullptr once it has been delimited
	SingleWindow* lSWindow = nullptr;
	Cohort* lCohort = nullptr; // Receives following text

	gWindow->window_span = num_windows;

	uint32FlatHashMap variables_set;
	uint32FlatHashSet variables_rem;
	uint32SortedVector variables_output;

	ux_stripBOM(input);

	auto add_end_tag = [&](Cohort* cohort) {
		for (auto iter : cohort->readings) {
			if (iter->tags.find(endtag) == iter->tags.end()) {
				addTagToReading(*iter, endtag);
			}
		}
	};

	auto new_window = [&]() {
		cSWindow = gWindow->allocAppendSingleWindow();
		initEmptySingleWindow(cSWindow);
		lSWindow = cSWindow;
		++numWindows;
		did_soft_lookback = false;

		cSWindow->variables_set.insert(variables_set.begin(), variables_set.end());
		variables_set.clear();
		cSWindow->variables_rem.insert(variables_rem.begin(), variables_rem.end());
		variables_rem.clear();
		cSWindow->variables_output.insert(variables_output.begin(), variables_output.end());
		variables_output.clear();
	};

	// JSONL has no flag for dependencies that span windows, so look for them once they have been resolved.
	// Windows are printed a few runs later, so the CG writer can still switch to its spanning enumeration.
	auto check_dep_span = [&](SingleWindow* window) {
		for (auto cohort : window->cohorts) {
			if (cohort->dep_parent == 0 || cohort->dep_parent == DEP_NO_PARENT || !(cohort->type & CT_DEP_DONE)) {
				continue;
			}
			auto it = gWindow->cohort_map.find(cohort->dep_parent);
			if (it != gWindow->cohort_map.end() && it->second->parent != cohort->parent) {
				dep_has_spanned = true;
				return;
			}
		}
	};

	auto run_window = [&]() {
		gWindow->shuffleWindowsDown();
		runGrammarOnWindow();
		if (has_dep && !dep_has_spanned) {
			check_dep_span(gWindow->current);
			for (auto window : gWindow->next) {
				check_dep_span(window);
			}
		}
		if (numWindows % resetAfter == 0) {
			resetIndexes();
		}
		if (verbosity_level > 0) {
			u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", numLines, numWindows, numCohorts, numReadings);
			u_fflush(ux_stderr);
		}
	};

	auto flush_windows = [&]() {
		while (!gWindow->next.empty()) {
			run_window();
		}
		gWindow->shuffleWindowsDown();
		while (!gWindow->previous.empty()) {
			auto tmp = gWindow->previous.front();
			printSingleWindow(tmp, output);
			free_swindow(tmp);
			gWindow->previous.erase(gWindow->previous.begin());
		}
	};

	// Same placement as the CG reader: text after a cohort belongs to that cohort, and --text-delimit ends the window
	auto add_text = [&](const UString& text) {
		if (lSWindow && lCohort && testStringAgainst(text, text_delimiters)) {
			lSWindow->text_post += text;
			if (cSWindow == lSWindow) {
				add_end_tag(cSWindow->cohorts.back());
				cSWindow = nullptr;
			}
			lCohort = nullptr;
		}
		else if (lCohort) {
			lCohort->text += text;
		}
		else if (lSWindow) {
			if (!lSWindow->text_post.empty()) {
				lSWindow->text_post += text;
			}
			else {
				lSWindow->text += text;
			}
		}
		else {
			printPlainTextLine(text, output);
		}
	};

	std::string line_str;
	while (std::getline(input, line_str)) {
		++numLines;

		if (line_str.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
			continue;
		}

		json::Document doc;
		json::ParseResult ok = doc.Parse(line_str.c_str(), line_str.size());

		if (!ok) {
			u_fprintf(ux_stderr, "Warning: Failed to parse JSON on line %u: %s (offset %u). Skipping line.\n", numLines, json::GetParseError_En(ok.Code()), UI32(ok.Offset()));
			continue;
		}

		if (!doc.IsObject()) {
			u_fprintf(ux_stderr, "Warning: JSON on line %u is not an object. Skipping line.\n", numLines);
			continue;
		}

		if (doc.HasMember("cmd")) {
			UString cmd;
			if (!getJsonString(doc, "cmd", cmd)) {
				continue;
			}
			if (cmd.empty()) {
				u_fprintf(ux_stderr, "Warning: Empty 'cmd' value on line %u.\n", numLines);
				continue;
			}

			auto starts_with = [&](UStringView prefix) {
				return cmd.size() > prefix.size() && cmd.back() == '>' && UStringView(cmd).substr(0, prefix.size()) == prefix;
			};

			if (cmd == STR_CMD_FLUSH) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Info: FLUSH encountered on line %u. Flushing...\n", numLines);
				}

				auto backSWindow = gWindow->back();
				if (backSWindow) {
					backSWindow->flush_after = true;
				}
				if (cSWindow) {
					add_end_tag(cSWindow->cohorts.back());
				}
				lCohort = nullptr;
				cSWindow = nullptr;
				lSWindow = nullptr;

				flush_windows();

				if (!backSWindow) {
					printStreamCommand(STR_CMD_FLUSH, output);
				}

				variables.clear();
				u_fflush(output);
				u_fflush(ux_stderr);
			}
			else if (cmd == STR_CMD_IGNORE) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Info: IGNORE encountered on line %u. Passing through all input...\n", numLines);
				}
				ignoreinput = true;
				printStreamCommand(STR_CMD_IGNORE, output);
			}
			else if (cmd == STR_CMD_RESUME) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Info: RESUME encountered on line %u. Resuming CG...\n", numLines);
				}
				ignoreinput = false;
				printStreamCommand(STR_CMD_RESUME, output);
			}
			else if (cmd == STR_CMD_EXIT) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Info: EXIT encountered on line %u. Exiting...\n", numLines);
				}
				printStreamCommand(STR_CMD_EXIT, output);
				goto CGCMD_EXIT_JSONL;
			}
			else if (starts_with(STR_CMD_SETVAR)) {
				// <STREAMCMD:SETVAR:a=1,b> sets a to 1 and b to *
				auto payload = UStringView(cmd).substr(STR_CMD_SETVAR.size(), cmd.size() - STR_CMD_SETVAR.size() - 1);
				for (size_t b = 0, e = 0; b <= payload.size(); b = e + 1) {
					e = payload.find(',', b);
					if (e == UStringView::npos) {
						e = payload.size();
					}
					auto item = payload.substr(b, e - b);
					auto eq = item.find('=');
					auto key = item.substr(0, eq);
					uint32_t a = grammar->tag_any;
					uint32_t v = grammar->tag_any;
					if (key.empty()) {
						u_fprintf(ux_stderr, "Warning: SETVAR on line %u had an empty identifier! Defaulting to identifier *.\n", numLines);
					}
					else {
						a = addTag(UString(key))->hash;
					}
					if (eq != UStringView::npos) {
						if (eq + 1 == item.size()) {
							u_fprintf(ux_stderr, "Warning: SETVAR on line %u had no value after the =! Defaulting to value *.\n", numLines);
						}
						else {
							v = addTag(UString(item.substr(eq + 1)))->hash;
						}
					}
					variables_set[a] = v;
					variables_rem.erase(a);
					variables_output.insert(a);
				}
			}
			else if (starts_with(STR_CMD_REMVAR)) {
				// <STREAMCMD:REMVAR:a,b> unsets a and b
				auto payload = UStringView(cmd).substr(STR_CMD_REMVAR.size(), cmd.size() - STR_CMD_REMVAR.size() - 1);
				for (size_t b = 0, e = 0; b <= payload.size(); b = e + 1) {
					e = payload.find(',', b);
					if (e == UStringView::npos) {
						e = payload.size();
					}
					if (e == b) {
						continue;
					}
					auto a = addTag(UString(payload.substr(b, e - b)))->hash;
					variables_set.erase(a);
					variables_rem.insert(a);
					variables_output.insert(a);
				}
			}
			else {
				u_fprintf(ux_stderr, "Warning: Unknown or malformed stream command %S on line %u - treated as text.\n", cmd.data(), numLines);
				cmd += '\n';
				add_text(cmd);
			}
			continue;
		}

		if (doc.HasMember("w")) {
			if (ignoreinput) {
				// Pass the cohort through untouched, the same way the CG reader passes through ignored cohort lines
				auto text = utf8_to_ustring(line_str.data(), line_str.size());
				text += '\n';
				add_text(text);
				continue;
			}

			UString wform;
			if (!getJsonString(doc, "w", wform) || wform.empty()) {
				u_fprintf(ux_stderr, "Warning: Cohort on line %u has an empty or invalid 'w' (wordform). Skipping line.\n", numLines);
				continue;
			}

			// Same as the CG reader: once past the soft limit, delimit at the last soft delimiter in the window, if any
			if (cSWindow && cSWindow->cohorts.size() > soft_limit && grammar->soft_delimiters && !did_soft_lookback) {
				did_soft_lookback = true;
				for (auto c : reversed(cSWindow->cohorts)) {
					if (doesSetMatchCohortNormal(*c, grammar->soft_delimiters->number)) {
						did_soft_lookback = false;
						cSWindow = delimitAt(*cSWindow, c)->parent->next;
						lSWindow = cSWindow;
						if (verbosity_level > 0) {
							u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter in buffer.\n", soft_limit, numLines);
						}
						break;
					}
				}
			}
			if (!cSWindow) {
				new_window();
			}
			// Same look-ahead as the CG reader, so rules can see num_windows windows ahead
			if (gWindow->next.size() > num_windows + 1) {
				run_window();
			}

			auto cCohort = parseJsonCohort(doc, cSWindow);

			// Check whether the cohort still belongs to the window, as per --dep-delimit. Cohorts without a dependency never start
			// a new window; the CG reader only checks on reading lines, so a cohort without readings doesn't either.
			if (dep_delimit && dep_highest_seen && cCohort->dep_self && cSWindow->cohorts.size() > 1 && (cCohort->dep_self <= dep_highest_seen || cCohort->dep_self - dep_highest_seen > dep_delimit)) {
				reflowDependencyWindow(cCohort->global_number);
				add_end_tag(cSWindow->cohorts.back());
				new_window();
				dep_highest_seen = 0;
				cCohort->parent = cSWindow;
				if (grammar->has_bag_of_tags) {
					for (auto rit : cCohort->readings) {
						reflowReading(*rit);
					}
				}
			}

			cSWindow->appendCohort(cCohort);
			lCohort = cCohort;

			// The CG reader checks the limits before appending the cohort, hence > instead of >=
			if (cSWindow->cohorts.size() > soft_limit && grammar->soft_delimiters && doesSetMatchCohortNormal(*cCohort, grammar->soft_delimiters->number)) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter.\n", soft_limit, numLines);
				}
				add_end_tag(cCohort);
				cSWindow = nullptr;
			}
			else if (cSWindow->cohorts.size() > hard_limit || (!dep_delimit && grammar->delimiters && doesSetMatchCohortNormal(*cCohort, grammar->delimiters->number))) {
				if (!is_conv && cSWindow->cohorts.size() > hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at line %u - forcing break.\n", hard_limit, numLines);
				}
				add_end_tag(cCohort);
				cSWindow = nullptr;
			}
			continue;
		}

		if (doc.HasMember("t")) {
			UString text;
			if (getJsonString(doc, "t", text)) {
				text += '\n';
				add_text(text);
			}
			continue;
		}

		u_fprintf(ux_stderr, "Warning: JSON object on line %u has none of 'w', 't' or 'cmd'. Skipping line.\n", numLines);
	}

	input_eof = true;

	if (cSWindow) {
		add_end_tag(cSWindow->cohorts.back());
		cSWindow = nullptr;
	}

	flush_windows();

	u_fflush(output);

	// Variables that were set or removed after the last window
	printVariables(variables_output, variables_set, output);

CGCMD_EXIT_JSONL:
	if (verbosity_level > 0) {
		u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u - Done.\n", numLines, numWindows, numCohorts, numReadings);
		u_fflush(ux_stderr);
	}
}

void JsonlApplicator::buildJsonTags(const Reading* reading, json::Value& tags_json, json::Document::AllocatorType& allocator) {
	assert(tags_json.IsArray());

	uint32SortedVector unique;
	TagList mappings;
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

		auto tag = grammar->single_tags[tter];
		if (tag->type & T_DEPENDENCY && has_dep && !dep_original) {
			continue;
		}
		if (tag->type & T_RELATION && has_relations) {
			continue;
		}
		if (tag->type & T_MAPPING) {
			// Move mappings to the end, like the CG writer
			mappings.push_back(tag);
			continue;
		}
		tags_json.PushBack(to_json(tag->tag, allocator), allocator);
	}
	for (auto tag : mappings) {
		tags_json.PushBack(to_json(tag->tag, allocator), allocator);
	}
}

void JsonlApplicator::buildJsonReading(const Reading* reading, json::Value& reading_json, json::Document::AllocatorType& allocator) {
	assert(reading_json.IsObject());

	UStringView baseform;
	if (reading->baseform) {
		auto it = grammar->single_tags.find(reading->baseform);
		if (it != grammar->single_tags.end()) {
			baseform = it->second->tag;
			if (baseform.size() >= 2 && baseform.front() == '"' && baseform.back() == '"') {
				baseform = baseform.substr(1, baseform.size() - 2);
			}
		}
	}
	reading_json.AddMember("l", to_json(baseform, allocator), allocator);

	json::Value tags_json(json::kArrayType);
	buildJsonTags(reading, tags_json, allocator);
	if (!tags_json.Empty()) {
		reading_json.AddMember("ts", tags_json, allocator);
	}

	if (trace && !reading->hit_by.empty()) {
		json::Value trace_json(json::kArrayType);
		for (auto hit_by : reading->hit_by) {
			std::ostringstream ss;
			printTrace(ss, hit_by);
			auto str = ss.str();
			trace_json.PushBack(json::Value(str.c_str(), json::SizeType(str.size()), allocator), allocator);
		}
		reading_json.AddMember("tr", trace_json, allocator);
	}

	if (reading->next) {
		json::Value sub_reading_obj(json::kObjectType);
		buildJsonReading(reading->next, sub_reading_obj, allocator);
		reading_json.AddMember("s", sub_reading_obj, allocator);
	}
}

void JsonlApplicator::printCohort(Cohort* cohort, std::ostream& output, bool profiling) {
	if (cohort->local_number == 0 || (cohort->type & CT_REMOVED)) {
		// The cohort isn't printed, but the text that followed it is
		printText(cohort->text, output);
		return;
	}

	if (!profiling) {
		cohort->unignoreAll();
	}

	json::Document doc;
	doc.SetObject();
	auto& allocator = doc.GetAllocator();

	UStringView wform = cohort->wordform->tag;
	if (wform.size() >= 4 && wform.substr(0, 2) == u"\"<" && wform.substr(wform.size() - 2) == u">\"") {
		wform = wform.substr(2, wform.size() - 4);
	}
	doc.AddMember("w", to_json(wform, allocator), allocator);

	if (!cohort->wblank.empty()) {
		doc.AddMember("wb", to_json(cohort->wblank, allocator), allocator);
	}

	if (cohort->wread) {
		json::Value static_tags_json(json::kArrayType);
		for (auto tter : cohort->wread->tags_list) {
			if (tter == cohort->wordform->hash) {
				continue;
			}
			static_tags_json.PushBack(to_json(grammar->single_tags[tter]->tag, allocator), allocator);
		}
		if (!static_tags_json.Empty()) {
			doc.AddMember("sts", static_tags_json, allocator);
		}
	}

	if (!cohort->text.empty() && cohort->text.find_first_not_of(ws) != UString::npos) {
		UStringView text = cohort->text;
		if (ISNL(text.back())) {
			text.remove_suffix(1);
		}
		doc.AddMember("z", to_json(text, allocator), allocator);
	}

	// Same numbering as GrammarApplicator::printReading(), except that the root is always 0 and "dp" is left out when there is no parent
	if (has_dep) {
		Cohort* pr = nullptr;
		if (cohort->dep_parent != DEP_NO_PARENT) {
			if (cohort->dep_parent == 0) {
				pr = cohort->parent->cohorts[0];
			}
			else {
				auto it = gWindow->cohort_map.find(cohort->dep_parent);
				if (it != gWindow->cohort_map.end()) {
					pr = it->second;
				}
			}
		}
		if (dep_absolute || dep_has_spanned) {
			doc.AddMember("ds", cohort->global_number, allocator);
			if (pr) {
				doc.AddMember("dp", pr->local_number == 0 ? 0 : pr->global_number, allocator);
			}
		}
		else {
			doc.AddMember("ds", cohort->local_number, allocator);
			if (pr) {
				doc.AddMember("dp", pr->local_number, allocator);
			}
		}
	}

	// Every cohort gets an ID when relations are in use, so relation targets can always be found; "rels" marks related cohorts
	if (has_relations || print_ids) {
		doc.AddMember("id", cohort->global_number, allocator);
	}
	if (cohort->type & CT_RELATED) {
		json::Value rels_json(json::kObjectType);
		for (const auto& rel : cohort->relations) {
			json::Value targets(json::kArrayType);
			for (auto target : rel.second) {
				targets.PushBack(target, allocator);
			}
			rels_json.AddMember(to_json(grammar->single_tags[rel.first]->tag, allocator), targets, allocator);
		}
		doc.AddMember("rels", rels_json, allocator);
	}

	std::sort(cohort->readings.begin(), cohort->readings.end(), Reading::cmp_number);

	json::Value readings_json(json::kArrayType);
	for (auto reading : cohort->readings) {
		if (reading->noprint) {
			continue;
		}
		json::Value reading_json(json::kObjectType);
		buildJsonReading(reading, reading_json, allocator);
		readings_json.PushBack(reading_json, allocator);
	}
	if (!readings_json.Empty()) {
		doc.AddMember("rs", readings_json, allocator);
	}

	// Same condition as the CG writer
	if (trace && !trace_no_removed) {
		json::Value deleted_readings_json(json::kArrayType);
		std::sort(cohort->delayed.begin(), cohort->delayed.end(), Reading::cmp_number);
		std::sort(cohort->deleted.begin(), cohort->deleted.end(), Reading::cmp_number);
		for (auto list : { &cohort->delayed, &cohort->deleted }) {
			for (auto reading : *list) {
				if (reading->noprint) {
					continue;
				}
				json::Value reading_json(json::kObjectType);
				buildJsonReading(reading, reading_json, allocator);
				deleted_readings_json.PushBack(reading_json, allocator);
			}
		}
		if (!deleted_readings_json.Empty()) {
			doc.AddMember("drs", deleted_readings_json, allocator);
		}
	}

	write_json_line(doc, output);
	output.flush();
}

void JsonlApplicator::printVariables(const uint32SortedVector& vars_output, const uint32FlatHashMap& vars_set, std::ostream& output) {
	for (auto var : vars_output) {
		auto key = grammar->single_tags[var];
		auto iter = vars_set.find(var);
		UString cmd_buf;
		if (iter != vars_set.end()) {
			if (iter->second != grammar->tag_any) {
				auto value = grammar->single_tags[iter->second];
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
}

void JsonlApplicator::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
	printVariables(window->variables_output, window->variables_set, output);

	printText(window->text, output);

	for (auto& cohort : window->all_cohorts) {
		printCohort(cohort, output, profiling);
	}

	printText(window->text_post, output);

	if (window->flush_after) {
		printStreamCommand(STR_CMD_FLUSH, output);
	}
	u_fflush(output);
}

void JsonlApplicator::printStreamCommand(UStringView cmd, std::ostream& output) {
	json::Document doc;
	doc.SetObject();
	doc.AddMember("cmd", to_json(cmd, doc.GetAllocator()), doc.GetAllocator());
	write_json_line(doc, output);
}

// Skips text that is only whitespace, like the CG writer
void JsonlApplicator::printText(const UString& text, std::ostream& output) {
	if (!text.empty() && text.find_first_not_of(ws) != UString::npos) {
		printPlainTextLine(text, output);
	}
}

// Writes one {"t":...} object per line of text, without the newlines
void JsonlApplicator::printPlainTextLine(UStringView line, std::ostream& output) {
	if (!line.empty() && ISNL(line.back())) {
		line.remove_suffix(1);
	}
	for (;;) {
		auto nl = line.find('\n');
		json::Document doc;
		doc.SetObject();
		doc.AddMember("t", to_json(line.substr(0, nl), doc.GetAllocator()), doc.GetAllocator());
		write_json_line(doc, output);
		if (nl == UStringView::npos) {
			break;
		}
		line.remove_prefix(nl + 1);
	}
}

}
