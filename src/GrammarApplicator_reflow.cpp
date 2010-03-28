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

Tag *GrammarApplicator::makeBaseFromWord(uint32_t tag) {
	return makeBaseFromWord(single_tags.find(tag)->second);
}

Tag *GrammarApplicator::makeBaseFromWord(Tag *tag) {
	size_t len = u_strlen(tag->tag);
	if (len < 5) {
		return tag;
	}
	UChar *n = new UChar[len-1];
	n[0] = n[len-3] = '"';
	n[len-2] = 0;
	u_strncpy(n+1, tag->tag+2, len-4);
	Tag *nt = addTag(n);
	delete[] n;
	return nt;
}

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
			if (parent->dep_parent == 0 || parent->dep_parent == std::numeric_limits<uint32_t>::max()) {
				retval = false;
				break;
			}
			std::map<uint32_t,Cohort*>::iterator it = gWindow->cohort_map.find(parent->dep_parent);
			if (it != gWindow->cohort_map.end()) {
				parent = it->second;
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

bool GrammarApplicator::wouldParentChildCross(Cohort *parent, Cohort *child) {
	uint32_t mn = std::min(parent->global_number, child->global_number);
	uint32_t mx = std::max(parent->global_number, child->global_number);

	for (uint32_t i = mn+1 ; i<mx ; ++i) {
		std::map<uint32_t,Cohort*>::iterator it = gWindow->cohort_map.find(parent->dep_parent);
		if (it != gWindow->cohort_map.end() && it->second->dep_parent != std::numeric_limits<uint32_t>::max()) {
			if (it->second->dep_parent < mn || it->second->dep_parent > mx) {
				return true;
			}
		}
	}

	return false;
}

bool GrammarApplicator::attachParentChild(Cohort &parent, Cohort &child, bool allowloop, bool allowcrossing) {
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

	if (!allowcrossing && dep_block_crossing && wouldParentChildCross(&parent, &child)) {
		if (verbosity_level > 0) {
			u_fprintf(
				ux_stderr,
				"Warning: Dependency between %u and %u would cause crossing branches. Will not attach them.\n",
				child.global_number, parent.global_number
				);
		}
		return false;
	}

	if (child.dep_parent == std::numeric_limits<uint32_t>::max()) {
		child.dep_parent = child.dep_self;
	}
	std::map<uint32_t,Cohort*>::iterator it = gWindow->cohort_map.find(child.dep_parent);
	if (it != gWindow->cohort_map.end()) {
		it->second->remChild(child.dep_self);
	}

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
		// Turns out g++ evaluates left side of = first, and MSVC++ does right side first, so g++ accessed its own newly created [0] at .begin()
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
			if (cohort->dep_parent == std::numeric_limits<uint32_t>::max()) {
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
					cohort->dep_parent = std::numeric_limits<uint32_t>::max();
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
	reading.mapping = 0;

	if (grammar->sets_any && !grammar->sets_any->empty()) {
		reading.parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
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
			tmp.findAndReplace(stringbits[S_VS1+i].getTerminatedBuffer(), regexgrps[1+i]);
		}
		// Handle %U %u %L %l markers.
		bool found;
		do {
			found = false;
			int32_t pos = -1, mpos = -1;
			if ((pos = tmp.lastIndexOf(stringbits[S_VSu].getTerminatedBuffer(), stringbits[S_VSu].length(), 0)) != -1) {
				found = true;
				mpos = std::max(mpos, pos);
			}
			if ((pos = tmp.lastIndexOf(stringbits[S_VSU].getTerminatedBuffer(), stringbits[S_VSU].length(), mpos)) != -1) {
				found = true;
				mpos = std::max(mpos, pos);
			}
			if ((pos = tmp.lastIndexOf(stringbits[S_VSl].getTerminatedBuffer(), stringbits[S_VSl].length(), mpos)) != -1) {
				found = true;
				mpos = std::max(mpos, pos);
			}
			if ((pos = tmp.lastIndexOf(stringbits[S_VSL].getTerminatedBuffer(), stringbits[S_VSL].length(), mpos)) != -1) {
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

	uint32HashSetuint32HashMap::const_iterator it = grammar->sets_by_tag.find(utag);
	if (it != grammar->sets_by_tag.end()) {
		reading.parent->possible_sets.insert(it->second.begin(), it->second.end());
	}
	if (ordered || reading.tags.find(utag) == reading.tags.end()) {
		reading.tags.insert(utag);
		reading.tags_list.push_back(utag);
		reading.tags_bloom.insert(utag);
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
		reading.tags_textual_bloom.insert(utag);
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
			reading.parent->dep_parent = std::numeric_limits<uint32_t>::max();
		}
		has_dep = true;
	}
	if (!tag->is_special) {
		reading.tags_plain.insert(utag);
		reading.tags_plain_bloom.insert(utag);
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
		foreach (ReadingList, cohort.readings, itr, itr_end) {
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
		Reading *nr = new Reading(reading);
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
	std::map<uint32_t, ReadingList> mlist;
	foreach (ReadingList, cohort.readings, iter, iter_end) {
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

	std::map<uint32_t, ReadingList>::iterator miter;
	for (miter = mlist.begin() ; miter != mlist.end() ; miter++) {
		ReadingList clist = miter->second;
		Reading *nr = new Reading(*(clist.front()));
		if (nr->mapping) {
			nr->tags_list.remove(nr->mapping->hash);
		}
		foreach (ReadingList, clist, iter1, iter1_end) {
			if ((*iter1)->mapping && std::find(nr->tags_list.begin(), nr->tags_list.end(), (*iter1)->mapping->hash) == nr->tags_list.end()) {
				nr->tags_list.push_back((*iter1)->mapping->hash);
			}
			delete (*iter1);
		}
		order.push_back(nr);
	}

	std::sort(order.begin(), order.end(), CG3::Reading::cmp_number);
	cohort.readings.insert(cohort.readings.begin(), order.begin(), order.end());
}

void GrammarApplicator::rebuildCohortLinks() {
	SingleWindow *sWindow = 0;
	if (!gWindow->previous.empty()) {
		sWindow = gWindow->previous.front();
	}
	else if (gWindow->current) {
		sWindow = gWindow->current;
	}
	else if (!gWindow->next.empty()) {
		sWindow = gWindow->next.front();
	}

	Cohort *prev = 0;
	while (sWindow) {
		foreach (CohortVector, sWindow->cohorts, citer, citer_end) {
			(*citer)->prev = prev;
			(*citer)->next = 0;
			if (prev) {
				prev->next = *citer;
			}
			prev = *citer;
		}
		sWindow = sWindow->next;
	}
}

}
