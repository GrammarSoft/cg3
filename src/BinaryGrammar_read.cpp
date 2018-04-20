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

int BinaryGrammar::parse_grammar(const UChar*, size_t) {
	throw "UChar* interface doesn't make sense for binary grammars.";
}
int BinaryGrammar::parse_grammar(UString&) {
	throw "UString interface doesn't make sense for binary grammars.";
}

int BinaryGrammar::parse_grammar(const std::string& buffer) {
	return parse_grammar(buffer.c_str(), buffer.size());
}

int BinaryGrammar::parse_grammar(const char* buffer, size_t length) {
	std::stringstream input;
	input.write(buffer, length);
	input.seekg(0);
	return parse_grammar(input);
}

int BinaryGrammar::parse_grammar(std::istream& input) {
	input.exceptions(std::ios::failbit | std::ios::eofbit | std::ios::badbit);

	uint32_t fields = 0;
	uint32_t u32tmp = 0;
	int32_t i32tmp = 0;
	uint8_t u8tmp = 0;
	UErrorCode err = U_ZERO_ERROR;
	UConverter* conv = ucnv_open("UTF-8", &err);
	std::stringstream buffer;

	if (fread_throw(&cbuffers[0][0], 1, 4, input) != 4) {
		std::cerr << "Error: Error reading first 4 bytes from grammar!" << std::endl;
		CG3Quit(1);
	}
	if (cbuffers[0][0] != 'C' || cbuffers[0][1] != 'G' || cbuffers[0][2] != '3' || cbuffers[0][3] != 'B') {
		u_fprintf(ux_stderr, "Error: Grammar does not begin with magic bytes - cannot load as binary!\n");
		CG3Quit(1);
	}

	fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
	u32tmp = (uint32_t)ntohl(u32tmp);
	if (u32tmp <= 10297) {
		if (verbosity >= 1) {
			u_fprintf(ux_stderr, "Warning: Grammar revision is %u, but current format is %u or later. Please recompile the binary grammar with latest CG-3.\n", u32tmp, CG3_FEATURE_REV);
			u_fflush(ux_stderr);
		}
		input.seekg(0);
		return readBinaryGrammar_10043(input);
	}
	if (u32tmp < CG3_TOO_OLD) {
		u_fprintf(ux_stderr, "Error: Grammar revision is %u, but this loader requires %u or later!\n", u32tmp, CG3_TOO_OLD);
		CG3Quit(1);
	}
	if (u32tmp > CG3_FEATURE_REV) {
		u_fprintf(ux_stderr, "Error: Grammar revision is %u, but this loader only knows up to revision %u!\n", u32tmp, CG3_FEATURE_REV);
		CG3Quit(1);
	}

	grammar->is_binary = true;

	fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
	fields = (uint32_t)ntohl(u32tmp);

	grammar->has_dep = (fields & (1 << 0)) != 0;
	grammar->sub_readings_ltr = (fields & (1 << 2)) != 0;
	grammar->has_relations = (fields & (1 << 13)) != 0;
	grammar->has_bag_of_tags = (fields & (1 << 14)) != 0;

	if (fields & (1 << 1)) {
		ucnv_reset(conv);
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		fread_throw(&cbuffers[0][0], 1, u32tmp, input);
		i32tmp = ucnv_toUChars(conv, &grammar->mapping_prefix, 1, &cbuffers[0][0], u32tmp, &err);
	}

	// Keep track of which sets that the varstring tags used; we can't just assign them as sets are not loaded yet
	typedef std::map<uint32_t, uint32Vector> tag_varsets_t;
	tag_varsets_t tag_varsets;

	u32tmp = 0;
	if (fields & (1 << 3)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
	}
	uint32_t num_single_tags = u32tmp;
	grammar->single_tags_list.resize(num_single_tags);
	for (uint32_t i = 0; i < num_single_tags; i++) {
		Tag* t = grammar->allocateTag();
		t->type |= T_GRAMMAR;

		uint32_t fields = 0;
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		fields = (uint32_t)ntohl(u32tmp);

		if (fields & (1 << 0)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			t->number = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 1)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			t->hash = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 2)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			t->plain_hash = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 3)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			t->seed = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 4)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			t->type = (uint32_t)ntohl(u32tmp);
		}

		if (fields & (1 << 5)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			t->comparison_hash = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 6)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			t->comparison_op = (C_OPS)ntohl(u32tmp);
		}
		if (fields & (1 << 7)) {
			fread_throw(&i32tmp, sizeof(int32_t), 1, input);
			t->comparison_val = (int32_t)ntohl(i32tmp);
			if (t->comparison_val <= std::numeric_limits<int32_t>::min()) {
				t->comparison_val = NUMERIC_MIN;
			}
			if (t->comparison_val >= std::numeric_limits<int32_t>::max()) {
				t->comparison_val = NUMERIC_MAX;
			}
		}
		if (fields & (1 << 12)) {
			char buf[sizeof(uint64_t) + sizeof(int32_t)] = {};
			fread_throw(&buf[0], sizeof(buf), 1, input);
			buffer.str("");
			buffer.clear();
			buffer.write(buf, sizeof(buf));
			t->comparison_val = readSwapped<double>(buffer);
		}

		if (fields & (1 << 8)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			if (u32tmp) {
				ucnv_reset(conv);
				fread_throw(&cbuffers[0][0], 1, u32tmp, input);
				i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);
				t->tag = &gbuffers[0][0];
			}
		}

		if (fields & (1 << 9)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			if (u32tmp) {
				ucnv_reset(conv);
				fread_throw(&cbuffers[0][0], 1, u32tmp, input);
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
					u_fprintf(ux_stderr, "Error: uregex_open returned %s trying to parse tag %S - cannot continue!\n", u_errorName(status), t->tag.c_str());
					CG3Quit(1);
				}
			}
		}

		if (fields & (1 << 10)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			uint32_t num = (uint32_t)ntohl(u32tmp);
			t->allocateVsSets();
			t->vs_sets->reserve(num);
			tag_varsets[t->number].reserve(num);
			for (size_t i = 0; i < num; ++i) {
				fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				tag_varsets[t->number].push_back(u32tmp);
			}
		}
		if (fields & (1 << 11)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			uint32_t num = (uint32_t)ntohl(u32tmp);
			t->allocateVsNames();
			t->vs_names->reserve(num);
			for (size_t i = 0; i < num; ++i) {
				fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				if (u32tmp) {
					ucnv_reset(conv);
					fread_throw(&cbuffers[0][0], 1, u32tmp, input);
					i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);
					t->vs_names->push_back(&gbuffers[0][0]);
				}
			}
		}
		// 1 << 12 used earlier

		grammar->single_tags[t->hash] = t;
		grammar->single_tags_list[t->number] = t;
		if (t->tag.size() == 1 && t->tag[0] == '*') {
			grammar->tag_any = t->hash;
		}
	}

	u32tmp = 0;
	if (fields & (1 << 4)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
	}
	uint32_t num_remaps = u32tmp;
	for (uint32_t i = 0; i < num_remaps; ++i) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		grammar->reopen_mappings.insert(u32tmp);
	}

	u32tmp = 0;
	if (fields & (1 << 5)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
	}
	uint32_t num_pref_targets = u32tmp;
	for (uint32_t i = 0; i < num_pref_targets; i++) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		grammar->preferred_targets.push_back(u32tmp);
	}

	u32tmp = 0;
	if (fields & (1 << 6)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
	}
	uint32_t num_par_pairs = u32tmp;
	for (uint32_t i = 0; i < num_par_pairs; i++) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t left = (uint32_t)ntohl(u32tmp);
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t right = (uint32_t)ntohl(u32tmp);
		grammar->parentheses[left] = right;
		grammar->parentheses_reverse[right] = left;
	}

	u32tmp = 0;
	if (fields & (1 << 7)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
	}
	uint32_t num_par_anchors = u32tmp;
	for (uint32_t i = 0; i < num_par_anchors; i++) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t left = (uint32_t)ntohl(u32tmp);
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t right = (uint32_t)ntohl(u32tmp);
		grammar->anchors[left] = right;
	}

	u32tmp = 0;
	if (fields & (1 << 8)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
	}
	uint32_t num_sets = u32tmp;
	grammar->sets_list.resize(num_sets);
	for (uint32_t i = 0; i < num_sets; i++) {
		Set* s = grammar->allocateSet();

		uint32_t fields = 0;
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		fields = (uint32_t)ntohl(u32tmp);

		if (fields & (1 << 0)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			s->number = (uint32_t)ntohl(u32tmp);
		}
		// Field 1 is unused
		if (fields & (1 << 2)) {
			fread_throw(&u8tmp, sizeof(uint8_t), 1, input);
			s->type = u8tmp;
		}

		if (fields & (1 << 3)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			if (u32tmp) {
				trie_unserialize(s->trie, input, *grammar, u32tmp);
			}
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			if (u32tmp) {
				trie_unserialize(s->trie_special, input, *grammar, u32tmp);
			}
		}
		if (fields & (1 << 4)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_set_ops = u32tmp;
			for (uint32_t j = 0; j < num_set_ops; j++) {
				fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->set_ops.push_back(u32tmp);
			}
		}
		if (fields & (1 << 5)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			uint32_t num_sets = u32tmp;
			for (uint32_t j = 0; j < num_sets; j++) {
				fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
				u32tmp = (uint32_t)ntohl(u32tmp);
				s->sets.push_back(u32tmp);
			}
		}
		if (fields & (1 << 6)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			if (u32tmp) {
				ucnv_reset(conv);
				fread_throw(&cbuffers[0][0], 1, u32tmp, input);
				i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);
				s->setName(&gbuffers[0][0]);
			}
		}
		grammar->sets_list[s->number] = s;
	}

	// Actually assign sets to the varstring tags now that sets are loaded
	for (auto iter : tag_varsets) {
		Tag* t = grammar->single_tags_list[iter.first];
		for (auto uit : iter.second) {
			Set* s = grammar->sets_list[uit];
			t->vs_sets->push_back(s);
		}
	}

	if (fields & (1 << 9)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		grammar->delimiters = grammar->sets_list[u32tmp];
	}

	if (fields & (1 << 10)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		grammar->soft_delimiters = grammar->sets_list[u32tmp];
	}

	u32tmp = 0;
	if (fields & (1 << 11)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
	}
	uint32_t num_contexts = u32tmp;
	for (uint32_t i = 0; i < num_contexts; i++) {
		ContextualTest* t = readContextualTest(input);
		grammar->contexts[t->hash] = t;
	}

	u32tmp = 0;
	if (fields & (1 << 12)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
	}
	uint32_t num_rules = u32tmp;
	grammar->rule_by_number.resize(num_rules);
	for (uint32_t i = 0; i < num_rules; i++) {
		Rule* r = grammar->allocateRule();

		uint32_t fields = 0;
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		fields = (uint32_t)ntohl(u32tmp);

		if (fields & (1 << 0)) {
			fread_throw(&i32tmp, sizeof(int32_t), 1, input);
			r->section = (int32_t)ntohl(i32tmp);
		}
		if (fields & (1 << 1)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->type = (KEYWORDS)ntohl(u32tmp);
		}
		if (fields & (1 << 2)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->line = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 3)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->flags = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 4)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			if (u32tmp) {
				ucnv_reset(conv);
				fread_throw(&cbuffers[0][0], 1, u32tmp, input);
				i32tmp = ucnv_toUChars(conv, &gbuffers[0][0], CG3_BUFFER_SIZE - 1, &cbuffers[0][0], u32tmp, &err);
				r->setName(&gbuffers[0][0]);
			}
		}
		if (fields & (1 << 5)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->target = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 6)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->wordform = grammar->single_tags_list[(uint32_t)ntohl(u32tmp)];
		}
		if (fields & (1 << 7)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->varname = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 8)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->varvalue = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 9)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			int32_t v = u32tmp;
			if (u32tmp & (1 << 31)) {
				u32tmp &= ~(1 << 31);
				v = u32tmp;
				v = -v;
			}
			r->sub_reading = v;
		}
		if (fields & (1 << 10)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->childset1 = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 11)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->childset2 = (uint32_t)ntohl(u32tmp);
		}
		if (fields & (1 << 12)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->maplist = grammar->sets_list[(uint32_t)ntohl(u32tmp)];
		}
		if (fields & (1 << 13)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->sublist = grammar->sets_list[(uint32_t)ntohl(u32tmp)];
		}
		if (fields & (1 << 14)) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			r->number = (uint32_t)ntohl(u32tmp);
		}

		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		if (u32tmp) {
			r->dep_target = grammar->contexts[u32tmp];
		}

		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_dep_tests = u32tmp;
		for (uint32_t j = 0; j < num_dep_tests; j++) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			ContextualTest* t = grammar->contexts[u32tmp];
			r->addContextualTest(t, r->dep_tests);
		}

		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		uint32_t num_tests = u32tmp;
		for (uint32_t j = 0; j < num_tests; j++) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			ContextualTest* t = grammar->contexts[u32tmp];
			r->addContextualTest(t, r->tests);
		}
		grammar->rule_by_number[r->number] = r;
	}

	// Bind the templates to where they are used
	for (auto it : deferred_tmpls) {
		it.first->tmpl = grammar->contexts.find(it.second)->second;
	}

	// Bind the OR'ed contexts to where they are used
	for (auto it : deferred_ors) {
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
	uint32_t fields = 0;
	uint32_t u32tmp = 0;
	int32_t i32tmp = 0;
	uint32_t tmpl = 0;

	fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
	fields = (uint32_t)ntohl(u32tmp);

	if (fields & (1 << 0)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		t->hash = (uint32_t)ntohl(u32tmp);
	}
	if (fields & (1 << 1)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		t->pos = (uint32_t)ntohl(u32tmp);
		if (t->pos & POS_64BIT) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			t->pos |= ((uint64_t)ntohl(u32tmp)) << 32;
		}
	}
	if (fields & (1 << 2)) {
		fread_throw(&i32tmp, sizeof(int32_t), 1, input);
		t->offset = (int32_t)ntohl(i32tmp);
	}
	if (fields & (1 << 3)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		tmpl = (uint32_t)ntohl(u32tmp);
		deferred_tmpls[t] = tmpl;
	}
	if (fields & (1 << 4)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		t->target = (uint32_t)ntohl(u32tmp);
	}
	if (fields & (1 << 5)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		t->line = (uint32_t)ntohl(u32tmp);
	}
	if (fields & (1 << 6)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		t->relation = (uint32_t)ntohl(u32tmp);
	}
	if (fields & (1 << 7)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		t->barrier = (uint32_t)ntohl(u32tmp);
	}
	if (fields & (1 << 8)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		t->cbarrier = (uint32_t)ntohl(u32tmp);
	}
	if (fields & (1 << 9)) {
		fread_throw(&i32tmp, sizeof(int32_t), 1, input);
		t->offset_sub = (int32_t)ntohl(i32tmp);
	}
	if (fields & (1 << 10)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		uint32_t num_ors = (uint32_t)ntohl(u32tmp);
		for (uint32_t i = 0; i < num_ors; ++i) {
			fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
			u32tmp = (uint32_t)ntohl(u32tmp);
			deferred_ors[t].push_back(u32tmp);
		}
	}
	if (fields & (1 << 11)) {
		fread_throw(&u32tmp, sizeof(uint32_t), 1, input);
		u32tmp = (uint32_t)ntohl(u32tmp);
		t->linked = grammar->contexts[u32tmp];
	}

	return t;
}
}
