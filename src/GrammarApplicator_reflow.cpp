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
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"

using namespace CG3;
using namespace CG3::Strings;

bool GrammarApplicator::wouldParentChildLoop(Cohort *parent, Cohort *child) {
	bool retval = false;
	int i = 0;

	if (parent->global_number == child->global_number) {
		retval = true;
	}
	else if (parent->global_number == child->dep_parent) {
		retval = false;
	}
	else if (parent->global_number == parent->dep_parent) {
		retval = false;
	}
	else if (parent->dep_parent == child->global_number) {
		retval = true;
	}
	else {
		for (;i<1000;i++) {
			if (parent->dep_parent == 0 || parent->dep_parent == UINT_MAX) {
				retval = false;
				break;
			}
			if (gWindow->cohort_map.find(parent->dep_parent) != gWindow->cohort_map.end()) {
				parent = gWindow->cohort_map.find(parent->dep_parent)->second;
			}
			else {
				break;
			}
			if (parent->dep_parent == child->global_number) {
				retval = true;
				break;
			}
		}
		if (i == 1000) {
			if (verbosity_level > 0) {
				u_fprintf(
					ux_stderr,
					"Warning: While testing whether %u and %u would loop the counter exceeded 1000 indicating a loop higher up in the tree.\n",
					child->global_number, parent->global_number
					);
			}
		}
	}
	return retval;
}

bool GrammarApplicator::attachParentChild(Cohort &parent, Cohort &child, bool allowloop) {
	parent.dep_self = parent.global_number;
	child.dep_self = child.global_number;

	if (!allowloop && dep_block_loops && wouldParentChildLoop(&parent, &child)) {
		if (verbosity_level > 0) {
			u_fprintf(
				ux_stderr,
				"Warning: Dependency between %u and %u would cause a loop. Will not attach them.\n",
				child.global_number, parent.global_number
				);
		}
		return false;
	}

	if (child.dep_parent == UINT_MAX) {
		child.dep_parent = child.dep_self;
	}
	gWindow->cohort_map.find(child.dep_parent)->second->remChild(child.dep_self);

	child.dep_parent = parent.global_number;
	parent.addChild(child.global_number);

	parent.dep_done = true;
	child.dep_done = true;

	if (!dep_has_spanned && child.parent != parent.parent) {
		u_fprintf(
			ux_stderr,
			"Info: Dependency between %u and %u spans the window boundaries. Enumeration will be global from here on.\n",
			child.global_number, parent.global_number
			);
		dep_has_spanned = true;
	}
	return true;
}

void GrammarApplicator::reflowDependencyWindow(uint32_t max) {
	bool did_dep = false;
	if (gWindow->dep_window.empty()) {
		gWindow->dep_window[0] = gWindow->current->cohorts.at(0);
	}
	else if (gWindow->dep_window.find(0) == gWindow->dep_window.end()) {
		// This has to be done in 2 steps or it will segfault on Linux for some reason...
		Cohort *tmp = gWindow->dep_window.begin()->second->parent->cohorts.at(0);
		gWindow->dep_window[0] = tmp;
	}
	if (gWindow->cohort_map.empty()) {
		gWindow->cohort_map[0] = gWindow->current->cohorts.at(0);
	}
	else if (gWindow->cohort_map.find(0) == gWindow->cohort_map.end()) {
		Cohort *tmp = gWindow->cohort_map.begin()->second->parent->cohorts.at(0);
		gWindow->cohort_map[0] = tmp;
	}

	std::map<uint32_t, Cohort*>::iterator dIter;
	for (dIter = gWindow->dep_window.begin() ; dIter != gWindow->dep_window.end() ; dIter++) {
		Cohort *cohort = dIter->second;
		if (cohort->dep_done) {
			continue;
		}
		if (max && cohort->global_number >= max) {
			break;
		}

		if (cohort->dep_self) {
			did_dep = true;
			if (gWindow->dep_map.find(cohort->dep_self) == gWindow->dep_map.end()) {
				gWindow->dep_map[cohort->dep_self] = cohort->global_number;
				cohort->dep_self = cohort->global_number;
			}
		}
	}

	if (did_dep) {
		gWindow->dep_map[0] = 0;
		for (dIter = gWindow->dep_window.begin() ; dIter != gWindow->dep_window.end() ; dIter++) {
			Cohort *cohort = dIter->second;
			if (max && cohort->global_number >= max) {
				break;
			}
			if (cohort->dep_parent == UINT_MAX) {
				continue;
			}
			if (cohort->dep_self == cohort->global_number) {
				if (!cohort->dep_done && gWindow->dep_map.find(cohort->dep_parent) == gWindow->dep_map.end()) {
					if (verbosity_level > 0) {
						u_fprintf(
							ux_stderr,
							"Warning: Parent %u of dep %u in cohort %u of window %u does not exist - ignoring.\n",
							cohort->dep_parent, cohort->dep_self, cohort->local_number, cohort->parent->number
							);
						u_fflush(ux_stderr);
					}
					cohort->dep_parent = UINT_MAX;
				}
				else {
					if (!cohort->dep_done) {
						uint32_t dep_real = gWindow->dep_map.find(cohort->dep_parent)->second;
						cohort->dep_parent = dep_real;
					}
					std::map<uint32_t, Cohort*>::iterator tmp = gWindow->cohort_map.find(cohort->dep_parent);
					if (tmp != gWindow->cohort_map.end()) {
						tmp->second->addChild(cohort->dep_self);
					}
					cohort->dep_done = true;
				}
			}
		}
	}
}

void GrammarApplicator::reflowReading(Reading &reading) {
	reading.tags.clear();
	reading.tags_plain.clear();
	reading.tags_textual.clear();
	reading.tags_numerical.clear();
	reading.possible_sets.clear();
	reading.mapping = 0;

	if (grammar->sets_any && !grammar->sets_any->empty()) {
		reading.parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
		reading.possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
	}

	const_foreach (uint32List, reading.tags_list, tter, tter_end) {
		addTagToReading(reading, *tter, false);
	}

	reading.rehash();
}

void GrammarApplicator::addTagToReading(Reading &reading, uint32_t utag, bool rehash) {
	Tag *tag = single_tags.find(utag)->second;

	if (tag->type & T_VARSTRING && !regexgrps.empty()) {
		UnicodeString tmp(tag->tag);
		// Replace $1-$9 with their respective match groups
		for (size_t i=0 ; i<regexgrps.size()-1 ; ++i) {
			tmp.findAndReplace(stringbits[S_VS1+i], regexgrps[1+i]);
		}
		// Handle %U %u %L %l markers.
		bool found;
		do {
			found = false;
			int32_t pos = -1, mpos = -1;
			if ((pos = tmp.lastIndexOf(stringbits[S_VSu], stringbit_lengths[S_VSu], 0)) != -1) {
				found = true;
				mpos = std::max(mpos, pos);
			}
			if ((pos = tmp.lastIndexOf(stringbits[S_VSU], stringbit_lengths[S_VSU], mpos)) != -1) {
				found = true;
				mpos = std::max(mpos, pos);
			}
			if ((pos = tmp.lastIndexOf(stringbits[S_VSl], stringbit_lengths[S_VSl], mpos)) != -1) {
				found = true;
				mpos = std::max(mpos, pos);
			}
			if ((pos = tmp.lastIndexOf(stringbits[S_VSL], stringbit_lengths[S_VSL], mpos)) != -1) {
				found = true;
				mpos = std::max(mpos, pos);
			}
			if (found && mpos != -1) {
				UChar mode = tmp[mpos+1];
				tmp.remove(mpos, 2);
				if (mode == 'u') {
					UnicodeString range(tmp, mpos, 1);
					range.toUpper();
					tmp.setCharAt(mpos, range[0]);
				}
				else if (mode == 'U') {
					UnicodeString range(tmp, mpos);
					range.toUpper();
					tmp.truncate(mpos);
					tmp.append(range);
				}
				else if (mode == 'l') {
					UnicodeString range(tmp, mpos, 1);
					range.toLower();
					tmp.setCharAt(mpos, range[0]);
				}
				else if (mode == 'L') {
					UnicodeString range(tmp, mpos);
					range.toLower();
					tmp.truncate(mpos);
					tmp.append(range);
				}
			}
		} while (found);
		const UChar *nt = tmp.getTerminatedBuffer();
		tag = addTag(nt);
		utag = tag->hash;
	}

	if (grammar->sets_by_tag.find(utag) != grammar->sets_by_tag.end()) {
		reading.parent->possible_sets.insert(grammar->sets_by_tag.find(utag)->second->begin(), grammar->sets_by_tag.find(utag)->second->end());
		reading.possible_sets.insert(grammar->sets_by_tag.find(utag)->second->begin(), grammar->sets_by_tag.find(utag)->second->end());
	}
	if (reading.tags.find(utag) == reading.tags.end()) {
		reading.tags.insert(utag);
		reading.tags_list.push_back(utag);
	}
	if (grammar->parentheses.find(utag) != grammar->parentheses.end()) {
		reading.parent->is_pleft = utag;
	}
	if (grammar->parentheses_reverse.find(utag) != grammar->parentheses_reverse.end()) {
		reading.parent->is_pright = utag;
	}

	if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
		if (reading.mapping && reading.mapping != tag) {
			u_fprintf(ux_stderr, "Error: addTagToReading() cannot add a mapping tag to a reading which already is mapped!\n");
			CG3Quit(1);
		}
		reading.mapping = tag;
	}
	if (tag->type & (T_TEXTUAL|T_WORDFORM|T_BASEFORM)) {
		reading.tags_textual.insert(utag);
	}
	if (tag->type & T_NUMERICAL) {
		reading.tags_numerical[utag] = tag;
		reading.parent->num_is_current = false;
	}
	if (!reading.baseform && tag->type & T_BASEFORM) {
		reading.baseform = tag->hash;
	}
	if (!reading.wordform && tag->type & T_WORDFORM) {
		reading.wordform = tag->hash;
	}
	if (grammar->has_dep && tag->type & T_DEPENDENCY && !reading.parent->dep_done) {
		reading.parent->dep_self = tag->dep_self;
		reading.parent->dep_parent = tag->dep_parent;
		if (tag->dep_parent == tag->dep_self) {
			reading.parent->dep_parent = UINT_MAX;
		}
		has_dep = true;
	}
	if (!tag->is_special) {
		reading.tags_plain.insert(utag);
	}
	if (rehash) {
		reading.rehash();
	}
}

void GrammarApplicator::delTagFromReading(Reading &reading, uint32_t utag) {
	reading.tags_list.remove(utag);
	reading.tags.erase(utag);
	reading.tags_textual.erase(utag);
	reading.tags_numerical.erase(utag);
	reading.tags_plain.erase(utag);
	if (utag == reading.mapping->hash) {
		reading.mapping = 0;
	}
	reading.rehash();
	reading.parent->num_is_current = false;
}

void GrammarApplicator::splitMappings(TagList mappings, Cohort &cohort, Reading &reading, bool mapped) {
	if (reading.mapping) {
		mappings.push_back(reading.mapping);
		delTagFromReading(reading, reading.mapping->hash);
	}
	Tag *tag = mappings.back();
	mappings.pop_back();
	foreach (TagList, mappings, ttag, ttag_end) {
		// To avoid duplicating needlessly many times, check for a similar reading in the cohort that's already got this mapping
		bool found = false;
		foreach (std::list<Reading*>, cohort.readings, itr, itr_end) {
			if ((*itr)->hash_plain == reading.hash_plain
				&& (*itr)->mapping
				&& (*itr)->mapping->hash == (*ttag)->hash
				) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		}
		Reading *nr = new Reading(&cohort);
		nr->duplicateFrom(reading);
		nr->mapped = mapped;
		addTagToReading(*nr, (*ttag)->hash);
		nr->mapping = *ttag;
		cohort.appendReading(nr);
		numReadings++;
	}
	reading.mapped = mapped;
	addTagToReading(reading, tag->hash);
	reading.mapping = tag;
}

void GrammarApplicator::mergeMappings(Cohort &cohort) {
	std::map<uint32_t, std::list<Reading*> > mlist;
	foreach (std::list<Reading*>, cohort.readings, iter, iter_end) {
		Reading *r = *iter;
		uint32_t hp = r->hash_plain;
		if (trace) {
			foreach (uint32Vector, r->hit_by, iter_hb, iter_hb_end) {
				hp = hash_sdbm_uint32_t(*iter_hb, hp);
			}
		}
		mlist[hp].push_back(r);
	}

	if (mlist.size() == cohort.readings.size()) {
		return;
	}

	cohort.readings.clear();
	std::vector<Reading*> order;

	std::map<uint32_t, std::list<Reading*> >::iterator miter;
	for (miter = mlist.begin() ; miter != mlist.end() ; miter++) {
		std::list<Reading*> clist = miter->second;
		Reading *nr = new Reading(&cohort);
		nr->duplicateFrom(*(clist.front()));
		if (nr->mapping) {
			nr->tags_list.remove(nr->mapping->hash);
		}
		foreach (std::list<Reading*>, clist, iter1, iter1_end) {
			if ((*iter1)->mapping) {
				nr->tags_list.push_back((*iter1)->mapping->hash);
			}
			delete (*iter1);
		}
		order.push_back(nr);
	}

	std::sort(order.begin(), order.end(), CG3::Reading::cmp_number);
	cohort.readings.insert(cohort.readings.begin(), order.begin(), order.end());
}
