/*
* Copyright (C) 2007-2013, GrammarSoft ApS
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

#include "GrammarApplicator.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"

namespace CG3 {

GrammarApplicator::GrammarApplicator(UFILE *ux_err) :
always_span(false),
apply_mappings(true),
apply_corrections(true),
no_before_sections(false),
no_sections(false),
no_after_sections(false),
trace(false),
trace_name_only(false),
trace_no_removed(false),
trace_encl(false),
allow_magic_readings(true),
no_pass_origin(false),
unsafe(false),
ordered(false),
show_end_tags(false),
unicode_tags(false),
owns_grammar(false),
input_eof(false),
seen_barrier(false),
dep_has_spanned(false),
dep_delimit(0),
dep_humanize(false),
dep_original(false),
dep_block_loops(true),
dep_block_crossing(false),
num_windows(2),
soft_limit(300),
hard_limit(500),
verbosity_level(0),
debug_level(0),
section_max_count(0),
has_dep(false),
dep_highest_seen(0),
gWindow(0),
has_relations(false),
grammar(0),
ux_stderr(ux_err),
numLines(0),
numWindows(0),
numCohorts(0),
numReadings(0),
did_index(false),
numsections(0),
ci_depths(5, 0),
match_single(0),
match_comp(0),
match_sub(0),
begintag(0),
endtag(0),
par_left_tag(0),
par_right_tag(0),
par_left_pos(0),
par_right_pos(0),
did_final_enclosure(false),
target(0),
mark(0),
attach_to(0),
unif_last_wordform(0),
unif_last_baseform(0),
unif_last_textual(0),
unif_sets_firstrun(false),
statistics(false)
{
	gWindow = new Window(this);
}

GrammarApplicator::~GrammarApplicator() {
	Taguint32HashMap::iterator iter_stag;
	for (iter_stag = single_tags.begin() ; iter_stag != single_tags.end() ; iter_stag++) {
		if (iter_stag->second && !(iter_stag->second->type & T_GRAMMAR)) {
			delete iter_stag->second;
			iter_stag->second = 0;
		}
	}

	foreach (externals_t, externals, ei, ei_end) {
		try {
			writeRaw(ei->second->in(), static_cast<uint32_t>(0));
			delete ei->second;
		}
		catch (...) {
			// We don't really care about errors since we're shutting down anyway.
		}
	}

	delete gWindow;

	if (owns_grammar) {
		delete grammar;
	}
	grammar = 0;
	ux_stderr = 0;
}

void GrammarApplicator::resetIndexes() {
	index_readingSet_yes.clear();
	index_readingSet_no.clear();
	index_regexp_yes.clear();
	index_regexp_no.clear();
	index_icase_yes.clear();
	index_icase_no.clear();
}

void GrammarApplicator::setGrammar(Grammar *res) {
	grammar = res;
	single_tags = grammar->single_tags;
	begintag = addTag(stringbits[S_BEGINTAG].getTerminatedBuffer())->hash;
	endtag = addTag(stringbits[S_ENDTAG].getTerminatedBuffer())->hash;
}

void GrammarApplicator::index() {
	if (did_index) {
		return;
	}

	if (!grammar->before_sections.empty()) {
		uint32IntervalVector& m = runsections[-1];
		const_foreach (RuleVector, grammar->before_sections, iter_rules, iter_rules_end) {
			const Rule *r = *iter_rules;
			m.insert(r->number);
		}
	}

	if (!grammar->after_sections.empty()) {
		uint32IntervalVector& m = runsections[-2];
		const_foreach (RuleVector, grammar->after_sections, iter_rules, iter_rules_end) {
			const Rule *r = *iter_rules;
			m.insert(r->number);
		}
	}

	if (!grammar->null_section.empty()) {
		uint32IntervalVector& m = runsections[-3];
		const_foreach (RuleVector, grammar->null_section, iter_rules, iter_rules_end) {
			const Rule *r = *iter_rules;
			m.insert(r->number);
		}
	}

	if (sections.empty()) {
		int32_t smax = (int32_t)grammar->sections.size();
		for (int32_t i=0 ; i < smax ; i++) {
			const_foreach (RuleVector, grammar->rules, iter_rules, iter_rules_end) {
				const Rule *r = *iter_rules;
				if (r->section < 0 || r->section > i) {
					continue;
				}
				uint32IntervalVector& m = runsections[i];
				m.insert(r->number);
			}
		}
	}
	else {
		numsections = sections.size();
		for (uint32_t n=0 ; n<numsections ; n++) {
			for (uint32_t e=0 ; e<=n ; e++) {
				const_foreach (RuleVector, grammar->rules, iter_rules, iter_rules_end) {
					const Rule *r = *iter_rules;
					if (r->section != (int32_t)sections[e]-1) {
						continue;
					}
					uint32IntervalVector& m = runsections[n];
					m.insert(r->number);
				}
			}
		}
	}

	if (!valid_rules.empty()) {
		uint32IntervalVector vr;
		const_foreach (RuleVector, grammar->rule_by_number, iter, iter_end) {
			if (valid_rules.contains((*iter)->line)) {
				vr.push_back((*iter)->number);
			}
		}
		valid_rules = vr;
	}

	did_index = true;
}

void GrammarApplicator::enableStatistics() {
	statistics = true;
}

void GrammarApplicator::disableStatistics() {
	statistics = false;
}

Tag *GrammarApplicator::addTag(const UChar *txt, bool vstr) {
	Taguint32HashMap::iterator it;
	uint32_t thash = hash_sdbm_uchar(txt);
	if ((it = single_tags.find(thash)) != single_tags.end() && !it->second->tag.empty() && u_strcmp(it->second->tag.c_str(), txt) == 0) {
		return it->second;
	}

	Tag *tag = new Tag();
	if (vstr) {
		tag->parseTag(txt, ux_stderr, grammar);
	}
	else {
		tag->parseTagRaw(txt, grammar);
	}
	uint32_t hash = tag->rehash();
	uint32_t seed = 0;
	for ( ; seed < 10000 ; seed++) {
		uint32_t ih = hash + seed;
		if ((it = single_tags.find(ih)) != single_tags.end()) {
			Tag *t = it->second;
			if (t->tag == tag->tag) {
				hash += seed;
				delete tag;
				break;
			}
		}
		else {
			if (seed && verbosity_level > 0) {
				u_fprintf(ux_stderr, "Warning: Tag %S got hash seed %u.\n", txt, seed);
				u_fflush(ux_stderr);
			}
			tag->seed = seed;
			hash = tag->rehash();
			single_tags[hash] = tag;
			break;
		}
	}
	tag = single_tags[hash];
	bool reflow = false;
	if ((tag->type & T_REGEXP) && tag->tag[0] == '/') {
		if (grammar->regex_tags.insert(tag->regexp).second) {
			foreach (Taguint32HashMap, single_tags, titer, titer_end) {
				if (titer->second->type & T_TEXTUAL) {
					continue;
				}
				foreach (Grammar::regex_tags_t, grammar->regex_tags, iter, iter_end) {
					UErrorCode status = U_ZERO_ERROR;
					uregex_setText(*iter, titer->second->tag.c_str(), titer->second->tag.length(), &status);
					if (status == U_ZERO_ERROR) {
						if (uregex_find(*iter, -1, &status)) {
							titer->second->type |= T_TEXTUAL;
							reflow = true;
						}
					}
				}
			}
		}
	}
	if ((tag->type & T_CASE_INSENSITIVE) && tag->tag[0] == '/') {
		if (grammar->icase_tags.insert(tag).second) {
			foreach (Taguint32HashMap, single_tags, titer, titer_end) {
				if (titer->second->type & T_TEXTUAL) {
					continue;
				}
				foreach (Grammar::icase_tags_t, grammar->icase_tags, iter, iter_end) {
					UErrorCode status = U_ZERO_ERROR;
					if (u_strCaseCompare(titer->second->tag.c_str(), titer->second->tag.length(), (*iter)->tag.c_str(), (*iter)->tag.length(), U_FOLD_CASE_DEFAULT, &status) == 0) {
						titer->second->type |= T_TEXTUAL;
						reflow = true;
					}
					if (status != U_ZERO_ERROR) {
						u_fprintf(ux_stderr, "Error: u_strCaseCompare(addTag) returned %s - cannot continue!\n", u_errorName(status));
						CG3Quit(1);
					}
				}
			}
		}
	}
	if (reflow) {
		reflowTextuals();
	}
	return tag;
}


Tag *GrammarApplicator::addTag(const UString& txt, bool vstr) {
	assert(txt.length() && "addTag() will not work with empty strings.");
	return addTag(txt.c_str(), vstr);
}

void GrammarApplicator::printTrace(UFILE *output, uint32_t hit_by) {
	if (hit_by < grammar->rule_by_number.size()) {
		const Rule *r = grammar->rule_by_number[hit_by];
		u_fprintf(output, "%S", keywords[r->type].getTerminatedBuffer());
		if (r->type == K_ADDRELATION || r->type == K_SETRELATION || r->type == K_REMRELATION
		|| r->type == K_ADDRELATIONS || r->type == K_SETRELATIONS || r->type == K_REMRELATIONS
			) {
				u_fprintf(output, "(%S", r->maplist->tags_list.front().getTag()->tag.c_str());
				if (r->type == K_ADDRELATIONS || r->type == K_SETRELATIONS || r->type == K_REMRELATIONS) {
					u_fprintf(output, ",%S", r->sublist->tags_list.front().getTag()->tag.c_str());
				}
				u_fprintf(output, ")");
		}
		if (!trace_name_only || !r->name) {
			u_fprintf(output, ":%u", r->line);
		}
		if (r->name) {
			u_fputc(':', output);
			u_fprintf(output, "%S", r->name);
		}
	}
	else {
		uint32_t pass = std::numeric_limits<uint32_t>::max() - (hit_by);
		u_fprintf(output, "ENCL:%u", pass);
	}
}

void GrammarApplicator::printReading(const Reading *reading, UFILE *output, size_t sub) {
	if (reading->noprint) {
		return;
	}

	if (reading->deleted) {
		u_fputc(';', output);
	}

	for (size_t i=0 ; i<sub ; ++i) {
		u_fputc('\t', output);
	}

	if (reading->baseform) {
		u_fprintf(output, "%S", single_tags.find(reading->baseform)->second->tag.c_str());
		u_fputc(' ', output);
	}

	const_foreach (uint32List, reading->tags_list, tter, tter_end) {
		if ((!show_end_tags && *tter == endtag) || *tter == begintag) {
			continue;
		}
		if (*tter == reading->baseform || *tter == reading->wordform) {
			continue;
		}
		const Tag *tag = single_tags[*tter];
		if (tag->type & T_DEPENDENCY && has_dep && !dep_original) {
			continue;
		}
		if (tag->type & T_RELATION && has_relations) {
			continue;
		}
		u_fprintf(output, "%S ", tag->tag.c_str());
	}

	if (has_dep && !(reading->parent->type & CT_REMOVED)) {
		if (!reading->parent->dep_self) {
			reading->parent->dep_self = reading->parent->global_number;
		}
		const Cohort *pr = 0;
		pr = reading->parent;
		if (reading->parent->dep_parent != std::numeric_limits<uint32_t>::max()) {
			if (reading->parent->dep_parent == 0) {
				pr = reading->parent->parent->cohorts[0];
			}
			else if (reading->parent->parent->parent->cohort_map.find(reading->parent->dep_parent) != reading->parent->parent->parent->cohort_map.end()) {
				pr = reading->parent->parent->parent->cohort_map[reading->parent->dep_parent];
			}
		}
		if (dep_humanize) {
			const UChar *pattern = 0;
			if (unicode_tags) {
				const UChar local_pattern[] = {'#', 'w', '%', 'u', ',', 'c', '%', 'u', L'\u2192', 'w', '%', 'u', ',', 'c', '%', 'u', ' ', 0};
				pattern = local_pattern;
			}
			else {
				const UChar local_pattern[] = {'#', 'w', '%', 'u', ',', 'c', '%', 'u', '-', '>', 'w', '%', 'u', ',', 'c', '%', 'u', ' ', 0};
				pattern = local_pattern;
			}
			u_fprintf_u(output, pattern,
				pr->parent->number,
				reading->parent->local_number,
				reading->parent->parent->number,
				pr->local_number);
		}

		const UChar local_utf_pattern[] = {'#', '%', 'u', L'\u2192', '%', 'u', ' ', 0};
		const UChar local_latin_pattern[] = {'#', '%', 'u', '-', '>', '%', 'u', ' ', 0};
		const UChar *pattern = local_latin_pattern;
		if (unicode_tags) {
			pattern = local_utf_pattern;
		}
		if (!dep_has_spanned) {
			u_fprintf_u(output, pattern,
				reading->parent->local_number,
				pr->local_number);
		}
		else {
			if (reading->parent->dep_parent == std::numeric_limits<uint32_t>::max()) {
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
		u_fprintf(output, "ID:%u ", reading->parent->global_number);
		if (!reading->parent->relations.empty()) {
			foreach (RelationCtn, reading->parent->relations, miter, miter_end) {
				foreach (uint32Set, miter->second, siter, siter_end) {
					u_fprintf(output, "R:%S:%u ", grammar->single_tags.find(miter->first)->second->tag.c_str(), *siter);
				}
			}
		}
	}

	if (trace) {
		const_foreach (uint32Vector, reading->hit_by, iter_hb, iter_hb_end) {
			printTrace(output, *iter_hb);
			u_fputc(' ', output);
		}
	}

	u_fputc('\n', output);

	if (reading->next) {
		reading->next->deleted = reading->deleted;
		printReading(reading->next, output, sub+1);
	}
}

void GrammarApplicator::printCohort(Cohort *cohort, UFILE *output) {
	const UChar ws[] = {' ', '\t', 0};

	if (cohort->local_number == 0) {
		goto removed;
	}

	if (cohort->type & CT_REMOVED) {
		if (!trace || trace_no_removed) {
			goto removed;
		}
		u_fputc(';', output);
		u_fputc(' ', output);
	}
	u_fprintf(output, "%S", single_tags.find(cohort->wordform)->second->tag.c_str());
	if (cohort->wread) {
		const_foreach (uint32List, cohort->wread->tags_list, tter, tter_end) {
			if (*tter == cohort->wread->wordform) {
				continue;
			}
			const Tag *tag = single_tags[*tter];
			u_fprintf(output, " %S", tag->tag.c_str());
		}
	}
	u_fputc('\n', output);

	mergeMappings(*cohort);

	foreach (ReadingList, cohort->readings, rter1, rter1_end) {
		printReading(*rter1, output);
	}
	if (trace && !trace_no_removed) {
		foreach (ReadingList, cohort->delayed, rter3, rter3_end) {
			printReading(*rter3, output);
		}
		foreach (ReadingList, cohort->deleted, rter2, rter2_end) {
			printReading(*rter2, output);
		}
	}

removed:
	if (!cohort->text.empty() && cohort->text.find_first_not_of(ws) != UString::npos) {
		u_fprintf(output, "%S", cohort->text.c_str());
		if (!ISNL(cohort->text[cohort->text.length()-1])) {
			u_fputc('\n', output);
		}
	}

	foreach (CohortVector, cohort->removed, iter, iter_end) {
		printCohort(*iter, output);
	}
}

void GrammarApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {
	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.c_str());
		if (!ISNL(window->text[window->text.length()-1])) {
			u_fputc('\n', output);
		}
	}

	uint32_t cs = (uint32_t)window->cohorts.size();
	for (uint32_t c=0 ; c < cs ; c++) {
		Cohort *cohort = window->cohorts[c];
		printCohort(cohort, output);
	}
	u_fputc('\n', output);
	u_fflush(output);
}

void GrammarApplicator::pipeOutReading(const Reading *reading, std::ostream& output) {
	std::ostringstream ss;

	uint32_t flags = 0;

	if (reading->noprint) {
		flags |= (1 << 1);
	}
	if (reading->deleted) {
		flags |= (1 << 2);
	}
	if (reading->baseform) {
		flags |= (1 << 3);
	}

	writeRaw(ss, flags);

	if (reading->baseform) {
		writeUTF8String(ss, single_tags.find(reading->baseform)->second->tag);
	}

	uint32_t cs = 0;
	const_foreach (uint32List, reading->tags_list, tter, tter_end) {
		if (*tter == reading->baseform || *tter == reading->wordform) {
			continue;
		}
		const Tag *tag = single_tags.find(*tter)->second;
		if (tag->type & T_DEPENDENCY && has_dep) {
			continue;
		}
		++cs;
	}

	writeRaw(ss, cs);
	const_foreach (uint32List, reading->tags_list, tter, tter_end) {
		if (*tter == reading->baseform || *tter == reading->wordform) {
			continue;
		}
		const Tag *tag = single_tags.find(*tter)->second;
		if (tag->type & T_DEPENDENCY && has_dep) {
			continue;
		}
		writeUTF8String(ss, tag->tag);
	}

	std::string str = ss.str();
	cs = str.length();
	writeRaw(output, cs);
	output.write(str.c_str(), str.length());
}

void GrammarApplicator::pipeOutCohort(const Cohort *cohort, std::ostream& output) {
	std::ostringstream ss;

	writeRaw(ss, cohort->global_number);

	uint32_t flags = 0;
	if (!cohort->text.empty()) {
		flags |= (1 << 0);
	}
	if (has_dep && cohort->dep_parent != std::numeric_limits<uint32_t>::max()) {
		flags |= (1 << 1);
	}
	writeRaw(ss, flags);

	if (has_dep && cohort->dep_parent != std::numeric_limits<uint32_t>::max()) {
		writeRaw(ss, cohort->dep_parent);
	}

	writeUTF8String(ss, single_tags.find(cohort->wordform)->second->tag);

	uint32_t cs = cohort->readings.size();
	writeRaw(ss, cs);
	const_foreach (ReadingList, cohort->readings, rter1, rter1_end) {
		pipeOutReading(*rter1, ss);
	}
	if (!cohort->text.empty()) {
		writeUTF8String(ss, cohort->text);
	}

	std::string str = ss.str();
	cs = str.length();
	writeRaw(output, cs);
	output.write(str.c_str(), str.length());
}

void GrammarApplicator::pipeOutSingleWindow(const SingleWindow& window, std::ostream& output) {
	std::ostringstream ss;

	writeRaw(ss, window.number);

	uint32_t cs = (uint32_t)window.cohorts.size()-1;
	writeRaw(ss, cs);

	for (uint32_t c=1 ; c < cs+1 ; c++) {
		pipeOutCohort(window.cohorts[c], ss);
	}

	std::string str = ss.str();
	cs = str.length();
	writeRaw(output, cs);
	output.write(str.c_str(), str.length());

	output << std::flush;
}

void GrammarApplicator::pipeInReading(Reading *reading, std::istream& input, bool force) {
	uint32_t cs = 0;
	readRaw(input, cs);
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: reading packet length %u\n", cs);

	std::string buf(cs, 0);
	input.read(&buf[0], cs);
	std::istringstream ss(buf);

	uint32_t flags = 0;
	readRaw(ss, flags);
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: reading flags %u\n", flags);

	// Not marked modified, so don't bother with the heavy lifting...
	if (!force && !(flags & (1 << 0))) {
		return;
	}

	reading->noprint = (flags & (1 << 1)) != 0;
	reading->deleted = (flags & (1 << 2)) != 0;

	if (flags & (1 << 3)) {
		UString str = readUTF8String(ss);
		if (str != single_tags.find(reading->baseform)->second->tag) {
			Tag *tag = addTag(str);
			reading->baseform = tag->hash;
		}
		if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: reading baseform %S\n", str.c_str());
	}
	else {
		reading->baseform = 0;
	}

	reading->tags_list.clear();
	reading->tags_list.push_back(reading->parent->wordform);
	if (reading->baseform) {
		reading->tags_list.push_back(reading->baseform);
	}

	readRaw(ss, cs);
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: num tags %u\n", cs);

	for (size_t i=0 ; i<cs ; ++i) {
		UString str = readUTF8String(ss);
		Tag *tag = addTag(str);
		reading->tags_list.push_back(tag->hash);
		if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: tag %S\n", tag->tag.c_str());
	}

	reflowReading(*reading);
}

void GrammarApplicator::pipeInCohort(Cohort *cohort, std::istream& input) {
	uint32_t cs = 0;
	readRaw(input, cs);
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: cohort packet length %u\n", cs);

	readRaw(input, cs);
	if (cs != cohort->global_number) {
		u_fprintf(ux_stderr, "Error: External returned data for cohort %u but we expected cohort %u!\n", cs, cohort->global_number);
		CG3Quit(1);
	}
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: cohort number %u\n", cohort->global_number);

	uint32_t flags = 0;
	readRaw(input, flags);
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: cohort flags %u\n", flags);

	if (flags & (1 << 1)) {
		readRaw(input, cohort->dep_parent);
		if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: cohort parent %u\n", cohort->dep_parent);
	}

	bool force_readings = false;
	UString str = readUTF8String(input);
	if (str != single_tags.find(cohort->wordform)->second->tag) {
		Tag *tag = addTag(str);
		cohort->wordform = tag->hash;
		force_readings = true;
		if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: cohort wordform %S\n", tag->tag.c_str());
	}

	readRaw(input, cs);
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: num readings %u\n", cs);
	for (size_t i=0 ; i<cs ; ++i) {
		pipeInReading(cohort->readings[i], input, force_readings);
	}

	if (flags & (1 << 0)) {
		cohort->text = readUTF8String(input);
		if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: cohort text %S\n", cohort->text.c_str());
	}
}

void GrammarApplicator::pipeInSingleWindow(SingleWindow& window, std::istream& input) {
	uint32_t cs = 0;
	readRaw(input, cs);
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: window packet length %u\n", cs);

	if (cs == 0) {
		return;
	}

	readRaw(input, cs);
	if (cs != window.number) {
		u_fprintf(ux_stderr, "Error: External returned data for window %u but we expected window %u!\n", cs, window.number);
		CG3Quit(1);
	}
	if (debug_level > 1) u_fprintf(ux_stderr, "DEBUG: window number %u\n", window.number);

	readRaw(input, cs);
	for (size_t i=0 ; i<cs ; ++i) {
		pipeInCohort(window.cohorts[i+1], input);
	}
}

}
