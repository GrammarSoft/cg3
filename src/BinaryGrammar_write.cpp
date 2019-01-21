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

#include "BinaryGrammar.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"
#include "ContextualTest.hpp"
#include "version.hpp"

namespace CG3 {

int BinaryGrammar::writeBinaryGrammar(FILE* output) {
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

	fprintf(output, "CG3B");

	// Write out the revision of the binary format
	u32tmp = hton32(CG3_FEATURE_REV);
	fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);

	if (grammar->has_dep) {
		fields |= (1 << 0);
	}
	if (grammar->mapping_prefix) {
		fields |= (1 << 1);
	}
	if (grammar->sub_readings_ltr) {
		fields |= (1 << 2);
	}
	if (!grammar->single_tags_list.empty()) {
		fields |= (1 << 3);
	}
	if (!grammar->reopen_mappings.empty()) {
		fields |= (1 << 4);
	}
	if (!grammar->preferred_targets.empty()) {
		fields |= (1 << 5);
	}
	if (!grammar->parentheses.empty()) {
		fields |= (1 << 6);
	}
	if (!grammar->anchors.empty()) {
		fields |= (1 << 7);
	}
	if (!grammar->sets_list.empty()) {
		fields |= (1 << 8);
	}
	if (grammar->delimiters) {
		fields |= (1 << 9);
	}
	if (grammar->soft_delimiters) {
		fields |= (1 << 10);
	}
	if (!grammar->contexts.empty()) {
		fields |= (1 << 11);
	}
	if (!grammar->rule_by_number.empty()) {
		fields |= (1 << 12);
	}
	if (grammar->has_relations) {
		fields |= (1 << 13);
	}
	if (grammar->has_bag_of_tags) {
		fields |= (1 << 14);
	}
	if (grammar->ordered) {
		fields |= (1 << 15);
	}

	u32tmp = hton32(fields);
	fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);

	if (grammar->mapping_prefix) {
		ucnv_reset(conv);
		i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, &grammar->mapping_prefix, 1, &err);
		u32tmp = hton32((uint32_t)i32tmp);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		fwrite_throw(&cbuffers[0][0], i32tmp, 1, output);
	}

	if (!grammar->single_tags_list.empty()) {
		u32tmp = hton32((uint32_t)grammar->single_tags_list.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
	for (auto t : grammar->single_tags_list) {
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
		// ToDo: Field 1 << 7 cannot be reused until hard format break
		if (t->comparison_val) {
			fields |= (1 << 12);
			writeSwapped(buffer, t->comparison_val);
		}

		if (!t->tag.empty()) {
			fields |= (1 << 8);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, t->tag.c_str(), static_cast<int32_t>(t->tag.size()), &err);
			writeSwapped(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		if (t->regexp) {
			fields |= (1 << 9);
			int32_t len = 0;
			const UChar* p = uregex_pattern(t->regexp, &len, &err);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, p, len, &err);
			writeSwapped(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		if (t->vs_sets) {
			fields |= (1 << 10);
			writeSwapped<uint32_t>(buffer, static_cast<uint32_t>(t->vs_sets->size()));
			for (auto iter : *t->vs_sets) {
				writeSwapped(buffer, iter->number);
			}
		}
		if (t->vs_names) {
			fields |= (1 << 11);
			writeSwapped<uint32_t>(buffer, static_cast<uint32_t>(t->vs_names->size()));
			for (auto iter : *t->vs_names) {
				ucnv_reset(conv);
				i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, iter.c_str(), static_cast<int32_t>(iter.size()), &err);
				writeSwapped(buffer, i32tmp);
				buffer.write(&cbuffers[0][0], i32tmp);
			}
		}
		// 1 << 12 used above

		u32tmp = hton32(fields);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		const auto& str = buffer.str();
		fwrite_throw(str.c_str(), str.size(), 1, output);
	}

	if (!grammar->reopen_mappings.empty()) {
		u32tmp = hton32((uint32_t)grammar->reopen_mappings.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
	for (auto iter : grammar->reopen_mappings) {
		u32tmp = hton32(iter);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}

	if (!grammar->preferred_targets.empty()) {
		u32tmp = hton32((uint32_t)grammar->preferred_targets.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
	for (auto iter : grammar->preferred_targets) {
		u32tmp = hton32(iter);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}

	if (!grammar->parentheses.empty()) {
		u32tmp = hton32((uint32_t)grammar->parentheses.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
	for (auto iter_par : grammar->parentheses) {
		u32tmp = hton32(iter_par.first);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		u32tmp = hton32(iter_par.second);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}

	if (!grammar->anchors.empty()) {
		u32tmp = hton32((uint32_t)grammar->anchors.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
	for (auto iter_anchor : grammar->anchors) {
		u32tmp = hton32(iter_anchor.first);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		u32tmp = hton32(iter_anchor.second);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}

	if (!grammar->sets_list.empty()) {
		u32tmp = hton32((uint32_t)grammar->sets_list.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
	for (auto s : grammar->sets_list) {
		uint32_t fields = 0;
		buffer.str("");
		buffer.clear();

		if (s->number) {
			fields |= (1 << 0);
			writeSwapped(buffer, s->number);
		}
		// Field 1 is unused
		if (s->type) {
			fields |= (1 << 2);
			writeSwapped(buffer, s->type);
		}
		if (!s->getNonEmpty().empty()) {
			fields |= (1 << 3);
			writeSwapped<uint32_t>(buffer, static_cast<uint32_t>(s->trie.size()));
			trie_serialize(s->trie, buffer);
			writeSwapped<uint32_t>(buffer, static_cast<uint32_t>(s->trie_special.size()));
			trie_serialize(s->trie_special, buffer);
		}
		if (!s->set_ops.empty()) {
			fields |= (1 << 4);
			writeSwapped<uint32_t>(buffer, static_cast<uint32_t>(s->set_ops.size()));
			for (auto iter : s->set_ops) {
				writeSwapped(buffer, iter);
			}
		}
		if (!s->sets.empty()) {
			fields |= (1 << 5);
			writeSwapped<uint32_t>(buffer, static_cast<uint32_t>(s->sets.size()));
			for (auto iter : s->sets) {
				writeSwapped(buffer, iter);
			}
		}
		if (s->type & ST_STATIC) {
			fields |= (1 << 6);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, s->name.c_str(), static_cast<int32_t>(s->name.size()), &err);
			writeSwapped(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}

		u32tmp = hton32(fields);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		const auto& str = buffer.str();
		fwrite_throw(str.c_str(), str.size(), 1, output);
	}

	if (grammar->delimiters) {
		u32tmp = hton32(grammar->delimiters->number);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}

	if (grammar->soft_delimiters) {
		u32tmp = hton32(grammar->soft_delimiters->number);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}

	seen_uint32.clear();
	if (!grammar->contexts.empty()) {
		u32tmp = hton32((uint32_t)grammar->contexts.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
	for (auto& cntx : grammar->contexts) {
		writeContextualTest(cntx.second, output);
	}

	if (!grammar->rule_by_number.empty()) {
		u32tmp = hton32((uint32_t)grammar->rule_by_number.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
	for (auto r : grammar->rule_by_number) {
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
		if (!r->name.empty()) {
			fields |= (1 << 4);
			ucnv_reset(conv);
			i32tmp = ucnv_fromUChars(conv, &cbuffers[0][0], CG3_BUFFER_SIZE - 1, r->name.c_str(), static_cast<int32_t>(r->name.size()), &err);
			writeSwapped(buffer, i32tmp);
			buffer.write(&cbuffers[0][0], i32tmp);
		}
		if (r->target) {
			fields |= (1 << 5);
			writeSwapped(buffer, r->target);
		}
		if (r->wordform) {
			fields |= (1 << 6);
			writeSwapped(buffer, r->wordform->number);
		}
		if (r->varname) {
			fields |= (1 << 7);
			writeSwapped(buffer, r->varname);
		}
		if (r->varvalue) {
			fields |= (1 << 8);
			writeSwapped(buffer, r->varvalue);
		}
		if (r->sub_reading) {
			fields |= (1 << 9);
			uint32_t v = std::abs(r->sub_reading);
			if (r->sub_reading < 0) {
				v |= (1 << 31);
			}
			writeSwapped(buffer, v);
		}
		if (r->childset1) {
			fields |= (1 << 10);
			writeSwapped(buffer, r->childset1);
		}
		if (r->childset2) {
			fields |= (1 << 11);
			writeSwapped(buffer, r->childset2);
		}
		if (r->maplist) {
			fields |= (1 << 12);
			writeSwapped(buffer, r->maplist->number);
		}
		if (r->sublist) {
			fields |= (1 << 13);
			writeSwapped(buffer, r->sublist->number);
		}
		if (r->number) {
			fields |= (1 << 14);
			writeSwapped(buffer, r->number);
		}

		u32tmp = hton32(fields);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		const auto& str = buffer.str();
		fwrite_throw(str.c_str(), str.size(), 1, output);

		u32tmp = 0;
		if (r->dep_target) {
			u32tmp = hton32(r->dep_target->hash);
		}
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);

		r->reverseContextualTests();
		u32tmp = hton32(static_cast<unsigned long>(r->dep_tests.size()));
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		for (auto it : r->dep_tests) {
			u32tmp = hton32(it->hash);
			fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		}

		u32tmp = hton32(static_cast<unsigned long>(r->tests.size()));
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		for (auto it : r->tests) {
			u32tmp = hton32(it->hash);
			fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		}
	}

	ucnv_close(conv);
	return 0;
}

void BinaryGrammar::writeContextualTest(ContextualTest* t, FILE* output) {
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
	uint32_t u32tmp = 0;

	if (t->hash) {
		fields |= (1 << 0);
		writeSwapped(buffer, t->hash);
	}
	else {
		u_fprintf(ux_stderr, "Error: Context on line %u had hash 0!\n", t->line);
		CG3Quit(1);
	}
	if (t->pos) {
		fields |= (1 << 1);
		writeSwapped(buffer, static_cast<uint32_t>(t->pos & 0xFFFFFFFF));
		if (t->pos & POS_64BIT) {
			writeSwapped(buffer, static_cast<uint32_t>((t->pos >> 32) & 0xFFFFFFFF));
		}
	}
	if (t->offset) {
		fields |= (1 << 2);
		writeSwapped(buffer, t->offset);
	}
	if (t->tmpl) {
		fields |= (1 << 3);
		writeSwapped(buffer, t->tmpl->hash);
	}
	if (t->target) {
		fields |= (1 << 4);
		writeSwapped(buffer, t->target);
	}
	if (t->line) {
		fields |= (1 << 5);
		writeSwapped(buffer, t->line);
	}
	if (t->relation) {
		fields |= (1 << 6);
		writeSwapped(buffer, t->relation);
	}
	if (t->barrier) {
		fields |= (1 << 7);
		writeSwapped(buffer, t->barrier);
	}
	if (t->cbarrier) {
		fields |= (1 << 8);
		writeSwapped(buffer, t->cbarrier);
	}
	if (t->offset_sub) {
		fields |= (1 << 9);
		writeSwapped(buffer, t->offset_sub);
	}
	if (!t->ors.empty()) {
		fields |= (1 << 10);
	}
	if (t->linked) {
		fields |= (1 << 11);
	}

	u32tmp = hton32(fields);
	fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	const auto& str = buffer.str();
	fwrite_throw(str.c_str(), str.size(), 1, output);

	if (!t->ors.empty()) {
		u32tmp = hton32((uint32_t)t->ors.size());
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);

		for (auto iter : t->ors) {
			u32tmp = hton32(iter->hash);
			fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
		}
	}

	if (t->linked) {
		u32tmp = hton32(t->linked->hash);
		fwrite_throw(&u32tmp, sizeof(u32tmp), 1, output);
	}
}
}
