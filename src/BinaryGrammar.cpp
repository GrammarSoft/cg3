/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */

#include "BinaryGrammar.h"

using namespace CG3;
using namespace CG3::Strings;

BinaryGrammar::BinaryGrammar(Grammar *res, UFILE *ux_err) {
	ux_stderr = ux_err;
	grammar = res;
}

BinaryGrammar::~BinaryGrammar() {
	grammar = 0;
}

int BinaryGrammar::writeBinaryGrammar(FILE *output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		return -1;
	}
	uint32_t u32tmp = 0;
	uint16_t u16tmp = 0;
	int32_t i32tmp = 0;
	uint8_t u8tmp = 0;
	UErrorCode err = U_ZERO_ERROR;
	UConverter *conv = ucnv_open("UTF-8", &err);

	fprintf(output, "CG3B");

	// Write out the revision of the binary format
	u32tmp = (uint32_t)htonl((uint32_t)CG3_REVISION);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);

	u8tmp = (uint8_t)grammar->has_dep;
	fwrite(&u8tmp, sizeof(uint8_t), 1, output);

	ucnv_reset(conv);
	i32tmp = ucnv_fromUChars(conv, cbuffers[0], BUFFER_SIZE-1, &grammar->mapping_prefix, 1, &err);
	u32tmp = (uint32_t)htonl((uint32_t)i32tmp);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	fwrite(cbuffers[0], i32tmp, 1, output);

	u32tmp = (uint32_t)htonl((uint32_t)grammar->single_tags.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	stdext::hash_map<uint32_t, Tag*>::const_iterator tags_iter;
	for (tags_iter = grammar->single_tags.begin() ; tags_iter != grammar->single_tags.end() ; tags_iter++) {
		Tag *t = tags_iter->second;
		u32tmp = (uint32_t)htonl((uint32_t)t->hash);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u16tmp = (uint16_t)htons((uint16_t)t->type);
		fwrite(&u16tmp, sizeof(uint16_t), 1, output);
		u8tmp = (uint8_t)t->is_special;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		if (t->type & T_NUMERICAL) {
			u32tmp = (uint32_t)htonl((uint32_t)t->comparison_hash);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			u32tmp = (uint32_t)htonl((uint32_t)t->comparison_op);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			i32tmp = (int32_t)htonl(t->comparison_val);
			fwrite(&i32tmp, sizeof(int32_t), 1, output);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, cbuffers[0], BUFFER_SIZE-1, t->comparison_key, u_strlen(t->comparison_key), &err);
			u32tmp = (uint32_t)htonl((uint32_t)i32tmp);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			fwrite(cbuffers[0], i32tmp, 1, output);
		}
		if (t->tag) {
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, cbuffers[0], BUFFER_SIZE-1, t->tag, u_strlen(t->tag), &err);
			u32tmp = (uint32_t)htonl((uint32_t)i32tmp);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			fwrite(cbuffers[0], i32tmp, 1, output);
		}
		else {
			u32tmp = (uint32_t)htonl((uint32_t)0);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		}
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->tags.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	stdext::hash_map<uint32_t, CompositeTag*>::const_iterator comp_iter;
	for (comp_iter = grammar->tags.begin() ; comp_iter != grammar->tags.end() ; comp_iter++) {
		CompositeTag *curcomptag = comp_iter->second;
		u32tmp = (uint32_t)htonl((uint32_t)curcomptag->hash);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)curcomptag->tags.size());
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		uint32Set::const_iterator tag_iter;
		for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
			u32tmp = (uint32_t)htonl((uint32_t)*tag_iter);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		}
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->preferred_targets.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	uint32Vector::const_iterator iter;
	for(iter = grammar->preferred_targets.begin() ; iter != grammar->preferred_targets.end() ; iter++ ) {
		u32tmp = (uint32_t)htonl((uint32_t)*iter);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->sets_by_contents.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	stdext::hash_map<uint32_t, Set*>::const_iterator set_iter;
	for (set_iter = grammar->sets_by_contents.begin() ; set_iter != grammar->sets_by_contents.end() ; set_iter++) {
		Set *s = set_iter->second;
		u32tmp = (uint32_t)htonl((uint32_t)s->hash);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u8tmp = (uint8_t)s->match_any;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		u8tmp = (uint8_t)s->is_special;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		u8tmp = (uint8_t)s->has_mappings;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);

		if (s->sets.empty()) {
			u8tmp = (uint8_t)0;
			fwrite(&u8tmp, sizeof(uint8_t), 1, output);
			u32tmp = (uint32_t)htonl((uint32_t)s->single_tags.size());
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			uint32HashSet::const_iterator comp_iter;
			for (comp_iter = s->single_tags.begin() ; comp_iter != s->single_tags.end() ; comp_iter++) {
				u32tmp = (uint32_t)htonl((uint32_t)*comp_iter);
				fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			}
			u32tmp = (uint32_t)htonl((uint32_t)s->tags.size());
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			for (comp_iter = s->tags.begin() ; comp_iter != s->tags.end() ; comp_iter++) {
				u32tmp = (uint32_t)htonl((uint32_t)*comp_iter);
				fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			}
		}
		else {
			u8tmp = (uint8_t)1;
			fwrite(&u8tmp, sizeof(uint8_t), 1, output);
			u32tmp = (uint32_t)htonl((uint32_t)s->set_ops.size());
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			for (uint32_t i=0;i<s->set_ops.size();i++) {
				u32tmp = (uint32_t)htonl((uint32_t)s->set_ops.at(i));
				fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			}
			u32tmp = (uint32_t)htonl((uint32_t)s->sets.size());
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			for (uint32_t i=0;i<s->sets.size();i++) {
				u32tmp = (uint32_t)htonl((uint32_t)s->sets.at(i));
				fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			}
		}
	}

	if (grammar->delimiters) {
		u32tmp = (uint32_t)htonl((uint32_t)grammar->delimiters->hash);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}
	else {
		u32tmp = (uint32_t)htonl((uint32_t)0);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}

	if (grammar->soft_delimiters) {
		u32tmp = (uint32_t)htonl((uint32_t)grammar->soft_delimiters->hash);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}
	else {
		u32tmp = (uint32_t)htonl((uint32_t)0);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->rule_by_line.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::map<uint32_t, Rule*>::const_iterator rule_iter;
	for (rule_iter = grammar->rule_by_line.begin() ; rule_iter != grammar->rule_by_line.end() ; rule_iter++) {
		Rule *r = rule_iter->second;
		i32tmp = (int32_t)htonl(r->section);
		fwrite(&i32tmp, sizeof(int32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)r->type);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)r->line);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)r->target);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)r->wordform);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)r->varname);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)r->varvalue);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);

		uint32List::const_iterator liter;
		u32tmp = (uint32_t)htonl((uint32_t)r->maplist.size());
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		for (liter = r->maplist.begin() ; liter != r->maplist.end() ; liter++) {
			u32tmp = (uint32_t)htonl((uint32_t)*liter);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		}
		u32tmp = (uint32_t)htonl((uint32_t)r->sublist.size());
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		for (liter = r->sublist.begin() ; liter != r->sublist.end() ; liter++) {
			u32tmp = (uint32_t)htonl((uint32_t)*liter);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		}

		if (r->dep_target) {
			u8tmp = (uint8_t)1;
			fwrite(&u8tmp, sizeof(uint8_t), 1, output);
			writeContextualTest(r->dep_target, output);
		}
		else {
			u8tmp = (uint8_t)0;
			fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		}

		std::list<ContextualTest*>::const_iterator citer;

		u32tmp = (uint32_t)htonl((uint32_t)r->dep_tests.size());
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		for (citer = r->dep_tests.begin() ; citer != r->dep_tests.end() ; citer++) {
			writeContextualTest(*citer, output);
		}

		u32tmp = (uint32_t)htonl((uint32_t)r->tests.size());
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		for (citer = r->tests.begin() ; citer != r->tests.end() ; citer++) {
			writeContextualTest(*citer, output);
		}
	}

	ucnv_close(conv);
	return 0;
}

void BinaryGrammar::writeContextualTest(ContextualTest *t, FILE *output) {
	uint32_t u32tmp = 0;
	int32_t i32tmp = 0;
	uint8_t u8tmp = 0;

	u32tmp = (uint32_t)htonl((uint32_t)t->hash);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	u32tmp = (uint32_t)htonl((uint32_t)t->line);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	u32tmp = (uint32_t)htonl((uint32_t)t->pos);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	u32tmp = (uint32_t)htonl((uint32_t)t->target);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	u32tmp = (uint32_t)htonl((uint32_t)t->barrier);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	u32tmp = (uint32_t)htonl((uint32_t)t->cbarrier);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	i32tmp = (int32_t)htonl(t->offset);
	fwrite(&i32tmp, sizeof(int32_t), 1, output);

	if (t->linked) {
		u8tmp = (uint8_t)1;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		writeContextualTest(t->linked, output);
	}
	else {
		u8tmp = (uint8_t)0;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
	}
}

int BinaryGrammar::readBinaryGrammar(FILE *input) {
	if (!input) {
		u_fprintf(ux_stderr, "Error: Input is null - cannot read from nothing!\n");
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		return -1;
	}
	uint32_t u32tmp = 0;
	uint16_t u16tmp = 0;
	int32_t i32tmp = 0;
	uint8_t u8tmp = 0;
	UErrorCode err = U_ZERO_ERROR;
	UConverter *conv = ucnv_open("UTF-8", &err);

	fread(cbuffers[0], 1, 4, input);
	if (cbuffers[0][0] != 'C' || cbuffers[0][1] != 'G' || cbuffers[0][2] != '3' || cbuffers[0][3] != 'B') {
		u_fprintf(ux_stderr, "Error: Grammar does not begin with magic bytes - cannot load as binary!\n");
		return -1;
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	if (u32tmp > CG3_REVISION) {
		u_fprintf(ux_stderr, "Error: Grammar revision is %u, but this loader only knows up to revision %u!\n", u32tmp, CG3_REVISION);
		return -1;
	}

	grammar->is_binary = true;

	fread(&u8tmp, sizeof(uint8_t), 1, input);
	grammar->has_dep = (u8tmp == 1);

	ucnv_reset(conv);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	fread(cbuffers[0], 1, u32tmp, input);
	i32tmp = ucnv_toUChars(conv, &grammar->mapping_prefix, 1, cbuffers[0], u32tmp, &err);

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_single_tags = u32tmp;
	for (uint32_t i=0 ; i<num_single_tags ; i++) {
		Tag *t = grammar->allocateTag();
		t->in_grammar = true;
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		t->hash = (uint32_t)ntohl(u32tmp);
		fread(&u16tmp, sizeof(uint16_t), 1, input);
		t->type = (uint16_t)ntohs(u16tmp);
		fread(&u8tmp, sizeof(uint8_t), 1, input);
		t->is_special = (u8tmp == 1);

		if (t->type & T_NUMERICAL) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			t->comparison_hash = (uint32_t)ntohl(u32tmp);
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			t->comparison_op = (C_OPS)ntohl(u32tmp);
			fread(&i32tmp, sizeof(int32_t), 1, input);
			t->comparison_val = (int32_t)ntohl(i32tmp);

			ucnv_reset(conv);
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			fread(cbuffers[0], 1, u32tmp, input);
			i32tmp = ucnv_toUChars(conv, gbuffers[0], BUFFER_SIZE-1, cbuffers[0], u32tmp, &err);
			t->comparison_key = t->allocateUChars(i32tmp+1);
			u_strcpy(t->comparison_key, gbuffers[0]);
		}

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		if (u32tmp) {
			ucnv_reset(conv);
			fread(cbuffers[0], 1, u32tmp, input);
			i32tmp = ucnv_toUChars(conv, gbuffers[0], BUFFER_SIZE-1, cbuffers[0], u32tmp, &err);
			t->tag = t->allocateUChars(i32tmp+1);
			u_strcpy(t->tag, gbuffers[0]);
		}
		if (t->type & T_REGEXP) {
			UParseError *pe = new UParseError;
			UErrorCode status = U_ZERO_ERROR;

			memset(pe, 0, sizeof(UParseError));
			status = U_ZERO_ERROR;
			if (t->type & T_CASE_INSENSITIVE) {
				t->regexp = uregex_open(t->tag, u_strlen(t->tag), UREGEX_CASE_INSENSITIVE, pe, &status);
			}
			else {
				t->regexp = uregex_open(t->tag, u_strlen(t->tag), 0, pe, &status);
			}
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse tag %S - cannot continue!\n", u_errorName(status), t->tag);
				exit(1);
			}
		}
		grammar->single_tags[t->hash] = t;
	}

	fread(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	uint32_t num_comp_tags = u32tmp;
	for (uint32_t i=0 ; i<num_comp_tags ; i++) {
		CompositeTag *curcomptag = grammar->allocateCompositeTag();
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		curcomptag->hash = (uint32_t)ntohl(u32tmp);

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_single_tags = u32tmp;
		for (uint32_t j=0 ; j<num_single_tags ; j++) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			curcomptag->tags.insert(u32tmp);
			curcomptag->tags_set.insert(u32tmp);
		}
		grammar->tags[curcomptag->hash] = curcomptag;
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
	uint32_t num_sets = u32tmp;
	for (uint32_t i=0 ; i<num_sets ; i++) {
		Set *s = grammar->allocateSet();
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		s->hash = (uint32_t)ntohl(u32tmp);
		fread(&u8tmp, sizeof(uint8_t), 1, input);
		s->match_any = (u8tmp == 1);
		fread(&u8tmp, sizeof(uint8_t), 1, input);
		s->is_special = (u8tmp == 1);
		fread(&u8tmp, sizeof(uint8_t), 1, input);
		s->has_mappings = (u8tmp == 1);

		fread(&u8tmp, sizeof(uint8_t), 1, input);
		if (u8tmp == 0) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_single_tags = u32tmp;
			for (uint32_t j=0 ; j<num_single_tags ; j++) {
				fread(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->single_tags.insert(u32tmp);
				s->tags_set.insert(u32tmp);
			}
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_comp_tags = u32tmp;
			for (uint32_t j=0 ; j<num_comp_tags ; j++) {
				fread(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->tags.insert(u32tmp);
				s->tags_set.insert(u32tmp);
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
		r->target = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->wordform = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->varname = (uint32_t)ntohl(u32tmp);
		fread(&u32tmp, sizeof(uint32_t), 1, input);
		r->varvalue = (uint32_t)ntohl(u32tmp);

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_maps = u32tmp;
		for (uint32_t j=0 ; j<num_maps ; j++) {
			fread(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			r->maplist.push_back(u32tmp);
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
			r->dep_tests.push_back(t);
		}

		fread(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_tests = u32tmp;
		for (uint32_t j=0 ; j<num_tests ; j++) {
			ContextualTest *t = r->allocateContextualTest();
			readContextualTest(t, input);
			r->tests.push_back(t);
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
	t->line = (uint32_t)ntohl(u32tmp);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->pos = (uint32_t)ntohl(u32tmp);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->target = (uint32_t)ntohl(u32tmp);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->barrier = (uint32_t)ntohl(u32tmp);
	fread(&u32tmp, sizeof(uint32_t), 1, input);
	t->cbarrier = (uint32_t)ntohl(u32tmp);
	fread(&i32tmp, sizeof(int32_t), 1, input);
	t->offset = (int32_t)ntohl(i32tmp);

	fread(&u8tmp, sizeof(uint8_t), 1, input);

	if (u8tmp == 1) {
		t->linked = t->allocateContextualTest();
		readContextualTest(t->linked, input);
	}
	else {
		t->linked = 0;
	}
}

void BinaryGrammar::setCompatible(bool) {
}

void BinaryGrammar::setResult(CG3::Grammar *result) {
	grammar = result;
}

int BinaryGrammar::parse_grammar_from_file(const char *filename, const char *, const char *) {
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: Cannot parse into nothing - hint: call setResult() before trying.\n");
		return -1;
	}

	struct stat _stat;
	int error = stat(filename, &_stat);

	if (error != 0) {
		u_fprintf(ux_stderr, "Error: Cannot stat %s due to error %d - bailing out!\n", filename, error);
		exit(1);
	} else {
		grammar->last_modified = (uint32_t)_stat.st_mtime;
		grammar->grammar_size = (uint32_t)_stat.st_size;
	}

	grammar->setName(filename);

	FILE *input = fopen(filename, "rb");
	if (!input) {
		u_fprintf(ux_stderr, "Error: Error opening %s for reading!\n", filename);
		return -1;
	}
	return readBinaryGrammar(input);
}

