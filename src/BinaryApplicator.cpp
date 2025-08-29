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

#include "BinaryApplicator.hpp"
#include "Grammar.hpp"
#include "version.hpp"

namespace CG3 {

BinaryApplicator::BinaryApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
{
}

void BinaryApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
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

	{
		std::string header(8, 0);
		if (!input.read(&header[0], 8)) {
			u_fprintf(ux_stderr, "Error: Could not read stream header!\n");
			CG3Quit(1);
		}
		if (!is_cg3bsf(header)) {
			u_fprintf(ux_stderr, "Error: Stream does not start with magic bytes - cannot read as binary!\n");
			CG3Quit(1);
		}
		uint32_t version = reinterpret_cast<uint32_t*>(&header[4])[0];
		if (version != CG3_BINARY_STREAM) {
			u_fprintf(ux_stderr, "Error: Stream is version %u but this reader only knows version %u!\n", version, CG3_BINARY_STREAM);
			CG3Quit(1);
		}
	}

	index();

	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);

	gWindow->window_span = num_windows;

	auto flush = [&](bool flush_after = false) {
		auto backSWindow = gWindow->back();
		if (backSWindow) {
			backSWindow->flush_after = flush_after;
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

		return backSWindow;
	};

	while (!input.eof()) {
		auto packet = readPacket();
		if (packet.type == BFP_WINDOW) {
			//auto cSWindow = static_cast<SingleWindow*>(packet.payload);
			++numWindows;
			if (gWindow->next.size() > num_windows) {
				gWindow->shuffleWindowsDown();
				runGrammarOnWindow();
				if (numWindows % resetAfter == 0) {
					resetIndexes();
				}
			}
		}
		else if (packet.type == BFP_COMMAND) {
			auto cmd = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(packet.payload));
			if (cmd == BFC_FLUSH) {
				if (!flush(true)) {
					printStreamCommand(STR_CMD_FLUSH, *ux_stdout);
				}
			}
			else if (cmd == BFC_EXIT) {
				printStreamCommand(STR_CMD_EXIT, *ux_stdout);
				return;
			}
			else if (cmd == BFC_IGNORE) {
				printStreamCommand(STR_CMD_IGNORE, *ux_stdout);
			}
			else if (cmd == BFC_RESUME) {
				printStreamCommand(STR_CMD_RESUME, *ux_stdout);
			}
		}
		else if (packet.type == BFP_TEXT) {
			auto& text = *static_cast<UString*>(packet.payload);
			printPlainTextLine(text, *ux_stdout);
		}
	}
	flush(false);
}

BinaryPacket BinaryApplicator::readPacket() {
	BinaryPacket packet;
	readLE(*ux_stdin, packet.type);
	if (packet.type == BFP_WINDOW) {
		readWindow(packet.payload);
	}
	else if (packet.type == BFP_COMMAND) {
		readCommand(packet.payload);
	}
	if (packet.type == BFP_TEXT) {
		readText(packet.payload);
	}
	return packet;
}

void BinaryApplicator::readWindow(void*& payload) {
	uint32_t cs = 0;
	readLE(*ux_stdin, cs);

	if (ux_stdin->eof()) {
		payload = nullptr;
		return;
	}

	SingleWindow* cSWindow = gWindow->allocAppendSingleWindow();
	initEmptySingleWindow(cSWindow);

	std::string buf(cs, 0);
	ux_stdin->read(&buf[0], cs);
	uint32_t pos = 0;

	auto READ_U16_INTO = [&](uint16_t& dest) {
		dest = *reinterpret_cast<uint16_t*>(&buf[pos]);
		be::little_to_native_inplace(dest);
		pos += sizeof(dest);
	};

	auto READ_U16 = [&]() {
		uint16_t dest;
		READ_U16_INTO(dest);
		return dest;
	};

	auto READ_U32_INTO = [&](uint32_t& dest) {
		dest = *reinterpret_cast<uint32_t*>(&buf[pos]);
		be::little_to_native_inplace(dest);
		pos += sizeof(dest);
	};

	auto READ_U32 = [&]() {
		uint32_t dest;
		READ_U32_INTO(dest);
		return dest;
	};

	auto READ_STR_INTO = [&](UString& dest) {
		auto tl = READ_U16();
		dest.clear();
		dest.resize(tl);
		int32_t olen = 0;
		UErrorCode status = U_ZERO_ERROR;
		u_strFromUTF8(&(dest)[0], tl, &olen, &buf[pos], tl, &status);
		dest.resize(olen);
		pos += tl;
	};

	// TODO: flags
	auto flags = READ_U16();
	if (flags & BFW_DEP_SPAN) {
		dep_has_spanned = true;
	}

	TagVector window_tags;
	auto tag_count = READ_U16();
	window_tags.reserve(tag_count);
	for (uint16_t i = 0; i < tag_count; ++i) {
		UString tg;
		READ_STR_INTO(tg);
		window_tags.push_back(addTag(tg));
		if (tg[0] == grammar->mapping_prefix) {
			window_tags.back()->type |= T_MAPPING;
		}
		else {
			window_tags.back()->type &= ~T_MAPPING;
		}
	}

	auto var_count = READ_U16();
	for (uint16_t vn = 0; vn < var_count; ++vn) {
		char mode = buf[pos];
		++pos;
		auto tag1 = READ_U16();
		auto tag2 = READ_U16();
		auto hash1 = window_tags[tag1]->hash;
		if (mode == BFV_SETVAR) {
			cSWindow->variables_set[hash1] = window_tags[tag2]->hash;
			cSWindow->variables_rem.erase(hash1);
			cSWindow->variables_output.insert(hash1);
		}
		else if (mode == BFV_SETVAR_ANY) {
			cSWindow->variables_set[hash1] = grammar->tag_any;
			cSWindow->variables_rem.erase(hash1);
			cSWindow->variables_output.insert(hash1);
		}
		else if (mode == BFV_REMVAR) {
			cSWindow->variables_set.erase(hash1);
			cSWindow->variables_rem.insert(hash1);
			cSWindow->variables_output.insert(hash1);
		}
	}

	READ_STR_INTO(cSWindow->text);
	READ_STR_INTO(cSWindow->text_post);

	auto cohort_count = READ_U16();
	uint16_t tag;
	for (uint16_t cn = 0; cn < cohort_count; ++cn) {
		Cohort* cCohort = alloc_cohort(cSWindow);
		cCohort->global_number = gWindow->cohort_counter++;
		++numCohorts;

		READ_U16_INTO(flags);
		if (flags & BFC_RELATED) {
			cCohort->type |= CT_RELATED;
			has_relations = true;
		}

		READ_U16_INTO(tag);
		cCohort->wordform = window_tags[tag];

		READ_U16_INTO(tag_count);
		if (tag_count) {
			cCohort->wread = alloc_reading(cCohort);
			addTagToReading(*cCohort->wread, cCohort->wordform);
			for (uint16_t tn = 0; tn < tag_count; ++tn) {
				READ_U16_INTO(tag);
				addTagToReading(*cCohort->wread, window_tags[tag], (tn + 1 == tag_count));
			}
		}

		READ_U32_INTO(cCohort->dep_self);
		READ_U32_INTO(cCohort->dep_parent);
		gWindow->relation_map[cCohort->dep_self] = cCohort->global_number;

		if (cCohort->dep_parent != DEP_NO_PARENT) {
			has_dep = true;
		}

		auto rel_count = READ_U16();
		for (uint16_t rn = 0; rn < rel_count; ++rn) {
			READ_U16_INTO(tag);
			auto head = READ_U32();
			cCohort->relations_input[window_tags[tag]->hash].insert(head);
		}
		if (rel_count) {
			has_relations = true;
			gWindow->relation_map[cCohort->dep_self] = cCohort->global_number;
			cCohort->type |= CT_RELATED;
		}

		READ_STR_INTO(cCohort->text);
		READ_STR_INTO(cCohort->wblank);

		auto reading_count = READ_U16();
		if (!reading_count) {
			initEmptyCohort(*cCohort);
		}
		Reading* prev = nullptr;
		for (uint16_t rn = 0; rn < reading_count; ++rn) {
			Reading* cReading = alloc_reading(cCohort);
			addTagToReading(*cReading, cCohort->wordform);

			READ_U16_INTO(flags);

			READ_U16_INTO(tag);
			addTagToReading(*cReading, window_tags[tag]);

			READ_U16_INTO(tag_count);
			TagList mappings;
			for (uint16_t tn = 0; tn < tag_count; ++tn) {
				READ_U16_INTO(tag);
				if (window_tags[tag]->type & T_MAPPING) {
					mappings.push_back(window_tags[tag]);
				}
				else {
					addTagToReading(*cReading, window_tags[tag]);
				}
			}
			if (!mappings.empty()) {
				splitMappings(mappings, *cCohort, *cReading, true);
			}

			if (prev && (flags & BFR_SUBREADING)) {
				prev->next = cReading;
			}
			else if (flags & BFR_DELETED) {
				cCohort->deleted.push_back(cReading);
			}
			else {
				cCohort->appendReading(cReading);
			}
			prev = cReading;
			++numReadings;
		}

		if (cn + 1 == cohort_count) {
			for (auto iter : cCohort->readings) {
				if (iter->tags.find(endtag) == iter->tags.end()) {
					addTagToReading(*iter, endtag);
				}
			}
		}

		insert_if_exists(cCohort->possible_sets, grammar->sets_any);
		cSWindow->appendCohort(cCohort);
	}

	payload = cSWindow;
}

void BinaryApplicator::readCommand(void*& payload) {
	auto cmd = readLE<uint8_t>(*ux_stdin);
	payload = reinterpret_cast<void*>(static_cast<uintptr_t>(cmd));
}

void BinaryApplicator::readText(void*& payload) {
	readUTF8_LE(*ux_stdin, text);
	payload = &text;
}

void BinaryApplicator::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
	if (!header_done) {
		output.write("CGBF", 4);
		writeLE(output, CG3_BINARY_STREAM);
		header_done = true;
	}

	writeLE(output, UI8(BFP_WINDOW));

	TagVector tags_to_write;
	std::map<Tag*, uint16_t> tag_index;

	auto WRITE_U16_INTO = [&](uint16_t n, std::string& buffer) {
		be::native_to_little_inplace(n);
		auto chr = reinterpret_cast<char*>(&n);
		buffer += chr[0];
		buffer += chr[1];
	};

	auto WRITE_U32_INTO = [&](uint32_t n, std::string& buffer) {
		be::native_to_little_inplace(n);
		auto chr = reinterpret_cast<char*>(&n);
		buffer += chr[0];
		buffer += chr[1];
		buffer += chr[2];
		buffer += chr[3];
	};

	auto WRITE_TAG_INTO = [&](Tag* tag, std::string& buffer) {
		if (tag_index.find(tag) == tag_index.end()) {
			tag_index[tag] = UI16(tags_to_write.size());
			tags_to_write.push_back(tag);
		}
		WRITE_U16_INTO(tag_index[tag], buffer);
	};

	auto WRITE_STR_INTO = [&](const UString& s, std::string& buffer) {
		std::string tmp(s.size() * 4, 0);
		int32_t olen = 0;
		UErrorCode status = U_ZERO_ERROR;
		u_strToUTF8(&tmp[0], SI32(s.size() * 4 - 1), &olen, s.data(), SI32(s.size()), &status);
		tmp.resize(olen);
		WRITE_U16_INTO(UI16(olen), buffer);
		buffer += tmp;
	};

	uint16_t var_count = 0;
	std::string var_buffer;
	for (auto var : window->variables_output) {
		++var_count;
		Tag* key = grammar->single_tags[var];
		auto iter = window->variables_set.find(var);
		if (iter != window->variables_set.end()) {
			if (iter->second != grammar->tag_any) {
				var_buffer += static_cast<char>(BFV_SETVAR);
				WRITE_TAG_INTO(key, var_buffer);
				WRITE_TAG_INTO(grammar->single_tags[iter->second], var_buffer);
			}
			else {
				var_buffer += static_cast<char>(BFV_SETVAR_ANY);
				WRITE_TAG_INTO(key, var_buffer);
				WRITE_U16_INTO(0, var_buffer);
			}
		}
		else {
			var_buffer += static_cast<char>(BFV_REMVAR);
			WRITE_TAG_INTO(key, var_buffer);
			WRITE_U16_INTO(0, var_buffer);
		}
	}

	// Move text belonging to removed cohorts to prior not-removed cohorts, or the containing window
	for (size_t i = 0; i < window->all_cohorts.size(); ++i) {
		auto cohort = window->all_cohorts[i];
		if (cohort->local_number == 0 || (cohort->type & CT_REMOVED)) {
			if (!cohort->text.empty()) {
				for (size_t j = i; j > 0; --j) {
					if (window->all_cohorts[j - 1]->local_number == 0 || (window->all_cohorts[j - 1]->type & CT_REMOVED)) {
						continue;
					}
					window->all_cohorts[j - 1]->text += cohort->text;
					cohort->text.clear();
				}
				window->text += cohort->text;
				cohort->text.clear();
			}
		}
	}

	std::string cohort_buffer;
	uint16_t cohort_count = 0;
	for (auto& cohort : window->all_cohorts) {
		if (cohort->local_number == 0 || (cohort->type & CT_REMOVED)) {
			continue;
		}
		cohort->unignoreAll();
		++cohort_count;

		uint16_t flags = 0;
		if (cohort->type & CT_RELATED) {
			flags |= BFC_RELATED;
		}
		WRITE_U16_INTO(flags, cohort_buffer);

		WRITE_TAG_INTO(cohort->wordform, cohort_buffer);
		if (cohort->wread) {
			std::string tag_buffer;
			uint16_t tag_count = 0;
			for (auto tter : cohort->wread->tags_list) {
				if (tter == cohort->wordform->hash) {
					continue;
				}
				WRITE_TAG_INTO(grammar->single_tags[tter], tag_buffer);
				++tag_count;
			}
			WRITE_U16_INTO(tag_count, cohort_buffer);
			cohort_buffer += tag_buffer;
		}
		else {
			WRITE_U16_INTO(0, cohort_buffer);
		}

		WRITE_U32_INTO(cohort->global_number, cohort_buffer);
		if (cohort->dep_parent == 0 || cohort->dep_parent == DEP_NO_PARENT) {
			WRITE_U32_INTO(cohort->dep_parent, cohort_buffer);
		}
		else {
			if (gWindow->cohort_map.find(cohort->dep_parent) != gWindow->cohort_map.end()) {
				auto pr = gWindow->cohort_map[cohort->dep_parent];
				if (pr->local_number == 0) {
					WRITE_U32_INTO(0, cohort_buffer);
				}
				else {
					WRITE_U32_INTO(pr->global_number, cohort_buffer);
				}
			}
			else {
				WRITE_U32_INTO(DEP_NO_PARENT, cohort_buffer);
			}
		}

		std::string rel_buffer;
		uint16_t rel_count = 0;
		for (const auto& miter : cohort->relations) {
			auto it = grammar->single_tags.find(miter.first);
			if (it == grammar->single_tags.end()) {
				it = grammar->single_tags.find(miter.first);
			}
			for (auto siter : miter.second) {
				++rel_count;
				WRITE_TAG_INTO(it->second, rel_buffer);
				WRITE_U32_INTO(siter, rel_buffer);
			}
		}
		WRITE_U16_INTO(rel_count, cohort_buffer);
		cohort_buffer += rel_buffer;

		WRITE_STR_INTO(cohort->text, cohort_buffer);
		WRITE_STR_INTO(cohort->wblank, cohort_buffer);

		std::string reading_buffer;
		uint16_t reading_count = 0;
		std::sort(cohort->readings.begin(), cohort->readings.end(), Reading::cmp_number);
		for (auto top_reading : cohort->readings) {
			if (top_reading->noprint) {
				continue;
			}
			auto reading = top_reading;
			while (reading) {
				++reading_count;
				uint16_t flags = 0;
				if (reading != top_reading) {
					flags |= BFR_SUBREADING;
				}
				WRITE_U16_INTO(flags, reading_buffer);
				WRITE_TAG_INTO(grammar->single_tags[reading->baseform], reading_buffer);
				std::string tag_buffer;
				uint16_t tag_count = 0;
				uint32SortedVector unique;
				for (auto& tter : reading->tags_list) {
					auto tag = grammar->single_tags[tter];
					if (tter == reading->baseform || tter == reading->parent->wordform->hash) {
						continue;
					}
					if (tag->type & (T_DEPENDENCY | T_RELATION)) {
						continue;
					}
					if (unique_tags) {
						if (unique.find(tter) != unique.end()) {
							continue;
						}
						unique.insert(tter);
					}
					WRITE_TAG_INTO(tag, tag_buffer);
					++tag_count;
				}
				WRITE_U16_INTO(tag_count, reading_buffer);
				reading_buffer += tag_buffer;
				reading = reading->next;
			}
		}
		WRITE_U16_INTO(reading_count, cohort_buffer);
		cohort_buffer += reading_buffer;
	}

	std::string header_buffer;

	uint16_t flags = 0;
	if (dep_has_spanned) {
		flags |= BFW_DEP_SPAN;
	}
	WRITE_U16_INTO(flags, header_buffer);

	WRITE_U16_INTO(UI16(tags_to_write.size()), header_buffer);
	for (auto& tag : tags_to_write) {
		WRITE_STR_INTO(tag->tag, header_buffer);
	}

	WRITE_U16_INTO(var_count, header_buffer);
	header_buffer += var_buffer;

	WRITE_STR_INTO(window->text, header_buffer);
	WRITE_STR_INTO(window->text_post, header_buffer);

	WRITE_U16_INTO(cohort_count, header_buffer);

	auto total_size = UI32(header_buffer.size() + cohort_buffer.size());
	writeLE(output, total_size);
	output.write(header_buffer.data(), header_buffer.size());
	output.write(cohort_buffer.data(), cohort_buffer.size());

	if (window->flush_after) {
		printStreamCommand(STR_CMD_FLUSH, output);
	}

	output.flush();
}

void BinaryApplicator::printStreamCommand(UStringView cmd, std::ostream& output) {
	if (!header_done) {
		output.write("CGBF", 4);
		writeLE(output, CG3_BINARY_STREAM);
		header_done = true;
	}

	writeLE(output, UI8(BFP_COMMAND));
	if (cmd == STR_CMD_FLUSH) {
		writeLE(output, UI8(BFC_FLUSH));
	}
	else if (cmd == STR_CMD_EXIT) {
		writeLE(output, UI8(BFC_EXIT));
	}
	else if (cmd == STR_CMD_IGNORE) {
		writeLE(output, UI8(BFC_IGNORE));
	}
	else if (cmd == STR_CMD_RESUME) {
		writeLE(output, UI8(BFC_RESUME));
	}
}

void BinaryApplicator::printPlainTextLine(UStringView line, std::ostream& output) {
	if (!header_done) {
		output.write("CGBF", 4);
		writeLE(output, CG3_BINARY_STREAM);
		header_done = true;
	}

	writeLE(output, UI8(BFP_TEXT));
	writeUTF8_LE(output, line);
}

}
