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

#include "stdafx.h"
#include "Strings.h"
#include "Grammar.h"
#include "GrammarWriter.h"
#include "uextras.h"

using namespace CG3;
using namespace CG3::Strings;

GrammarWriter::GrammarWriter() {
	grammar = 0;
}

GrammarWriter::~GrammarWriter() {
	grammar = 0;
}

void GrammarWriter::write_set_to_ufile(UFILE *output, Set *curset) {
	if (curset->sets.empty() && !curset->used) {
		curset->used = true;
		u_fprintf(output, "LIST %S = ", curset->getName());
		stdext::hash_map<uint32_t, uint32_t>::iterator comp_iter;
		for (comp_iter = curset->single_tags.begin() ; comp_iter != curset->single_tags.end() ; comp_iter++) {
			grammar->single_tags[comp_iter->second]->print(output);
			u_fprintf(output, " ");
		}
		for (comp_iter = curset->tags.begin() ; comp_iter != curset->tags.end() ; comp_iter++) {
			if (grammar->tags.find(comp_iter->second) != grammar->tags.end()) {
				CompositeTag *curcomptag = grammar->tags[comp_iter->second];
				if (curcomptag->tags.size() == 1) {
					grammar->single_tags[curcomptag->tags.begin()->second]->print(output);
					u_fprintf(output, " ");
				} else {
					u_fprintf(output, "(");
					std::map<uint32_t, uint32_t>::iterator tag_iter;
					for (tag_iter = curcomptag->tags_map.begin() ; tag_iter != curcomptag->tags_map.end() ; tag_iter++) {
						grammar->single_tags[tag_iter->second]->print(output);
						u_fprintf(output, " ");
					}
					u_fprintf(output, ") ");
				}
			}
		}
		u_fprintf(output, "\n");
	} else if (!curset->sets.empty() && !curset->used) {
		curset->used = true;
		for (uint32_t i=0;i<curset->sets.size();i++) {
			write_set_to_ufile(output, grammar->sets_by_contents[curset->sets.at(i)]);
		}
		u_fprintf(output, "SET %S = ", curset->getName());
		u_fprintf(output, "%S ", grammar->sets_by_contents[curset->sets.at(0)]->name);
		for (uint32_t i=0;i<curset->sets.size()-1;i++) {
			u_fprintf(output, "%S %S ", stringbits[curset->set_ops.at(i)], grammar->sets_by_contents[curset->sets.at(i+1)]->name);
		}
		u_fprintf(output, "\n");
	}
}

int GrammarWriter::write_grammar_to_ufile_text(UFILE *output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue! Hint: call setGrammar() first.\n");
		return -1;
	}

	free_strings();
	free_keywords();
	int error = init_keywords();
	if (error) {
		u_fprintf(ux_stderr, "Error: init_keywords returned %u!\n", error);
		return error;
	}
	error = init_strings();
	if (error) {
		u_fprintf(ux_stderr, "Error: init_strings returned %u!\n", error);
		return error;
	}

	u_fprintf(output, "# DELIMITERS does not exist. Instead, look for the set _S_DELIMITERS_\n");

	u_fprintf(output, "PREFERRED-TARGETS = ");
	std::vector<UChar*>::iterator iter;
	for(iter = grammar->preferred_targets.begin() ; iter != grammar->preferred_targets.end() ; iter++ ) {
		u_fprintf(output, "%S ", *iter);
	}
	u_fprintf(output, "\n");

	u_fprintf(output, "\n");

	stdext::hash_map<uint32_t, Set*>::iterator set_iter;
	for (set_iter = grammar->sets_by_contents.begin() ; set_iter != grammar->sets_by_contents.end() ; set_iter++) {
		write_set_to_ufile(output, set_iter->second);
	}
	u_fprintf(output, "\n");

	for (uint32_t i=1;i<grammar->sections.size();i++) {
		u_fprintf(output, "\nSECTION\n");
		for (uint32_t j=grammar->sections[i-1];j<grammar->sections[i];j++) {
			grammar->printRule(output, grammar->rules[j]);
			u_fprintf(output, "\n");
		}
	}

	return 0;
}

int GrammarWriter::write_grammar_to_file_binary(FILE *output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue! Hint: call setGrammar() first.\n");
		return -1;
	}

	fprintf(output, "CG3B");
	uint32_t tmp = (uint32_t)htonl((uint32_t)grammar->sets_by_contents.size());
	fwrite(&tmp, sizeof(uint32_t), 1, output);

	return 0;
}

void GrammarWriter::setGrammar(Grammar *res) {
	grammar = res;
}
