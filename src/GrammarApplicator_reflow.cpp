/*
* Copyright (C) 2007-2015, GrammarSoft ApS
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

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

Tag *GrammarApplicator::makeBaseFromWord(uint32_t tag) {
	return makeBaseFromWord(single_tags.find(tag)->second);
}

Tag *GrammarApplicator::makeBaseFromWord(Tag *tag) {
	const size_t len = tag->tag.length();
	if (len < 5) {
		return tag;
	}
	UChar *n = new UChar[len-1];
	n[0] = n[len-3] = '"';
	n[len-2] = 0;
	u_strncpy(n+1, tag->tag.c_str()+2, len-4);
	Tag *nt = addTag(n);
	delete[] n;
	return nt;
}

bool GrammarApplicator::isChildOf(const Cohort *child, const Cohort *parent) {
	bool retval = false;

	if (parent->global_number == child->global_number) {
		retval = true;
	}
	else if (parent->global_number == child->dep_parent) {
		retval = true;
	}
	else {
		int i = 0;
		for (const Cohort *inner = child ; i<1000;i++) {
			if (inner->dep_parent == 0 || inner->dep_parent == std::numeric_limits<uint32_t>::max()) {
				retval = false;
				break;
			}
			std::map<uint32_t,Cohort*>::iterator it = gWindow->cohort_map.find(inner->dep_parent);
			if (it != gWindow->cohort_map.end()) {
				inner = it->second;
			}
			else {
				break;
			}
			if (inner->dep_parent == parent->global_number) {
				retval = true;
				break;
			}
		}
		if (i == 1000) {
			if (verbosity_level > 0) {
				u_fprintf(
					ux_stderr,
					"Warning: While testing whether %u is a child of %u the counter exceeded 1000 indicating a loop higher up in the tree.\n",
					child->global_number, parent->global_number
					);
			}
		}
	}
	return retval;
}

bool GrammarApplicator::wouldParentChildLoop(const Cohort *parent, const Cohort *child) {
	bool retval = false;

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
		int i = 0;
		for (const Cohort *inner = parent ;i<1000;i++) {
			if (inner->dep_parent == 0 || inner->dep_parent == std::numeric_limits<uint32_t>::max()) {
				retval = false;
				break;
			}
			std::map<uint32_t,Cohort*>::iterator it = gWindow->cohort_map.find(inner->dep_parent);
			if (it != gWindow->cohort_map.end()) {
				inner = it->second;
			}
			else {
				break;
			}
			if (inner->dep_parent == child->global_number) {
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

bool GrammarApplicator::wouldParentChildCross(const Cohort *parent, const Cohort *child) {
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

bool GrammarApplicator::attachParentChild(Cohort& parent, Cohort& child, bool allowloop, bool allowcrossing) {
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

	parent.type |= CT_DEP_DONE;
	child.type |= CT_DEP_DONE;

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
	if (dep_delimit && !max && !input_eof && !gWindow->next.empty() && gWindow->next.back()->cohorts.size() > 1) {
		max = gWindow->next.back()->cohorts[1]->global_number;
	}

	if (gWindow->dep_window.empty()) {
		gWindow->dep_window[0] = gWindow->current->cohorts[0];
	}
	else if (gWindow->dep_window.find(0) == gWindow->dep_window.end()) {
		// This has to be done in 2 steps or it will segfault on Linux for some reason...
		// Turns out g++ evaluates left side of = first, and MSVC++ does right side first, so g++ accessed its own newly created [0] at .begin()
		Cohort *tmp = gWindow->dep_window.begin()->second->parent->cohorts[0];
		gWindow->dep_window[0] = tmp;
	}
	if (gWindow->cohort_map.empty()) {
		gWindow->cohort_map[0] = gWindow->current->cohorts[0];
	}
	else if (gWindow->cohort_map.find(0) == gWindow->cohort_map.end()) {
		Cohort *tmp = gWindow->current->cohorts[0];
		Cohort *c = gWindow->cohort_map.begin()->second;
		if (c->parent) {
			tmp = c->parent->cohorts[0];
		}
		gWindow->cohort_map[0] = tmp;
	}

	for (BOOST_AUTO(begin, gWindow->dep_window.begin()); begin != gWindow->dep_window.end();) {
		while (begin != gWindow->dep_window.end() && (begin->second->type & CT_DEP_DONE || !begin->second->dep_self)) {
			++begin;
		}
		gWindow->dep_map.clear();

		BOOST_AUTO(end, begin);
		for (; end != gWindow->dep_window.end(); ++end) {
			Cohort *cohort = end->second;
			if (cohort->type & CT_DEP_DONE) {
				continue;
			}
			if (!cohort->dep_self) {
				continue;
			}
			if (max && cohort->global_number >= max) {
				break;
			}
			if (gWindow->dep_map.find(cohort->dep_self) != gWindow->dep_map.end()) {
				break;
			}
			gWindow->dep_map[cohort->dep_self] = cohort->global_number;
			cohort->dep_self = cohort->global_number;
		}

		if (gWindow->dep_map.empty()) {
			break;
		}

		gWindow->dep_map[0] = 0;
		for (; begin != end; ++begin) {
			Cohort *cohort = begin->second;
			if (max && cohort->global_number >= max) {
				break;
			}
			if (cohort->dep_parent == std::numeric_limits<uint32_t>::max()) {
				continue;
			}
			if (cohort->dep_self == cohort->global_number) {
				if (!(cohort->type & CT_DEP_DONE) && gWindow->dep_map.find(cohort->dep_parent) == gWindow->dep_map.end()) {
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
					if (!(cohort->type & CT_DEP_DONE)) {
						uint32_t dep_real = gWindow->dep_map.find(cohort->dep_parent)->second;
						cohort->dep_parent = dep_real;
					}
					std::map<uint32_t, Cohort*>::iterator tmp = gWindow->cohort_map.find(cohort->dep_parent);
					if (tmp != gWindow->cohort_map.end()) {
						tmp->second->addChild(cohort->dep_self);
					}
					cohort->type |= CT_DEP_DONE;
				}
			}
		}
	}
}

void GrammarApplicator::reflowRelationWindow(uint32_t max) {
	if (!max && !input_eof && !gWindow->next.empty() && gWindow->next.back()->cohorts.size() > 1) {
		max = gWindow->next.back()->cohorts[1]->global_number;
	}

	Cohort *cohort = gWindow->current->cohorts[1];
	while (cohort->prev) {
		cohort = cohort->prev;
	}

	for ( ; cohort ; cohort = cohort->next) {
		if (max && cohort->global_number >= max) {
			break;
		}

		if (!cohort->relations_input.empty()) {
			for (RelationCtn::iterator rel = cohort->relations_input.begin() ; rel != cohort->relations_input.end() ; ) {
				uint32SortedVector newrel;

				boost_foreach (uint32_t target, rel->second) {
					uint32FlatHashMap::iterator it = gWindow->relation_map.find(target);
					if (it != gWindow->relation_map.end()) {
						cohort->relations[rel->first].insert(it->second);
					}
					else {
						newrel.insert(target);
					}
				}

				if (newrel.empty()) {
					cohort->relations_input.erase(rel++);
				}
				else {
					rel->second = newrel;
					++rel;
				}
			}
		}
	}
}

void GrammarApplicator::reflowReading(Reading& reading) {
	reading.tags.clear();
	reading.tags_plain.clear();
	reading.tags_textual.clear();
	reading.tags_numerical.clear();
	reading.tags_bloom.clear();
	reading.tags_textual_bloom.clear();
	reading.tags_plain_bloom.clear();
	reading.mapping = 0;

	insert_if_exists(reading.parent->possible_sets, grammar->sets_any);

	Reading::tags_list_t tlist;
	tlist.swap(reading.tags_list);

	const_foreach (Reading::tags_list_t, tlist, tter, tter_end) {
		addTagToReading(reading, *tter, false);
	}

	reading.rehash();
}

Tag *GrammarApplicator::generateVarstringTag(const Tag *tag) {
	UnicodeString tmp(tag->tag.c_str(), tag->tag.length());
	bool did_something = false;

	// Replace unified sets with their matching tags
	if (tag->vs_sets) {
		for (size_t i=0 ; i<tag->vs_sets->size() ; ++i) {
			TagList tags = getTagList(*(*tag->vs_sets)[i]);
			UString rpl;
			// If there are multiple tags, such as from CompositeTags, put _ between them
			const_foreach (TagList, tags, iter, iter_end) {
				rpl += (*iter)->tag;
				if (std::distance(iter, iter_end) > 1) {
					rpl += '_';
				}
			}
			tmp.findAndReplace((*tag->vs_names)[i].c_str(), rpl.c_str());
			did_something = true;
		}
	}

	// Replace $1-$9 with their respective match groups
	for (size_t i = 0; i<regexgrps->first && i<9; ++i) {
		tmp.findAndReplace(stringbits[S_VS1+i], (*regexgrps->second)[i]);
		did_something = true;
	}

	// Handle %U %u %L %l markers.
	// ToDo: Split %[UuLl] markers from the main string to avoid having combined %$1 accidentally match %L
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
			did_something = true;
		}
	} while (found);

	if (tag->type & T_CASE_INSENSITIVE) {
		tmp += 'i';
	}
	if (tag->type & T_REGEXP) {
		tmp += 'r';
	}

	const UChar *nt = tmp.getTerminatedBuffer();
	if (!did_something && u_strcmp(nt, tag->tag.c_str()) == 0) {
		u_fprintf(ux_stderr, "Warning: Unable to generate from tag '%S'! Possibly missing KEEPORDER and/or capturing regex from grammar on line %u before input line %u.\n", tag->tag.c_str(), grammar->lines, numLines);
		u_fflush(ux_stderr);
	}
	return addTag(nt, true);
}

uint32_t GrammarApplicator::addTagToReading(Reading& reading, uint32_t utag, bool rehash) {
	Tag *tag = single_tags.find(utag)->second;
	return addTagToReading(reading, tag, rehash);
}

uint32_t GrammarApplicator::addTagToReading(Reading& reading, Tag *tag, bool rehash) {
	if (tag->type & T_VARSTRING) {
		tag = generateVarstringTag(tag);
	}

	Grammar::sets_by_tag_t::const_iterator it = grammar->sets_by_tag.find(tag->hash);
	if (it != grammar->sets_by_tag.end()) {
		reading.parent->possible_sets.resize(std::max(reading.parent->possible_sets.size(), it->second.size()));
		reading.parent->possible_sets |= it->second;
	}
	reading.tags.insert(tag->hash);
	reading.tags_list.push_back(tag->hash);
	reading.tags_bloom.insert(tag->hash);
	if (grammar->parentheses.find(tag->hash) != grammar->parentheses.end()) {
		reading.parent->is_pleft = tag->hash;
	}
	if (grammar->parentheses_reverse.find(tag->hash) != grammar->parentheses_reverse.end()) {
		reading.parent->is_pright = tag->hash;
	}

	if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
		if (reading.mapping && reading.mapping != tag) {
			u_fprintf(ux_stderr, "Error: addTagToReading() cannot add a mapping tag to a reading which already is mapped!\n");
			CG3Quit(1);
		}
		reading.mapping = tag;
	}
	if (tag->type & (T_TEXTUAL|T_WORDFORM|T_BASEFORM)) {
		reading.tags_textual.insert(tag->hash);
		reading.tags_textual_bloom.insert(tag->hash);
	}
	if (tag->type & T_NUMERICAL) {
		reading.tags_numerical[tag->hash] = tag;
		reading.parent->type &= ~CT_NUM_CURRENT;
	}
	if (!reading.baseform && (tag->type & T_BASEFORM)) {
		reading.baseform = tag->hash;
	}
	if (grammar->has_dep && (tag->type & T_DEPENDENCY) && !(reading.parent->type & CT_DEP_DONE)) {
		reading.parent->dep_self = tag->dep_self;
		reading.parent->dep_parent = tag->dep_parent;
		if (tag->dep_parent == tag->dep_self) {
			reading.parent->dep_parent = std::numeric_limits<uint32_t>::max();
		}
		has_dep = true;
	}
	if (grammar->has_relations && (tag->type & T_RELATION)) {
		if (tag->dep_parent && tag->comparison_hash) {
			reading.parent->relations_input[tag->comparison_hash].insert(tag->dep_parent);
		}
		if (tag->dep_self) {
			gWindow->relation_map[tag->dep_self] = reading.parent->global_number;
		}
		has_relations = true;
		reading.parent->type |= CT_RELATED;
	}
	if (!(tag->type & T_SPECIAL)) {
		reading.tags_plain.insert(tag->hash);
		reading.tags_plain_bloom.insert(tag->hash);
	}
	if (rehash) {
		reading.rehash();
	}
	return tag->hash;
}

void GrammarApplicator::delTagFromReading(Reading& reading, uint32_t utag) {
	erase(reading.tags_list, utag);
	reading.tags.erase(utag);
	reading.tags_textual.erase(utag);
	reading.tags_numerical.erase(utag);
	reading.tags_plain.erase(utag);
	if (reading.mapping && utag == reading.mapping->hash) {
		reading.mapping = 0;
	}
	if (utag == reading.baseform) {
		reading.baseform = 0;
	}
	reading.rehash();
	reading.parent->type &= ~CT_NUM_CURRENT;
}

void GrammarApplicator::delTagFromReading(Reading& reading, Tag *tag) {
	return delTagFromReading(reading, tag->hash);
}

bool GrammarApplicator::unmapReading(Reading& reading, const uint32_t rule) {
	bool readings_changed = false;
	if (reading.mapping) {
		reading.noprint = false;
		delTagFromReading(reading, reading.mapping->hash);
		readings_changed = true;
	}
	if (reading.mapped) {
		reading.mapped = false;
		readings_changed = true;
	}
	if (readings_changed) {
		reading.hit_by.push_back(rule);
	}
	return readings_changed;
}

void GrammarApplicator::splitMappings(TagList& mappings, Cohort& cohort, Reading& reading, bool mapped) {
	for (TagList::iterator it = mappings.begin() ; it != mappings.end() ;) {
		Tag *& tag = *it;
		while (tag->type & T_VARSTRING) {
			tag = generateVarstringTag(tag);
		}
		if (!(tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix)) {
			addTagToReading(reading, tag);
			it = mappings.erase(it);
		}
		else {
			++it;
		}
	}

	if (reading.mapping) {
		mappings.push_back(reading.mapping);
		delTagFromReading(reading, reading.mapping->hash);
	}

	Tag *tag = mappings.back();
	mappings.pop_back();
	size_t i = mappings.size();
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
		Reading *nr = alloc_reading(reading);
		nr->mapped = mapped;
		nr->number = reading.number - i--;
		uint32_t mp = addTagToReading(*nr, *ttag);
		if (mp != (*ttag)->hash) {
			nr->mapping = single_tags.find(mp)->second;
		}
		else {
			nr->mapping = *ttag;
		}
		cohort.appendReading(nr);
		numReadings++;
	}

	reading.mapped = mapped;
	uint32_t mp = addTagToReading(reading, tag);
	if (mp != tag->hash) {
		reading.mapping = single_tags.find(mp)->second;
	}
	else {
		reading.mapping = tag;
	}
}

void GrammarApplicator::splitAllMappings(all_mappings_t& all_mappings, Cohort& cohort, bool mapped) {
	if (all_mappings.empty()) {
		return;
	}
	ReadingList readings = cohort.readings;
	boost_foreach (Reading *reading, readings) {
		BOOST_AUTO(iter, all_mappings.find(reading));
		if (iter == all_mappings.end()) {
			continue;
		}
		splitMappings(iter->second, cohort, *reading, mapped);
	}
	std::sort(cohort.readings.begin(), cohort.readings.end(), CG3::Reading::cmp_number);
	if (!grammar->reopen_mappings.empty()) {
		boost_foreach (Reading *reading, cohort.readings) {
			if (reading->mapping && grammar->reopen_mappings.count(reading->mapping->hash)) {
				reading->mapped = false;
			}
		}
	}
	all_mappings.clear();
}

void GrammarApplicator::mergeReadings(ReadingList& readings) {
	bc::flat_map<uint32_t, std::pair<uint32_t,Reading*> > mapped;
	mapped.reserve(readings.size());
	bc::flat_map<uint32_t, ReadingList> mlist;
	mlist.reserve(readings.size());
	foreach (ReadingList, readings, iter, iter_end) {
		Reading *r = *iter;
		uint32_t hp = r->hash_plain, hplain = r->hash_plain;
		uint32_t nm = 0;
		if (trace) {
			foreach (uint32Vector, r->hit_by, iter_hb, iter_hb_end) {
				hp = hash_value(*iter_hb, hp);
			}
		}
		if (r->mapping) {
			++nm;
		}
		Reading *sub = r->next;
		while (sub) {
			hp = hash_value(sub->hash_plain, hp);
			hplain = hash_value(sub->hash_plain, hplain);
			if (trace) {
				foreach (uint32Vector, sub->hit_by, iter_hb, iter_hb_end) {
					hp = hash_value(*iter_hb, hp);
				}
			}
			if (sub->mapping) {
				++nm;
			}
			sub = sub->next;
		}
		if (mapped.count(hplain)) {
			if (mapped[hplain].first != 0 && nm == 0) {
				r->deleted = true;
			}
			else if (mapped[hplain].first != nm && mapped[hplain].first == 0) {
				mapped[hplain].second->deleted = true;
			}
		}
		mapped[hplain] = std::make_pair(nm, r);
		mlist[hp+nm].push_back(r);
	}

	if (mlist.size() == readings.size()) {
		return;
	}

	readings.clear();
	std::vector<Reading*> order;

	for (BOOST_AUTO(miter, mlist.begin()) ; miter != mlist.end() ; miter++) {
		ReadingList clist = miter->second;
		Reading *nr = alloc_reading(*(clist.front()));
		if (nr->mapping) {
			erase(nr->tags_list, nr->mapping->hash);
		}
		foreach (ReadingList, clist, iter1, iter1_end) {
			if ((*iter1)->mapping && std::find(nr->tags_list.begin(), nr->tags_list.end(), (*iter1)->mapping->hash) == nr->tags_list.end()) {
				nr->tags_list.push_back((*iter1)->mapping->hash);
			}
			free_reading(*iter1);
		}
		order.push_back(nr);
	}

	std::sort(order.begin(), order.end(), CG3::Reading::cmp_number);
	readings.insert(readings.begin(), order.begin(), order.end());
}

void GrammarApplicator::mergeMappings(Cohort& cohort) {
	mergeReadings(cohort.readings);
	if (trace) {
		mergeReadings(cohort.deleted);
		mergeReadings(cohort.delayed);
	}
}

Cohort *GrammarApplicator::delimitAt(SingleWindow& current, Cohort *cohort) {
	SingleWindow *nwin = 0;
	if (current.parent->current == &current) {
		nwin = current.parent->allocPushSingleWindow();
	}
	else {
		foreach (SingleWindowCont, current.parent->next, iter, iter_end) {
			if (*iter == &current) {
				nwin = current.parent->allocSingleWindow();
				current.parent->next.insert(++iter, nwin);
				break;
			}
		}
		if (!nwin) {
			foreach (SingleWindowCont, current.parent->previous, iter, iter_end) {
				if (*iter == &current) {
					nwin = current.parent->allocSingleWindow();
					current.parent->previous.insert(iter, nwin);
					break;
				}
			}
		}
		gWindow->rebuildSingleWindowLinks();
	}

	assert(nwin != 0);

	nwin->has_enclosures = current.has_enclosures;

	current.parent->cohort_counter++;
	Cohort *cCohort = alloc_cohort(nwin);
	cCohort->global_number = 0;
	cCohort->wordform = tag_begin;

	Reading *cReading = alloc_reading(cCohort);
	cReading->baseform = begintag;
	insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
	addTagToReading(*cReading, begintag);

	cCohort->appendReading(cReading);

	nwin->appendCohort(cCohort);

	uint32_t c = cohort->local_number;
	size_t nc = c+1;
	for ( ; nc < current.cohorts.size() ; nc++) {
		current.cohorts[nc]->parent = nwin;
		nwin->appendCohort(current.cohorts[nc]);
	}
	c = current.cohorts.size()-c;
	for (nc = 0 ; nc < c-1 ; nc++) {
		current.cohorts.pop_back();
	}

	cohort = current.cohorts.back();
	foreach (ReadingList, cohort->readings, rter3, rter3_end) {
		Reading *reading = *rter3;
		addTagToReading(*reading, endtag);
	}
	gWindow->rebuildCohortLinks();

	return cohort;
}

void GrammarApplicator::reflowTextuals_Reading(Reading& r) {
	if (r.next) {
		reflowTextuals_Reading(*r.next);
	}
	const_foreach (uint32SortedVector, r.tags, it, it_end) {
		Tag *tag = single_tags.find(*it)->second;
		if (tag->type & T_TEXTUAL) {
			r.tags_textual.insert(*it);
			r.tags_textual_bloom.insert(*it);
		}
	}
}

void GrammarApplicator::reflowTextuals_Cohort(Cohort& c) {
	foreach (CohortVector, c.enclosed, it, it_end) {
		reflowTextuals_Cohort(**it);
	}
	foreach (CohortVector, c.removed, it, it_end) {
		reflowTextuals_Cohort(**it);
	}
	foreach (ReadingList, c.readings, it, it_end) {
		reflowTextuals_Reading(**it);
	}
	foreach (ReadingList, c.deleted, it, it_end) {
		reflowTextuals_Reading(**it);
	}
	foreach (ReadingList, c.delayed, it, it_end) {
		reflowTextuals_Reading(**it);
	}
}

void GrammarApplicator::reflowTextuals_SingleWindow(SingleWindow& sw) {
	foreach (CohortVector, sw.cohorts, it, it_end) {
		reflowTextuals_Cohort(**it);
	}
}

void GrammarApplicator::reflowTextuals() {
	foreach (SingleWindowCont, gWindow->previous, swit, swit_end) {
		reflowTextuals_SingleWindow(**swit);
	}
	reflowTextuals_SingleWindow(*gWindow->current);
	foreach (SingleWindowCont, gWindow->next, swit, swit_end) {
		reflowTextuals_SingleWindow(**swit);
	}
}

}
