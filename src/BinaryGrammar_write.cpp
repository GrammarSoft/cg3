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

#include "BinaryGrammar.h"
#include "Strings.h"
#include "Grammar.h"
#include "ContextualTest.h"
#include "version.h"

namespace CG3 {

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
	int32_t i32tmp = 0;
	uint8_t u8tmp = 0;
	UErrorCode err = U_ZERO_ERROR;
	UConverter *conv = ucnv_open("UTF-8", &err);
	std::ostringstream buffer;

	fprintf(output, "CG3B");

	// Write out the revision of the binary format
	u32tmp = (uint32_t)htonl((uint32_t)CG3_REVISION);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);

	u8tmp = (uint8_t)grammar->has_dep;
	fwrite(&u8tmp, sizeof(uint8_t), 1, output);

	ucnv_reset(conv);
	i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE-1, &grammar->mapping_prefix, 1, &err);
	u32tmp = (uint32_t)htonl((uint32_t)i32tmp);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	fwrite(&cbuffers[0][0], i32tmp, 1, output);

	u32tmp = (uint32_t)htonl((uint32_t)grammar->single_tags_list.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::vector<Tag*>::const_iterator tags_iter;
	for (tags_iter = grammar->single_tags_list.begin() ; tags_iter != grammar->single_tags_list.end() ; tags_iter++) {
		const Tag *t = *tags_iter;

		uint32_t fields = 0;
		buffer.str("");
		buffer.clear();

		if (t->number) {
			fields |= (1 << 0);
			writeSwapped(buffer, t->number);
		}
		if (t->hash) {
			fields |= (1 << 1);
			writeSwapped(buffer, t->hash);
		}
		if (t->plain_hash) {
			fields |= (1 << 2);
			writeSwapped(buffer, t->plain_hash);
		}
		if (t->seed) {
			fields |= (1 << 3);
			writeSwapped(buffer, t->seed);
		}
		if (t->type) {
			fields |= (1 << 4);
			writeSwapped(buffer, t->type);
		}

		if (t->comparison_hash) {
			fields |= (1 << 5);
			writeSwapped(buffer, t->comparison_hash);
		}
		if (t->comparison_op) {
			fields |= (1 << 6);
			writeSwapped<uint32_t>(buffer, t->comparison_op);
		}
		if (t->comparison_val) {
			fields |= (1 << 7);
			writeSwapped(buffer, t->comparison_val);
		}

		if (!t->tag.empty()) {
			fields |= (1 << 8);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE-1, t->tag.c_str(), t->tag.length(), &err);
			writeSwapped(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		if (t->regexp) {
			fields |= (1 << 9);
			int32_t len = 0;
			const UChar *p = uregex_pattern(t->regexp, &len, &err);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE-1, p, len, &err);
			writeSwapped(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		if (t->vs_sets) {
			fields |= (1 << 10);
			writeSwapped<uint32_t>(buffer, t->vs_sets->size());
			const_foreach (SetVector, *t->vs_sets, iter, iter_end) {
				writeSwapped(buffer, (*iter)->number);
			}
		}
		if (t->vs_names) {
			fields |= (1 << 11);
			writeSwapped<uint32_t>(buffer, t->vs_names->size());
			const_foreach (std::vector<UString>, *t->vs_names, iter, iter_end) {
				ucnv_reset(conv);
				i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE-1, (*iter).c_str(), (*iter).length(), &err);
				writeSwapped(buffer, i32tmp);
				buffer.write(&cbuffers[0][0], i32tmp);
			}
		}

		u32tmp = (uint32_t)htonl(fields);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		fwrite(buffer.str().c_str(), buffer.str().length(), 1, output);
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->tags_list.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::vector<CompositeTag*>::const_iterator comp_iter;
	for (comp_iter = grammar->tags_list.begin() ; comp_iter != grammar->tags_list.end() ; comp_iter++) {
		const CompositeTag *curcomptag = *comp_iter;
		u32tmp = (uint32_t)htonl((uint32_t)curcomptag->number);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)curcomptag->hash);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u8tmp = (uint8_t)curcomptag->is_special;
		fwrite(&u8tmp, sizeof(uint8_t), 1, output);

		u32tmp = (uint32_t)htonl((uint32_t)curcomptag->tags.size());
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		const_foreach (TagList, curcomptag->tags, tag_iter, tag_iter_end) {
			u32tmp = (uint32_t)htonl((*tag_iter)->number);
			fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		}
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->preferred_targets.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	uint32Vector::const_iterator iter;
	for (iter = grammar->preferred_targets.begin() ; iter != grammar->preferred_targets.end() ; iter++ ) {
		u32tmp = (uint32_t)htonl((uint32_t)*iter);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->parentheses.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	const_foreach (uint32Map, grammar->parentheses, iter_par, iter_par_end) {
		u32tmp = (uint32_t)htonl((uint32_t)iter_par->first);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)iter_par->second);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->anchor_by_hash.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	const_foreach (uint32Map, grammar->anchor_by_hash, iter_anchor, iter_anchor_end) {
		u32tmp = (uint32_t)htonl((uint32_t)iter_anchor->first);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		u32tmp = (uint32_t)htonl((uint32_t)iter_anchor->second);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}

	u32tmp = (uint32_t)htonl((uint32_t)grammar->sets_list.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	std::vector<Set*>::const_iterator set_iter;
	for (set_iter = grammar->sets_list.begin() ; set_iter != grammar->sets_list.end() ; set_iter++) {
		Set *s = *set_iter;

		uint32_t fields = 0;
		buffer.str("");
		buffer.clear();

		if (s->number) {
			fields |= (1 << 0);
			writeSwapped(buffer, s->number);
		}
		if (s->hash) {
			fields |= (1 << 1);
			writeSwapped(buffer, s->hash);
		}
		if (s->type) {
			fields |= (1 << 2);
			writeSwapped(buffer, s->type);
		}
		if (!s->tags_list.empty()) {
			fields |= (1 << 3);
			writeSwapped<uint32_t>(buffer, s->tags_list.size());
			const_foreach (AnyTagVector, s->tags_list, iter, iter_end) {
				writeSwapped(buffer, iter->which);
				writeSwapped(buffer, iter->number());
			}
		}
		if (!s->set_ops.empty()) {
			fields |= (1 << 4);
			writeSwapped<uint32_t>(buffer, s->set_ops.size());
			const_foreach (uint32Vector, s->set_ops, iter, iter_end) {
				writeSwapped(buffer, *iter);
			}
		}
		if (!s->sets.empty()) {
			fields |= (1 << 5);
			writeSwapped<uint32_t>(buffer, s->sets.size());
			const_foreach (uint32Vector, s->sets, iter, iter_end) {
				writeSwapped(buffer, *iter);
			}
		}
		if (s->type & ST_STATIC) {
			fields |= (1 << 6);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE-1, s->name.c_str(), s->name.length(), &err);
			writeSwapped(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		u32tmp = (uint32_t)htonl(fields);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		fwrite(buffer.str().c_str(), buffer.str().length(), 1, output);
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

	u32tmp = (uint32_t)htonl((uint32_t)grammar->rule_by_number.size());
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	const_foreach (RuleVector, grammar->rule_by_number, rule_iter, rule_iter_end) {
		Rule *r = *rule_iter;

		uint32_t fields = 0;
		buffer.str("");
		buffer.clear();

		if (r->section) {
			fields |= (1 << 0);
			writeSwapped(buffer, r->section);
		}
		if (r->type) {
			fields |= (1 << 1);
			writeSwapped(buffer, r->type);
		}
		if (r->line) {
			fields |= (1 << 2);
			writeSwapped(buffer, r->line);
		}
		if (r->flags) {
			fields |= (1 << 3);
			writeSwapped(buffer, r->flags);
		}
		if (r->name) {
			fields |= (1 << 4);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE-1, r->name, u_strlen(r->name), &err);
			writeSwapped(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}
		if (r->target) {
			fields |= (1 << 5);
			writeSwapped(buffer, r->target);
		}
		if (r->wordform) {
			fields |= (1 << 6);
			writeSwapped(buffer, r->wordform);
		}
		if (r->varname) {
			fields |= (1 << 7);
			writeSwapped(buffer, r->varname);
		}
		if (r->varvalue) {
			fields |= (1 << 8);
			writeSwapped(buffer, r->varvalue);
		}
		if (r->jumpstart) {
			fields |= (1 << 9);
			writeSwapped(buffer, r->jumpstart);
		}
		if (r->jumpend) {
			fields |= (1 << 10);
			writeSwapped(buffer, r->jumpend);
		}
		if (r->childset1) {
			fields |= (1 << 11);
			writeSwapped(buffer, r->childset1);
		}
		if (r->childset2) {
			fields |= (1 << 12);
			writeSwapped(buffer, r->childset2);
		}
		if (r->maplist) {
			fields |= (1 << 13);
			writeSwapped(buffer, r->maplist->number);
		}
		if (r->sublist) {
			fields |= (1 << 14);
			writeSwapped(buffer, r->sublist->number);
		}
		if (r->number) {
			fields |= (1 << 15);
			writeSwapped(buffer, r->number);
		}

		u32tmp = (uint32_t)htonl(fields);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		fwrite(buffer.str().c_str(), buffer.str().length(), 1, output);

		if (r->dep_target) {
			u8tmp = (uint8_t)1;
			fwrite(&u8tmp, sizeof(uint8_t), 1, output);
			writeContextualTest(r->dep_target, output);
		}
		else {
			u8tmp = (uint8_t)0;
			fwrite(&u8tmp, sizeof(uint8_t), 1, output);
		}

		r->reverseContextualTests();
		u32tmp = 0;
		ContextualTest *test = r->dep_test_head;
		while (test) {
			u32tmp++;
			test = test->next;
		}
		u32tmp = (uint32_t)htonl(u32tmp);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		test = r->dep_test_head;
		while (test) {
			writeContextualTest(test, output);
			test = test->next;
		}

		u32tmp = 0;
		test = r->test_head;
		while (test) {
			u32tmp++;
			test = test->next;
		}
		u32tmp = (uint32_t)htonl(u32tmp);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
		test = r->test_head;
		while (test) {
			writeContextualTest(test, output);
			test = test->next;
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

	u32tmp = (uint32_t)htonl((uint32_t)t->pos);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	i32tmp = (int32_t)htonl(t->offset);
	fwrite(&i32tmp, sizeof(int32_t), 1, output);

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

		u32tmp = (uint32_t)htonl((uint32_t)t->target);
		fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	}

	u32tmp = (uint32_t)htonl((uint32_t)t->line);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	u32tmp = (uint32_t)htonl((uint32_t)t->relation);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	u32tmp = (uint32_t)htonl((uint32_t)t->barrier);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);
	u32tmp = (uint32_t)htonl((uint32_t)t->cbarrier);
	fwrite(&u32tmp, sizeof(uint32_t), 1, output);

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

}
