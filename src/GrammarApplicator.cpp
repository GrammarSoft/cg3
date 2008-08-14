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

#include "GrammarApplicator.h"

using namespace CG3;
using namespace CG3::Strings;

GrammarApplicator::GrammarApplicator(UFILE *ux_in, UFILE *ux_out, UFILE *ux_err) {
	ux_stdin = ux_in;
	ux_stdout = ux_out;
	ux_stderr = ux_err;
	always_span = false;
	apply_mappings = true;
	apply_corrections = true;
	allow_magic_readings = true;
	trace = false;
	trace_name_only = false;
	trace_no_removed = false;
	single_run = false;
	statistics = false;
	dep_reenum = false;
	dep_delimit = false;
	dep_humanize = false;
	dep_original = false;
	dep_block_loops = true;
	dep_highest_seen = 0;
	has_dep = false;
	sections = 0;
	verbosity_level = 0;
	num_windows = 2;
	begintag = 0;
	endtag = 0;
	grammar = 0;
	skipped_rules = 0;
	cache_hits = 0;
	cache_miss = 0;
	match_single = 0;
	match_comp = 0;
	match_sub = 0;
	soft_limit = 300;
	hard_limit = 500;
	gWindow = 0;
	numLines = 0;
	numWindows = 0;
	numCohorts = 0;
	numReadings = 0;
	no_sections = false;
	no_after_sections = false;
	no_before_sections = false;
}

GrammarApplicator::~GrammarApplicator() {
	stdext::hash_map<uint32_t, Tag*>::iterator iter_stag;
	for (iter_stag = single_tags.begin() ; iter_stag != single_tags.end() ; iter_stag++) {
		if (iter_stag->second && !iter_stag->second->in_grammar) {
			delete iter_stag->second;
			iter_stag->second = 0;
		}
	}

	Recycler *r = Recycler::instance();
	stdext::hash_map<uint32_t, uint32HashSet*>::iterator indt;
	for (indt = index_reading_yes.begin() ; indt != index_reading_yes.end() ; indt++) {
		r->delete_uint32HashSet(indt->second);
	}
	for (indt = index_reading_no.begin() ; indt != index_reading_no.end() ; indt++) {
		r->delete_uint32HashSet(indt->second);
	}
	for (indt = index_tags_regexp.begin() ; indt != index_tags_regexp.end() ; indt++) {
		r->delete_uint32HashSet(indt->second);
	}

	if (gWindow) {
		delete gWindow;
	}
	grammar = 0;
	ux_stdin = ux_stderr = ux_stdout = 0;
}

void GrammarApplicator::resetIndexes() {
	Recycler *r = Recycler::instance();
	stdext::hash_map<uint32_t, uint32HashSet*>::iterator indt;

	for (indt = index_reading_yes.begin() ; indt != index_reading_yes.end() ; indt++) {
		r->delete_uint32HashSet(indt->second);
	}
	index_reading_yes.clear();

	for (indt = index_reading_no.begin() ; indt != index_reading_no.end() ; indt++) {
		r->delete_uint32HashSet(indt->second);
	}
	index_reading_no.clear();

	for (indt = index_tags_regexp.begin() ; indt != index_tags_regexp.end() ; indt++) {
		r->delete_uint32HashSet(indt->second);
	}
	index_tags_regexp.clear();
}

void GrammarApplicator::setGrammar(const Grammar *res) {
	grammar = res;
	single_tags.insert(grammar->single_tags.begin(), grammar->single_tags.end());
}

void GrammarApplicator::enableStatistics() {
	statistics = true;
}

void GrammarApplicator::disableStatistics() {
	statistics = false;
}

Tag *GrammarApplicator::addTag(const UChar *txt) {
	uint32_t hash = hash_sdbm_uchar(txt);
	if (single_tags.find(hash) != single_tags.end()) {
		return single_tags[hash];
	}

	Tag *tag = new Tag();
	tag->parseTagRaw(txt);
	hash = tag->rehash();
	if (single_tags.find(hash) == single_tags.end()) {
		single_tags[hash] = tag;
	} else {
		delete tag;
	}
	return single_tags[hash];
}

void GrammarApplicator::printReading(Reading *reading, UFILE *output) {
	if (reading->noprint) {
		return;
	}

	if (reading->deleted) {
		u_fprintf(output, ";");
	}

	u_fprintf(output, "\t");

	if (reading->baseform) {
		Tag::printTagRaw(output, single_tags[reading->baseform]);
		u_fprintf(output, " ");
	}

	stdext::hash_map<uint32_t, uint32_t> used_tags;
	foreach (uint32List, reading->tags_list, tter, tter_end) {
		if (used_tags.find(*tter) != used_tags.end()) {
			continue;
		}
		if (*tter == endtag || *tter == begintag) {
			continue;
		}
		used_tags[*tter] = *tter;
		const Tag *tag = single_tags[*tter];
		if (!(tag->type & T_BASEFORM) && !(tag->type & T_WORDFORM)) {
			Tag::printTagRaw(output, tag);
			u_fprintf(output, " ");
		}
	}

	if (has_dep && !dep_original) {
		if (!reading->parent->dep_self) {
			reading->parent->dep_self = reading->parent->global_number;
		}
		u_fprintf(output, "#%u->%u ", reading->parent->dep_self, reading->parent->dep_parent);
	}

	if (reading->parent->is_related) {
		u_fprintf(output, "ID:%u ", reading->parent->global_number);
		if (!reading->parent->relations.empty()) {
			std::multimap<uint32_t,uint32_t>::iterator miter;
			for (miter = reading->parent->relations.begin() ; miter != reading->parent->relations.end() ; miter++) {
				u_fprintf(output, "R:");
				Tag::printTagRaw(output, single_tags[miter->second]);
				u_fprintf(output, ":%u ", miter->first);
			}
		}
	}

	if (trace) {
		foreach (uint32Vector, reading->hit_by, iter_hb, iter_hb_end) {
			Rule *r = grammar->rule_by_line.find(*iter_hb)->second;
			u_fprintf(output, "%S", keywords[r->type]);
			if (!trace_name_only || !r->name) {
				u_fprintf(output, ":%u", *iter_hb);
			}
			if (r->name) {
				u_fprintf(output, ":%S", r->name);
			}
			u_fprintf(output, " ");
		}
	}

	u_fprintf(output, "\n");
}

void GrammarApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {
	if (window->text) {
		u_fprintf(output, "%S", window->text);
	}

	uint32_t cs = (uint32_t)window->cohorts.size();
	for (uint32_t c=1 ; c < cs ; c++) {
		Cohort *cohort = window->cohorts.at(c);
		Tag::printTagRaw(output, single_tags[cohort->wordform]);
		//u_fprintf(output, " %u", cohort->number);
		u_fprintf(output, "\n");
		if (cohort->text) {
			u_fprintf(output, "%S", cohort->text);
		}

		mergeMappings(cohort);

		foreach (std::list<Reading*>, cohort->readings, rter1, rter1_end) {
			printReading(*rter1, output);
		}
		if (trace && !trace_no_removed) {
			foreach (std::list<Reading*>, cohort->deleted, rter2, rter2_end) {
				printReading(*rter2, output);
			}
		}
		foreach (std::list<Reading*>, cohort->readings, rter3, rter3_end) {
			Reading *reading = *rter3;
			if (reading->text) {
				u_fprintf(output, "%S", reading->text);
			}
		}
		foreach (std::list<Reading*>, cohort->deleted, rter4, rter4_end) {
			Reading *reading = *rter4;
			if (reading->text) {
				u_fprintf(output, "%S", reading->text);
			}
		}
	}
	u_fprintf(output, "\n");
}
