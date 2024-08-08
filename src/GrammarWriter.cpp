/*
* Copyright (C) 2007-2024, GrammarSoft ApS
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

#include "GrammarWriter.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"
#include "ContextualTest.hpp"

namespace CG3 {

GrammarWriter::GrammarWriter(Grammar& res, std::ostream& ux_err) {
	ux_stderr = &ux_err;
	grammar = &res;

	for (auto at : res.anchors) {
		anchors.insert(std::make_pair(at.second, at.first));
	}
}

GrammarWriter::~GrammarWriter() {
	grammar = nullptr;
}

void GrammarWriter::printSet(std::ostream& output, const Set& curset) {
	if (used_sets.find(curset.number) != used_sets.end()) {
		return;
	}

	if (curset.sets.empty()) {
		used_sets.insert(curset.number);
		if (curset.type & ST_ORDERED) {
			u_fprintf(output, "O");
		}
		u_fprintf(output, "LIST %S = ", curset.name.data());
		TagVectorSet tagsets[] = { trie_getTagsOrdered(curset.trie), trie_getTagsOrdered(curset.trie_special) };
		for (auto& tvs : tagsets) {
			for (auto& tags : tvs) {
				if (tags.size() > 1) {
					u_fprintf(output, "(");
				}
				for (auto tag : tags) {
					printTag(output, *tag);
					u_fprintf(output, " ");
				}
				if (tags.size() > 1) {
					u_fprintf(output, ") ");
				}
			}
		}
		u_fprintf(output, " ;\n");
	}
	else {
		used_sets.insert(curset.number);
		for (auto s : curset.sets) {
			printSet(output, *(grammar->sets_list[s]));
		}
		const UChar* n = curset.name.data();
		if ((n[0] == '$' && n[1] == '$') || (n[0] == '&' && n[1] == '&')) {
			u_fprintf(output, "# ");
		}
		if (curset.type & ST_ORDERED) {
			u_fprintf(output, "O");
		}
		u_fprintf(output, "SET %S = ", n);
		u_fprintf(output, "%S ", grammar->sets_list[curset.sets[0]]->name.data());
		for (uint32_t i = 0; i < curset.sets.size() - 1; i++) {
			u_fprintf(output, "%S %S ", stringbits[curset.set_ops[i]].data(), grammar->sets_list[curset.sets[i + 1]]->name.data());
		}
		u_fprintf(output, " ;\n\n");
	}
}

int GrammarWriter::writeGrammar(std::ostream& output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		CG3Quit(1);
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		CG3Quit(1);
	}

	u_fprintf(output, "# DELIMITERS and SOFT-DELIMITERS do not exist. Instead, look for the sets _S_DELIMITERS_ and _S_SOFT_DELIMITERS_.\n");

	u_fprintf(output, "MAPPING-PREFIX = %C ;\n", grammar->mapping_prefix);

	if (grammar->sub_readings_ltr) {
		u_fprintf(output, "SUBREADINGS = LTR ;\n");
	}
	else {
		u_fprintf(output, "SUBREADINGS = RTL ;\n");
	}

	if (!grammar->cmdargs.empty()) {
		u_fprintf(output, "CMDARGS += %s ;\n", grammar->cmdargs.c_str());
	}
	if (!grammar->cmdargs_override.empty()) {
		u_fprintf(output, "CMDARGS-OVERRIDE += %s ;\n", grammar->cmdargs_override.c_str());
	}

	if (!grammar->static_sets.empty()) {
		u_fprintf(output, "STATIC-SETS =");
		for (auto& str : grammar->static_sets) {
			u_fprintf(output, " %S", str.data());
		}
		u_fprintf(output, " ;\n");
	}

	if (!grammar->preferred_targets.empty()) {
		u_fprintf(output, "PREFERRED-TARGETS = ");
		for (auto iter : grammar->preferred_targets) {
			printTag(output, *(grammar->single_tags.find(iter)->second));
			u_fprintf(output, " ");
		}
		u_fprintf(output, " ;\n");
	}

	if (!grammar->parentheses.empty()) {
		u_fprintf(output, "PARENTHESES = ");
		for (auto iter : grammar->parentheses) {
			u_fprintf(output, "(");
			printTag(output, *(grammar->single_tags.find(iter.first)->second));
			u_fprintf(output, " ");
			printTag(output, *(grammar->single_tags.find(iter.second)->second));
			u_fprintf(output, ") ");
		}
		u_fprintf(output, ";\n");
	}

	if (grammar->ordered) {
		u_fprintf(output, "OPTIONS += ordered ;\n");
	}
	if (grammar->addcohort_attach) {
		u_fprintf(output, "OPTIONS += addcohort-attach ;\n");
	}

	u_fprintf(output, "\n");

	used_sets.clear();
	for (auto s : grammar->sets_list) {
		if (s->name.empty()) {
			if (s == grammar->delimiters) {
				s->name = STR_DELIMITSET;
			}
			else if (s == grammar->soft_delimiters) {
				s->name = STR_SOFTDELIMITSET;
			}
			else if (s == grammar->text_delimiters) {
				s->name = STR_TEXTDELIMITSET;
			}
			else {
				s->name.resize(12);
				s->name.resize(u_sprintf(&s->name[0], "S%u", s->number));
			}
		}
		if (is_internal(s->name)) {
			s->name.insert(s->name.begin(), '3');
			s->name.insert(s->name.begin(), 'G');
			s->name.insert(s->name.begin(), 'C');
		}
	}
	for (auto s : grammar->sets_list) {
		printSet(output, *s);
	}
	u_fprintf(output, "\n");

	for (auto t : grammar->templates) {
		u_fprintf(output, "TEMPLATE %u = ", t.second->hash);
		printContextualTest(output, *t.second);
		u_fprintf(output, " ;\n");
	}

	/*
	for (BOOST_AUTO(cntx, grammar->templates.begin()); cntx != grammar->templates.end(); ++cntx) {
		u_fprintf(output, "TEMPLATE %u = ", cntx->second->hash);
		printContextualTest(output, *cntx->second);
		u_fprintf(output, " ;\n");
	}
	u_fprintf(output, "\n");
	//*/

	bool found = false;
	for (auto rule_iter : grammar->rule_by_number) {
		const Rule& r = *rule_iter;
		if (r.section == -1) {
			if (!found) {
				u_fprintf(output, "\nBEFORE-SECTIONS\n");
				found = true;
			}
			printRule(output, r);
			u_fprintf(output, " ;\n");
		}
	}
	for (auto isec : grammar->sections) {
		found = false;
		for (auto rule_iter : grammar->rule_by_number) {
			const Rule& r = *rule_iter;
			if (r.section == SI32(isec)) {
				if (!found) {
					u_fprintf(output, "\nSECTION\n");
					found = true;
				}
				printRule(output, r);
				u_fprintf(output, " ;\n");
			}
		}
	}
	found = false;
	for (auto rule_iter : grammar->rule_by_number) {
		const Rule& r = *rule_iter;
		if (r.section == -2) {
			if (!found) {
				u_fprintf(output, "\nAFTER-SECTIONS\n");
				found = true;
			}
			printRule(output, r);
			u_fprintf(output, " ;\n");
		}
	}
	found = false;
	for (auto rule_iter : grammar->rule_by_number) {
		const Rule& r = *rule_iter;
		if (r.section == -3) {
			if (!found) {
				u_fprintf(output, "\nNULL-SECTION\n");
				found = true;
			}
			printRule(output, r);
			u_fprintf(output, " ;\n");
		}
	}

	return 0;
}

void GrammarWriter::printRule(std::ostream& to, const Rule& rule) {
	if (seen_rules.count(rule.number)) {
		return;
	}
	seen_rules.insert(rule.number);

	auto as = anchors.equal_range(rule.number);
	for (auto ait = as.first; ait != as.second; ++ait) {
		auto tag = USV(grammar->single_tags.find(ait->second)->second->tag);
		if (tag == keywords[K_START] || tag == keywords[K_END] || tag == rule.name) {
			continue;
		}
		u_fprintf(to, "ANCHOR %S ;\n", tag.data());
	}

	if (rule.wordform) {
		printTag(to, *rule.wordform);
		u_fprintf(to, " ");
	}

	auto type = rule.type;
	if (rule.type == K_MOVE_BEFORE || rule.type == K_MOVE_AFTER) {
		type = K_MOVE;
	}
	if (rule.type == K_ADDCOHORT_BEFORE || rule.type == K_ADDCOHORT_AFTER) {
		type = K_ADDCOHORT;
	}
	if (rule.type == K_EXTERNAL_ONCE || rule.type == K_EXTERNAL_ALWAYS) {
		type = K_EXTERNAL;
	}

	u_fprintf(to, "%S", keywords[type].data());

	if (!rule.name.empty() && !(rule.name[0] == '_' && rule.name[1] == 'R' && rule.name[2] == '_')) {
		u_fprintf(to, ":%S", rule.name.data());
	}
	u_fprintf(to, " ");

	for (uint32_t i = 0; i < FLAGS_COUNT; i++) {
		if (i == FL_BEFORE || i == FL_AFTER || i == FL_WITHCHILD) {
			continue;
		}
		if (rule.flags & (1ull << i)) {
			if (i == FL_SUB) {
				u_fprintf(to, "%S:%d ", g_flags[i].data(), rule.sub_reading);
			}
			else {
				u_fprintf(to, "%S ", g_flags[i].data());
			}
		}
	}

	if (rule.flags & RF_WITHCHILD) {
		u_fprintf(to, "WITHCHILD %S ", grammar->sets_list[rule.childset1]->name.data());
	}

	if (rule.type == K_SUBSTITUTE || rule.type == K_EXECUTE) {
		u_fprintf(to, "%S ", rule.sublist->name.data());
	}

	if (rule.maplist) {
		u_fprintf(to, "%S ", rule.maplist->name.data());
	}

	if (rule.sublist && (rule.type == K_ADDRELATIONS || rule.type == K_SETRELATIONS || rule.type == K_REMRELATIONS || rule.type == K_SETVARIABLE || rule.type == K_COPY || rule.type == K_COPYCOHORT)) {
		if (rule.type == K_COPY || rule.type == K_COPYCOHORT) {
			u_fprintf(to, "EXCEPT ");
		}
		u_fprintf(to, "%S ", rule.sublist->name.data());
	}

	if (rule.type == K_ADD || rule.type == K_MAP || rule.type == K_SUBSTITUTE || rule.type == K_COPY || rule.type == K_COPYCOHORT) {
		if (rule.flags & RF_BEFORE) {
			u_fprintf(to, "BEFORE ");
		}
		if (rule.flags & RF_AFTER) {
			u_fprintf(to, "AFTER ");
		}
		if (rule.childset1) {
			if (rule.type == K_COPYCOHORT) {
				u_fprintf(to, "WITHCHILD ");
			}
			u_fprintf(to, "%S ", grammar->sets_list[rule.childset1]->name.data());
		}
	}

	if (rule.type == K_ADDCOHORT_BEFORE) {
		u_fprintf(to, "BEFORE ");
	}
	else if (rule.type == K_ADDCOHORT_AFTER) {
		u_fprintf(to, "AFTER ");
	}

	if (rule.target) {
		u_fprintf(to, "%S ", grammar->sets_list[rule.target]->name.data());
	}

	for (auto it : rule.tests) {
		u_fprintf(to, "(");
		printContextualTest(to, *it);
		u_fprintf(to, ") ");
	}

	if (rule.type == K_SETPARENT || rule.type == K_SETCHILD || rule.type == K_ADDRELATIONS || rule.type == K_ADDRELATION || rule.type == K_SETRELATIONS || rule.type == K_SETRELATION || rule.type == K_REMRELATIONS || rule.type == K_REMRELATION || rule.type == K_COPYCOHORT) {
		u_fprintf(to, "TO ");
	}
	else if (rule.type == K_MOVE_AFTER) {
		u_fprintf(to, "AFTER ");
	}
	else if (rule.type == K_MOVE_BEFORE) {
		u_fprintf(to, "BEFORE ");
	}
	else if (rule.type == K_SWITCH || rule.type == K_MERGECOHORTS) {
		u_fprintf(to, "WITH ");
	}

	if (rule.dep_target) {
		if (rule.childset2) {
			u_fprintf(to, "WITHCHILD %S ", grammar->sets_list[rule.childset2]->name.data());
		}
		u_fprintf(to, "(");
		printContextualTest(to, *(rule.dep_target));
		u_fprintf(to, ") ");
	}
	for (auto it : rule.dep_tests) {
		u_fprintf(to, "(");
		printContextualTest(to, *it);
		u_fprintf(to, ") ");
	}

	if (rule.type == K_WITH) {
		u_fprintf(to, "{\n");
		for (auto r : rule.sub_rules) {
			u_fprintf(to, "\t");
			printRule(to, *r);
			u_fprintf(to, " ;\n");
		}
		u_fprintf(to, "}\n");
	}
}

void GrammarWriter::printContextualTest(std::ostream& to, const ContextualTest& test) {
	if (test.pos & POS_NEGATE) {
		u_fprintf(to, "NEGATE ");
	}
	if ((test.pos & POS_TMPL_OVERRIDE) || (!test.tmpl && test.ors.empty())) {
		if (test.pos & POS_ALL) {
			u_fprintf(to, "ALL ");
		}
		if (test.pos & POS_NONE) {
			u_fprintf(to, "NONE ");
		}
		if (test.pos & POS_NOT) {
			u_fprintf(to, "NOT ");
		}
		if (test.pos & POS_ABSOLUTE) {
			u_fprintf(to, "@");
		}
		if (test.pos & POS_SCANALL) {
			u_fprintf(to, "**");
		}
		else if (test.pos & (POS_SCANFIRST | POS_DEP_DEEP)) {
			u_fprintf(to, "*");
		}

		if (test.pos & POS_LEFTMOST) {
			u_fprintf(to, "ll");
		}
		if (test.pos & POS_LEFT) {
			u_fprintf(to, "l");
		}
		if (test.pos & POS_RIGHTMOST) {
			u_fprintf(to, "rr");
		}
		if (test.pos & POS_RIGHT) {
			u_fprintf(to, "r");
		}
		if (test.pos & POS_DEP_CHILD) {
			u_fprintf(to, "c");
		}
		if (test.pos & POS_DEP_PARENT) {
			if (test.pos & POS_DEP_GLOB) {
				u_fprintf(to, "p");
			}
			u_fprintf(to, "p");
		}
		else if (test.pos & POS_DEP_GLOB) {
			u_fprintf(to, "cc");
		}
		if (test.pos & POS_DEP_SIBLING) {
			u_fprintf(to, "s");
		}
		if (test.pos & POS_SELF) {
			u_fprintf(to, "S");
		}
		if (test.pos & POS_NO_BARRIER) {
			u_fprintf(to, "N");
		}

		if (test.pos & POS_UNKNOWN) {
			u_fprintf(to, "?");
		}
		else if (!(test.pos & (POS_DEP_CHILD | POS_DEP_SIBLING | POS_DEP_PARENT | POS_DEP_GLOB | POS_LEFT_PAR | POS_RIGHT_PAR | POS_RELATION | POS_BAG_OF_TAGS))) {
			u_fprintf(to, "%d", test.offset);
		}

		if (test.pos & POS_CAREFUL) {
			u_fprintf(to, "C");
		}
		if (test.pos & POS_SPAN_BOTH) {
			u_fprintf(to, "W");
		}
		if (test.pos & POS_SPAN_LEFT) {
			u_fprintf(to, "<");
		}
		if (test.pos & POS_SPAN_RIGHT) {
			u_fprintf(to, ">");
		}
		if (test.pos & POS_PASS_ORIGIN) {
			u_fprintf(to, "o");
		}
		if (test.pos & POS_NO_PASS_ORIGIN) {
			u_fprintf(to, "O");
		}
		if (test.pos & POS_LEFT_PAR) {
			u_fprintf(to, "L");
		}
		if (test.pos & POS_RIGHT_PAR) {
			u_fprintf(to, "R");
		}
		if (test.pos & POS_MARK_SET) {
			u_fprintf(to, "X");
		}
		if (test.pos & POS_JUMP) {
			if (test.jump_pos == JUMP_MARK) {
				u_fprintf(to, "x");
			}
			else if (test.jump_pos == JUMP_ATTACH) {
				u_fprintf(to, "jA");
			}
			else if (test.jump_pos == JUMP_TARGET) {
				u_fprintf(to, "jT");
			}
			else {
				u_fprintf(to, "jC%d", test.jump_pos);
			}
		}
		if (test.pos & POS_LOOK_DELETED) {
			u_fprintf(to, "D");
		}
		if (test.pos & POS_LOOK_DELAYED) {
			u_fprintf(to, "d");
		}
		if (test.pos & POS_ACTIVE) {
			u_fprintf(to, "T");
		}
		if (test.pos & POS_INACTIVE) {
			u_fprintf(to, "t");
		}
		if (test.pos & POS_LOOK_IGNORED) {
			u_fprintf(to, "I");
		}
		if (test.pos & POS_ATTACH_TO) {
			u_fprintf(to, "A");
		}
		if (test.pos & POS_WITH) {
			u_fprintf(to, "w");
		}
		if (test.pos & POS_BAG_OF_TAGS) {
			u_fprintf(to, "B");
		}
		if (test.pos & POS_RELATION) {
			u_fprintf(to, "r:");
			printTag(to, *grammar->single_tags.find(test.relation)->second);
		}
		if (test.offset_sub) {
			if (test.offset_sub == GSR_ANY) {
				u_fprintf(to, "/*");
			}
			else {
				u_fprintf(to, "/%d", test.offset_sub);
			}
		}

		u_fprintf(to, " ");
	}

	if (test.tmpl) {
		u_fprintf(to, "T:%u ", test.tmpl->hash);
	}
	else if (!test.ors.empty()) {
		for (auto iter = test.ors.begin(); iter != test.ors.end();) {
			u_fprintf(to, "(");
			printContextualTest(to, **iter);
			u_fprintf(to, ")");
			iter++;
			if (iter != test.ors.end()) {
				u_fprintf(to, " OR ");
			}
			else {
				u_fprintf(to, " ");
			}
		}
	}

	if (test.target) {
		u_fprintf(to, "%S ", grammar->sets_list[test.target]->name.data());
	}
	if (test.cbarrier) {
		u_fprintf(to, "CBARRIER %S ", grammar->sets_list[test.cbarrier]->name.data());
	}
	if (test.barrier) {
		u_fprintf(to, "BARRIER %S ", grammar->sets_list[test.barrier]->name.data());
	}

	if (test.linked) {
		u_fprintf(to, "LINK ");
		printContextualTest(to, *(test.linked));
	}
}

void GrammarWriter::printTag(std::ostream& to, const Tag& tag) {
	UString str = tag.toUString(true);
	u_fprintf(to, "%S", str.data());
}
}
