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

#include "GrammarWriter.h"
#include "Strings.h"
#include "Grammar.h"
#include "ContextualTest.h"

namespace CG3 {

GrammarWriter::GrammarWriter(Grammar &res, UFILE *ux_err) {
	statistics = false;
	ux_stderr = ux_err;
	grammar = &res;
}

GrammarWriter::~GrammarWriter() {
	grammar = 0;
}

void GrammarWriter::printSet(UFILE *output, const Set &curset) {
	if (curset.sets.empty() && used_sets.find(curset.hash) == used_sets.end()) {
		if (statistics) {
			u_fprintf(output, "#List Matched: %u ; NoMatch: %u ; TotalTime: %f\n", curset.num_match, curset.num_fail, curset.total_time);
		}
		used_sets.insert(curset.hash);
		u_fprintf(output, "LIST %S = ", curset.name.c_str());
		TagHashSet::const_iterator tomp_iter;
		for (tomp_iter = curset.single_tags.begin() ; tomp_iter != curset.single_tags.end() ; tomp_iter++) {
			printTag(output, **tomp_iter);
			u_fprintf(output, " ");
		}
		CompositeTagHashSet::const_iterator comp_iter;
		for (comp_iter = curset.tags.begin() ; comp_iter != curset.tags.end() ; comp_iter++) {
			CompositeTag *curcomptag = *comp_iter;
			if (curcomptag->tags.size() == 1) {
				printTag(output, **(curcomptag->tags.begin()));
				u_fprintf(output, " ");
			}
			else {
				u_fprintf(output, "(");
				TagSet::const_iterator tag_iter;
				for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
					printTag(output, **tag_iter);
					u_fprintf(output, " ");
				}
				u_fprintf(output, ") ");
			}
		}
		u_fprintf(output, " ;\n");
	}
	else if (!curset.sets.empty() && used_sets.find(curset.hash) == used_sets.end()) {
		used_sets.insert(curset.hash);
		for (uint32_t i=0;i<curset.sets.size();i++) {
			printSet(output, *(grammar->sets_by_contents.find(curset.sets.at(i))->second));
		}
		if (statistics) {
			u_fprintf(output, "#Set Matched: %u ; NoMatch: %u ; TotalTime: %f\n", curset.num_match, curset.num_fail, curset.total_time);
		}
		u_fprintf(output, "SET %S = ", curset.name.c_str());
		u_fprintf(output, "%S ", grammar->sets_by_contents.find(curset.sets.at(0))->second->name.c_str());
		for (uint32_t i=0;i<curset.sets.size()-1;i++) {
			u_fprintf(output, "%S %S ", stringbits[curset.set_ops.at(i)].getTerminatedBuffer(), grammar->sets_by_contents.find(curset.sets.at(i+1))->second->name.c_str());
		}
		u_fprintf(output, " ;\n\n");
	}
}

int GrammarWriter::writeGrammar(UFILE *output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		CG3Quit(1);
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		CG3Quit(1);
	}
	if (grammar->is_binary) {
		u_fprintf(ux_stderr, "Error: Grammar is binary and cannot be output in textual form!\n");
		CG3Quit(1);
	}

	if (statistics) {
		u_fprintf(output, "# Total ticks spent applying grammar: %f\n", grammar->total_time);
	}
	u_fprintf(output, "# DELIMITERS and SOFT-DELIMITERS do not exist. Instead, look for the sets _S_DELIMITERS_ and _S_SOFT_DELIMITERS_.\n");

	u_fprintf(output, "MAPPING-PREFIX = %C ;\n", grammar->mapping_prefix);

	if (!grammar->preferred_targets.empty()) {
		u_fprintf(output, "PREFERRED-TARGETS = ");
		uint32Vector::const_iterator iter;
		for (iter = grammar->preferred_targets.begin() ; iter != grammar->preferred_targets.end() ; iter++ ) {
			printTag(output, *(grammar->single_tags.find(*iter)->second));
			u_fprintf(output, " ");
		}
		u_fprintf(output, " ;\n");
	}

	u_fprintf(output, "\n");

	used_sets.clear();
	Setuint32HashMap::const_iterator set_iter;
	for (set_iter = grammar->sets_by_contents.begin() ; set_iter != grammar->sets_by_contents.end() ; set_iter++) {
		if (set_iter->second->is_used) {
			printSet(output, *(set_iter->second));
		}
	}
	u_fprintf(output, "\n");

	std::vector<ContextualTest*>::const_iterator tmpl_iter;
	for (tmpl_iter = grammar->template_list.begin() ; tmpl_iter != grammar->template_list.end() ; tmpl_iter++) {
		u_fprintf(output, "TEMPLATE %u = ", (*tmpl_iter)->name);
		printContextualTest(output, **tmpl_iter);
		u_fprintf(output, " ;\n");
	}
	u_fprintf(output, "\n");

	bool found = false;
	RuleByLineMap rule_by_line;
	rule_by_line.insert(grammar->rule_by_line.begin(), grammar->rule_by_line.end());
	const_foreach (RuleByLineMap, rule_by_line, rule_iter, rule_iter_end) {
		const Rule &r = *(rule_iter->second);
		if (r.section == -1) {
			if (!found) {
				u_fprintf(output, "\nBEFORE-SECTIONS\n");
				found = true;
			}
			printRule(output, r);
			u_fprintf(output, " ;\n");
		}
	}
	const_foreach (uint32Vector, grammar->sections, isec, isec_end) {
		found = false;
		for (rule_iter = rule_by_line.begin() ; rule_iter != rule_by_line.end() ; rule_iter++) {
			const Rule &r = *(rule_iter->second);
			if (r.section == (int32_t)*isec) {
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
	for (rule_iter = rule_by_line.begin() ; rule_iter != rule_by_line.end() ; rule_iter++) {
		const Rule &r = *(rule_iter->second);
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
	for (rule_iter = rule_by_line.begin() ; rule_iter != rule_by_line.end() ; rule_iter++) {
		const Rule &r = *(rule_iter->second);
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

void GrammarWriter::printRule(UFILE *to, const Rule &rule) {
	if (statistics) {
		u_fprintf(to, "\n#Rule Matched: %u ; NoMatch: %u ; TotalTime: %f\n", rule.num_match, rule.num_fail, rule.total_time);
	}
	if (rule.wordform) {
		printTag(to, *(grammar->single_tags.find(rule.wordform)->second));
		u_fprintf(to, " ");
	}

	u_fprintf(to, "%S", keywords[rule.type].getTerminatedBuffer());

	if (rule.name && !(rule.name[0] == '_' && rule.name[1] == 'R' && rule.name[2] == '_')) {
		u_fprintf(to, ":%S", rule.name);
	}
	u_fprintf(to, " ");

	for (uint32_t i=0 ; i<FLAGS_COUNT ; i++) {
		if (rule.flags & (1 << i)) {
			u_fprintf(to, "%S", flags[i].getTerminatedBuffer());
		}
	}

	if (!rule.sublist.empty()) {
		uint32List::const_iterator iter;
		u_fprintf(to, "(");
		for (iter = rule.sublist.begin() ; iter != rule.sublist.end() ; iter++) {
			printTag(to, *(grammar->single_tags.find(*iter)->second));
			u_fprintf(to, " ");
		}
		u_fprintf(to, ") ");
	}

	if (!rule.maplist.empty()) {
		u_fprintf(to, "(");
		const_foreach (TagList, rule.maplist, iter, iter_end) {
			printTag(to, **iter);
			u_fprintf(to, " ");
		}
		u_fprintf(to, ") ");
	}

	if (rule.target) {
		u_fprintf(to, "%S ", grammar->sets_by_contents.find(rule.target)->second->name.c_str());
	}

	ContextualTest *test = rule.test_head;
	while (test) {
		u_fprintf(to, "(");
		printContextualTest(to, *test);
		u_fprintf(to, ") ");
		test = test->next;
	}

	if (rule.dep_target) {
		u_fprintf(to, "TO (");
		printContextualTest(to, *(rule.dep_target));
		u_fprintf(to, ") ");
		ContextualTest *test = rule.dep_test_head;
		while (test) {
			u_fprintf(to, "(");
			printContextualTest(to, *test);
			u_fprintf(to, ") ");
			test = test->next;
		}
	}
}

void GrammarWriter::printContextualTest(UFILE *to, const ContextualTest &test) {
	if (statistics) {
		u_fprintf(to, "\n#Test Matched: %u ; NoMatch: %u ; TotalTime: %f\n", test.num_match, test.num_fail, test.total_time);
	}
	if (test.tmpl) {
		u_fprintf(to, "T:%u ", test.tmpl->name);
	}
	else if (!test.ors.empty()) {
		std::list<ContextualTest*>::const_iterator iter;
		for (iter = test.ors.begin() ; iter != test.ors.end() ; ) {
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
	else {
		if (test.pos & POS_NEGATED) {
			u_fprintf(to, "NEGATE ");
		}
		if (test.pos & POS_ALL) {
			u_fprintf(to, "ALL ");
		}
		if (test.pos & POS_NONE) {
			u_fprintf(to, "NONE ");
		}
		if (test.pos & POS_NEGATIVE) {
			u_fprintf(to, "NOT ");
		}
		if (test.pos & POS_ABSOLUTE) {
			u_fprintf(to, "@");
		}
		if (test.pos & POS_SCANALL) {
			u_fprintf(to, "**");
		}
		else if (test.pos & POS_SCANFIRST) {
			u_fprintf(to, "*");
		}

		if (test.pos & POS_DEP_CHILD) {
			u_fprintf(to, "c");
		}
		if (test.pos & POS_DEP_PARENT) {
			u_fprintf(to, "p");
		}
		if (test.pos & POS_DEP_SIBLING) {
			u_fprintf(to, "s");
		}

		u_fprintf(to, "%d", test.offset);

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
		if (test.pos & POS_MARK_JUMP) {
			u_fprintf(to, "x");
		}
		if (test.pos & POS_LOOK_DELETED) {
			u_fprintf(to, "D");
		}
		if (test.pos & POS_LOOK_DELAYED) {
			u_fprintf(to, "d");
		}
		if (test.pos & POS_UNKNOWN) {
			u_fprintf(to, "?");
		}
		if (test.pos & POS_RELATION) {
			u_fprintf(to, "r:%S", grammar->single_tags.find(test.relation)->second->tag);
		}

		u_fprintf(to, " ");

		if (test.target) {
			u_fprintf(to, "%S ", grammar->sets_by_contents.find(test.target)->second->name.c_str());
		}
		if (test.cbarrier) {
			u_fprintf(to, "CBARRIER %S ", grammar->sets_by_contents.find(test.cbarrier)->second->name.c_str());
		}
		if (test.barrier) {
			u_fprintf(to, "BARRIER %S ", grammar->sets_by_contents.find(test.barrier)->second->name.c_str());
		}
	}

	if (test.linked) {
		u_fprintf(to, "LINK ");
		printContextualTest(to, *(test.linked));
	}
}

void GrammarWriter::printTag(UFILE *to, const Tag &tag) {
	if (tag.type & T_NEGATIVE) {
		u_fprintf(to, "!");
	}
	if (tag.type & T_FAILFAST) {
		u_fprintf(to, "^");
	}
	if (tag.type & T_META) {
		u_fprintf(to, "META:");
	}
	if (tag.type & T_VARIABLE) {
		u_fprintf(to, "VAR:");
	}

	UChar *tmp = gbuffers[0];
	ux_escape(tmp, tag.tag);
	u_fprintf(to, "%S", tmp);

	if (tag.type & T_CASE_INSENSITIVE) {
		u_fprintf(to, "i");
	}
	if (tag.type & T_REGEXP) {
		u_fprintf(to, "r");
	}
	if (tag.type & T_VARSTRING) {
		u_fprintf(to, "v");
	}
}

}
