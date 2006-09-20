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

#include "GrammarApplicator.h"
#include "Window.h"
#include "SingleWindow.h"
#include "uextras.h"

using namespace CG3;
using namespace CG3::Strings;

GrammarApplicator::GrammarApplicator() {
	// ToDo: Make --always-span switch
	num_windows = 2;
	grammar = 0;
}

GrammarApplicator::~GrammarApplicator() {
	grammar = 0;
}

void GrammarApplicator::setGrammar(const Grammar *res) {
	grammar = res;
}

uint32_t GrammarApplicator::addTag(const UChar *txt) {
	Tag *tag = new Tag();
	tag->parseTag(txt);
	uint32_t hash = tag->rehash();
	if (single_tags.find(hash) == single_tags.end()) {
		single_tags[hash] = tag;
	} else {
		delete tag;
	}
	return hash;
}

void GrammarApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {
	if (window->text) {
		u_fprintf(output, "%S", window->text);
	}

	std::vector<Cohort*>::iterator cter;
	for (cter = window->cohorts.begin() ; cter != window->cohorts.end() ; cter++) {
		if (cter == window->cohorts.begin()) {
			continue;
		}
		Cohort *cohort = *cter;
		single_tags[cohort->wordform]->printRaw(output);
		u_fprintf(output, "\n");
		if (cohort->text) {
			u_fprintf(output, "%S", cohort->text);
		}

		std::vector<Reading*>::iterator rter;
		for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
			Reading *reading = *rter;
			if (reading->noprint) {
				continue;
			}
			if (reading->deleted) {
				u_fprintf(output, ";");
			}
			u_fprintf(output, "\t");
			single_tags[reading->baseform]->printRaw(output);
			u_fprintf(output, " ");

			std::list<uint32_t>::iterator tter;
			for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
				Tag *tag = single_tags[*tter];
				if (!(tag->type & T_BASEFORM) && !(tag->type & T_WORDFORM)) {
					tag->printRaw(output);
					u_fprintf(output, " ");
				}
			}
			u_fprintf(output, "\n");
			if (reading->text) {
				u_fprintf(output, "%S", reading->text);
			}
		}
	}
}
