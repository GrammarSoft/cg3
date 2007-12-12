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

#include "GBinaryWriter.h"

using namespace CG3;
using namespace CG3::Strings;

GBinaryWriter::GBinaryWriter(Grammar *res, UFILE *ux_err) {
	ux_stderr = ux_err;
	grammar = res;
}

GBinaryWriter::~GBinaryWriter() {
	grammar = 0;
}

int GBinaryWriter::writeBinaryGrammar(FILE *output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		return -1;
	}
	uint32_t u32tmp = 0;
	int32_t i32tmp = 0;
	uint8_t u8tmp = 0;
	UErrorCode err = U_ZERO_ERROR;
	UConverter *conv = ucnv_open("UTF-8", &err);

	fprintf(output, "CG3B");

	// Write out the revision of the binary format
	u32tmp = (uint32_t)htonl((uint32_t)CG3_REVISION);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);

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
		u32tmp = (uint32_t)htonl((uint32_t)t->type);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u8tmp = (uint8_t)t->is_special;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		if (t->type & T_NUMERICAL) {
			u32tmp = (uint32_t)htonl((uint32_t)t->comparison_hash);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			u32tmp = (uint32_t)htonl((uint32_t)t->comparison_op);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			i32tmp = (int32_t)htonl((int32_t)t->comparison_val);
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
		/*
		u32tmp = (uint32_t)htonl((uint32_t)s->line);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		if (s->name) {
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, cbuffers[0], BUFFER_SIZE-1, s->name, u_strlen(s->name), &err);
			u32tmp = (uint32_t)htonl((uint32_t)i32tmp);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			fwrite(cbuffers[0], i32tmp, 1, output);
		}
		else {
			u32tmp = (uint32_t)htonl((uint32_t)0);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		}
		//*/
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

	u32tmp = (uint32_t)htonl((uint32_t)grammar->rule_by_line.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::map<uint32_t, Rule*>::const_iterator rule_iter;
	for (rule_iter = grammar->rule_by_line.begin() ; rule_iter != grammar->rule_by_line.end() ; rule_iter++) {
		Rule *r = rule_iter->second;
		i32tmp = (int32_t)htonl((int32_t)r->section);
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

		/*
		if (r->name) {
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, cbuffers[0], BUFFER_SIZE-1, r->name, u_strlen(r->name), &err);
			u32tmp = (uint32_t)htonl((uint32_t)i32tmp);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			fwrite(cbuffers[0], i32tmp, 1, output);
		}
		else {
			u32tmp = (uint32_t)htonl((uint32_t)0);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		}
		//*/

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

void GBinaryWriter::writeContextualTest(ContextualTest *t, FILE *output) {
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
	i32tmp = (int32_t)htonl((int32_t)t->offset);
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
