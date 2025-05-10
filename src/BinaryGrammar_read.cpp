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

int BinaryGrammar::parse_grammar(const UChar*, size_t) {
	throw "UChar* interface doesn't make sense for binary grammars.";
}
int BinaryGrammar::parse_grammar(UString&) {
	throw "UString interface doesn't make sense for binary grammars.";
}

int BinaryGrammar::parse_grammar(const std::string& buffer) {
	return parse_grammar(buffer.data(), buffer.size());
}

int BinaryGrammar::parse_grammar(const char* buffer, size_t length) {
	std::stringstream input;
	input.write(buffer, length);
	input.seekg(0);
	return parse_grammar(input);
}

int BinaryGrammar::parse_grammar(std::istream& input) {
	input.exceptions(std::ios::failbit | std::ios::eofbit | std::ios::badbit);

	uint32_t u32tmp = 0;
	int32_t i32tmp = 0;
	UErrorCode err = U_ZERO_ERROR;
	UConverter* conv = ucnv_open("UTF-8", &err);
	std::stringstream buffer;

	if (!input.read(&cbuffers[0][0], 4)) {
		std::cerr << "Error: Error reading first 4 bytes from grammar!" << std::endl;
		CG3Quit(1);
	}
	if (!is_cg3b(cbuffers[0])) {
		u_fprintf(ux_stderr, "Error: Grammar does not begin with magic bytes - cannot load as binary!\n");
		CG3Quit(1);
	}

	auto bin_revision = readBE<uint32_t>(input);
	if (bin_revision <= BIN_REV_ANCIENT) {
		if (verbosity >= 1) {
			u_fprintf(ux_stderr, "Warning: Grammar revision is %u, but current format is %u or later. Please recompile the binary grammar with latest CG-3.\n", bin_revision, CG3_FEATURE_REV);
			u_fflush(ux_stderr);
		}
		input.seekg(0);
		return readBinaryGrammar_10043(input);
	}
	if (bin_revision < CG3_TOO_OLD) {
		u_fprintf(ux_stderr, "Error: Grammar revision is %u, but this loader requires %u or later!\n", bin_revision, CG3_TOO_OLD);
		CG3Quit(1);
	}
	if (bin_revision > CG3_FEATURE_REV) {
		u_fprintf(ux_stderr, "Error: Grammar revision is %u, but this loader only knows up to revision %u!\n", bin_revision, CG3_FEATURE_REV);
		CG3Quit(1);
	}

	grammar->is_binary = true;

	auto fields = readBE<uint32_t>(input);

	grammar->has_dep = (fields & BINF_DEP) != 0;
	grammar->sub_readings_ltr = (fields & BINF_SUB_LTR) != 0;
	grammar->has_relations = (fields & BINF_RELATIONS) != 0;
	grammar->has_bag_of_tags = (fields & BINF_BAG) != 0;
	grammar->ordered = (fields & BINF_ORDERED) != 0;
	grammar->addcohort_attach = (fields & BINF_ADDCOHORT_ATTACH) != 0;

	if (fields & BINF_PREFIX) {
		ucnv_reset(conv);
		u32tmp = readBE<uint32_t>(input);
		input.read(&cbuffers[0][0], u32tmp);
		i32tmp = ucnv_toUChars(conv, &grammar->mapping_prefix, 1, &cbuffers[0][0], u32tmp, &err);
	}

	if (bin_revision >= BIN_REV_CMDARGS) {
		u32tmp = readBE<uint32_t>(input);
		if (u32tmp) {
			grammar->cmdargs.resize(u32tmp);
			input.read(&grammar->cmdargs[0], u32tmp);
		}

		u32tmp = readBE<uint32_t>(input);
		if (u32tmp) {
			grammar->cmdargs_override.resize(u32tmp);
			input.read(&grammar->cmdargs_override[0], u32tmp);
		}
	}

	// Keep track of which sets that the varstring tags used; we can't just assign them as sets are not loaded yet
	typedef std::map<uint32_t, uint32Vector> tag_varsets_t;
	tag_varsets_t tag_varsets;

	u32tmp = 0;
	if (fields & BINF_TAGS) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_single_tags = u32tmp;
	grammar->num_tags = num_single_tags;
	grammar->single_tags_list.resize(num_single_tags);
	for (uint32_t i = 0; i < num_single_tags; i++) {
		Tag* t = grammar->allocateTag();

		auto fields = readBE<uint32_t>(input);

		if (fields & (1 << 0)) {
			t->number = readBE<uint32_t>(input);
		}
		if (fields & (1 << 1)) {
			t->hash = readBE<uint32_t>(input);
		}
		if (fields & (1 << 2)) {
			t->plain_hash = readBE<uint32_t>(input);
		}
		if (fields & (1 << 3)) {
			t->seed = readBE<uint32_t>(input);
		}
		if (fields & (1 << 4)) {
			t->type = readBE<uint32_t>(input);
		}

		if (fields & (1 << 5)) {
			t->comparison_hash = readBE<uint32_t>(input);
		}
		if (fields & (1 << 6)) {
			t->comparison_op = static_cast<C_OPS>(readBE<uint32_t>(input));
		}
		if (fields & (1 << 7)) {
			t->comparison_val = readBE<int32_t>(input);
			if (t->comparison_val <= std::numeric_limits<int32_t>::min()) {
				t->comparison_val = NUMERIC_MIN;
			}
			if (t->comparison_val >= std::numeric_limits<int32_t>::max()) {
				t->comparison_val = NUMERIC_MAX;
			}
		}
		if (fields & (1 << 12)) {
			char buf[sizeof(uint64_t) + sizeof(int32_t)] = {};
			input.read(&buf[0], sizeof(buf));
			buffer.str("");
			buffer.clear();
			buffer.write(buf, sizeof(buf));
			t->comparison_val = readBE<double>(buffer);
		}

		if (fields & (1 << 8)) {
			u32tmp = readBE<uint32_t>(input);
			if (u32tmp) {
				ucnv_reset(conv);
				input.read(&cbuffers[0][0], u32tmp);
				i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);
				t->tag = &gbuffers[0][0];
			}
		}

		if (fields & (1 << 9)) {
			u32tmp = readBE<uint32_t>(input);
			if (u32tmp) {
				ucnv_reset(conv);
				input.read(&cbuffers[0][0], u32tmp);
				i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);

				UParseError pe;
				UErrorCode status = U_ZERO_ERROR;

				if (t->type & T_CASE_INSENSITIVE) {
					t->regexp = uregex_open(&gbuffers[0][0], i32tmp, UREGEX_CASE_INSENSITIVE, &pe, &status);
				}
				else {
					t->regexp = uregex_open(&gbuffers[0][0], i32tmp, 0, &pe, &status);
				}
				if (status != U_ZERO_ERROR) {
					u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse tag %S - cannot continue!\n", u_errorName(status), t->tag.data());
					CG3Quit(1);
				}
			}
		}

		if (fields & (1 << 10)) {
			auto num = readBE<uint32_t>(input);
			t->allocateVsSets();
			t->vs_sets->reserve(num);
			tag_varsets[t->number].reserve(num);
			for (size_t i = 0; i < num; ++i) {
				u32tmp = readBE<uint32_t>(input);
				tag_varsets[t->number].push_back(u32tmp);
			}
		}
		if (fields & (1 << 11)) {
			auto num = readBE<uint32_t>(input);
			t->allocateVsNames();
			t->vs_names->reserve(num);
			for (size_t i = 0; i < num; ++i) {
				u32tmp = readBE<uint32_t>(input);
				if (u32tmp) {
					ucnv_reset(conv);
					input.read(&cbuffers[0][0], u32tmp);
					i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);
					t->vs_names->push_back(&gbuffers[0][0]);
				}
			}
		}
		// 1 << 12 used earlier
		if (fields & (1 << 13)) {
			t->variable_hash = readBE<uint32_t>(input);
		}
		if (fields & (1 << 14)) {
			t->context_ref_pos = readBE<uint32_t>(input);
		}

		grammar->single_tags[t->hash] = t;
		grammar->single_tags_list[t->number] = t;
		if (t->tag.size() == 1 && t->tag[0] == '*') {
			grammar->tag_any = t->hash;
		}
	}

	u32tmp = 0;
	if (fields & BINF_REOPEN_MAP) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_remaps = u32tmp;
	for (uint32_t i = 0; i < num_remaps; ++i) {
		u32tmp = readBE<uint32_t>(input);
		grammar->reopen_mappings.insert(u32tmp);
	}

	u32tmp = 0;
	if (fields & BINF_PREF_TARGETS) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_pref_targets = u32tmp;
	for (uint32_t i = 0; i < num_pref_targets; i++) {
		u32tmp = readBE<uint32_t>(input);
		grammar->preferred_targets.push_back(u32tmp);
	}

	u32tmp = 0;
	if (fields & BINF_ENCLS) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_par_pairs = u32tmp;
	for (uint32_t i = 0; i < num_par_pairs; i++) {
		auto left = readBE<uint32_t>(input);
		auto right = readBE<uint32_t>(input);
		grammar->parentheses[left] = right;
		grammar->parentheses_reverse[right] = left;
	}

	u32tmp = 0;
	if (fields & BINF_ANCHORS) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_par_anchors = u32tmp;
	for (uint32_t i = 0; i < num_par_anchors; i++) {
		auto left = readBE<uint32_t>(input);
		auto right = readBE<uint32_t>(input);
		grammar->anchors[left] = right;
	}

	u32tmp = 0;
	if (fields & BINF_SETS) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_sets = u32tmp;
	grammar->sets_list.resize(num_sets);
	for (uint32_t i = 0; i < num_sets; i++) {
		Set* s = grammar->allocateSet();

		auto fields = readBE<uint32_t>(input);

		if (fields & (1 << 0)) {
			s->number = readBE<uint32_t>(input);
		}
		if (fields & (1 << 1)) {
			s->type = UI16(readBE<uint32_t>(input));
		}
		if (fields & (1 << 2)) {
			s->type = readBE<uint8_t>(input);
		}

		if (fields & (1 << 3)) {
			u32tmp = readBE<uint32_t>(input);
			if (u32tmp) {
				trie_unserialize(s->trie, input, *grammar, u32tmp);
			}
			u32tmp = readBE<uint32_t>(input);
			if (u32tmp) {
				trie_unserialize(s->trie_special, input, *grammar, u32tmp);
			}
		}
		if (fields & (1 << 4)) {
			u32tmp = readBE<uint32_t>(input);
			auto num_set_ops = u32tmp;
			for (uint32_t j = 0; j < num_set_ops; j++) {
				u32tmp = readBE<uint32_t>(input);
				s->set_ops.push_back(u32tmp);
			}
		}
		if (fields & (1 << 5)) {
			u32tmp = readBE<uint32_t>(input);
			auto num_sets = u32tmp;
			for (uint32_t j = 0; j < num_sets; j++) {
				u32tmp = readBE<uint32_t>(input);
				s->sets.push_back(u32tmp);
			}
		}
		if (fields & (1 << 6)) {
			u32tmp = readBE<uint32_t>(input);
			if (u32tmp) {
				ucnv_reset(conv);
				input.read(&cbuffers[0][0], u32tmp);
				i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);
				s->setName(&gbuffers[0][0]);
			}
		}
		grammar->sets_list[s->number] = s;
	}

	// Actually assign sets to the varstring tags now that sets are loaded
	for (const auto& iter : tag_varsets) {
		Tag* t = grammar->single_tags_list[iter.first];
		for (auto uit : iter.second) {
			Set* s = grammar->sets_list[uit];
			t->vs_sets->push_back(s);
		}
	}

	if (fields & BINF_DELIMS) {
		u32tmp = readBE<uint32_t>(input);
		grammar->delimiters = grammar->sets_list[u32tmp];
	}

	if (fields & BINF_SOFT_DELIMS) {
		u32tmp = readBE<uint32_t>(input);
		grammar->soft_delimiters = grammar->sets_list[u32tmp];
	}

	if (fields & BINF_TEXT_DELIMS) {
		u32tmp = readBE<uint32_t>(input);
		grammar->text_delimiters = grammar->sets_list[u32tmp];
	}

	u32tmp = 0;
	if (fields & BINF_CONTEXTS) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_contexts = u32tmp;
	for (uint32_t i = 0; i < num_contexts; i++) {
		ContextualTest* t = readContextualTest(input);
		grammar->contexts[t->hash] = t;
	}

	u32tmp = 0;
	if (fields & BINF_RULES) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_rules = u32tmp;
	grammar->rule_by_number.resize(num_rules);
	for (uint32_t i = 0; i < num_rules; i++) {
		Rule* r = grammar->allocateRule();

		auto fields = readBE<uint32_t>(input);

		if (fields & (1 << 0)) {
			r->section = readBE<int32_t>(input);
		}
		if (fields & (1 << 1)) {
			r->type = static_cast<KEYWORDS>(readBE<uint32_t>(input));
		}
		if (fields & (1 << 2)) {
			r->line = readBE<uint32_t>(input);
		}
		if (fields & (1 << 3)) {
			if (fields & (1 << 16)) {
				r->flags = readBE<uint64_t>(input);
			}
			else {
				r->flags = readBE<uint32_t>(input);
			}
		}
		if (fields & (1 << 4)) {
			u32tmp = readBE<uint32_t>(input);
			if (u32tmp) {
				ucnv_reset(conv);
				input.read(&cbuffers[0][0], u32tmp);
				i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);
				r->setName(&gbuffers[0][0]);
			}
		}
		if (fields & (1 << 5)) {
			r->target = readBE<uint32_t>(input);
		}
		if (fields & (1 << 6)) {
			u32tmp = readBE<uint32_t>(input);
			r->wordform = grammar->single_tags_list[u32tmp];
		}
		if (fields & (1 << 7)) {
			r->varname = readBE<uint32_t>(input);
		}
		if (fields & (1 << 8)) {
			r->varvalue = readBE<uint32_t>(input);
		}
		if (fields & (1 << 9)) {
			u32tmp = readBE<uint32_t>(input);
			int32_t v = u32tmp;
			if (u32tmp & (1 << 31)) {
				u32tmp &= ~(1 << 31);
				v = u32tmp;
				v = -v;
			}
			r->sub_reading = v;
		}
		if (fields & (1 << 10)) {
			r->childset1 = readBE<uint32_t>(input);
		}
		if (fields & (1 << 11)) {
			r->childset2 = readBE<uint32_t>(input);
		}
		if (fields & (1 << 12)) {
			u32tmp = readBE<uint32_t>(input);
			r->maplist = grammar->sets_list[u32tmp];
		}
		if (fields & (1 << 13)) {
			u32tmp = readBE<uint32_t>(input);
			r->sublist = grammar->sets_list[u32tmp];
		}
		if (fields & (1 << 14)) {
			r->number = readBE<uint32_t>(input);
		}

		u32tmp = readBE<uint32_t>(input);
		if (u32tmp) {
			r->dep_target = grammar->contexts[u32tmp];
		}

		u32tmp = readBE<uint32_t>(input);
		auto num_dep_tests = u32tmp;
		for (uint32_t j = 0; j < num_dep_tests; j++) {
			u32tmp = readBE<uint32_t>(input);
			ContextualTest* t = grammar->contexts[u32tmp];
			r->addContextualTest(t, r->dep_tests);
		}

		u32tmp = readBE<uint32_t>(input);
		auto num_tests = u32tmp;
		for (uint32_t j = 0; j < num_tests; j++) {
			u32tmp = readBE<uint32_t>(input);
			ContextualTest* t = grammar->contexts[u32tmp];
			r->addContextualTest(t, r->tests);
		}

		if (fields & (1 << 15)) {
			u32tmp = readBE<uint32_t>(input);
			auto num_sub_rules = u32tmp;
			for (uint32_t j = 0; j < num_sub_rules; j++) {
				u32tmp = readBE<uint32_t>(input);
				r->sub_rules.push_back(grammar->rule_by_number[u32tmp]);
			}
		}

		UErrorCode status;
		if (nrules) {
			status = U_ZERO_ERROR;
			uregex_setText(nrules, r->name.c_str(), SI32(r->name.size()), &status);
			status = U_ZERO_ERROR;
			if (!uregex_find(nrules, -1, &status)) {
				r->type = K_IGNORE;
			}
		}
		if (nrules_inv) {
			status = U_ZERO_ERROR;
			uregex_setText(nrules_inv, r->name.c_str(), SI32(r->name.size()), &status);
			status = U_ZERO_ERROR;
			if (uregex_find(nrules_inv, -1, &status)) {
				r->type = K_IGNORE;
			}
		}

		grammar->rule_by_number[r->number] = r;
	}

	// Bind the templates to where they are used
	for (const auto& it : deferred_tmpls) {
		it.first->tmpl = grammar->contexts.find(it.second)->second;
	}

	// Bind the OR'ed contexts to where they are used
	for (const auto& it : deferred_ors) {
		it.first->ors.reserve(it.second.size());
		for (auto orit : it.second) {
			it.first->ors.push_back(grammar->contexts.find(orit)->second);
		}
	}

	ucnv_close(conv);
	return 0;
}

ContextualTest* BinaryGrammar::readContextualTest(std::istream& input) {
	ContextualTest* t = grammar->allocateContextualTest();
	uint32_t u32tmp = 0;

	auto fields = readBE<uint32_t>(input);

	if (fields & (1 << 0)) {
		t->hash = readBE<uint32_t>(input);
	}
	if (fields & (1 << 1)) {
		t->pos = readBE<uint32_t>(input);
		if (t->pos & POS_64BIT) {
			u32tmp = readBE<uint32_t>(input);
			t->pos |= UI64(u32tmp) << 32;
		}
	}
	if (fields & (1 << 2)) {
		t->offset = readBE<int32_t>(input);
	}
	if (fields & (1 << 3)) {
		u32tmp = readBE<uint32_t>(input);
		deferred_tmpls[t] = u32tmp;
	}
	if (fields & (1 << 4)) {
		t->target = readBE<uint32_t>(input);
	}
	if (fields & (1 << 5)) {
		t->line = readBE<uint32_t>(input);
	}
	if (fields & (1 << 6)) {
		t->relation = readBE<uint32_t>(input);
	}
	if (fields & (1 << 7)) {
		t->barrier = readBE<uint32_t>(input);
	}
	if (fields & (1 << 8)) {
		t->cbarrier = readBE<uint32_t>(input);
	}
	if (fields & (1 << 9)) {
		t->offset_sub = readBE<int32_t>(input);
	}
	if (fields & (1 << 12)) {
		t->jump_pos = readBE<int8_t>(input);
	}
	if (fields & (1 << 10)) {
		auto num_ors = readBE<uint32_t>(input);
		for (uint32_t i = 0; i < num_ors; ++i) {
			u32tmp = readBE<uint32_t>(input);
			deferred_ors[t].push_back(u32tmp);
		}
	}
	if (fields & (1 << 11)) {
		u32tmp = readBE<uint32_t>(input);
		t->linked = grammar->contexts[u32tmp];
	}

	return t;
}
}
