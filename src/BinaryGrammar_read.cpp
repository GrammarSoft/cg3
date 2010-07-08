/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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

#include "BinaryGrammar.h"
#include "Strings.h"
#include "Grammar.h"
#include "ContextualTest.h"
#include "version.h"

namespace CG3 {

int BinaryGrammar::readBinaryGrammar(FILE *input) {
	if (!input) {
		u_fprintf(ux_stderr, "Error: Input is null - cannot read from nothing!\n");
		CG3Quit(1);
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		CG3Quit(1);
	}
	uint32_t u32tmp = 0;
	int32_t i32tmp = 0;
	uint8_t u8tmp = 0;
	UErrorCode err = U_ZERO_ERROR;
	UConverter *conv = ucnv_open("UTF-8", &err);

	if (fread(&cbuffers[0][0], 1, 4, input) != 4) {
		std::cerr << "Error: Error reading first 4 bytes from grammar!" << std::endl;
		CG3Quit(1);
	}
	if (cbuffers[0][0] != 'C' || cbuffers[0][1] != 'G' || cbuffers[0][2] != '3' || cbuffers[0][3] != 'B') {
		u_fprintf(ux_stderr, "Error: Grammar does not begin with magic bytes - cannot load as binary!\n");
		CG3Quit(1);
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	if (u32tmp < CG3_TOO_OLD) {
		u_fprintf(ux_stderr, "Error: Grammar revision is %u, but this loader requires %u or later!\n", u32tmp, CG3_TOO_OLD);
		CG3Quit(1);
	}
	if (u32tmp > CG3_REVISION) {
		u_fprintf(ux_stderr, "Error: Grammar revision is %u, but this loader only knows up to revision %u!\n", u32tmp, CG3_REVISION);
		CG3Quit(1);
	}

	grammar->is_binary = true;

	fread(&u8tmp, sizeof(uint8_t), 1, input);
	grammar->has_dep = (u8tmp == 1);

	ucnv_reset(conv);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	fread(&cbuffers[0][0], 1, u32tmp, input);
	i32tmp = ucnv_toUChars(conv, &grammar->mapping_prefix, 1, &cbuffers[0][0], u32tmp, &err);

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_single_tags = u32tmp;
	grammar->single_tags_list.resize(num_single_tags);
	for (uint32_t i=0 ; i<num_single_tags ; i++) {
		Tag *t = grammar->allocateTag();
		t->type |= T_GRAMMAR;
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		t->number = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		t->hash = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		t->plain_hash = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		t->seed = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		t->type = (uint32_t)ntohl(u32tmp);

		if (t->type & T_NUMERICAL) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			t->comparison_hash = (uint32_t)ntohl(u32tmp);
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			t->comparison_op = (C_OPS)ntohl(u32tmp);
			fread(&i32tmp, sizeof(int32_t), 1, input);
			t->comparison_val = (int32_t)ntohl(i32tmp);
		}

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		if (u32tmp) {
			ucnv_reset(conv);
			fread(&cbuffers[0][0], 1, u32tmp, input);
			i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE-1, &cbuffers[0][0], u32tmp, &err);
			t->tag = t->allocateUChars(i32tmp+1);
			u_strcpy(t->tag, &gbuffers[0][0]);
		}
		if (t->type & T_REGEXP) {
			UParseError pe;
			UErrorCode status = U_ZERO_ERROR;

			if (t->type & T_CASE_INSENSITIVE) {
				t->regexp = uregex_open(t->tag, u_strlen(t->tag), UREGEX_CASE_INSENSITIVE, &pe, &status);
			}
			else {
				t->regexp = uregex_open(t->tag, u_strlen(t->tag), 0, &pe, &status);
			}
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse tag %S - cannot continue!\n", u_errorName(status), t->tag);
				CG3Quit(1);
			}
		}
		grammar->single_tags[t->hash] = t;
		grammar->single_tags_list[t->number] = t;
		if (t->tag && t->tag[0] == '*' && u_strcmp(t->tag, stringbits[S_ASTERIK].getTerminatedBuffer()) == 0) {
			grammar->tag_any = t->hash;
		}
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_comp_tags = u32tmp;
	grammar->tags_list.resize(num_comp_tags);
	for (uint32_t i=0 ; i<num_comp_tags ; i++) {
		CompositeTag *curcomptag = grammar->allocateCompositeTag();
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		curcomptag->number = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		curcomptag->hash = (uint32_t)ntohl(u32tmp);
		fread(&u8tmp, sizeof(uint8_t), 1, input);
		curcomptag->is_special = (u8tmp == 1);

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_single_tags = u32tmp;
		for (uint32_t j=0 ; j<num_single_tags ; j++) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			curcomptag->tags.insert(grammar->single_tags_list.at(u32tmp));
			curcomptag->tags_set.insert(grammar->single_tags_list.at(u32tmp));
		}
		grammar->tags[curcomptag->hash] = curcomptag;
		grammar->tags_list[curcomptag->number] = curcomptag;
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_pref_targets = u32tmp;
	for (uint32_t i=0 ; i<num_pref_targets ; i++) {
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		grammar->preferred_targets.push_back(u32tmp);
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_par_pairs = u32tmp;
	for (uint32_t i=0 ; i<num_par_pairs ; i++) {
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t left = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t right = (uint32_t)ntohl(u32tmp);
		grammar->parentheses[left] = right;
		grammar->parentheses_reverse[right] = left;
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_par_anchors = u32tmp;
	for (uint32_t i=0 ; i<num_par_anchors ; i++) {
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t left = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t right = (uint32_t)ntohl(u32tmp);
		grammar->anchor_by_hash[left] = right;
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_sets = u32tmp;
	grammar->sets_list.resize(num_sets);
	for (uint32_t i=0 ; i<num_sets ; i++) {
		Set *s = grammar->allocateSet();
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		s->number = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		s->hash = (uint32_t)ntohl(u32tmp);
		fread(&u8tmp, sizeof(uint8_t), 1, input);
		s->type = u8tmp;

		fread(&u8tmp, sizeof(uint8_t), 1, input);
		if (u8tmp == 0) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_single_tags = u32tmp;
			for (uint32_t j=0 ; j<num_single_tags ; j++) {
				fread(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->single_tags.insert(grammar->single_tags_list.at(u32tmp));
				s->single_tags_hash.insert(grammar->single_tags_list.at(u32tmp)->hash);
				s->tags_set.insert(grammar->single_tags_list.at(u32tmp)->hash);
			}
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_ff_tags = u32tmp;
			for (uint32_t j=0 ; j<num_ff_tags ; j++) {
				fread(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->ff_tags.insert(grammar->single_tags_list.at(u32tmp));
			}
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_comp_tags = u32tmp;
			for (uint32_t j=0 ; j<num_comp_tags ; j++) {
				fread(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->tags.insert(grammar->tags_list.at(u32tmp));
				s->tags_set.insert(grammar->tags_list.at(u32tmp)->hash);
			}
		}
		else {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_set_ops = u32tmp;
			for (uint32_t j=0 ; j<num_set_ops ; j++) {
				fread(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->set_ops.push_back(u32tmp);
			}
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_sets = u32tmp;
			for (uint32_t j=0 ; j<num_sets ; j++) {
				fread(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->sets.push_back(u32tmp);
			}
		}
		grammar->sets_by_contents[s->hash] = s;
		grammar->sets_list[s->number] = s;
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	if (u32tmp) {
		grammar->delimiters = grammar->sets_by_contents.find(u32tmp)->second;
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	if (u32tmp) {
		grammar->soft_delimiters = grammar->sets_by_contents.find(u32tmp)->second;
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_templates = u32tmp;
	for (uint32_t i=0 ; i<num_templates ; i++) {
		ContextualTest *t = grammar->allocateContextualTest();
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		t->name = (uint32_t)ntohl(u32tmp);
		grammar->templates[t->name] = t;
		grammar->template_list.push_back(t);
		readContextualTest(t, input);
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_rules = u32tmp;
	for (uint32_t i=0 ; i<num_rules ; i++) {
		Rule *r = grammar->allocateRule();
		fread(&i32tmp, sizeof(int32_t), 1, input);
		r->section = (int32_t)ntohl(i32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->type = (KEYWORDS)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->line = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->flags = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		if (u32tmp) {
			ucnv_reset(conv);
			fread(&cbuffers[0][0], 1, u32tmp, input);
			i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE-1, &cbuffers[0][0], u32tmp, &err);
			r->setName(&gbuffers[0][0]);
		}
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->target = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->wordform = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->varname = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->varvalue = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->jumpstart = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->jumpend = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->childset1 = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->childset2 = (uint32_t)ntohl(u32tmp);

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_maps = u32tmp;
		for (uint32_t j=0 ; j<num_maps ; j++) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			r->maplist.push_back(grammar->single_tags[u32tmp]);
		}
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_subs = u32tmp;
		for (uint32_t j=0 ; j<num_subs ; j++) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			r->sublist.push_back(u32tmp);
		}

		fread(&u8tmp, sizeof(uint8_t), 1, input);
		if (u8tmp == 1) {
			r->dep_target = r->allocateContextualTest();
			readContextualTest(r->dep_target, input);
		}

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_dep_tests = u32tmp;
		for (uint32_t j=0 ; j<num_dep_tests ; j++) {
			ContextualTest *t = r->allocateContextualTest();
			readContextualTest(t, input);
			r->addContextualTest(t, &r->dep_test_head);
		}

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_tests = u32tmp;
		for (uint32_t j=0 ; j<num_tests ; j++) {
			ContextualTest *t = r->allocateContextualTest();
			readContextualTest(t, input);
			r->addContextualTest(t, &r->test_head);
		}
		grammar->rule_by_line[r->line] = r;
	}

	ucnv_close(conv);
	return 0;
}

void BinaryGrammar::readContextualTest(ContextualTest *t, FILE *input) {
	uint32_t u32tmp = 0;
	int32_t i32tmp = 0;
	uint8_t u8tmp = 0;

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->hash = (uint32_t)ntohl(u32tmp);

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->pos = (uint32_t)ntohl(u32tmp);
	fread(&i32tmp, sizeof(int32_t), 1, input);
	t->offset = (int32_t)ntohl(i32tmp);

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	if (u32tmp) {
		t->tmpl = grammar->templates.find(u32tmp)->second;
	}
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	if (u32tmp) {
		for (uint32_t i=0 ; i<u32tmp ; i++) {
			ContextualTest *to = t->allocateContextualTest();
			readContextualTest(to, input);
			t->ors.push_back(to);
		}
	}

	if (!t->tmpl && t->ors.empty()) {
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		t->target = (uint32_t)ntohl(u32tmp);
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->line = (uint32_t)ntohl(u32tmp);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->relation = (uint32_t)ntohl(u32tmp);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->barrier = (uint32_t)ntohl(u32tmp);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->cbarrier = (uint32_t)ntohl(u32tmp);

	fread(&u8tmp, sizeof(uint8_t), 1, input);
	if (u8tmp == 1) {
		t->linked = t->allocateContextualTest();
		readContextualTest(t->linked, input);
	}
	else {
		t->linked = 0;
	}
}

}
