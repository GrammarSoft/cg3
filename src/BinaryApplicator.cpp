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
  bool flushAfter = false;

  gWindow->window_span = num_windows;

  auto flush = [&]() {
    if (gWindow->back()) {
      gWindow->back()->flush_after = true;
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
    flushAfter = false;
  };

  while (!input.eof()) {
    flushAfter = readWindow();
    ++numWindows;
    if (gWindow->next.size() > num_windows) {
      gWindow->shuffleWindowsDown();
      runGrammarOnWindow();
      if (numWindows % resetAfter == 0) {
	resetIndexes();
      }
    }
    if (flushAfter) {
      flush();
    }
  }
  flush();
}

#define READ_U16_INTO(dest) \
  do { \
    (dest) = reinterpret_cast<uint16_t*>(&buf[pos])[0]; \
    pos += 2; \
  } while (false)

#define READ_U32_INTO(dest) \
  do { \
    (dest) = reinterpret_cast<uint32_t*>(&buf[pos])[0]; \
    pos += 4; \
  } while (false)

#define READ_STR_INTO(dest)			\
  do { \
    uint16_t tl = reinterpret_cast<uint16_t*>(&buf[pos])[0]; \
    pos += 2; \
    (dest).clear(); \
    (dest).resize(tl, 0); \
    int32_t olen = 0; \
    UErrorCode status = U_ZERO_ERROR; \
    u_strFromUTF8(&(dest)[0], tl, &olen, &buf[pos], tl, &status); \
    (dest).resize(olen); \
    pos += tl; \
  } while (false)

bool BinaryApplicator::readWindow() {
  uint32_t cs = 0;
  readRaw(*ux_stdin, cs);

  if (ux_stdin->eof()) {
    return true;
  }

  SingleWindow* cSWindow = gWindow->allocAppendSingleWindow();
  initEmptySingleWindow(cSWindow);

  std::string buf(cs, 0);
  ux_stdin->read(&buf[0], cs);
  uint32_t pos = 0;

  // TODO: flags
  uint16_t flags;
  READ_U16_INTO(flags);
  if (flags & BFW_FLUSH) {
    cSWindow->flush_after = true;
  }

  TagVector window_tags;
  uint16_t tag_count;
  READ_U16_INTO(tag_count);
  window_tags.reserve(tag_count);
  for (uint16_t i = 0; i < tag_count; i++) {
    UString tg;
    READ_STR_INTO(tg);
    window_tags.push_back(addTag(tg));
  }

  uint16_t var_count;
  READ_U16_INTO(var_count);
  for (uint16_t vn = 0; vn < var_count; vn++) {
	  char mode = buf[pos];
	  pos++;
	  uint16_t tag1, tag2;
	  READ_U16_INTO(tag1);
	  READ_U16_INTO(tag2);
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

  uint16_t cohort_count;
  READ_U16_INTO(cohort_count);
  uint16_t tag;
  for (uint16_t cn = 0; cn < cohort_count; cn++) {
    Cohort* cCohort = alloc_cohort(cSWindow);
    cCohort->global_number = gWindow->cohort_counter++;
    numCohorts++;

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
		for (uint16_t tn = 0; tn < tag_count; tn++) {
			READ_U16_INTO(tag);
			addTagToReading(*cCohort->wread, window_tags[tag],
							(tn + 1 == tag_count));
		}
    }

	READ_U32_INTO(cCohort->dep_self);
	READ_U32_INTO(cCohort->dep_parent);
	gWindow->dep_window[cCohort->dep_self] = cCohort;
	gWindow->relation_map[cCohort->dep_self] = cCohort->global_number;

	if (cCohort->dep_parent != DEP_NO_PARENT) {
		has_dep = true;
	}

	uint16_t rel_count;
	READ_U16_INTO(rel_count);
	for (uint16_t rn = 0; rn < rel_count; rn++) {
		READ_U16_INTO(tag);
		uint32_t head;
		READ_U32_INTO(head);
		cCohort->relations_input[window_tags[tag]->hash].insert(head);
	}
	if (rel_count) {
		has_relations = true;
		gWindow->relation_map[cCohort->dep_self] = cCohort->global_number;
		cCohort->type |= CT_RELATED;
	}

    READ_STR_INTO(cCohort->text);
    READ_STR_INTO(cCohort->wblank);

    uint16_t reading_count;
    READ_U16_INTO(reading_count);
	if (!reading_count) initEmptyCohort(*cCohort);
    Reading* prev = nullptr;
    for (uint16_t rn = 0; rn < reading_count; rn++) {
      Reading* cReading = alloc_reading(cCohort);
      addTagToReading(*cReading, cCohort->wordform);

      READ_U16_INTO(flags);

      READ_U16_INTO(tag);
	  addTagToReading(*cReading, window_tags[tag]);

      READ_U16_INTO(tag_count);
      for (uint16_t tn = 0; tn < tag_count; tn++) {
		  READ_U16_INTO(tag);
		  addTagToReading(*cReading, window_tags[tag], (tn+1 == tag_count));
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

	if (cn+1 == cohort_count) {
		for (auto iter : cCohort->readings) {
			if (iter->tags.find(endtag) == iter->tags.end()) {
				addTagToReading(*iter, endtag);
			}
		}
	}

    insert_if_exists(cCohort->possible_sets, grammar->sets_any);
    cSWindow->appendCohort(cCohort);
  }

  return cSWindow->flush_after;
}

#define WRITE_U16_INTO(n, buffer) \
  do { \
    std::string tmp(2, 0);	       \
    uint16_t tmp_n = (n); \
    tmp.assign(reinterpret_cast<char*>(&tmp_n), 2);	\
    (buffer) += tmp; \
  } while (false)

#define WRITE_U32_INTO(n, buffer) \
  do { \
    std::string tmp(4, 0);	       \
    uint32_t tmp_n = (n); \
    tmp.assign(reinterpret_cast<char*>(&tmp_n), 4);	\
    (buffer) += tmp; \
  } while (false)

#define WRITE_TAG_INTO(tag, buffer) \
  do { \
    if (tag_index.find((tag)) == tag_index.end()) { \
      tag_index[(tag)] = tags_to_write.size(); \
      tags_to_write.push_back((tag)); \
    } \
    WRITE_U16_INTO(tag_index[(tag)], buffer); \
  } while (false)

#define WRITE_STR_INTO(s, buffer) \
  do { \
    std::string tmp((s).size() * 4, 0);		\
    int32_t olen = 0; \
    UErrorCode status = U_ZERO_ERROR; \
    u_strToUTF8(&tmp[0], SI32((s).size() * 4 - 1), &olen, (s).data(), SI32((s).size()), &status); \
    tmp.resize(olen); \
    WRITE_U16_INTO(UI16(olen), (buffer)); \
    (buffer) += tmp; \
  } while (false)

void BinaryApplicator::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
  if (window->number == 1) {
    output.write("CGBF", 4);
    std::string version;
    WRITE_U32_INTO(CG3_BINARY_STREAM, version);
    output.write(version.data(), 4);
  }

  TagVector tags_to_write;
  std::map<Tag*, uint32_t> tag_index;

  uint16_t var_count = 0;
  std::string var_buffer;
  for (auto var : window->variables_output) {
	  var_count++;
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

  std::string cohort_buffer;
  uint16_t cohort_count = 0;
  for (auto& cohort : window->all_cohorts) {
    if (cohort->local_number == 0 || (cohort->type & CT_REMOVED)) {
      continue;
    }
    cohort_count++;

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
			tag_count++;
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
			const Cohort* pr = gWindow->cohort_map[cohort->dep_parent];
			WRITE_U32_INTO(pr->global_number, cohort_buffer);
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
			rel_count += 1;
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
			reading_count++;
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
				if (tag->type & (T_WORDFORM | T_BASEFORM | T_DEPENDENCY | T_RELATION)) {
					continue;
				}
				if (unique_tags) {
					if (unique.find(tter) != unique.end()) {
						continue;
					}
					unique.insert(tter);
				}
				WRITE_TAG_INTO(tag, tag_buffer);
				tag_count++;
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
  if (window->flush_after) {
    flags |= BFW_FLUSH;
  }
  WRITE_U16_INTO(flags, header_buffer);

  WRITE_U16_INTO(tags_to_write.size(), header_buffer);
  for (auto& tag : tags_to_write) {
    WRITE_STR_INTO(tag->tag, header_buffer);
  }

  WRITE_U16_INTO(var_count, header_buffer);
  header_buffer += var_buffer;

  WRITE_STR_INTO(window->text, header_buffer);
  WRITE_STR_INTO(window->text_post, header_buffer);

  WRITE_U16_INTO(cohort_count, header_buffer);

  uint32_t total_size = header_buffer.size() + cohort_buffer.size();
  writeRaw(output, total_size);
  output.write(header_buffer.data(), header_buffer.size());
  output.write(cohort_buffer.data(), cohort_buffer.size());
  output.flush();
}
}
