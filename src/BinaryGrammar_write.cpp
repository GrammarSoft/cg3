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

#include "BinaryGrammar.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"
#include "ContextualTest.hpp"
#include "version.hpp"

namespace CG3 {

int BinaryGrammar::writeBinaryGrammar(std::ostream& output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		CG3Quit(1);
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		CG3Quit(1);
	}
	uint32_t fields = 0;
	uint32_t u32tmp = 0;
	int32_t i32tmp = 0;
	UErrorCode err = U_ZERO_ERROR;
	UConverter* conv = ucnv_open("UTF-8", &err);
	std::ostringstream buffer;

	output.write("CG3B", 4);

	// Write out the revision of the binary format
	writeBE(output, CG3_FEATURE_REV);

	if (grammar->has_dep) {
		fields |= BINF_DEP;
	}
	if (grammar->mapping_prefix) {
		fields |= BINF_PREFIX;
	}
	if (grammar->sub_readings_ltr) {
		fields |= BINF_SUB_LTR;
	}
	if (!grammar->single_tags_list.empty()) {
		fields |= BINF_TAGS;
	}
	if (!grammar->reopen_mappings.empty()) {
		fields |= BINF_REOPEN_MAP;
	}
	if (!grammar->preferred_targets.empty()) {
		fields |= BINF_PREF_TARGETS;
	}
	if (!grammar->parentheses.empty()) {
		fields |= BINF_ENCLS;
	}
	if (!grammar->anchors.empty()) {
		fields |= BINF_ANCHORS;
	}
	if (!grammar->sets_list.empty()) {
		fields |= BINF_SETS;
	}
	if (grammar->delimiters) {
		fields |= BINF_DELIMS;
	}
	if (grammar->soft_delimiters) {
		fields |= BINF_SOFT_DELIMS;
	}
	if (!grammar->contexts.empty()) {
		fields |= BINF_CONTEXTS;
	}
	if (!grammar->rule_by_number.empty()) {
		fields |= BINF_RULES;
	}
	if (grammar->has_relations) {
		fields |= BINF_RELATIONS;
	}
	if (grammar->has_bag_of_tags) {
		fields |= BINF_BAG;
	}
	if (grammar->ordered) {
		fields |= BINF_ORDERED;
	}
	if (grammar->text_delimiters) {
		fields |= BINF_TEXT_DELIMS;
	}
	if (grammar->addcohort_attach) {
		fields |= BINF_ADDCOHORT_ATTACH;
	}

	writeBE(output, fields);

	if (grammar->mapping_prefix) {
		ucnv_reset(conv);
		i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, &grammar->mapping_prefix, 1, &err);
		writeBE(output, UI32(i32tmp));
		output.write(&cbuffers[0][0], i32tmp);
	}

	i32tmp = SI32(grammar->cmdargs.size());
	writeBE(output, UI32(i32tmp));
	if (i32tmp) {
		output.write(&grammar->cmdargs[0], i32tmp);
	}

	i32tmp = SI32(grammar->cmdargs_override.size());
	writeBE(output, UI32(i32tmp));
	if (i32tmp) {
		output.write(&grammar->cmdargs_override[0], i32tmp);
	}

	if (grammar->num_tags) {
		writeBE(output, UI32(grammar->num_tags));
	}
	for (size_t i = 0; i < grammar->num_tags; ++i) {
		auto t = grammar->single_tags_list[i];
		uint32_t fields = 0;
		buffer.str("");
		buffer.clear();

		if (t->number) {
			fields |= (1 << 0);
			writeBE(buffer, t->number);
		}
		if (t->hash) {
			fields |= (1 << 1);
			writeBE(buffer, t->hash);
		}
		if (t->plain_hash) {
			fields |= (1 << 2);
			writeBE(buffer, t->plain_hash);
		}
		if (t->seed) {
			fields |= (1 << 3);
			writeBE(buffer, t->seed);
		}
		if (t->type) {
			fields |= (1 << 4);
			writeBE(buffer, t->type);
		}

		if (t->comparison_hash) {
			fields |= (1 << 5);
			writeBE(buffer, t->comparison_hash);
		}
		if (t->comparison_op) {
			fields |= (1 << 6);
			writeBE<uint32_t>(buffer, t->comparison_op);
		}
		// ToDo: Field 1 << 7 cannot be reused until hard format break
		if (t->comparison_val) {
			fields |= (1 << 12);
			writeBE(buffer, t->comparison_val);
		}

		if (!t->tag.empty()) {
			fields |= (1 << 8);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, t->tag.data(), SI32(t->tag.size()), &err);
			writeBE(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		if (t->regexp) {
			fields |= (1 << 9);
			int32_t len = 0;
			const UChar* p = uregex_pattern(t->regexp, &len, &err);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, p, len, &err);
			writeBE(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		if (t->vs_sets) {
			fields |= (1 << 10);
			writeBE<uint32_t>(buffer, UI32(t->vs_sets->size()));
			for (auto iter : *t->vs_sets) {
				writeBE(buffer, iter->number);
			}
		}
		if (t->vs_names) {
			fields |= (1 << 11);
			writeBE<uint32_t>(buffer, UI32(t->vs_names->size()));
			for (const auto& iter : *t->vs_names) {
				ucnv_reset(conv);
				i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, iter.data(), SI32(iter.size()), &err);
				writeBE(buffer, i32tmp);
				buffer.write(&cbuffers[0][0], i32tmp);
			}
		}
		// 1 << 12 used above
		if ((t->type & (T_VARIABLE|T_LOCAL_VARIABLE)) && t->variable_hash) {
			fields |= (1 << 13);
			writeBE(buffer, t->variable_hash);
		}
		if (t->type & T_CONTEXT) {
			fields |= (1 << 14);
			writeBE(buffer, t->context_ref_pos);
		}

		writeBE(output, fields);
		const auto& str = buffer.str();
		output.write(str.data(), str.size());
	}

	if (!grammar->reopen_mappings.empty()) {
		writeBE(output, UI32(grammar->reopen_mappings.size()));
	}
	for (auto iter : grammar->reopen_mappings) {
		writeBE(output, iter);
	}

	if (!grammar->preferred_targets.empty()) {
		writeBE(output, UI32(grammar->preferred_targets.size()));
	}
	for (auto iter : grammar->preferred_targets) {
		writeBE(output, iter);
	}

	if (!grammar->parentheses.empty()) {
		writeBE(output, UI32(grammar->parentheses.size()));
	}
	for (const auto& iter_par : grammar->parentheses) {
		writeBE(output, iter_par.first);
		writeBE(output, iter_par.second);
	}

	if (!grammar->anchors.empty()) {
		writeBE(output, UI32(grammar->anchors.size()));
	}
	for (const auto& iter_anchor : grammar->anchors) {
		writeBE(output, iter_anchor.first);
		writeBE(output, iter_anchor.second);
	}

	if (!grammar->sets_list.empty()) {
		writeBE(output, UI32(grammar->sets_list.size()));
	}
	for (auto s : grammar->sets_list) {
		uint32_t fields = 0;
		buffer.str("");
		buffer.clear();

		if (s->number) {
			fields |= (1 << 0);
			writeBE(buffer, s->number);
		}
		if (s->type >= ST_ORDERED) {
			fields |= (1 << 1);
			writeBE(buffer, UI32(s->type));
		}
		else {
			fields |= (1 << 2);
			writeBE(buffer, UI8(s->type));
		}
		if (!s->getNonEmpty().empty()) {
			fields |= (1 << 3);
			writeBE<uint32_t>(buffer, UI32(s->trie.size()));
			trie_serialize(s->trie, buffer);
			writeBE<uint32_t>(buffer, UI32(s->trie_special.size()));
			trie_serialize(s->trie_special, buffer);
		}
		if (!s->set_ops.empty()) {
			fields |= (1 << 4);
			writeBE<uint32_t>(buffer, UI32(s->set_ops.size()));
			for (auto iter : s->set_ops) {
				writeBE(buffer, iter);
			}
		}
		if (!s->sets.empty()) {
			fields |= (1 << 5);
			writeBE<uint32_t>(buffer, UI32(s->sets.size()));
			for (auto iter : s->sets) {
				writeBE(buffer, iter);
			}
		}
		if (s->type & ST_STATIC) {
			fields |= (1 << 6);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, s->name.data(), SI32(s->name.size()), &err);
			writeBE(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		writeBE(output, fields);
		const auto& str = buffer.str();
		output.write(str.data(), str.size());
	}

	if (grammar->delimiters) {
		writeBE(output, grammar->delimiters->number);
	}

	if (grammar->soft_delimiters) {
		writeBE(output, grammar->soft_delimiters->number);
	}

	if (grammar->text_delimiters) {
		writeBE(output, grammar->text_delimiters->number);
	}

	seen_uint32.clear();
	if (!grammar->contexts.empty()) {
		writeBE(output, UI32(grammar->contexts.size()));
	}
	for (auto& cntx : grammar->contexts) {
		writeContextualTest(cntx.second, output);
	}

	if (!grammar->rule_by_number.empty()) {
		writeBE(output, UI32(grammar->rule_by_number.size()));
	}
	for (auto r : grammar->rule_by_number) {
		uint32_t fields = 0;
		buffer.str("");
		buffer.clear();

		if (r->section) {
			fields |= (1 << 0);
			writeBE(buffer, r->section);
		}
		if (r->type) {
			fields |= (1 << 1);
			writeBE(buffer, UI32(r->type));
		}
		if (r->line) {
			fields |= (1 << 2);
			writeBE(buffer, r->line);
		}
		if (r->flags) {
			fields |= (1 << 3);
			if (r->flags > std::numeric_limits<uint32_t>::max()) {
				fields |= (1 << 16);
				writeBE(buffer, r->flags);
			}
			else {
				writeBE(buffer, UI32(r->flags));
			}
		}
		if (!r->name.empty()) {
			fields |= (1 << 4);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, r->name.data(), SI32(r->name.size()), &err);
			writeBE(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}
		if (r->target) {
			fields |= (1 << 5);
			writeBE(buffer, r->target);
		}
		if (r->wordform) {
			fields |= (1 << 6);
			writeBE(buffer, r->wordform->number);
		}
		if (r->varname) {
			fields |= (1 << 7);
			writeBE(buffer, r->varname);
		}
		if (r->varvalue) {
			fields |= (1 << 8);
			writeBE(buffer, r->varvalue);
		}
		if (r->sub_reading) {
			fields |= (1 << 9);
			uint32_t v = std::abs(r->sub_reading);
			if (r->sub_reading < 0) {
				v |= (1 << 31);
			}
			writeBE(buffer, v);
		}
		if (r->childset1) {
			fields |= (1 << 10);
			writeBE(buffer, r->childset1);
		}
		if (r->childset2) {
			fields |= (1 << 11);
			writeBE(buffer, r->childset2);
		}
		if (r->maplist) {
			fields |= (1 << 12);
			writeBE(buffer, r->maplist->number);
		}
		if (r->sublist) {
			fields |= (1 << 13);
			writeBE(buffer, r->sublist->number);
		}
		if (r->number) {
			fields |= (1 << 14);
			writeBE(buffer, r->number);
		}
		if (!r->sub_rules.empty()) {
			fields |= (1 << 15);
		}

		writeBE(output, fields);
		const auto& str = buffer.str();
		output.write(str.data(), str.size());

		u32tmp = 0;
		if (r->dep_target) {
			u32tmp = r->dep_target->hash;
		}
		writeBE(output, u32tmp);

		r->reverseContextualTests();
		writeBE(output, UI32(r->dep_tests.size()));
		for (auto it : r->dep_tests) {
			writeBE(output, it->hash);
		}

		writeBE(output, UI32(r->tests.size()));
		for (auto it : r->tests) {
			writeBE(output, it->hash);
		}

		if (!r->sub_rules.empty()) {
			writeBE(output, UI32(r->sub_rules.size()));
			for (auto it : r->sub_rules) {
				writeBE(output, it->number);
			}
		}
	}

	ucnv_close(conv);
	return 0;
}

void BinaryGrammar::writeContextualTest(ContextualTest* t, std::ostream& output) {
	if (seen_uint32.count(t->hash)) {
		return;
	}
	seen_uint32.insert(t->hash);

	if (t->tmpl) {
		writeContextualTest(t->tmpl, output);
	}
	for (auto iter : t->ors) {
		writeContextualTest(iter, output);
	}
	if (t->linked) {
		writeContextualTest(t->linked, output);
	}

	std::ostringstream buffer;
	uint32_t fields = 0;

	if (t->hash) {
		fields |= (1 << 0);
		writeBE(buffer, t->hash);
	}
	else {
		u_fprintf(ux_stderr, "Error: Context on line %u had hash 0!\n", t->line);
		CG3Quit(1);
	}
	if (t->pos) {
		fields |= (1 << 1);
		writeBE(buffer, UI32(t->pos & 0xFFFFFFFF));
		if (t->pos & POS_64BIT) {
			writeBE(buffer, UI32((t->pos >> 32) & 0xFFFFFFFF));
		}
	}
	if (t->offset) {
		fields |= (1 << 2);
		writeBE(buffer, t->offset);
	}
	if (t->tmpl) {
		fields |= (1 << 3);
		writeBE(buffer, t->tmpl->hash);
	}
	if (t->target) {
		fields |= (1 << 4);
		writeBE(buffer, t->target);
	}
	if (t->line) {
		fields |= (1 << 5);
		writeBE(buffer, t->line);
	}
	if (t->relation) {
		fields |= (1 << 6);
		writeBE(buffer, t->relation);
	}
	if (t->barrier) {
		fields |= (1 << 7);
		writeBE(buffer, t->barrier);
	}
	if (t->cbarrier) {
		fields |= (1 << 8);
		writeBE(buffer, t->cbarrier);
	}
	if (t->offset_sub) {
		fields |= (1 << 9);
		writeBE(buffer, t->offset_sub);
	}
	if (!t->ors.empty()) {
		fields |= (1 << 10);
	}
	if (t->linked) {
		fields |= (1 << 11);
	}
	if (t->jump_pos) {
		fields |= (1 << 12);
		writeBE(buffer, t->jump_pos);
	}

	writeBE(output, fields);
	const auto& str = buffer.str();
	output.write(str.data(), str.size());

	if (!t->ors.empty()) {
		writeBE(output, UI32(t->ors.size()));

		for (auto iter : t->ors) {
			writeBE(output, iter->hash);
		}
	}

	if (t->linked) {
		writeBE(output, t->linked->hash);
	}
}
}
