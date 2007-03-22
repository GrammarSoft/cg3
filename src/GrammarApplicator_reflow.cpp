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

void GrammarApplicator::reflowSingleWindow(SingleWindow *swindow) {
	swindow->tags.clear();
	swindow->tags_mapped.clear();
	swindow->tags_plain.clear();
	swindow->tags_textual.clear();
	swindow->parent->dep_map.clear();

	for (uint32_t c=0 ; c < swindow->cohorts.size() ; c++) {
		Cohort *cohort = swindow->cohorts[c];

		std::list<Reading*>::iterator rter;
		for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
			Reading *reading = *rter;

			std::list<uint32_t>::const_iterator tter;
			for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
				swindow->tags[*tter] = *tter;
				Tag *tag = 0;
				if (grammar->single_tags.find(*tter) != grammar->single_tags.end()) {
					tag = grammar->single_tags.find(*tter)->second;
				}
				else {
					tag = single_tags.find(*tter)->second;
				}
				assert(tag != 0);
				if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
					swindow->tags_mapped[*tter] = *tter;
				}
				if (tag->type & T_TEXTUAL) {
					swindow->tags_textual[*tter] = *tter;
				}
				if (tag->type & T_DEPENDENCY) {
					dep_highest_seen = max(tag->dep_self, dep_highest_seen);
					if (swindow->parent->dep_map.find(tag->dep_self) == swindow->parent->dep_map.end()) {
						swindow->parent->dep_map[tag->dep_self] = c;
					}
				}
				if (!tag->type) {
					swindow->tags_plain[*tter] = *tter;
				}
			}
		}
	}

	if (!swindow->parent->dep_map.empty()) {
		swindow->parent->dep_map[0] = 0;
		for (uint32_t c=0 ; c < swindow->cohorts.size() ; c++) {
			Cohort *cohort = swindow->cohorts[c];

			std::list<Reading*>::iterator rter;
			for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
				Reading *reading = *rter;

				std::list<uint32_t>::const_iterator tter;
				for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
					Tag *tag = 0;
					if (grammar->single_tags.find(*tter) != grammar->single_tags.end()) {
						tag = grammar->single_tags.find(*tter)->second;
					}
					else {
						tag = single_tags.find(*tter)->second;
					}
					assert(tag != 0);
					if (tag->type & T_DEPENDENCY) {
						if (swindow->parent->dep_map.find(tag->dep_parent) == swindow->parent->dep_map.end()) {
							u_fprintf(ux_stderr, "Warning: Parent %u does not exist - ignoring.\n", tag->dep_parent);
						}
						else {
							swindow->cohorts[swindow->parent->dep_map.find(tag->dep_parent)->second]->addChild(tag->dep_self);
						}
					}
				}
			}
		}

		for (uint32_t c=0 ; c < swindow->cohorts.size() ; c++) {
			Cohort *cohort = swindow->cohorts[c];

			std::list<Reading*>::iterator rter;
			for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
				Reading *reading = *rter;

				uint32_t dep_real = 0;
				std::set<uint32_t>::const_iterator tter;
				for (tter = reading->dep_children.begin() ; tter != reading->dep_children.end() ; tter++) {
					dep_real = swindow->parent->dep_map.find(*tter)->second;
					std::set<uint32_t>::const_iterator ster;
					for (ster = reading->dep_children.begin() ; ster != reading->dep_children.end() ; ster++) {
						swindow->cohorts[dep_real]->addSibling(*ster);
					}
					swindow->cohorts[dep_real]->remSibling(dep_real);
				}
			}
		}
	}

	swindow->rehash();
}

void GrammarApplicator::reflowReading(Reading *reading) {
	reading->tags.clear();
	reading->tags_mapped.clear();
	reading->tags_plain.clear();
	reading->tags_textual.clear();
	reading->tags_numerical.clear();

	std::list<uint32_t>::const_iterator tter;
	for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
		reading->tags[*tter] = *tter;
		Tag *tag = 0;
		if (grammar->single_tags.find(*tter) != grammar->single_tags.end()) {
			tag = grammar->single_tags.find(*tter)->second;
		}
		else {
			tag = single_tags.find(*tter)->second;
		}
		assert(tag != 0);
		if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
			reading->tags_mapped[*tter] = *tter;
		}
		if (tag->type & (T_TEXTUAL|T_WORDFORM|T_BASEFORM)) {
			reading->tags_textual[*tter] = *tter;
		}
		if (tag->type & T_NUMERICAL) {
			reading->tags_numerical[*tter] = *tter;
		}
		if (!reading->baseform && tag->type & T_BASEFORM) {
			reading->baseform = tag->hash;
		}
		if (!reading->wordform && tag->type & T_WORDFORM) {
			reading->wordform = tag->hash;
		}
		if (tag->type & T_DEPENDENCY) {
			reading->dep_self = tag->dep_self;
			reading->dep_parents.insert(tag->dep_parent);
		}
		if (!tag->type) {
			reading->tags_plain[*tter] = *tter;
		}
	}
	assert(!reading->tags.empty());
	reading->rehash();
}
