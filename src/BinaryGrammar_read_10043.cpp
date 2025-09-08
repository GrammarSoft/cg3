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

static thread_local std::vector<ContextualTest*> contexts_list;
static thread_local Grammar::contexts_t templates;

int BinaryGrammar::readBinaryGrammar_10043(std::istream& input) {
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
	UErrorCode err = U_ZERO_ERROR;
	UConverter* conv = ucnv_open("UTF-8", &err);

	if (!input.read(&cbuffers[0][0], 4)) {
		std::cerr << "Error: Error reading first 4 bytes from grammar!" << std::endl;
		CG3Quit(1);
	}
	if (!is_cg3b(cbuffers[0])) {
		u_fprintf(ux_stderr, "Error: Grammar does not begin with magic bytes - cannot load as binary!\n");
		CG3Quit(1);
	}

	u32tmp = readBE<uint32_t>(input);
	if (u32tmp < 10043) {
		u_fprintf(ux_stderr, "Error: Grammar revision is %u, but this loader requires %u or later!\n", u32tmp, 10043);
		CG3Quit(1);
	}

	grammar->is_binary = true;

	auto fields = readBE<uint32_t>(input);

	grammar->has_dep = (fields & (1 << 0)) != 0;
	grammar->sub_readings_ltr = (fields & (1 << 2)) != 0;
	grammar->has_relations = (fields & (1 << 13)) != 0;

	if (fields & (1 << 1)) {
		ucnv_reset(conv);
		u32tmp = readBE<uint32_t>(input);
		input.read(&cbuffers[0][0], u32tmp);
		i32tmp = ucnv_toUChars(conv, &grammar->mapping_prefix, 1, &cbuffers[0][0], u32tmp, &err);
	}

	// Keep track of which sets that the varstring tags used; we can't just assign them as sets are not loaded yet
	typedef std::map<uint32_t, uint32Vector> tag_varsets_t;
	tag_varsets_t tag_varsets;

	u32tmp = 0;
	if (fields & (1 << 3)) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_single_tags = u32tmp;
	grammar->num_tags = num_single_tags;
	grammar->single_tags_list.resize(num_single_tags);
	for (uint32_t i = 0; i < num_single_tags; ++i) {
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

		grammar->single_tags[t->hash] = t;
		grammar->single_tags_list[t->number] = t;
		if (t->tag.size() == 1 && t->tag[0] == '*') {
			grammar->tag_any = t->hash;
		}
	}

	u32tmp = 0;
	if (fields & (1 << 5)) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_pref_targets = u32tmp;
	for (uint32_t i = 0; i < num_pref_targets; ++i) {
		u32tmp = readBE<uint32_t>(input);
		grammar->preferred_targets.push_back(u32tmp);
	}

	u32tmp = 0;
	if (fields & (1 << 6)) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_par_pairs = u32tmp;
	for (uint32_t i = 0; i < num_par_pairs; ++i) {
		auto left = readBE<uint32_t>(input);
		auto right = readBE<uint32_t>(input);
		grammar->parentheses[left] = right;
		grammar->parentheses_reverse[right] = left;
	}

	u32tmp = 0;
	if (fields & (1 << 7)) {
		u32tmp = readBE<uint32_t>(input);
	}
	uint32_t num_par_anchors = u32tmp;
	for (uint32_t i = 0; i < num_par_anchors; ++i) {
		auto left = readBE<uint32_t>(input);
		auto right = readBE<uint32_t>(input);
		grammar->anchors[left] = right;
	}

	u32tmp = 0;
	if (fields & (1 << 8)) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_sets = u32tmp;
	grammar->sets_list.resize(num_sets);
	for (uint32_t i = 0; i < num_sets; ++i) {
		Set* s = grammar->allocateSet();

		auto fields = readBE<uint32_t>(input);

		if (fields & (1 << 0)) {
			s->number = readBE<uint32_t>(input);
		}
		if (fields & (1 << 1)) {
			s->hash = readBE<uint32_t>(input);
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
			for (uint32_t j = 0; j < num_set_ops; ++j) {
				u32tmp = readBE<uint32_t>(input);
				s->set_ops.push_back(u32tmp);
			}
		}
		if (fields & (1 << 5)) {
			auto num_sets = readBE<uint32_t>(input);
			for (uint32_t j = 0; j < num_sets; ++j) {
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
		grammar->sets_by_contents[s->hash] = s;
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

	if (fields & (1 << 9)) {
		u32tmp = readBE<uint32_t>(input);
		grammar->delimiters = grammar->sets_by_contents.find(u32tmp)->second;
	}

	if (fields & (1 << 10)) {
		u32tmp = readBE<uint32_t>(input);
		grammar->soft_delimiters = grammar->sets_by_contents.find(u32tmp)->second;
	}

	u32tmp = 0;
	if (fields & (1 << 11)) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_contexts = u32tmp;
	contexts_list.resize(num_contexts);
	for (uint32_t i = 0; i < num_contexts; ++i) {
		ContextualTest* t = readContextualTest_10043(input);
		grammar->contexts[t->hash] = t;
		contexts_list[i] = t;
	}

	u32tmp = 0;
	if (fields & (1 << 12)) {
		u32tmp = readBE<uint32_t>(input);
	}
	auto num_rules = u32tmp;
	grammar->rule_by_number.resize(num_rules);
	for (uint32_t i = 0; i < num_rules; ++i) {
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
			r->flags = readBE<uint32_t>(input);
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
			r->dep_target = contexts_list[u32tmp - 1];
		}

		auto num_dep_tests = readBE<uint32_t>(input);
		for (uint32_t j = 0; j < num_dep_tests; ++j) {
			u32tmp = readBE<uint32_t>(input);
			ContextualTest* t = contexts_list[u32tmp - 1];
			r->addContextualTest(t, r->dep_tests);
		}

		auto num_tests = readBE<uint32_t>(input);
		for (uint32_t j = 0; j < num_tests; ++j) {
			u32tmp = readBE<uint32_t>(input);
			ContextualTest* t = contexts_list[u32tmp - 1];
			r->addContextualTest(t, r->tests);
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

	// Bind the named templates to where they are used
	for (auto& it : deferred_tmpls) {
		auto tmt = templates.find(it.second);
		it.first->tmpl = tmt->second;
	}

	ucnv_close(conv);
	// Create the dummy set
	grammar->allocateDummySet();
	grammar->is_binary = false;
	return 0;
}

ContextualTest* BinaryGrammar::readContextualTest_10043(std::istream& input) {
	ContextualTest* t = grammar->allocateContextualTest();
	uint32_t u32tmp = 0;
	uint32_t tmpl = 0;

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
		tmpl = readBE<uint32_t>(input);
		t->tmpl = reinterpret_cast<ContextualTest*>(u32tmp);
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
		u32tmp = readBE<uint32_t>(input);
		templates[u32tmp] = t;
	}
	if (fields & (1 << 10)) {
		auto num_ors = readBE<uint32_t>(input);
		for (uint32_t i = 0; i < num_ors; ++i) {
			u32tmp = readBE<uint32_t>(input);
			ContextualTest* to = contexts_list[u32tmp - 1];
			t->ors.push_back(to);
		}
	}
	if (fields & (1 << 11)) {
		u32tmp = readBE<uint32_t>(input);
		t->linked = contexts_list[u32tmp - 1];
	}

	if (tmpl) {
		deferred_tmpls[t] = tmpl;
	}
	return t;
}
}
