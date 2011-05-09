/*
* Copyright (C) 2007-2011, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
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

#include "stdafx.h"
#include "icu_uoptions.h"
#include "Grammar.h"
#include "TextualParser.h"
#include "BinaryGrammar.h"
#include "GrammarApplicator.h"
#include "Window.h"
#include "SingleWindow.h"
#include "version.h"
using namespace CG3;

#include "cg3.h"

namespace {
	UFILE *ux_stdin = 0;
	UFILE *ux_stdout = 0;
	UFILE *ux_stderr = 0;
}

cg3_status cg3_init(FILE *in, FILE *out, FILE *err) {
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

	ux_stdin = u_finit(in, uloc_getDefault(), ucnv_getDefaultName());
	if (!ux_stdin) {
		fprintf(err, "CG3 Error: The input stream could not be inited.\n");
		return CG3_ERROR;
	}

	ux_stdout = u_finit(out, uloc_getDefault(), ucnv_getDefaultName());
	if (!ux_stdout) {
		fprintf(err, "CG3 Error: The output stream could not be inited.\n");
		return CG3_ERROR;
	}

	ux_stderr = u_finit(err, uloc_getDefault(), ucnv_getDefaultName());
	if (!ux_stderr) {
		fprintf(err, "CG3 Error: The error stream could not be inited.\n");
		return CG3_ERROR;
	}

	return CG3_SUCCESS;
}

cg3_status cg3_cleanup(void) {
	u_fclose(ux_stdin);
	u_fclose(ux_stdout);
	u_fclose(ux_stderr);

	u_cleanup();

	return CG3_SUCCESS;
}

cg3_grammar *cg3_grammar_load(const char *filename) {
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

	Grammar *grammar = new Grammar;
	grammar->ux_stderr = ux_stderr;
	grammar->ux_stdout = ux_stdout;

	std::auto_ptr<IGrammarParser> parser;

	if (cbuffers[0][0] == 'C' && cbuffers[0][1] == 'G' && cbuffers[0][2] == '3' && cbuffers[0][3] == 'B') {
		u_fprintf(ux_stderr, "CG3 Info: Binary grammar detected.\n");
		parser.reset(new BinaryGrammar(*grammar, ux_stderr));
	}
	else {
		parser.reset(new TextualParser(*grammar, ux_stderr));
	}
	if (parser->parse_grammar_from_file(filename, uloc_getDefault(), ucnv_getDefaultName())) {
		u_fprintf(ux_stderr, "CG3 Error: Grammar could not be parsed!\n");
		return 0;
	}

	grammar->reindex();

	return grammar;
}

void cg3_grammar_free(cg3_grammar *grammar_) {
	Grammar *grammar = static_cast<Grammar*>(grammar_);
	delete grammar;
}

cg3_applicator *cg3_applicator_create(cg3_grammar *grammar_) {
	Grammar *grammar = static_cast<Grammar*>(grammar_);
	GrammarApplicator *applicator = new GrammarApplicator(ux_stderr);
	applicator->setGrammar(grammar);
	applicator->index();
	return applicator;
}

void cg3_applicator_free(cg3_applicator *applicator_) {
	GrammarApplicator *applicator = static_cast<GrammarApplicator*>(applicator_);
	delete applicator;
}

cg3_sentence *cg3_sentence_new(cg3_applicator *applicator_) {
	GrammarApplicator *applicator = static_cast<GrammarApplicator*>(applicator_);
	SingleWindow *current = applicator->gWindow->allocSingleWindow();
	applicator->initEmptySingleWindow(current);
	return current;
}

void cg3_sentence_runrules(cg3_applicator *applicator_, cg3_sentence *sentence_) {
	GrammarApplicator *applicator = static_cast<GrammarApplicator*>(applicator_);
	SingleWindow *sentence = static_cast<SingleWindow*>(sentence_);
	applicator->gWindow->current = sentence;
	applicator->runGrammarOnWindow();
	applicator->gWindow->current = 0;
}

size_t cg3_sentence_numcohorts(cg3_sentence *sentence_) {
	SingleWindow *sentence = static_cast<SingleWindow*>(sentence_);
	return sentence->cohorts.size();
}

cg3_cohort *cg3_sentence_getcohort(cg3_sentence *sentence_, size_t which) {
	SingleWindow *sentence = static_cast<SingleWindow*>(sentence_);
	return sentence->cohorts[which];
}

void cg3_sentence_free(cg3_sentence *sentence_) {
	SingleWindow *sentence = static_cast<SingleWindow*>(sentence_);
	delete sentence;
}

void cg3_sentence_addcohort(cg3_sentence *sentence_, cg3_cohort *cohort_) {
	SingleWindow *sentence = static_cast<SingleWindow*>(sentence_);
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	sentence->appendCohort(cohort);
}

cg3_cohort *cg3_cohort_create(cg3_sentence *sentence_) {
	SingleWindow *sentence = static_cast<SingleWindow*>(sentence_);
	Cohort *cohort = new Cohort(sentence);
	cohort->global_number = sentence->parent->cohort_counter++;
	return cohort;
}

void cg3_cohort_setwordform(cg3_cohort *cohort_, cg3_tag *tag_) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	Tag *tag = static_cast<Tag*>(tag_);
	cohort->wordform = tag->hash;
}

cg3_tag *cg3_cohort_getwordform(cg3_cohort *cohort_) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	GrammarApplicator *ga = cohort->parent->parent->parent;
	return ga->single_tags.find(cohort->wordform)->second;
}

void cg3_cohort_setdependency(cg3_cohort *cohort_, uint32_t dep_self, uint32_t dep_parent) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	cohort->parent->parent->parent->has_dep = true;
	cohort->dep_self = dep_self;
	cohort->dep_parent = dep_parent;
}

void cg3_cohort_addreading(cg3_cohort *cohort_, cg3_reading *reading_) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	Reading *reading = static_cast<Reading*>(reading_);
	cohort->appendReading(reading);
}

size_t cg3_cohort_numreadings(cg3_cohort *cohort_) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	return cohort->readings.size();
}

cg3_reading *cg3_cohort_getreading(cg3_cohort *cohort_, size_t which) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	ReadingList::iterator it = cohort->readings.begin();
	std::advance(it, which);
	return *it;
}

void cg3_cohort_free(cg3_cohort *cohort_) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	delete cohort;
}

cg3_reading *cg3_reading_create(cg3_cohort *cohort_) {
	Cohort *cohort = static_cast<Cohort*>(cohort_);
	GrammarApplicator *ga = cohort->parent->parent->parent;
	Reading *reading = new Reading(cohort);
	reading->wordform = cohort->wordform;
	insert_if_exists(reading->parent->possible_sets, ga->grammar->sets_any);
	ga->addTagToReading(*reading, reading->wordform);
	return reading;
}

cg3_status cg3_reading_addtag(cg3_reading *reading_, cg3_tag *tag_) {
	Reading *reading = static_cast<Reading*>(reading_);
	Tag *tag = static_cast<Tag*>(tag_);
	if (tag->type & T_MAPPING) {
		if (reading->mapping && reading->mapping != tag) {
			u_fprintf(ux_stderr, "CG3 Error: Cannot add a mapping tag to a reading which already is mapped!\n");
			return CG3_ERROR;
		}
	}

	GrammarApplicator *ga = reading->parent->parent->parent->parent;
	ga->addTagToReading(*reading, tag->hash);

	return CG3_SUCCESS;
}

size_t cg3_reading_numtags(cg3_reading *reading_) {
	Reading *reading = static_cast<Reading*>(reading_);
	return reading->tags_list.size();
}

cg3_tag *cg3_reading_gettag(cg3_reading *reading_, size_t which) {
	Reading *reading = static_cast<Reading*>(reading_);
	uint32List::iterator it = reading->tags_list.begin();
	std::advance(it, which);
	GrammarApplicator *ga = reading->parent->parent->parent->parent;
	return ga->single_tags.find(*it)->second;
}

void cg3_reading_free(cg3_reading *reading_) {
	Reading *reading = static_cast<Reading*>(reading_);
	delete reading;
}

cg3_tag *cg3_tag_create_u(cg3_applicator *applicator_, const UChar *text) {
	GrammarApplicator *applicator = static_cast<GrammarApplicator*>(applicator_);
	return applicator->addTag(text);
}

cg3_tag *cg3_tag_create_u8(cg3_applicator *applicator, const char *text) {
	UErrorCode status = U_ZERO_ERROR;

	u_strFromUTF8(&gbuffers[0][0], CG3_BUFFER_SIZE-1, 0, text, strlen(text), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UTF-8 to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

cg3_tag *cg3_tag_create_u16(cg3_applicator *applicator, const uint16_t *text) {
	return cg3_tag_create_u(applicator, reinterpret_cast<const UChar*>(text));
}

cg3_tag *cg3_tag_create_u32(cg3_applicator *applicator, const uint32_t *text) {
	UErrorCode status = U_ZERO_ERROR;

	size_t length = 0;
	while (text[length]) {
		++length;
	}

	u_strFromUTF32(&gbuffers[0][0], CG3_BUFFER_SIZE-1, 0, reinterpret_cast<const UChar32*>(text), length, &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UTF-32 to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

cg3_tag *cg3_tag_create_w(cg3_applicator *applicator, const wchar_t *text) {
	UErrorCode status = U_ZERO_ERROR;

	u_strFromWCS(&gbuffers[0][0], CG3_BUFFER_SIZE-1, 0, text, wcslen(text), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from wchar_t to UTF-16. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return cg3_tag_create_u(applicator, &gbuffers[0][0]);
}

const UChar *cg3_tag_gettext_u(cg3_tag *tag_) {
	Tag *tag = static_cast<Tag*>(tag_);
	return tag->tag.c_str();
}

const char *cg3_tag_gettext_u8(cg3_tag *tag_) {
	Tag *tag = static_cast<Tag*>(tag_);
	UErrorCode status = U_ZERO_ERROR;

	u_strToUTF8(&cbuffers[0][0], CG3_BUFFER_SIZE-1, 0, tag->tag.c_str(), tag->tag.length(), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-8. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return &cbuffers[0][0];
}

const uint16_t *cg3_tag_gettext_u16(cg3_tag *tag_) {
	Tag *tag = static_cast<Tag*>(tag_);
	return reinterpret_cast<const uint16_t*>(tag->tag.c_str());
}

const uint32_t *cg3_tag_gettext_u32(cg3_tag *tag_) {
	Tag *tag = static_cast<Tag*>(tag_);
	UErrorCode status = U_ZERO_ERROR;

	UChar32 *tmp = reinterpret_cast<UChar32*>(&cbuffers[0][0]);

	u_strToUTF32(tmp, (CG3_BUFFER_SIZE/sizeof(UChar32))-1, 0, tag->tag.c_str(), tag->tag.length(), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-32. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return reinterpret_cast<const uint32_t*>(tmp);
}

const wchar_t *cg3_tag_gettext_w(cg3_tag *tag_) {
	Tag *tag = static_cast<Tag*>(tag_);
	UErrorCode status = U_ZERO_ERROR;

	wchar_t *tmp = reinterpret_cast<wchar_t*>(&cbuffers[0][0]);

	u_strToWCS(tmp, (CG3_BUFFER_SIZE/sizeof(wchar_t))-1, 0, tag->tag.c_str(), tag->tag.length(), &status);
	if (U_FAILURE(status)) {
		u_fprintf(ux_stderr, "CG3 Error: Failed to convert text from UChar to UTF-32. Status = %s\n", u_errorName(status));
		return 0;
	}
	status = U_ZERO_ERROR;

	return reinterpret_cast<const wchar_t*>(tmp);
}
