/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
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
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		return -1;
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

	if (!grammar->before_sections.empty()) {
		u_fprintf(output, "BEFORE-SECTIONS\n");
		for (uint32_t j=0;j<grammar->before_sections.size();j++) {
			printRule(output, grammar->before_sections[j]);
			u_fprintf(output, " ;\n");
		}
	}

	for (uint32_t i=1;i<grammar->sections.size();i++) {
		u_fprintf(output, "\nSECTION\n");
		for (uint32_t j=grammar->sections[i-1];j<grammar->sections[i];j++) {
			printRule(output, grammar->rules[j]);
			u_fprintf(output, " ;\n");
		}
	}

	if (!grammar->after_sections.empty()) {
		u_fprintf(output, "AFTER-SECTIONS\n");
		for (uint32_t j=0;j<grammar->after_sections.size();j++) {
			printRule(output, grammar->after_sections[j]);
			u_fprintf(output, " ;\n");
		}
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
