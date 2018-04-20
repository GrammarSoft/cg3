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

#include "GrammarWriter.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"
#include "ContextualTest.hpp"

namespace CG3 {

GrammarWriter::GrammarWriter(Grammar& res, std::ostream& ux_err) {
	statistics = false;
	ux_stderr = &ux_err;
	grammar = &res;
}

GrammarWriter::~GrammarWriter() {
	grammar = 0;
}

void GrammarWriter::printSet(std::ostream& output, const Set& curset) {
	if (used_sets.find(curset.number) != used_sets.end()) {
		return;
	}

	if (curset.sets.empty()) {
		if (statistics) {
			if (ceil(curset.total_time) == floor(curset.total_time)) {
				u_fprintf(output, "#List Matched: %u ; NoMatch: %u ; TotalTime: %.0f\n", curset.num_match, curset.num_fail, curset.total_time);
			}
			else {
				u_fprintf(output, "#List Matched: %u ; NoMatch: %u ; TotalTime: %f\n", curset.num_match, curset.num_fail, curset.total_time);
			}
		}
		used_sets.insert(curset.number);
		u_fprintf(output, "LIST %S = ", curset.name.c_str());
		std::set<TagVector> tagsets[] = { trie_getTagsOrdered(curset.trie), trie_getTagsOrdered(curset.trie_special) };
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
					u_fprintf(output, ")");
				}
			}
		}
		u_fprintf(output, " ;\n");
	}
	else {
		used_sets.insert(curset.number);
		for (uint32_t i = 0; i < curset.sets.size(); i++) {
			printSet(output, *(grammar->sets_list[curset.sets[i]]));
		}
		if (statistics) {
			if (ceil(curset.total_time) == floor(curset.total_time)) {
				u_fprintf(output, "#Set Matched: %u ; NoMatch: %u ; TotalTime: %.0f\n", curset.num_match, curset.num_fail, curset.total_time);
			}
			else {
				u_fprintf(output, "#Set Matched: %u ; NoMatch: %u ; TotalTime: %f\n", curset.num_match, curset.num_fail, curset.total_time);
			}
		}
		const UChar* n = curset.name.c_str();
		if ((n[0] == '$' && n[1] == '$') || (n[0] == '&' && n[1] == '&')) {
			u_fprintf(output, "# ");
		}
		u_fprintf(output, "SET %S = ", n);
		u_fprintf(output, "%S ", grammar->sets_list[curset.sets[0]]->name.c_str());
		for (uint32_t i = 0; i < curset.sets.size() - 1; i++) {
			u_fprintf(output, "%S %S ", stringbits[curset.set_ops[i]].getTerminatedBuffer(), grammar->sets_list[curset.sets[i + 1]]->name.c_str());
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

	if (statistics) {
		if (ceil(grammar->total_time) == floor(grammar->total_time)) {
			u_fprintf(output, "# Total ticks spent applying grammar: %.0f\n", grammar->total_time);
		}
		else {
			u_fprintf(output, "# Total ticks spent applying grammar: %f\n", grammar->total_time);
		}
	}
	u_fprintf(output, "# DELIMITERS and SOFT-DELIMITERS do not exist. Instead, look for the sets _S_DELIMITERS_ and _S_SOFT_DELIMITERS_.\n");

	u_fprintf(output, "MAPPING-PREFIX = %C ;\n", grammar->mapping_prefix);

	if (grammar->sub_readings_ltr) {
		u_fprintf(output, "SUBREADINGS = LTR ;\n");
	}
	else {
		u_fprintf(output, "SUBREADINGS = RTL ;\n");
	}

	if (!grammar->static_sets.empty()) {
		u_fprintf(output, "STATIC-SETS =");
		for (auto& str : grammar->static_sets) {
			u_fprintf(output, " %S", str.c_str());
		}
		u_fprintf(output, " ;\n");
	}

	if (!grammar->preferred_targets.empty()) {
		u_fprintf(output, "PREFERRED-TARGETS = ");
		uint32Vector::const_iterator iter;
		for (iter = grammar->preferred_targets.begin(); iter != grammar->preferred_targets.end(); iter++) {
			printTag(output, *(grammar->single_tags.find(*iter)->second));
			u_fprintf(output, " ");
		}
		u_fprintf(output, " ;\n");
	}

	u_fprintf(output, "\n");

	used_sets.clear();
	for (auto s : grammar->sets_list) {
		if (s->name.empty()) {
			if (s == grammar->delimiters) {
				s->name.assign(stringbits[S_DELIMITSET].getTerminatedBuffer());
			}
			else if (s == grammar->soft_delimiters) {
				s->name.assign(stringbits[S_SOFTDELIMITSET].getTerminatedBuffer());
			}
			else {
				s->name.resize(12);
				s->name.resize(u_sprintf(&s->name[0], "S%u", s->number));
			}
		}
		if (s->name[0] == '_' && s->name[1] == 'G' && s->name[2] == '_') {
			s->name.insert(s->name.begin(), '3');
			s->name.insert(s->name.begin(), 'G');
			s->name.insert(s->name.begin(), 'C');
		}
	}
	for (auto s : grammar->sets_list) {
		printSet(output, *s);
	}
	u_fprintf(output, "\n");

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
			if (r.section == static_cast<int32_t>(isec)) {
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

// ToDo: Make printRule do the right thing for MOVE_ and ADDCOHORT_ BEFORE|AFTER
void GrammarWriter::printRule(std::ostream& to, const Rule& rule) {
	if (statistics) {
		if (ceil(rule.total_time) == floor(rule.total_time)) {
			u_fprintf(to, "\n#Rule Matched: %u ; NoMatch: %u ; TotalTime: %.0f\n", rule.num_match, rule.num_fail, rule.total_time);
		}
		else {
			u_fprintf(to, "\n#Rule Matched: %u ; NoMatch: %u ; TotalTime: %f\n", rule.num_match, rule.num_fail, rule.total_time);
		}
	}
	if (rule.wordform) {
		printTag(to, *rule.wordform);
		u_fprintf(to, " ");
	}

	u_fprintf(to, "%S", keywords[rule.type].getTerminatedBuffer());

	if (!rule.name.empty() && !(rule.name[0] == '_' && rule.name[1] == 'R' && rule.name[2] == '_')) {
		u_fprintf(to, ":%S", rule.name.c_str());
	}
	u_fprintf(to, " ");

	for (uint32_t i = 0; i < FLAGS_COUNT; i++) {
		if (rule.flags & (1 << i)) {
			if (i == FL_SUB) {
				u_fprintf(to, "%S:%d ", g_flags[i].getTerminatedBuffer(), rule.sub_reading);
			}
			else {
				u_fprintf(to, "%S ", g_flags[i].getTerminatedBuffer());
			}
		}
	}

	if (rule.sublist) {
		u_fprintf(to, "%S ", rule.sublist->name.c_str());
	}

	if (rule.maplist) {
		u_fprintf(to, "%S ", rule.maplist->name.c_str());
	}

	if (rule.target) {
		u_fprintf(to, "%S ", grammar->sets_list[rule.target]->name.c_str());
	}

	for (auto it : rule.tests) {
		u_fprintf(to, "(");
		printContextualTest(to, *it);
		u_fprintf(to, ") ");
	}

	if (rule.dep_target) {
		u_fprintf(to, "TO (");
		printContextualTest(to, *(rule.dep_target));
		u_fprintf(to, ") ");
		for (auto it : rule.dep_tests) {
			u_fprintf(to, "(");
			printContextualTest(to, *it);
			u_fprintf(to, ") ");
		}
	}
}

void GrammarWriter::printContextualTest(std::ostream& to, const ContextualTest& test) {
	if (statistics) {
		if (ceil(test.total_time) == floor(test.total_time)) {
			u_fprintf(to, "\n#Test Matched: %u ; NoMatch: %u ; TotalTime: %.0f\n", test.num_match, test.num_fail, test.total_time);
		}
		else {
			u_fprintf(to, "\n#Test Matched: %u ; NoMatch: %u ; TotalTime: %f\n", test.num_match, test.num_fail, test.total_time);
		}
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
	else {
		if (test.pos & POS_NEGATE) {
			u_fprintf(to, "NEGATE ");
		}
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

		if (test.pos & POS_UNKNOWN) {
			u_fprintf(to, "?");
		}
		else {
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
		if (test.pos & POS_MARK_JUMP) {
			u_fprintf(to, "x");
		}
		if (test.pos & POS_LOOK_DELETED) {
			u_fprintf(to, "D");
		}
		if (test.pos & POS_LOOK_DELAYED) {
			u_fprintf(to, "d");
		}
		if (test.pos & POS_RELATION) {
			u_fprintf(to, "r:%S", grammar->single_tags.find(test.relation)->second->tag.c_str());
		}

		u_fprintf(to, " ");

		if (test.target) {
			u_fprintf(to, "%S ", grammar->sets_list[test.target]->name.c_str());
		}
		if (test.cbarrier) {
			u_fprintf(to, "CBARRIER %S ", grammar->sets_list[test.cbarrier]->name.c_str());
		}
		if (test.barrier) {
			u_fprintf(to, "BARRIER %S ", grammar->sets_list[test.barrier]->name.c_str());
		}
	}

	if (test.linked) {
		u_fprintf(to, "LINK ");
		printContextualTest(to, *(test.linked));
	}
}

void GrammarWriter::printTag(std::ostream& to, const Tag& tag) {
	UString str = tag.toUString(true);
	u_fprintf(to, "%S", str.c_str());
}
}
