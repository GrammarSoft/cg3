/*
* Copyright (C) 2007, GrammarSoft ApS
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

using namespace CG3;
using namespace CG3::Strings;

int BinaryGrammar::writeBinaryGrammar(FILE *output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		CG3Quit(1);
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		CG3Quit(1);
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

	u32tmp = (uint32_t)htonl((uint32_t)grammar->single_tags_list.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::vector<Tag*>::const_iterator tags_iter;
	for (tags_iter = grammar->single_tags_list.begin() ; tags_iter != grammar->single_tags_list.end() ; tags_iter++) {
		Tag *t = *tags_iter;
		u32tmp = (uint32_t)htonl((uint32_t)t->number);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
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

	u32tmp = (uint32_t)htonl((uint32_t)grammar->tags_list.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::vector<CompositeTag*>::const_iterator comp_iter;
	for (comp_iter = grammar->tags_list.begin() ; comp_iter != grammar->tags_list.end() ; comp_iter++) {
		CompositeTag *curcomptag = *comp_iter;
		u32tmp = (uint32_t)htonl((uint32_t)curcomptag->number);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)curcomptag->hash);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);

		u32tmp = (uint32_t)htonl((uint32_t)curcomptag->q_tags_set.size());
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		TagSet::const_iterator tag_iter;
		for (tag_iter = curcomptag->q_tags_set.begin() ; tag_iter != curcomptag->q_tags_set.end() ; tag_iter++) {
			u32tmp = (uint32_t)htonl((*tag_iter)->number);
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

	u32tmp = (uint32_t)htonl((uint32_t)grammar->sets_list.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::vector<Set*>::const_iterator set_iter;
	for (set_iter = grammar->sets_list.begin() ; set_iter != grammar->sets_list.end() ; set_iter++) {
		Set *s = *set_iter;
		u32tmp = (uint32_t)htonl((uint32_t)s->number);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)s->hash);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u8tmp = (uint8_t)s->match_any;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		u8tmp = (uint8_t)s->is_special;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		u8tmp = (uint8_t)s->is_unified;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		u8tmp = (uint8_t)s->has_mappings;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);

		if (s->sets.empty()) {
			u8tmp = (uint8_t)0;
			fwrite(&u8tmp, sizeof(uint8_t), 1, output);
			u32tmp = (uint32_t)htonl((uint32_t)s->q_single_tags.size());
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			TagHashSet::const_iterator tomp_iter;
			for (tomp_iter = s->q_single_tags.begin() ; tomp_iter != s->q_single_tags.end() ; tomp_iter++) {
				u32tmp = (uint32_t)htonl((*tomp_iter)->number);
				fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			}
			u32tmp = (uint32_t)htonl((uint32_t)s->q_tags.size());
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
			CompositeTagHashSet::const_iterator comp_iter;
			for (comp_iter = s->q_tags.begin() ; comp_iter != s->q_tags.end() ; comp_iter++) {
				u32tmp = (uint32_t)htonl((*comp_iter)->number);
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

	u32tmp = (uint32_t)htonl((uint32_t)grammar->template_list.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::vector<ContextualTest*>::const_iterator tmpl_iter;
	for (tmpl_iter = grammar->template_list.begin() ; tmpl_iter != grammar->template_list.end() ; tmpl_iter++) {
		u32tmp = (uint32_t)htonl((uint32_t)(*tmpl_iter)->name);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		writeContextualTest(*tmpl_iter, output);
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

	if (t->tmpl) {
		u32tmp = (uint32_t)htonl((uint32_t)t->tmpl->name);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = 0;
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}
	else if (!t->ors.empty()) {
		u32tmp = 0;
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)t->ors.size());
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		std::list<ContextualTest*>::const_iterator iter;
		for (iter = t->ors.begin() ; iter != t->ors.end() ; iter++) {
			writeContextualTest(*iter, output);
		}
	}
	else {
		u32tmp = 0;
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = 0;
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
	}

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
