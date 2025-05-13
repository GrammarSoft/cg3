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
#include "parser_helpers.hpp"
#include "process.hpp"
#include "pool.hpp"
#include "options.hpp"

namespace CG3 {

// Order is important - we want SingleWindows to be destroyed first, then Cohorts, then Readings
pool<Reading> pool_readings;
pool<Cohort> pool_cohorts;
pool<SingleWindow> pool_swindows;

GrammarApplicator::GrammarApplicator(std::ostream& ux_err)
  : ux_stderr(&ux_err)
  , ci_depths(6, 0)
{
	gWindow.reset(new Window(this));
}

GrammarApplicator::~GrammarApplicator() {
	if (owns_grammar) {
		delete grammar;
	}
	grammar = nullptr;
	ux_stderr = nullptr;

	for (auto rx : text_delimiters) {
		uregex_close(rx);
	}
}

void GrammarApplicator::resetIndexes() {
	for (auto& sv : index_readingSet_yes) {
		sv.clear();
	}
	for (auto& sv : index_readingSet_no) {
		sv.clear();
	}
	index_regexp_yes.clear();
	index_regexp_no.clear();
	index_icase_yes.clear();
	index_icase_no.clear();
}

void GrammarApplicator::setGrammar(Grammar* res) {
	grammar = res;
	tag_begin = addTag(STR_BEGINTAG);
	tag_end = addTag(STR_ENDTAG);
	tag_subst = addTag(STR_DUMMY);
	begintag = tag_begin->hash;
	endtag = tag_end->hash;
	substtag = tag_subst->hash;

	UString mp{ grammar->mapping_prefix };
	mprefix_key = addTag(UStringView{ u"_MPREFIX" })->hash;
	mprefix_value = addTag(mp)->hash;

	index_readingSet_yes.clear();
	index_readingSet_yes.resize(grammar->sets_list.size());
	index_readingSet_no.clear();
	index_readingSet_no.resize(grammar->sets_list.size());

	if (res->text_delimiters) {
		TagList theTags;
		trie_getTagList(res->text_delimiters->trie, theTags);
		trie_getTagList(res->text_delimiters->trie_special, theTags);
		for (auto& t : theTags) {
			UParseError pe;
			UErrorCode status = U_ZERO_ERROR;

			uint32_t flags = (t->type & T_CASE_INSENSITIVE) ? UREGEX_CASE_INSENSITIVE : 0;
			text_delimiters.push_back(uregex_open(t->tag.data(), SI32(t->tag.size()), flags, &pe, &status));
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse pattern %S - cannot continue!\n", u_errorName(status), t->tag.data());
				CG3Quit(1);
			}
		}
	}
}

void GrammarApplicator::setTextDelimiter(UString rx) {
	for (auto r : text_delimiters) {
		uregex_close(r);
	}
	text_delimiters.clear();

	if (rx.empty()) {
		return;
	}

	bool icase = false;
	if (rx.size() >= 3 && rx.front() == '/') {
		rx.erase(rx.begin());
		while (rx.back() == '/' || rx.back() == 'r' || rx.back() == 'i') {
			if (rx.back() == 'i') {
				icase = true;
			}
			else if (rx.back() == '/') {
				rx.pop_back();
				break;
			}
			rx.pop_back();
		}
	}

	UParseError pe;
	UErrorCode status = U_ZERO_ERROR;

	uint32_t flags = icase ? UREGEX_CASE_INSENSITIVE : 0;
	text_delimiters.push_back(uregex_open(rx.data(), SI32(rx.size()), flags, &pe, &status));
	if (status != U_ZERO_ERROR) {
		u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse pattern %S - cannot continue!\n", u_errorName(status), rx.data());
		CG3Quit(1);
	}
}

void GrammarApplicator::index() {
	if (!add_spacing) {
		ws[2] = '\n';
	}
	if (did_index) {
		return;
	}

	if (grammar->ordered) {
		ordered = true;
	}
	if (grammar->has_dep || dep_delimit) {
		parse_dep = true;
	}

	if (!grammar->before_sections.empty()) {
		uint32IntervalVector& m = runsections[-1];
		for (auto r : grammar->before_sections) {
			m.insert(r->number);
		}
	}

	if (!grammar->after_sections.empty()) {
		uint32IntervalVector& m = runsections[-2];
		for (auto r : grammar->after_sections) {
			m.insert(r->number);
		}
	}

	if (!grammar->null_section.empty()) {
		uint32IntervalVector& m = runsections[-3];
		for (auto r : grammar->null_section) {
			m.insert(r->number);
		}
	}

	if (sections.empty()) {
		int32_t smax = SI32(grammar->sections.size());
		for (int32_t i = 0; i < smax; i++) {
			for (auto r : grammar->rules) {
				if (r->section < 0 || r->section > i) {
					continue;
				}
				uint32IntervalVector& m = runsections[i];
				m.insert(r->number);
			}
		}
	}
	else {
		numsections = UI32(sections.size());
		for (uint32_t n = 0; n < numsections; n++) {
			for (uint32_t e = 0; e <= n; e++) {
				for (auto r : grammar->rules) {
					if (r->section != SI32(sections[e]) - 1) {
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
		for (auto iter : grammar->rule_by_number) {
			if (valid_rules.contains(iter->line)) {
				vr.push_back(iter->number);
			}
		}
		valid_rules = vr;
	}

	constexpr UChar local_utf_pattern[] = { ' ', '#', '%', 'u', '%', '0', '?', 'u', u'\u2192', '%', 'u', '%', '0', '?', 'u', 0 };
	constexpr UChar local_latin_pattern[] = { ' ', '#', '%', 'u', '%', '0', '?', 'u', '-', '>', '%', 'u', '%', '0', '?', 'u', 0 };

	span_pattern_utf = local_utf_pattern;
	span_pattern_latin = local_latin_pattern;

	auto w = UI8(floor(log10(hard_limit)) + 1);
	span_pattern_utf[6] = span_pattern_utf[13] = '0' + w;
	span_pattern_latin[6] = span_pattern_latin[14] = '0' + w;

	did_index = true;
}

Tag* GrammarApplicator::addTag(Tag* tag) {
	uint32_t hash = tag->rehash();
	uint32_t seed = 0;
	for (; seed < 10000; seed++) {
		uint32_t ih = hash + seed;
		Taguint32HashMap::iterator it;
		if ((it = grammar->single_tags.find(ih)) != grammar->single_tags.end()) {
			Tag* t = it->second;
			if (t == tag) {
				return tag;
			}
			if (t->tag == tag->tag) {
				hash += seed;
				delete tag;
				break;
			}
		}
		else {
			if (seed && verbosity_level > 0) {
				u_fprintf(ux_stderr, "Warning: Tag %S got hash seed %u.\n", tag->tag.data(), seed);
				u_fflush(ux_stderr);
			}
			tag->seed = seed;
			hash = tag->rehash();
			grammar->single_tags[hash] = tag;
			break;
		}
	}
	return grammar->single_tags[hash];
}

Tag* GrammarApplicator::addTag(const UChar* txt, bool vstr) {
	Taguint32HashMap::iterator it;
	uint32_t thash = hash_value(txt);
	if ((it = grammar->single_tags.find(thash)) != grammar->single_tags.end() && !it->second->tag.empty() && it->second->tag == txt) {
		return it->second;
	}

	Tag* tag = nullptr;
	if (vstr) {
		tag = ::CG3::parseTag(txt, 0, *this);
	}
	else {
		tag = new Tag();
		tag->parseTagRaw(txt, grammar);
		tag = addTag(tag);
	}

	bool reflow = false;
	if ((tag->type & T_REGEXP) && !is_textual(tag->tag)) {
		if (grammar->regex_tags.insert(tag->regexp).second) {
			for (auto& titer : grammar->single_tags) {
				if (titer.second->type & T_TEXTUAL) {
					continue;
				}
				for (auto iter : grammar->regex_tags) {
					UErrorCode status = U_ZERO_ERROR;
					uregex_setText(iter, titer.second->tag.data(), SI32(titer.second->tag.size()), &status);
					if (status == U_ZERO_ERROR) {
						if (uregex_find(iter, -1, &status)) {
							titer.second->type |= T_TEXTUAL;
							reflow = true;
						}
					}
				}
			}
		}
	}
	if ((tag->type & T_CASE_INSENSITIVE) && !is_textual(tag->tag)) {
		if (grammar->icase_tags.insert(tag).second) {
			for (auto& titer : grammar->single_tags) {
				if (titer.second->type & T_TEXTUAL) {
					continue;
				}
				for (auto iter : grammar->icase_tags) {
					if (ux_strCaseCompare(titer.second->tag, iter->tag)) {
						titer.second->type |= T_TEXTUAL;
						reflow = true;
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

Tag* GrammarApplicator::addTag(const UString& txt, bool vstr) {
	assert(txt.size() && "addTag() will not work with empty strings.");
	return addTag(txt.data(), vstr);
}

void GrammarApplicator::printTrace(std::ostream& output, uint32_t hit_by) {
	if (hit_by < grammar->rule_by_number.size()) {
		const Rule* r = grammar->rule_by_number[hit_by];
		u_fprintf(output, "%S", keywords[r->type].data());
		if (r->type == K_ADDRELATION || r->type == K_SETRELATION || r->type == K_REMRELATION || r->type == K_ADDRELATIONS || r->type == K_SETRELATIONS || r->type == K_REMRELATIONS) {
			u_fprintf(output, "(%S", r->maplist->getNonEmpty().begin()->first->tag.data());
			if (r->type == K_ADDRELATIONS || r->type == K_SETRELATIONS || r->type == K_REMRELATIONS) {
				u_fprintf(output, ",%S", r->sublist->getNonEmpty().begin()->first->tag.data());
			}
			u_fprintf(output, ")");
		}
		if (!trace_name_only || r->name.empty()) {
			u_fprintf(output, ":%u", r->line);
		}
		if (!r->name.empty()) {
			u_fputc(':', output);
			u_fprintf(output, "%S", r->name.data());
		}
	}
	else {
		uint32_t pass = std::numeric_limits<uint32_t>::max() - (hit_by);
		u_fprintf(output, "ENCL:%u", pass);
	}
}

void GrammarApplicator::printReading(const Reading* reading, std::ostream& output, size_t sub) {
	if (reading->noprint) {
		return;
	}

	if (reading->deleted) {
		if (!trace) {
			return;
		}
		u_fputc(';', output);
	}

	for (size_t i = 0; i < sub; ++i) {
		u_fputc('\t', output);
	}

	if (reading->baseform) {
		u_fprintf(output, "%S", grammar->single_tags.find(reading->baseform)->second->tag.data());
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
			pattern = span_pattern_latin.data();
			if (unicode_tags) {
				pattern = span_pattern_utf.data();
			}
			if (reading->parent->dep_parent == DEP_NO_PARENT) {
				u_fprintf_u(output, pattern,
				  reading->parent->parent->number,
				  reading->parent->local_number,
				  reading->parent->parent->number,
				  reading->parent->local_number);
			}
			else {
				u_fprintf_u(output, pattern,
				  reading->parent->parent->number,
				  reading->parent->local_number,
				  pr->parent->number,
				  pr->local_number);
			}
		}
	}

	if (print_ids || reading->parent->type & CT_RELATED) {
		u_fprintf(output, " ID:%u", reading->parent->global_number);
		if (!reading->parent->relations.empty()) {
			for (const auto& miter : reading->parent->relations) {
				for (auto siter : miter.second) {
					auto it = grammar->single_tags.find(miter.first);
					if (it == grammar->single_tags.end()) {
						it = grammar->single_tags.find(miter.first);
					}
					u_fprintf(output, " R:%S:%u", it->second->tag.data(), siter);
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

	u_fputc('\n', output);

	if (reading->next) {
		reading->next->deleted = reading->deleted;
		printReading(reading->next, output, sub + 1);
	}
}

void GrammarApplicator::printCohort(Cohort* cohort, std::ostream& output, bool profiling) {
	if (cohort->local_number == 0) {
		goto removed;
	}

	if (profiling && cohort == rule_target) {
		u_fprintf(output, "# RULE TARGET BEGIN\n");
	}

	if (!cohort->wblank.empty()) {
		u_fprintf(output, "%S", cohort->wblank.data());
		if (!ISNL(cohort->wblank.back())) {
			u_fputc('\n', output);
		}
	}

	if (cohort->type & CT_REMOVED) {
		if (!trace || trace_no_removed) {
			goto removed;
		}
		u_fputc(';', output);
		u_fputc(' ', output);
	}
	u_fprintf(output, "%S", cohort->wordform->tag.data());
	if (cohort->wread) {
		for (auto tter : cohort->wread->tags_list) {
			if (tter == cohort->wordform->hash) {
				continue;
			}
			const Tag* tag = grammar->single_tags[tter];
			u_fprintf(output, " %S", tag->tag.data());
		}
	}
	u_fputc('\n', output);

	if (!profiling) {
		cohort->unignoreAll();

		if (!split_mappings) {
			mergeMappings(*cohort);
		}
	}

	std::sort(cohort->readings.begin(), cohort->readings.end(), Reading::cmp_number);
	for (auto rter1 : cohort->readings) {
		printReading(rter1, output);
	}
	if (trace && !trace_no_removed) {
		std::sort(cohort->delayed.begin(), cohort->delayed.end(), Reading::cmp_number);
		for (auto rter3 : cohort->delayed) {
			printReading(rter3, output);
		}
		std::sort(cohort->deleted.begin(), cohort->deleted.end(), Reading::cmp_number);
		for (auto rter2 : cohort->deleted) {
			printReading(rter2, output);
		}
	}

removed:
	if (!cohort->text.empty() && cohort->text.find_first_not_of(ws) != UString::npos) {
		u_fprintf(output, "%S", cohort->text.data());
		if (!ISNL(cohort->text.back())) {
			u_fputc('\n', output);
		}
	}

	if (profiling && cohort == rule_target) {
		u_fprintf(output, "# RULE TARGET END\n");
	}
}

void GrammarApplicator::printSingleWindow(SingleWindow* window, std::ostream& output, bool profiling) {
	for (auto var : window->variables_output) {
		Tag* key = grammar->single_tags[var];
		auto iter = window->variables_set.find(var);
		if (iter != window->variables_set.end()) {
			if (iter->second != grammar->tag_any) {
				Tag* value = grammar->single_tags[iter->second];
				u_fprintf(output, "%S%S=%S>\n", STR_CMD_SETVAR.data(), key->tag.data(), value->tag.data());
			}
			else {
				u_fprintf(output, "%S%S>\n", STR_CMD_SETVAR.data(), key->tag.data());
			}
		}
		else {
			u_fprintf(output, "%S%S>\n", STR_CMD_REMVAR.data(), key->tag.data());
		}
	}

	if (!window->text.empty() && window->text.find_first_not_of(ws) != UString::npos) {
		u_fprintf(output, "%S", window->text.data());
		if (!ISNL(window->text.back())) {
			u_fputc('\n', output);
		}
	}

	for (auto& cohort : window->all_cohorts) {
		printCohort(cohort, output, profiling);
	}

	if (!window->text_post.empty() && window->text_post.find_first_not_of(ws) != UString::npos) {
		u_fprintf(output, "%S", window->text_post.data());
		if (!ISNL(window->text_post.back())) {
			u_fputc('\n', output);
		}
	}

	if (add_spacing) {
		u_fputc('\n', output);
	}
	if (window->flush_after) {
		u_fprintf(output, "%S\n", STR_CMD_FLUSH.data());
	}
	u_fflush(output);
}

void GrammarApplicator::pipeOutReading(const Reading* reading, std::ostream& output) {
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
		writeUTF8String(ss, grammar->single_tags.find(reading->baseform)->second->tag);
	}

	uint32_t cs = 0;
	for (auto tter : reading->tags_list) {
		if (tter == reading->baseform || tter == reading->parent->wordform->hash) {
			continue;
		}
		const Tag* tag = grammar->single_tags.find(tter)->second;
		if (tag->type & T_DEPENDENCY && has_dep) {
			continue;
		}
		++cs;
	}

	writeRaw(ss, cs);
	for (auto tter : reading->tags_list) {
		if (tter == reading->baseform || tter == reading->parent->wordform->hash) {
			continue;
		}
		const Tag* tag = grammar->single_tags.find(tter)->second;
		if (tag->type & T_DEPENDENCY && has_dep) {
			continue;
		}
		writeUTF8String(ss, tag->tag);
	}

	const auto& str = ss.str();
	cs = UI32(str.size());
	writeRaw(output, cs);
	output.write(str.data(), str.size());
}

void GrammarApplicator::pipeOutCohort(const Cohort* cohort, std::ostream& output) {
	std::ostringstream ss;

	writeRaw(ss, cohort->global_number);

	uint32_t flags = 0;
	if (!cohort->text.empty()) {
		flags |= (1 << 0);
	}
	if (has_dep && cohort->dep_parent != DEP_NO_PARENT) {
		flags |= (1 << 1);
	}
	writeRaw(ss, flags);

	if (has_dep && cohort->dep_parent != DEP_NO_PARENT) {
		writeRaw(ss, cohort->dep_parent);
	}

	writeUTF8String(ss, cohort->wordform->tag);

	uint32_t cs = UI32(cohort->readings.size());
	writeRaw(ss, cs);
	for (auto rter1 : cohort->readings) {
		pipeOutReading(rter1, ss);
	}
	if (!cohort->text.empty()) {
		writeUTF8String(ss, cohort->text);
	}

	const auto& str = ss.str();
	cs = UI32(str.size());
	writeRaw(output, cs);
	output.write(str.data(), str.size());
}

void GrammarApplicator::pipeOutSingleWindow(const SingleWindow& window, Process& output) {
	std::ostringstream ss;

	writeRaw(ss, window.number);

	auto cs = UI32(window.cohorts.size()) - 1;
	writeRaw(ss, cs);

	for (uint32_t c = 1; c < cs + 1; c++) {
		pipeOutCohort(window.cohorts[c], ss);
	}

	const auto& str = ss.str();
	cs = UI32(str.size());
	writeRaw(output, cs);
	output.write(str.data(), str.size());

	output.flush();
}

void GrammarApplicator::pipeInReading(Reading* reading, Process& input, bool force) {
	uint32_t cs = 0;
	readRaw(input, cs);
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: reading packet length %u\n", cs);
	}

	std::string buf(cs, 0);
	input.read(&buf[0], cs);
	std::istringstream ss(buf);

	uint32_t flags = 0;
	readRaw(ss, flags);
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: reading flags %u\n", flags);
	}

	// Not marked modified, so don't bother with the heavy lifting...
	if (!force && !(flags & (1 << 0))) {
		return;
	}

	reading->noprint = (flags & (1 << 1)) != 0;
	reading->deleted = (flags & (1 << 2)) != 0;

	if (flags & (1 << 3)) {
		UString str = readUTF8String(ss);
		if (str != grammar->single_tags.find(reading->baseform)->second->tag) {
			Tag* tag = addTag(str);
			reading->baseform = tag->hash;
		}
		if (debug_level > 1) {
			u_fprintf(ux_stderr, "DEBUG: reading baseform %S\n", str.data());
		}
	}
	else {
		reading->baseform = 0;
	}

	reading->tags_list.clear();
	reading->tags_list.push_back(reading->parent->wordform->hash);
	if (reading->baseform) {
		reading->tags_list.push_back(reading->baseform);
	}

	readRaw(ss, cs);
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: num tags %u\n", cs);
	}

	for (size_t i = 0; i < cs; ++i) {
		UString str = readUTF8String(ss);
		Tag* tag = addTag(str);
		reading->tags_list.push_back(tag->hash);
		if (debug_level > 1) {
			u_fprintf(ux_stderr, "DEBUG: tag %S\n", tag->tag.data());
		}
	}

	reflowReading(*reading);
}

void GrammarApplicator::pipeInCohort(Cohort* cohort, Process& input) {
	uint32_t cs = 0;
	readRaw(input, cs);
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: cohort packet length %u\n", cs);
	}

	readRaw(input, cs);
	if (cs != cohort->global_number) {
		u_fprintf(ux_stderr, "Error: External returned data for cohort %u but we expected cohort %u!\n", cs, cohort->global_number);
		CG3Quit(1);
	}
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: cohort number %u\n", cohort->global_number);
	}

	uint32_t flags = 0;
	readRaw(input, flags);
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: cohort flags %u\n", flags);
	}

	if (flags & (1 << 1)) {
		readRaw(input, cohort->dep_parent);
		if (debug_level > 1) {
			u_fprintf(ux_stderr, "DEBUG: cohort parent %u\n", cohort->dep_parent);
		}
	}

	bool force_readings = false;
	UString str = readUTF8String(input);
	if (str != cohort->wordform->tag) {
		Tag* tag = addTag(str);
		cohort->wordform = tag;
		force_readings = true;
		if (debug_level > 1) {
			u_fprintf(ux_stderr, "DEBUG: cohort wordform %S\n", tag->tag.data());
		}
	}

	readRaw(input, cs);
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: num readings %u\n", cs);
	}
	for (size_t i = 0; i < cs; ++i) {
		pipeInReading(cohort->readings[i], input, force_readings);
	}

	if (flags & (1 << 0)) {
		cohort->text = readUTF8String(input);
		if (debug_level > 1) {
			u_fprintf(ux_stderr, "DEBUG: cohort text %S\n", cohort->text.data());
		}
	}
}

void GrammarApplicator::pipeInSingleWindow(SingleWindow& window, Process& input) {
	uint32_t cs = 0;
	readRaw(input, cs);
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: window packet length %u\n", cs);
	}

	if (cs == 0) {
		return;
	}

	readRaw(input, cs);
	if (cs != window.number) {
		u_fprintf(ux_stderr, "Error: External returned data for window %u but we expected window %u!\n", cs, window.number);
		CG3Quit(1);
	}
	if (debug_level > 1) {
		u_fprintf(ux_stderr, "DEBUG: window number %u\n", window.number);
	}

	readRaw(input, cs);
	for (size_t i = 0; i < cs; ++i) {
		pipeInCohort(window.cohorts[i + 1], input);
	}
}

void GrammarApplicator::error(const char* str, const UChar* p) {
	(void)p;
	if (current_rule && current_rule->line) {
		constexpr UChar buf[] = { 'R', 'T', ' ', 'R', 'U', 'L', 'E', 0 };
		u_fprintf(ux_stderr, str, buf, current_rule->line, buf);
	}
	else {
		constexpr UChar buf[] = { 'R', 'T', ' ', 'I', 'N', 'P', 'U', 'T', 0 };
		u_fprintf(ux_stderr, str, buf, numLines, buf);
	}
}

void GrammarApplicator::error(const char* str, const char* s, const UChar* p) {
	(void)p;
	if (current_rule && current_rule->line) {
		constexpr UChar buf[] = { 'R', 'T', ' ', 'R', 'U', 'L', 'E', 0 };
		u_fprintf(ux_stderr, str, buf, s, current_rule->line, buf);
	}
	else {
		constexpr UChar buf[] = { 'R', 'T', ' ', 'I', 'N', 'P', 'U', 'T', 0 };
		u_fprintf(ux_stderr, str, buf, s, numLines, buf);
	}
}

void GrammarApplicator::error(const char* str, const UChar* s, const UChar* p) {
	(void)p;
	if (current_rule && current_rule->line) {
		constexpr UChar buf[] = { 'R', 'T', ' ', 'R', 'U', 'L', 'E', 0 };
		u_fprintf(ux_stderr, str, buf, s, current_rule->line, buf);
	}
	else {
		constexpr UChar buf[] = { 'R', 'T', ' ', 'I', 'N', 'P', 'U', 'T', 0 };
		u_fprintf(ux_stderr, str, buf, s, numLines, buf);
	}
}

void GrammarApplicator::error(const char* str, const char* s, const UChar* S, const UChar* p) {
	(void)p;
	if (current_rule && current_rule->line) {
		constexpr UChar buf[] = { 'R', 'T', ' ', 'R', 'U', 'L', 'E', 0 };
		u_fprintf(ux_stderr, str, buf, s, S, current_rule->line, buf);
	}
	else {
		constexpr UChar buf[] = { 'R', 'T', ' ', 'I', 'N', 'P', 'U', 'T', 0 };
		u_fprintf(ux_stderr, str, buf, s, S, numLines, buf);
	}
}

void GrammarApplicator::setOptions(UConverter* conv) {
	using namespace Options;
	bool needs_del = false;
	if (!conv) {
		needs_del = true;
		UErrorCode status;
		conv = ucnv_open(ucnv_getDefaultName(), &status);
	}

	if (options[ALWAYS_SPAN].doesOccur) {
		always_span = true;
	}
	unicode_tags = false;
	if (options[UNICODE_TAGS].doesOccur) {
		unicode_tags = true;
	}
	unique_tags = false;
	if (options[UNIQUE_TAGS].doesOccur) {
		unique_tags = true;
	}
	apply_mappings = true;
	if (options[NOMAPPINGS].doesOccur) {
		apply_mappings = false;
	}
	apply_corrections = true;
	if (options[NOCORRECTIONS].doesOccur) {
		apply_corrections = false;
	}
	no_before_sections = false;
	if (options[NOBEFORESECTIONS].doesOccur) {
		no_before_sections = true;
	}
	no_sections = false;
	if (options[NOSECTIONS].doesOccur) {
		no_sections = true;
	}
	no_after_sections = false;
	if (options[NOAFTERSECTIONS].doesOccur) {
		no_after_sections = true;
	}
	unsafe = false;
	if (options[UNSAFE].doesOccur) {
		unsafe = true;
	}
	if (options[ORDERED].doesOccur) {
		ordered = true;
	}
	if (options[TRACE].doesOccur) {
		trace = true;
		if (!options[TRACE].value.empty()) {
			GAppSetOpts_ranged(options[TRACE].value.c_str(), trace_rules, false);
		}
	}
	if (options[TRACE_NAME_ONLY].doesOccur) {
		trace = true;
		trace_name_only = true;
	}
	if (options[TRACE_NO_REMOVED].doesOccur) {
		trace = true;
		trace_no_removed = true;
	}
	if (options[TRACE_ENCL].doesOccur) {
		trace = true;
		trace_encl = true;
	}
	if (options[PIPE_DELETED].doesOccur) {
		pipe_deleted = true;
	}
	if (options[DRYRUN].doesOccur) {
		dry_run = true;
	}
	if (options[SINGLERUN].doesOccur) {
		section_max_count = 1;
	}
	if (options[MAXRUNS].doesOccur) {
		section_max_count = std::stoul(options[MAXRUNS].value);
	}
	if (options[SECTIONS].doesOccur) {
		GAppSetOpts_ranged(options[SECTIONS].value.c_str(), sections);
	}
	if (options[RULES].doesOccur) {
		GAppSetOpts_ranged(options[RULES].value.c_str(), valid_rules);
	}
	if (options[RULE].doesOccur) {
		if (options[RULE].value[0] >= '0' && options[RULE].value[0] <= '9') {
			valid_rules.push_back(std::stoi(options[RULE].value));
		}
		else {
			UErrorCode status = U_ZERO_ERROR;
			auto sn = options[RULE].value.size();
			UString buf(sn * 3, 0);
			ucnv_reset(conv);
			ucnv_toUChars(conv, &buf[0], SI32(sn * 3), options[RULE].value.c_str(), SI32(sn), &status);

			for (auto rule : grammar->rule_by_number) {
				if (rule->name == buf) {
					valid_rules.push_back(rule->number);
				}
			}
		}
	}
	if (options[DEBUG_RULES].doesOccur) {
		GAppSetOpts_ranged(options[DEBUG_RULES].value.c_str(), debug_rules, false);
	}
	if (options[VERBOSE].doesOccur) {
		if (!options[VERBOSE].value.empty()) {
			verbosity_level = std::stoul(options[VERBOSE].value);
		}
		else {
			verbosity_level = 1;
		}
	}
	if (options[DODEBUG].doesOccur) {
		if (!options[DODEBUG].value.empty()) {
			debug_level = std::stoul(options[DODEBUG].value);
		}
		else {
			debug_level = 1;
		}
		std::cerr << "Debug level set to " << debug_level << std::endl;
	}
	if (options[PRINT_IDS].doesOccur) {
		print_ids = true;
	}
	if (options[NUM_WINDOWS].doesOccur) {
		num_windows = std::stoul(options[NUM_WINDOWS].value);
	}
	if (options[SOFT_LIMIT].doesOccur) {
		soft_limit = std::stoul(options[SOFT_LIMIT].value);
	}
	if (options[HARD_LIMIT].doesOccur) {
		hard_limit = std::stoul(options[HARD_LIMIT].value);
	}
	if (options[TEXT_DELIMIT].doesOccur) {
		UString rx{ STR_TEXTDELIM_DEFAULT };
		if (!options[TEXT_DELIMIT].value.empty()) {
			UErrorCode status = U_ZERO_ERROR;
			auto sn = options[TEXT_DELIMIT].value.size();
			UString buf(sn * 3, 0);
			ucnv_reset(conv);
			auto len = ucnv_toUChars(conv, &buf[0], SI32(sn * 3), options[TEXT_DELIMIT].value.c_str(), SI32(sn), &status);
			rx.assign(buf.begin(), buf.begin() + len);
		}
		setTextDelimiter(rx);
	}
	if (options[DEP_DELIMIT].doesOccur) {
		if (!options[DEP_DELIMIT].value.empty()) {
			dep_delimit = std::stoul(options[DEP_DELIMIT].value);
		}
		else {
			dep_delimit = 10;
		}
		parse_dep = true;
	}
	if (options[DEP_ABSOLUTE].doesOccur) {
		dep_absolute = true;
	}
	if (options[DEP_ORIGINAL].doesOccur) {
		dep_original = true;
	}
	if (options[DEP_ALLOW_LOOPS].doesOccur) {
		dep_block_loops = false;
	}
	if (options[DEP_BLOCK_CROSSING].doesOccur) {
		dep_block_crossing = true;
	}
	if (options[MAGIC_READINGS].doesOccur) {
		allow_magic_readings = false;
	}
	if (options[NO_PASS_ORIGIN].doesOccur) {
		no_pass_origin = true;
	}
	if (options[SPLIT_MAPPINGS].doesOccur) {
		split_mappings = true;
	}
	if (options[SHOW_END_TAGS].doesOccur) {
		show_end_tags = true;
	}
	if (options[NO_BREAK].doesOccur) {
		add_spacing = false;
	}

	if (needs_del) {
		ucnv_close(conv);
	}
}
}
