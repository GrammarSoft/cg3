/*
* Copyright (C) 2007-2018, GrammarSoft ApS
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

#include "cg3.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Error: First argument must be a CG-3 grammar to load!\n");
		return 1;
	}

	if (!cg3_init(stdin, stdout, stderr)) {
		fprintf(stderr, "Error: Failed cg3_init()!\n");
		return 2;
	}

	cg3_grammar* grammar = cg3_grammar_load(argv[1]);
	if (!grammar) {
		fprintf(stderr, "Error: Failed cg3_grammar_load( %s )!\n", argv[1]);
		return 3;
	}

	cg3_applicator* applicator = cg3_applicator_create(grammar);
	cg3_applicator_setflags(applicator, CG3F_TRACE);
	cg3_sentence* sentence = cg3_sentence_new(applicator);

	cg3_cohort* cohort = cg3_cohort_create(sentence);
	cg3_tag* tag = cg3_tag_create_u8(applicator, "\"<wordform>\"");
	cg3_cohort_setwordform(cohort, tag);

	cg3_reading* reading = cg3_reading_create(cohort);
	tag = cg3_tag_create_w(applicator, L"\"baseform\"");
	cg3_reading_addtag(reading, tag);
	tag = cg3_tag_create_u8(applicator, "notwanted");
	cg3_reading_addtag(reading, tag);
	tag = cg3_tag_create_w(applicator, L"@mapping");
	cg3_reading_addtag(reading, tag);
	cg3_cohort_addreading(cohort, reading);

	reading = cg3_reading_create(cohort);
	tag = cg3_tag_create_w(applicator, L"\"baseform\"");
	cg3_reading_addtag(reading, tag);
	tag = cg3_tag_create_u8(applicator, "wanted");
	cg3_reading_addtag(reading, tag);
	cg3_cohort_addreading(cohort, reading);

	reading = cg3_reading_create(cohort);
	tag = cg3_tag_create_w(applicator, L"\"baseform\"");
	cg3_reading_addtag(reading, tag);
	tag = cg3_tag_create_u8(applicator, "alsonotwanted");
	cg3_reading_addtag(reading, tag);
	cg3_cohort_addreading(cohort, reading);

	cg3_sentence_addcohort(sentence, cohort);

	cg3_sentence_runrules(applicator, sentence);

	for (size_t ci = 0, ce = cg3_sentence_numcohorts(sentence); ci != ce; ++ci) {
		cohort = cg3_sentence_getcohort(sentence, ci);
		tag = cg3_cohort_getwordform(cohort);
		const char* tmp = cg3_tag_gettext_u8(tag);
		fprintf(stdout, "%s\n", tmp);

		for (size_t ri = 0, re = cg3_cohort_numreadings(cohort); ri != re; ++ri) {
			reading = cg3_cohort_getreading(cohort, ri);
			fprintf(stdout, "\t");
			for (size_t ti = 0, te = cg3_reading_numtags(reading); ti != te; ++ti) {
				tag = cg3_reading_gettag(reading, ti);
				tmp = cg3_tag_gettext_u8(tag);
				fprintf(stdout, "%s ", tmp);
			}
			for (size_t ti = 0, te = cg3_reading_numtraces(reading); ti != te; ++ti) {
				uint32_t rule_line = cg3_reading_gettrace(reading, ti);
				fprintf(stdout, "TRACE:%u ", rule_line);
			}
			fprintf(stdout, "\n");
		}
	}

	cg3_sentence_free(sentence);
	cg3_applicator_free(applicator);
	cg3_grammar_free(grammar);

	cg3_cleanup();
	return 0;
}
