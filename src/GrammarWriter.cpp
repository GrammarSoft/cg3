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

using namespace CG3;
using namespace CG3::Strings;

GrammarWriter::GrammarWriter(Grammar *res, UFILE *ux_err) {
	used_sets.clear();
	statistics = false;
	ux_stderr = ux_err;
	grammar = res;
}

GrammarWriter::~GrammarWriter() {
	grammar = 0;
}

void GrammarWriter::printSet(UFILE *output, const Set *curset) {
	if (curset->sets.empty() && used_sets.find(curset->hash) == used_sets.end()) {
		if (statistics) {
			u_fprintf(output, "#List Matched: %u ; NoMatch: %u ; TotalTime: %u\n", curset->num_match, curset->num_fail, curset->total_time);
		}
		used_sets.insert(curset->hash);
		u_fprintf(output, "LIST %S = ", curset->name);
		uint32HashSet::const_iterator comp_iter;
		for (comp_iter = curset->single_tags.begin() ; comp_iter != curset->single_tags.end() ; comp_iter++) {
			printTag(output, grammar->single_tags.find(*comp_iter)->second);
			u_fprintf(output, " ");
		}
		for (comp_iter = curset->tags.begin() ; comp_iter != curset->tags.end() ; comp_iter++) {
			if (grammar->tags.find(*comp_iter) != grammar->tags.end()) {
				CompositeTag *curcomptag = grammar->tags.find(*comp_iter)->second;
				if (curcomptag->tags.size() == 1) {
					printTag(output, grammar->single_tags.find(*(curcomptag->tags.begin()))->second);
					u_fprintf(output, " ");
				} else {
					u_fprintf(output, "(");
					uint32Set::const_iterator tag_iter;
					for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
						printTag(output, grammar->single_tags.find(*tag_iter)->second);
						u_fprintf(output, " ");
					}
					u_fprintf(output, ") ");
				}
			}
		}
		u_fprintf(output, " ;\n");
	} else if (!curset->sets.empty() && used_sets.find(curset->hash) == used_sets.end()) {
		used_sets.insert(curset->hash);
		for (uint32_t i=0;i<curset->sets.size();i++) {
			printSet(output, grammar->sets_by_contents.find(curset->sets.at(i))->second);
		}
		if (statistics) {
			u_fprintf(output, "#Set Matched: %u ; NoMatch: %u ; TotalTime: %u\n", curset->num_match, curset->num_fail, curset->total_time);
		}
		u_fprintf(output, "SET %S = ", curset->name);
		u_fprintf(output, "%S ", grammar->sets_by_contents.find(curset->sets.at(0))->second->name);
		for (uint32_t i=0;i<curset->sets.size()-1;i++) {
			u_fprintf(output, "%S %S ", stringbits[curset->set_ops.at(i)], grammar->sets_by_contents.find(curset->sets.at(i+1))->second->name);
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
		u_fprintf(output, "# Total clock() time spent applying grammar: %u\n", grammar->total_time);
	}
	u_fprintf(output, "# DELIMITERS and SOFT-DELIMITERS do not exist. Instead, look for the sets _S_DELIMITERS_ and _S_SOFT_DELIMITERS_.\n");

	u_fprintf(output, "MAPPING-PREFIX = %C ;\n", grammar->mapping_prefix);

	if (!grammar->preferred_targets.empty()) {
		u_fprintf(output, "PREFERRED-TARGETS = ");
		uint32Vector::const_iterator iter;
		for(iter = grammar->preferred_targets.begin() ; iter != grammar->preferred_targets.end() ; iter++ ) {
			printTag(output, grammar->single_tags.find(*iter)->second);
			u_fprintf(output, " ");
		}
		u_fprintf(output, " ;\n");
	}

	u_fprintf(output, "\n");

	used_sets.clear();
	stdext::hash_map<uint32_t, Set*>::const_iterator set_iter;
	for (set_iter = grammar->sets_by_contents.begin() ; set_iter != grammar->sets_by_contents.end() ; set_iter++) {
		printSet(output, set_iter->second);
	}
	u_fprintf(output, "\n");

	std::vector<ContextualTest*>::const_iterator tmpl_iter;
	for (tmpl_iter = grammar->template_list.begin() ; tmpl_iter != grammar->template_list.end() ; tmpl_iter++) {
		u_fprintf(output, "TEMPLATE %u = ", (*tmpl_iter)->name);
		printContextualTest(output, *tmpl_iter);
		u_fprintf(output, " ;\n");
	}
	u_fprintf(output, "\n");

	int32_t lsect = -999;
	std::map<uint32_t, Rule*>::const_iterator rule_iter;
	for (rule_iter = grammar->rule_by_line.begin() ; rule_iter != grammar->rule_by_line.end() ; rule_iter++) {
		const Rule *r = rule_iter->second;
		if (lsect != r->section) {
			if (r->section == -2) {
				u_fprintf(output, "AFTER-SECTIONS\n");
			}
			else if (r->section == -1) {
				u_fprintf(output, "BEFORE-SECTIONS\n");
			}
			else {
				u_fprintf(output, "\nSECTION\n");
			}
			lsect = r->section;
		}
		printRule(output, r);
		u_fprintf(output, " ;\n");
	}

	return 0;
}

void GrammarWriter::printRule(UFILE *to, const Rule *rule) {
	if (statistics) {
		u_fprintf(to, "\n#Rule Matched: %u ; NoMatch: %u ; TotalTime: %u\n", rule->num_match, rule->num_fail, rule->total_time);
	}
	if (rule->wordform) {
		printTag(to, grammar->single_tags.find(rule->wordform)->second);
		u_fprintf(to, " ");
	}

	u_fprintf(to, "%S", keywords[rule->type]);

	if (rule->name && !(rule->name[0] == '_' && rule->name[1] == 'R' && rule->name[2] == '_')) {
		u_fprintf(to, ":%S", rule->name);
	}
	u_fprintf(to, " ");

	if (!rule->sublist.empty()) {
		uint32List::const_iterator iter;
		u_fprintf(to, "(");
		for (iter = rule->sublist.begin() ; iter != rule->sublist.end() ; iter++) {
			printTag(to, grammar->single_tags.find(*iter)->second);
			u_fprintf(to, " ");
		}
		u_fprintf(to, ") ");
	}

	if (!rule->maplist.empty()) {
		uint32List::const_iterator iter;
		u_fprintf(to, "(");
		for (iter = rule->maplist.begin() ; iter != rule->maplist.end() ; iter++) {
			printTag(to, grammar->single_tags.find(*iter)->second);
			u_fprintf(to, " ");
		}
		u_fprintf(to, ") ");
	}

	if (rule->target) {
		u_fprintf(to, "%S ", grammar->sets_by_contents.find(rule->target)->second->name);
	}

	if (rule->tests.size()) {
		std::list<ContextualTest*>::const_iterator iter;
		for (iter = rule->tests.begin() ; iter != rule->tests.end() ; iter++) {
			u_fprintf(to, "(");
			printContextualTest(to, *iter);
			u_fprintf(to, ") ");
		}
	}

	if (rule->dep_target) {
		u_fprintf(to, "TO (");
		printContextualTest(to, rule->dep_target);
		u_fprintf(to, ") ");
		if (rule->dep_tests.size()) {
			std::list<ContextualTest*>::const_iterator iter;
			for (iter = rule->dep_tests.begin() ; iter != rule->dep_tests.end() ; iter++) {
				u_fprintf(to, "(");
				printContextualTest(to, *iter);
				u_fprintf(to, ") ");
			}
		}
	}
}

void GrammarWriter::printContextualTest(UFILE *to, const ContextualTest *test) {
	if (statistics) {
		u_fprintf(to, "\n#Test Matched: %u ; NoMatch: %u ; TotalTime: %u\n", test->num_match, test->num_fail, test->total_time);
	}
	if (test->tmpl) {
		u_fprintf(to, "T:%u ", test->tmpl->name);
	}
	else if (!test->ors.empty()) {
		std::list<ContextualTest*>::const_iterator iter;
		for (iter = test->ors.begin() ; iter != test->ors.end() ; ) {
			u_fprintf(to, "(");
			printContextualTest(to, *iter);
			u_fprintf(to, ")");
			iter++;
			if (iter != test->ors.end()) {
				u_fprintf(to, " OR ");
			}
			else {
				u_fprintf(to, " ");
			}
		}
	}
	else {
		if (test->pos & POS_NEGATED) {
			u_fprintf(to, "NEGATE ");
		}
		if (test->pos & POS_NEGATIVE) {
			u_fprintf(to, "NOT ");
		}
		if (test->pos & POS_ABSOLUTE) {
			u_fprintf(to, "@");
		}
		if (test->pos & POS_SCANALL) {
			u_fprintf(to, "**");
		}
		else if (test->pos & POS_SCANFIRST) {
			u_fprintf(to, "*");
		}

		if (test->pos & POS_DEP_CHILD) {
			u_fprintf(to, "c");
		}
		if (test->pos & POS_DEP_PARENT) {
			u_fprintf(to, "p");
		}
		if (test->pos & POS_DEP_SIBLING) {
			u_fprintf(to, "s");
		}

		u_fprintf(to, "%d", test->offset);

		if (test->pos & POS_CAREFUL) {
			u_fprintf(to, "C");
		}
		if (test->pos & POS_SPAN_BOTH) {
			u_fprintf(to, "W");
		}
		if (test->pos & POS_SPAN_LEFT) {
			u_fprintf(to, "<");
		}
		if (test->pos & POS_SPAN_RIGHT) {
			u_fprintf(to, ">");
		}

		u_fprintf(to, " ");

		if (test->target) {
			u_fprintf(to, "%S ", grammar->sets_by_contents.find(test->target)->second->name);
		}
		if (test->cbarrier) {
			u_fprintf(to, "CBARRIER %S ", grammar->sets_by_contents.find(test->cbarrier)->second->name);
		}
		if (test->barrier) {
			u_fprintf(to, "BARRIER %S ", grammar->sets_by_contents.find(test->barrier)->second->name);
		}
	}

	if (test->linked) {
		u_fprintf(to, "LINK ");
		printContextualTest(to, test->linked);
	}
}

void GrammarWriter::printTag(UFILE *to, const Tag *tag) {
	if (tag->type & T_NEGATIVE) {
		u_fprintf(to, "!");
	}
	if (tag->type & T_FAILFAST) {
		u_fprintf(to, "^");
	}
	if (tag->type & T_META) {
		u_fprintf(to, "META:");
	}
	if (tag->type & T_VARIABLE) {
		u_fprintf(to, "VAR:");
	}

	UChar *tmp = gbuffers[0];
	ux_escape(tmp, tag->tag);
	u_fprintf(to, "%S", tmp);

	if (tag->type & T_CASE_INSENSITIVE) {
		u_fprintf(to, "i");
	}
	if (tag->type & T_REGEXP) {
		u_fprintf(to, "r");
	}
}
