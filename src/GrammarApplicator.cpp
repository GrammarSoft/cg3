/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"

namespace CG3 {

GrammarApplicator::GrammarApplicator(UFILE *ux_err) {
	ux_stderr = ux_err;
	always_span = false;
	apply_mappings = true;
	apply_corrections = true;
	allow_magic_readings = true;
	trace = false;
	trace_name_only = false;
	trace_no_removed = false;
	trace_encl = false;
	statistics = false;
	dep_has_spanned = false;
	dep_delimit = false;
	dep_humanize = false;
	dep_original = false;
	dep_block_loops = true;
	dep_block_crossing = false;
	dep_highest_seen = 0;
	has_dep = false;
	verbosity_level = 0;
	debug_level = 0;
	num_windows = 2;
	begintag = 0;
	endtag = 0;
	grammar = 0;
	target = 0;
	mark = 0;
	attach_to = 0;
	match_single = 0;
	match_comp = 0;
	match_sub = 0;
	soft_limit = 300;
	hard_limit = 500;
	section_max_count = 0;
	numLines = 0;
	numWindows = 0;
	numCohorts = 0;
	numReadings = 0;
	numsections = 0;
	unif_last_wordform = 0;
	unif_last_baseform = 0;
	unif_last_textual = 0;
	unif_sets_firstrun = false;
	no_sections = false;
	no_after_sections = false;
	no_before_sections = false;
	no_pass_origin = false;
	has_enclosures = false;
	did_final_enclosure = false;
	did_index = false;
	unsafe = false;
	ordered = false;
	show_end_tags = false;
	gWindow = new Window(this);
	ci_depths.resize(4, 0);
}

GrammarApplicator::~GrammarApplicator() {
	Taguint32HashMap::iterator iter_stag;
	for (iter_stag = single_tags.begin() ; iter_stag != single_tags.end() ; iter_stag++) {
		if (iter_stag->second && !iter_stag->second->in_grammar) {
			delete iter_stag->second;
			iter_stag->second = 0;
		}
	}

	delete gWindow;
	grammar = 0;
	ux_stderr = 0;
}

void GrammarApplicator::resetIndexes() {
	index_readingSet_yes.clear();
	index_readingSet_no.clear();
	index_regexp_yes.clear();
	index_regexp_no.clear();
	index_icase_yes.clear();
	index_icase_no.clear();
}

void GrammarApplicator::setGrammar(const Grammar *res) {
	grammar = res;
	single_tags = grammar->single_tags;
}

void GrammarApplicator::index() {
	if (did_index) {
		return;
	}

	if (!grammar->before_sections.empty()) {
		uint32SortedVector& m = runsections[-1];
		const_foreach (RuleVector, grammar->before_sections, iter_rules, iter_rules_end) {
			const Rule *r = *iter_rules;
			m.insert(r->line);
		}
	}

	if (!grammar->after_sections.empty()) {
		uint32SortedVector& m = runsections[-2];
		const_foreach (RuleVector, grammar->after_sections, iter_rules, iter_rules_end) {
			const Rule *r = *iter_rules;
			m.insert(r->line);
		}
	}

	if (!grammar->null_section.empty()) {
		uint32SortedVector& m = runsections[-3];
		const_foreach (RuleVector, grammar->null_section, iter_rules, iter_rules_end) {
			const Rule *r = *iter_rules;
			m.insert(r->line);
		}
	}

	if (sections.empty()) {
		int32_t smax = (int32_t)grammar->sections.size();
		for (int32_t i=0 ; i < smax ; i++) {
			const_foreach (RuleVector, grammar->rules, iter_rules, iter_rules_end) {
				const Rule *r = *iter_rules;
				if (r->section < 0 || r->section > i) {
					continue;
				}
				uint32SortedVector& m = runsections[i];
				m.insert(r->line);
			}
		}
	}
	else {
		numsections = sections.size();
		for (uint32_t n=0 ; n<numsections ; n++) {
			for (uint32_t e=0 ; e<=n ; e++) {
				const_foreach (RuleVector, grammar->rules, iter_rules, iter_rules_end) {
					const Rule *r = *iter_rules;
					if (r->section != (int32_t)sections.at(e)-1) {
						continue;
					}
					uint32SortedVector& m = runsections[n];
					m.insert(r->line);
				}
			}
		}
	}

	did_index = true;
}

void GrammarApplicator::enableStatistics() {
	statistics = true;
}

void GrammarApplicator::disableStatistics() {
	statistics = false;
}

Tag *GrammarApplicator::addTag(const UChar *txt) {
	Taguint32HashMap::iterator it;
	uint32_t thash = hash_sdbm_uchar(txt);
	if ((it = single_tags.find(thash)) != single_tags.end() && it->second->tag && u_strcmp(it->second->tag, txt) == 0) {
		return it->second;
	}

	Tag *tag = new Tag();
	tag->parseTagRaw(txt);
	uint32_t hash = tag->rehash();
	uint32_t seed = 0;
	for ( ; seed < 10000 ; seed++) {
		uint32_t ih = hash + seed;
		if ((it = single_tags.find(ih)) != single_tags.end()) {
			Tag *t = it->second;
			if (t->tag && u_strcmp(t->tag, tag->tag) == 0) {
				hash += seed;
				delete tag;
				break;
			}
		}
		else {
			if (seed && verbosity_level > 0) {
				u_fprintf(ux_stderr, "Warning: Tag %S got hash seed %u.\n", txt, seed);
				u_fflush(ux_stderr);
			}
			tag->seed = seed;
			hash = tag->rehash();
			single_tags[hash] = tag;
			break;
		}
	}
	return single_tags[hash];
}

void GrammarApplicator::printReading(Reading *reading, UFILE *output) {
	if (reading->noprint) {
		return;
	}

	if (reading->deleted) {
		u_fputc(';', output);
	}

	u_fputc('\t', output);

	if (reading->baseform) {
		u_fprintf(output, "%S", single_tags.find(reading->baseform)->second->tag);
		u_fputc(' ', output);
	}

	uint32HashMap used_tags;
	foreach (uint32List, reading->tags_list, tter, tter_end) {
		if ((!show_end_tags && *tter == endtag) || *tter == begintag) {
			continue;
		}
		if (!ordered) {
			if (used_tags.find(*tter) != used_tags.end()) {
				continue;
			}
			used_tags[*tter] = *tter;
		}
		const Tag *tag = single_tags[*tter];
		if (tag->type & T_DEPENDENCY && has_dep && !dep_original) {
			continue;
		}
		if (!(tag->type & T_BASEFORM) && !(tag->type & T_WORDFORM)) {
			u_fprintf(output, "%S", tag->tag);
			u_fputc(' ', output);
		}
	}

	if (has_dep) {
		if (!reading->parent->dep_self) {
			reading->parent->dep_self = reading->parent->global_number;
		}
		const Cohort *pr = 0;
		pr = reading->parent;
		if (reading->parent->dep_parent != std::numeric_limits<uint32_t>::max()) {
			if (reading->parent->dep_parent == 0) {
				pr = reading->parent->parent->cohorts.at(0);
			}
			else if (reading->parent->parent->parent->cohort_map.find(reading->parent->dep_parent) != reading->parent->parent->parent->cohort_map.end()) {
				pr = reading->parent->parent->parent->cohort_map[reading->parent->dep_parent];
			}
		}
		if (dep_humanize) {
			u_fprintf(output, "#w%u,c%u->w%u,c%u ",
				pr->parent->number,
				reading->parent->local_number,
				reading->parent->parent->number,
				pr->local_number);
		}
		if (!dep_has_spanned) {
			u_fprintf(output, "#%u->%u ",
				reading->parent->local_number,
				pr->local_number);
		}
		else {
			if (reading->parent->dep_parent == std::numeric_limits<uint32_t>::max()) {
				u_fprintf(output, "#%u->%u ",
					reading->parent->dep_self,
					reading->parent->dep_self);
			}
			else {
				u_fprintf(output, "#%u->%u ",
					reading->parent->dep_self,
					reading->parent->dep_parent);
			}
		}
	}

	if (reading->parent->type & CT_RELATED) {
		u_fprintf(output, "ID:%u ", reading->parent->global_number);
		if (!reading->parent->relations.empty()) {
			foreach(RelationCtn, reading->parent->relations, miter, miter_end) {
				foreach(uint32Set, miter->second, siter, siter_end) {
					u_fprintf(output, "R:%S:%u ", grammar->single_tags.find(miter->first)->second->tag, *siter);
				}
			}
		}
	}

	if (trace) {
		foreach (uint32Vector, reading->hit_by, iter_hb, iter_hb_end) {
			RuleByLineHashMap::const_iterator ruit = grammar->rule_by_line.find(*iter_hb);
			if (ruit != grammar->rule_by_line.end()) {
				const Rule *r = ruit->second;
				u_fprintf(output, "%S", keywords[r->type].getTerminatedBuffer());
				if (r->type == K_ADDRELATION || r->type == K_SETRELATION || r->type == K_REMRELATION
				|| r->type == K_ADDRELATIONS || r->type == K_SETRELATIONS || r->type == K_REMRELATIONS
					) {
						u_fprintf(output, "(%S", grammar->single_tags.find(r->maplist.front()->hash)->second->tag);
						if (r->type == K_ADDRELATIONS || r->type == K_SETRELATIONS || r->type == K_REMRELATIONS) {
							u_fprintf(output, ",%S", grammar->single_tags.find(r->sublist.front())->second->tag);
						}
						u_fprintf(output, ")");
				}
				if (!trace_name_only || !r->name) {
					u_fprintf(output, ":%u", *iter_hb);
				}
				if (r->name) {
					u_fputc(':', output);
					u_fprintf(output, "%S", r->name);
				}
			}
			else {
				uint32_t pass = std::numeric_limits<uint32_t>::max() - (*iter_hb);
				u_fprintf(output, "ENCL:%u", pass);
			}
			u_fputc(' ', output);
		}
	}

	u_fputc('\n', output);
}

void GrammarApplicator::printCohort(Cohort *cohort, UFILE *output) {
	if (cohort->type & CT_REMOVED) {
		u_fputc(';', output);
		u_fputc(' ', output);
	}
	u_fprintf(output, "%S", single_tags.find(cohort->wordform)->second->tag);
	u_fputc('\n', output);

	mergeMappings(*cohort);

	foreach (ReadingList, cohort->readings, rter1, rter1_end) {
		printReading(*rter1, output);
	}
	if (trace && !trace_no_removed) {
		foreach (ReadingList, cohort->delayed, rter3, rter3_end) {
			printReading(*rter3, output);
		}
		foreach (ReadingList, cohort->deleted, rter2, rter2_end) {
			printReading(*rter2, output);
		}
	}
	if (cohort->text) {
		u_fprintf(output, "%S", cohort->text);
	}

	foreach(CohortVector, cohort->removed, iter, iter_end) {
		if (trace && !trace_no_removed) {
			printCohort(*iter, output);
		}
		else {
			if ((*iter)->text) {
				u_fprintf(output, "%S", (*iter)->text);
			}
		}
	}
}

void GrammarApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {
	if (window->text) {
		u_fprintf(output, "%S", window->text);
	}

	uint32_t cs = (uint32_t)window->cohorts.size();
	for (uint32_t c=1 ; c < cs ; c++) {
		Cohort *cohort = window->cohorts.at(c);
		printCohort(cohort, output);
	}
	u_fputc('\n', output);
	u_fflush(output);
}

}
