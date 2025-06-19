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

#include "stdafx.hpp"
#include "Grammar.hpp"
#include "TextualParser.hpp"
#include "BinaryGrammar.hpp"
#include "GrammarApplicator.hpp"
#include "MweSplitApplicator.hpp"
#include "FormatConverter.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "version.hpp"
#include "streambuf.hpp"
using namespace CG3;

#include "cg3.h"

namespace {
std::unique_ptr<std::istream> ux_stdin;
std::unique_ptr<std::ostream> ux_stdout;
std::unique_ptr<std::ostream> ux_stderr;

bool did_init = false;
bool did_cleanup = false;
}

cg3_status cg3_init(FILE* in, FILE* out, FILE* err) {
	if (did_cleanup) {
		fprintf(err, "CG3 Error: Cannot init after cleanup!\n");
		return CG3_ERROR;
	}
	if (did_init) {
		return CG3_SUCCESS;
	}

	UErrorCode status = U_ZERO_ERROR;
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		fprintf(err, "CG3 Error: Cannot initialize ICU. Status = %s\n", u_errorName(status));
		return CG3_ERROR;
	}
	status = U_ZERO_ERROR;

	ucnv_setDefaultName("UTF-8");

	uloc_setDefault("en_US_POSIX", &status);
	if (U_FAILURE(status)) {
		fprintf(err, "CG3 Error: Failed to set default locale. Status = %s\n", u_errorName(status));
		return CG3_ERROR;
	}
	status = U_ZERO_ERROR;

	ux_stdin.reset(new std::istream(new cstreambuf(in)));
	if (!ux_stdin) {
		fprintf(err, "CG3 Error: The input stream could not be inited.\n");
		return CG3_ERROR;
	}

	ux_stdout.reset(new std::ostream(new cstreambuf(out)));
	if (!ux_stdout) {
		fprintf(err, "CG3 Error: The output stream could not be inited.\n");
		return CG3_ERROR;
	}

	ux_stderr.reset(new std::ostream(new cstreambuf(err)));
	if (!ux_stderr) {
		fprintf(err, "CG3 Error: The error stream could not be inited.\n");
		return CG3_ERROR;
	}

	did_init = true;
	return CG3_SUCCESS;
}

cg3_status cg3_cleanup(void) {
	if (!did_init) {
		u_fprintf(ux_stderr, "CG3 Error: Must init before cleanup!\n");
		return CG3_ERROR;
	}
	if (did_cleanup) {
		return CG3_SUCCESS;
	}

	ux_stdin.reset();
	ux_stdout.reset();
	ux_stderr.reset();

	u_cleanup();

	did_cleanup = true;
	return CG3_SUCCESS;
}

cg3_grammar* cg3_grammar_load(const char* filename) {
	std::ifstream input(filename, std::ios::binary);
	if (!input) {
		u_fprintf(ux_stderr, "CG3 Error: Error opening %s for reading!\n", filename);
		return 0;
	}
	if (!input.read(&cbuffers[0][0], 4)) {
		u_fprintf(ux_stderr, "CG3 Error: Error reading first 4 bytes from grammar!\n");
		return 0;
	}
	input.close();

	auto grammar = new Grammar;
	grammar->ux_stderr = ux_stderr.get();
	grammar->ux_stdout = ux_stdout.get();

	std::unique_ptr<IGrammarParser> parser;

	if (is_cg3b(cbuffers[0])) {
		parser.reset(new BinaryGrammar(*grammar, *ux_stderr));
	}
	else {
		parser.reset(new TextualParser(*grammar, *ux_stderr));
	}
	if (parser->parse_grammar(filename)) {
		u_fprintf(ux_stderr, "CG3 Error: Grammar could not be parsed!\n");
		return 0;
	}

	grammar->reindex();

	return grammar;
}

cg3_grammar* cg3_grammar_load_buffer(const char* buffer, size_t length) {
	if (length == 0) {
		length = strlen(buffer);
	}
	if (length < 4) {
		u_fprintf(ux_stderr, "CG3 Error: Error reading first 4 bytes from grammar!\n");
		return 0;
	}

	auto grammar = new Grammar;
	grammar->ux_stderr = ux_stderr.get();
	grammar->ux_stdout = ux_stdout.get();

	std::unique_ptr<IGrammarParser> parser;

	if (is_cg3b(buffer)) {
		parser.reset(new BinaryGrammar(*grammar, *ux_stderr));
	}
	else {
		parser.reset(new TextualParser(*grammar, *ux_stderr));
	}
	if (parser->parse_grammar(buffer, length)) {
		u_fprintf(ux_stderr, "CG3 Error: Grammar could not be parsed!\n");
		return 0;
	}

	grammar->reindex();

	return grammar;
}

void cg3_grammar_free(cg3_grammar* grammar_) {
	auto grammar = static_cast<Grammar*>(grammar_);
	delete grammar;
}

cg3_sformat cg3_detect_sformat(const char* filename) {
	std::ifstream input(filename, std::ios::binary);
	if (!input) {
		u_fprintf(ux_stderr, "CG3 Error: Error opening %s for reading!\n", filename);
		return CG3SF_INVALID;
	}

	std::string buf8 = read_utf8(input);

	return detectFormat(buf8);
}

cg3_sformat cg3_detect_sformat_buffer(const char* buffer, size_t length) {
	if (length == 0) {
		length = strlen(buffer);
	}
	return detectFormat(std::string_view(buffer, length));
}

cg3_sconverter* cg3_sconverter_create(cg3_sformat fmt_in, cg3_sformat fmt_out) {
	FormatConverter* applicator = new FormatConverter(*ux_stderr);
	applicator->is_conv = true;
	applicator->trace = true;
	applicator->verbosity_level = 0;
	applicator->fmt_input = fmt_in;
	applicator->fmt_output = fmt_out;
	return applicator;
}

void cg3_sconverter_free(cg3_sconverter* converter_) {
	FormatConverter* fc_applicator = static_cast<FormatConverter*>(converter_);
	delete fc_applicator; // Delete as the concrete type FormatConverter
}

void cg3_sconverter_run_fns(cg3_sconverter* converter_, const char* input, const char* output) {
	FormatConverter* applicator = static_cast<FormatConverter*>(converter_);
	std::ifstream is(input, std::ios::binary);
	std::ofstream os(output, std::ios::binary);
	applicator->runGrammarOnText(is, os);
}

cg3_applicator* cg3_applicator_create(cg3_grammar* grammar_) {
	auto grammar = static_cast<Grammar*>(grammar_);
	GrammarApplicator* applicator = new GrammarApplicator(*ux_stderr);
	applicator->setGrammar(grammar);
	applicator->index();
	return applicator;
}

cg3_mwesplitapplicator* cg3_mwesplitapplicator_create() {
	auto mwe = new MweSplitApplicator(*ux_stderr);
	return static_cast<GrammarApplicator*>(mwe);
}

void cg3_applicator_setflags(cg3_applicator* applicator_, uint32_t flags) {
	GrammarApplicator* applicator = static_cast<GrammarApplicator*>(applicator_);
	applicator->ordered = (flags & CG3F_ORDERED) != 0;
	applicator->unsafe = (flags & CG3F_UNSAFE) != 0;
	applicator->apply_mappings = (flags & CG3F_NO_MAPPINGS) == 0;
	applicator->apply_corrections = (flags & CG3F_NO_CORRECTIONS) == 0;
	applicator->no_before_sections = (flags & CG3F_NO_BEFORE_SECTIONS) != 0;
	applicator->no_sections = (flags & CG3F_NO_SECTIONS) != 0;
	applicator->no_after_sections = (flags & CG3F_NO_AFTER_SECTIONS) != 0;
	applicator->trace = (flags & CG3F_TRACE) != 0;
	applicator->section_max_count = (flags & CG3F_SINGLE_RUN) != 0;
	applicator->always_span = (flags & CG3F_ALWAYS_SPAN) != 0;
	applicator->dep_block_loops = (flags & CG3F_DEP_ALLOW_LOOPS) == 0;
	applicator->dep_block_crossing = (flags & CG3F_DEP_NO_CROSSING) != 0;
	applicator->no_pass_origin = (flags & CG3F_NO_PASS_ORIGIN) != 0;
}

void cg3_applicator_setoption(cg3_applicator* applicator_, cg3_option option, void* value_) {
	GrammarApplicator* applicator = static_cast<GrammarApplicator*>(applicator_);
	switch (option) {
	case CG3O_SECTIONS: {
		auto value = static_cast<uint32_t*>(value_);
		for (uint32_t i = 1; i <= *value; ++i) {
			applicator->sections.push_back(i);
		}
		break;
	}
	case CG3O_SECTIONS_TEXT: {
		auto value = static_cast<const char*>(value_);
		GAppSetOpts_ranged(value, applicator->sections);
		break;
	}
	default:
		u_fprintf(ux_stderr, "CG3 Error: Unknown option type!\n");
	}
}

void cg3_applicator_free(cg3_applicator* applicator_) {
	GrammarApplicator* applicator = static_cast<GrammarApplicator*>(applicator_);
	delete applicator;
}

void cg3_run_grammar_on_text(cg3_applicator* applicator_, std_istream* is_, std_ostream* os_) {
	GrammarApplicator* applicator = static_cast<GrammarApplicator*>(applicator_);
	std::istream* is = static_cast<std::istream*>(is_);
	std::ostream* os = static_cast<std::ostream*>(os_);
	applicator->runGrammarOnText(*is, *os);
}

void cg3_run_grammar_on_text_fns(cg3_applicator* applicator_, const char* input, const char* output) {
	GrammarApplicator* applicator = static_cast<GrammarApplicator*>(applicator_);
	std::ifstream is(input, std::ios::binary);
	std::ofstream os(output, std::ios::binary);
	applicator->runGrammarOnText(is, os);
}

cg3_sentence* cg3_sentence_new(cg3_applicator* applicator_) {
	GrammarApplicator* applicator = static_cast<GrammarApplicator*>(applicator_);
	SingleWindow* current = applicator->gWindow->allocSingleWindow();
	applicator->initEmptySingleWindow(current);
	return current;
}

#ifndef _MSC_VER
	#pragma GCC visibility push(hidden)
#endif
inline Tag* _tag_copy(GrammarApplicator* to, Tag* t) {
	Tag* nt = to->addTag(t->tag);
	return nt;
}

inline Tag* _tag_copy(GrammarApplicator* from, GrammarApplicator* to, uint32_t hash) {
	Tag* t = from->grammar->single_tags[hash];
	return _tag_copy(to, t);
}

inline Reading* _reading_copy(Cohort* nc, Reading* oldr, bool is_sub = false) {
	auto nr = alloc_reading(nc);
	auto ga = nc->parent->parent->parent;
	insert_if_exists(nr->parent->possible_sets, ga->grammar->sets_any);
	ga->addTagToReading(*nr, nc->wordform);
	TagList mappings;
	for (auto tag : oldr->tags_list) {
		Tag* nt = _tag_copy(oldr->parent->parent->parent->parent, nc->parent->parent->parent, tag);
		if (nt->type & T_MAPPING || nt->tag[0] == ga->grammar->mapping_prefix) {
			mappings.push_back(nt);
		}
		else {
			ga->addTagToReading(*nr, nt);
		}
	}
	if (!mappings.empty() && (!is_sub || mappings.size() == 1)) {
		ga->splitMappings(mappings, *nc, *nr, true);
	}
	if (oldr->next) {
		nr->next = _reading_copy(nc, oldr->next, true);
	}
	return nr;
}

inline Cohort* _cohort_copy(SingleWindow* ns, Cohort* oc) {
	Cohort* nc = alloc_cohort(ns);
	nc->wordform = _tag_copy(ns->parent->parent, oc->wordform);
	for (auto r : oc->readings) {
		auto nr = _reading_copy(nc, r);
		nc->appendReading(nr);
	}
	return nc;
}
#ifndef _MSC_VER
	#pragma GCC visibility pop
#endif

cg3_sentence* cg3_sentence_copy(cg3_sentence* sentence_, cg3_applicator* applicator_) {
	GrammarApplicator* applicator = static_cast<GrammarApplicator*>(applicator_);
	SingleWindow* sentence = static_cast<SingleWindow*>(sentence_);

	SingleWindow* current = applicator->gWindow->allocSingleWindow();
	applicator->initEmptySingleWindow(current);
	current->has_enclosures = sentence->has_enclosures;
	current->text = sentence->text;
	for (auto c : sentence->cohorts) {
		Cohort* nc = _cohort_copy(current, c);
		current->appendCohort(nc);
	}
	return current;
}

void cg3_sentence_runrules(cg3_applicator* applicator_, cg3_sentence* sentence_) {
	GrammarApplicator* applicator = static_cast<GrammarApplicator*>(applicator_);
	SingleWindow* sentence = static_cast<SingleWindow*>(sentence_);
	applicator->gWindow->current = sentence;
	applicator->runGrammarOnWindow();
	applicator->gWindow->current = nullptr;
}

size_t cg3_sentence_numcohorts(cg3_sentence* sentence_) {
	SingleWindow* sentence = static_cast<SingleWindow*>(sentence_);
	return sentence->cohorts.size();
}

cg3_cohort* cg3_sentence_getcohort(cg3_sentence* sentence_, size_t which) {
	SingleWindow* sentence = static_cast<SingleWindow*>(sentence_);
	return sentence->cohorts[which];
}

void cg3_sentence_free(cg3_sentence* sentence_) {
	SingleWindow* sentence = static_cast<SingleWindow*>(sentence_);
	free_swindow(sentence);
}

void cg3_sentence_addcohort(cg3_sentence* sentence_, cg3_cohort* cohort_) {
	SingleWindow* sentence = static_cast<SingleWindow*>(sentence_);
	auto cohort = static_cast<Cohort*>(cohort_);
	sentence->appendCohort(cohort);
}

cg3_cohort* cg3_cohort_create(cg3_sentence* sentence_) {
	SingleWindow* sentence = static_cast<SingleWindow*>(sentence_);
	auto cohort = alloc_cohort(sentence);
	cohort->global_number = sentence->parent->cohort_counter++;
	return cohort;
}

void cg3_cohort_setwordform(cg3_cohort* cohort_, cg3_tag* tag_) {
	auto cohort = static_cast<Cohort*>(cohort_);
	auto tag = static_cast<Tag*>(tag_);
	cohort->wordform = tag;
}

cg3_tag* cg3_cohort_getwordform(cg3_cohort* cohort_) {
	auto cohort = static_cast<Cohort*>(cohort_);
	return cohort->wordform;
}

uint32_t cg3_cohort_getid(cg3_cohort* cohort_) {
	auto cohort = static_cast<Cohort*>(cohort_);
	return cohort->global_number;
}

void cg3_cohort_setdependency(cg3_cohort* cohort_, uint32_t dep_self, uint32_t dep_parent) {
	auto cohort = static_cast<Cohort*>(cohort_);
	cohort->parent->parent->parent->has_dep = true;
	cohort->dep_self = dep_self;
	cohort->dep_parent = dep_parent;
}

void cg3_cohort_getdependency(cg3_cohort* cohort_, uint32_t* dep_self, uint32_t* dep_parent) {
	auto cohort = static_cast<Cohort*>(cohort_);
	if (dep_self) {
		*dep_self = cohort->dep_self;
	}
	if (dep_parent) {
		*dep_parent = cohort->dep_parent;
	}
}

/*
void cg3_cohort_getrelation_u(cg3_cohort *cohort_, const UChar *rel, uint32_t *rel_parent) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	GrammarApplicator *ga = cohort->parent->parent->parent;

	if ((cohort->type & CT_RELATED) && !cohort->relations.empty()) {
		for (auto miter : cohort->relations) {
			for (auto siter : miter->second) {
				if (ga->grammar->single_tags.find(miter.first)->second->tag == rel) {
					*rel_parent = siter;
				}
			}
		}
	}
}

void cg3_cohort_getrelation_u8(cg3_cohort *cohort_, const char *rel, uint32_t *rel_parent) {
	UErrorCode status = U_ZERO_ERROR;

	u_strFromUTF8(&gbuffers[0][0], CG3_BUFFER_SIZE-1, 0, rel, strlen(rel), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UTF-8 to UTF-16. Status = %s\n", u_errorName(status));
		return;
	}
	cg3_cohort_getrelation_u(cohort_, &gbuffers[0][0], rel_parent);
}
//*/

void cg3_cohort_addreading(cg3_cohort* cohort_, cg3_reading* reading_) {
	auto cohort = static_cast<Cohort*>(cohort_);
	auto reading = static_cast<Reading*>(reading_);
	cohort->appendReading(reading);
}

size_t cg3_cohort_numreadings(cg3_cohort* cohort_) {
	auto cohort = static_cast<Cohort*>(cohort_);
	return cohort->readings.size();
}

cg3_reading* cg3_cohort_getreading(cg3_cohort* cohort_, size_t which) {
	auto cohort = static_cast<Cohort*>(cohort_);
	return cohort->readings[which];
}

void cg3_cohort_free(cg3_cohort* cohort_) {
	auto cohort = static_cast<Cohort*>(cohort_);
	free_cohort(cohort);
}

cg3_reading* cg3_reading_create(cg3_cohort* cohort_) {
	auto cohort = static_cast<Cohort*>(cohort_);
	auto ga = cohort->parent->parent->parent;
	auto reading = alloc_reading(cohort);
	insert_if_exists(reading->parent->possible_sets, ga->grammar->sets_any);
	ga->addTagToReading(*reading, cohort->wordform);
	return reading;
}

cg3_status cg3_reading_addtag(cg3_reading* reading_, cg3_tag* tag_) {
	auto reading = static_cast<Reading*>(reading_);
	auto tag = static_cast<Tag*>(tag_);
	if (tag->type & T_MAPPING) {
		if (reading->mapping && reading->mapping != tag) {
			u_fprintf(ux_stderr, "CG3 Error: Cannot add a mapping tag to a reading which already is mapped!\n");
			return CG3_ERROR;
		}
	}

	auto ga = reading->parent->parent->parent->parent;
	ga->addTagToReading(*reading, tag);

	return CG3_SUCCESS;
}

size_t cg3_reading_numtags(cg3_reading* reading_) {
	auto reading = static_cast<Reading*>(reading_);
	return reading->tags_list.size();
}

cg3_tag* cg3_reading_gettag(cg3_reading* reading_, size_t which) {
	auto reading = static_cast<Reading*>(reading_);
	auto it = reading->tags_list.begin();
	std::advance(it, which);
	auto ga = reading->parent->parent->parent->parent;
	return ga->grammar->single_tags.find(*it)->second;
}

size_t cg3_reading_numtraces(cg3_reading* reading_) {
	auto reading = static_cast<Reading*>(reading_);
	return reading->hit_by.size();
}

uint32_t cg3_reading_gettrace(cg3_reading* reading_, size_t which) {
	auto reading = static_cast<Reading*>(reading_);
	auto grammar = reading->parent->parent->parent->parent->grammar;
	auto r = grammar->rule_by_number[reading->hit_by[which]];
	return r->line;
}

void cg3_reading_free(cg3_reading* reading_) {
	auto reading = static_cast<Reading*>(reading_);
	free_reading(reading);
}

cg3_reading* cg3_subreading_create(cg3_reading* reading_) {
	auto reading = static_cast<Reading*>(reading_);
	return cg3_reading_create(reading->parent);
}

cg3_status cg3_reading_setsubreading(cg3_reading* reading_, cg3_reading* subreading_) {
	auto reading = static_cast<Reading*>(reading_);
	auto subreading = static_cast<Reading*>(subreading_);
	free_reading(reading->next);
	reading->next = subreading;
	return CG3_SUCCESS;
}

size_t cg3_reading_numsubreadings(cg3_reading* reading_) {
	auto reading = static_cast<Reading*>(reading_);
	return (reading->next != 0);
}

cg3_reading* cg3_reading_getsubreading(cg3_reading* reading_, size_t which) {
	(void)which;
	assert((which == 1) && "There can currently only be 1 sub-reading per reading, but the API is future-proof");
	auto reading = static_cast<Reading*>(reading_);
	return reading->next;
}

void cg3_subreading_free(cg3_reading* subreading_) {
	return cg3_reading_free(subreading_);
}

cg3_tag* cg3_tag_create_u(cg3_applicator* applicator_, const UChar* text) {
	auto applicator = static_cast<GrammarApplicator*>(applicator_);
	return applicator->addTag(text);
}

cg3_tag* cg3_tag_create_u8(cg3_applicator* applicator, const char* text) {
	UErrorCode status = U_ZERO_ERROR;

	u_strFromUTF8(&gbuffers[0][0], CG3_BUFFER_SIZE - 1, 0, text, SI32(strlen(text)), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UTF-8 to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

cg3_tag* cg3_tag_create_u16(cg3_applicator* applicator, const uint16_t* text) {
	return cg3_tag_create_u(applicator, reinterpret_cast<const UChar*>(text));
}

cg3_tag* cg3_tag_create_u32(cg3_applicator* applicator, const uint32_t* text) {
	UErrorCode status = U_ZERO_ERROR;

	int32_t length = 0;
	while (text[length]) {
		++length;
	}

	u_strFromUTF32(&gbuffers[0][0], CG3_BUFFER_SIZE - 1, 0, reinterpret_cast<const UChar32*>(text), length, &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UTF-32 to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

cg3_tag* cg3_tag_create_w(cg3_applicator* applicator, const wchar_t* text) {
	UErrorCode status = U_ZERO_ERROR;

	u_strFromWCS(&gbuffers[0][0], CG3_BUFFER_SIZE - 1, 0, text, SI32(wcslen(text)), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from wchar_t to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

const UChar* cg3_tag_gettext_u(cg3_tag* tag_) {
	auto tag = static_cast<Tag*>(tag_);
	return tag->tag.data();
}

const char* cg3_tag_gettext_u8(cg3_tag* tag_) {
	auto tag = static_cast<Tag*>(tag_);
	UErrorCode status = U_ZERO_ERROR;

	u_strToUTF8(&cbuffers[0][0], CG3_BUFFER_SIZE - 1, 0, tag->tag.data(), SI32(tag->tag.size()), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-8. Status = %s\n", u_errorName(status));
		return 0;
	}

	return &cbuffers[0][0];
}

const uint16_t* cg3_tag_gettext_u16(cg3_tag* tag_) {
	auto tag = static_cast<Tag*>(tag_);
	return reinterpret_cast<const uint16_t*>(tag->tag.data());
}

const uint32_t* cg3_tag_gettext_u32(cg3_tag* tag_) {
	auto tag = static_cast<Tag*>(tag_);
	UErrorCode status = U_ZERO_ERROR;

	UChar32* tmp = reinterpret_cast<UChar32*>(&cbuffers[0][0]);

	u_strToUTF32(tmp, (CG3_BUFFER_SIZE / sizeof(UChar32)) - 1, 0, tag->tag.data(), SI32(tag->tag.size()), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-32. Status = %s\n", u_errorName(status));
		return 0;
	}

	return reinterpret_cast<const uint32_t*>(tmp);
}

const wchar_t* cg3_tag_gettext_w(cg3_tag* tag_) {
	auto tag = static_cast<Tag*>(tag_);
	UErrorCode status = U_ZERO_ERROR;

	auto tmp = reinterpret_cast<wchar_t*>(&cbuffers[0][0]);

	u_strToWCS(tmp, (CG3_BUFFER_SIZE / sizeof(wchar_t)) - 1, 0, tag->tag.data(), SI32(tag->tag.size()), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-32. Status = %s\n", u_errorName(status));
		return 0;
	}

	return tmp;
}

// These 3 from Paul Meurer <paul.meurer@uni.no>
size_t cg3_cohort_numdelreadings(cg3_cohort* cohort_) {
	auto cohort = static_cast<Cohort*>(cohort_);
	return cohort->deleted.size();
}

cg3_reading* cg3_cohort_getdelreading(cg3_cohort* cohort_, size_t which) {
	auto cohort = static_cast<Cohort*>(cohort_);
	auto it = cohort->deleted.begin();
	std::advance(it, which);
	return *it;
}

size_t cg3_reading_gettrace_ruletype(cg3_reading* reading_, size_t which) {
	auto reading = static_cast<Reading*>(reading_);
	auto grammar = reading->parent->parent->parent->parent->grammar;
	auto r = grammar->rule_by_number[reading->hit_by[which]];
	return r->type;
}
