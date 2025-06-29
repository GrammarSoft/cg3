/*
* Copyright (C) 2007-2025, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

Tag* GrammarApplicator::makeBaseFromWord(uint32_t tag) {
	return makeBaseFromWord(grammar->single_tags.find(tag)->second);
}

Tag* GrammarApplicator::makeBaseFromWord(Tag* tag) {
	const size_t len = tag->tag.size();
	if (len < 4) {
		return tag;
	}
	static thread_local UString n;
	n.clear();
	n.resize(len - 2);
	n[0] = n[len - 3] = '"';
	u_strncpy(&n[1], tag->tag.data() + 2, SI32(len - 4));
	Tag* nt = addTag(n);
	return nt;
}

bool GrammarApplicator::isChildOf(const Cohort* child, const Cohort* parent) {
	bool retval = false;

	if (parent->global_number == child->global_number) {
		retval = true;
	}
	else if (parent->global_number == child->dep_parent) {
		retval = true;
	}
	else {
		size_t i = 0;
		for (const Cohort* inner = child; i < 1000; ++i) {
			if (inner->dep_parent == 0 || inner->dep_parent == DEP_NO_PARENT) {
				retval = false;
				break;
			}
			auto it = gWindow->cohort_map.find(inner->dep_parent);
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
				  child->global_number, parent->global_number);
			}
		}
	}
	return retval;
}

bool GrammarApplicator::wouldParentChildLoop(const Cohort* parent, const Cohort* child) {
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
		size_t i = 0;
		for (const Cohort* inner = parent; i < 1000; ++i) {
			if (inner->dep_parent == 0 || inner->dep_parent == DEP_NO_PARENT) {
				retval = false;
				break;
			}
			auto it = gWindow->cohort_map.find(inner->dep_parent);
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
				  child->global_number, parent->global_number);
			}
		}
	}
	return retval;
}

bool GrammarApplicator::wouldParentChildCross(const Cohort* parent, const Cohort* child) {
	uint32_t mn = std::min(parent->global_number, child->global_number);
	uint32_t mx = std::max(parent->global_number, child->global_number);

	for (uint32_t i = mn + 1; i < mx; ++i) {
		auto it = gWindow->cohort_map.find(parent->dep_parent);
		if (it != gWindow->cohort_map.end() && it->second->dep_parent != DEP_NO_PARENT) {
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
			  child.global_number, parent.global_number);
		}
		return false;
	}

	if (!allowcrossing && dep_block_crossing && wouldParentChildCross(&parent, &child)) {
		if (verbosity_level > 0) {
			u_fprintf(
			  ux_stderr,
			  "Warning: Dependency between %u and %u would cause crossing branches. Will not attach them.\n",
			  child.global_number, parent.global_number);
		}
		return false;
	}

	if (child.dep_parent == DEP_NO_PARENT) {
		child.dep_parent = child.dep_self;
	}
	auto it = gWindow->cohort_map.find(child.dep_parent);
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
		  child.global_number, parent.global_number);
		dep_has_spanned = true;
	}
	return true;
}

void GrammarApplicator::reflowDependencyWindow(uint32_t max) {
	if (dep_delimit && !max && !input_eof && !gWindow->next.empty() && gWindow->next.back()->cohorts.size() > 1) {
		max = gWindow->next.back()->cohorts[1]->global_number;
	}

	if (gWindow->dep_window.empty() || gWindow->dep_window.begin()->second->parent == 0) {
		gWindow->dep_window[0] = gWindow->current->cohorts[0];
	}
	else if (gWindow->dep_window.find(0) == gWindow->dep_window.end()) {
		// This has to be done in 2 steps or it will segfault on Linux for some reason...
		// Turns out g++ evaluates left side of = first, and MSVC++ does right side first, so g++ accessed its own newly created [0] at .begin()
		Cohort* tmp = gWindow->dep_window.begin()->second->parent->cohorts[0];
		gWindow->dep_window[0] = tmp;
	}
	if (gWindow->cohort_map.empty()) {
		gWindow->cohort_map[0] = gWindow->current->cohorts[0];
	}
	else if (gWindow->cohort_map.find(0) == gWindow->cohort_map.end()) {
		Cohort* tmp = gWindow->current->cohorts[0];
		Cohort* c = gWindow->cohort_map.begin()->second;
		if (c->parent) {
			tmp = c->parent->cohorts[0];
		}
		gWindow->cohort_map[0] = tmp;
	}

	for (auto begin = gWindow->dep_window.begin(); begin != gWindow->dep_window.end();) {
		while (begin != gWindow->dep_window.end() && (begin->second->type & CT_DEP_DONE || !begin->second->dep_self)) {
			++begin;
		}
		gWindow->dep_map.clear();

		auto end = begin;
		for (; end != gWindow->dep_window.end(); ++end) {
			Cohort* cohort = end->second;
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
			Cohort* cohort = begin->second;
			if (max && cohort->global_number >= max) {
				break;
			}
			if (cohort->dep_parent == DEP_NO_PARENT) {
				continue;
			}
			if (cohort->dep_self == cohort->global_number) {
				if (!(cohort->type & CT_DEP_DONE) && gWindow->dep_map.find(cohort->dep_parent) == gWindow->dep_map.end()) {
					if (verbosity_level > 0) {
						u_fprintf(
						  ux_stderr,
						  "Warning: Parent %u of dep %u in cohort %u of window %u does not exist - ignoring.\n",
						  cohort->dep_parent, cohort->dep_self, cohort->local_number, cohort->parent->number);
						u_fflush(ux_stderr);
					}
					cohort->dep_parent = DEP_NO_PARENT;
				}
				else {
					if (!(cohort->type & CT_DEP_DONE)) {
						auto dep_real = gWindow->dep_map.find(cohort->dep_parent)->second;
						cohort->dep_parent = dep_real;
					}
					gWindow->cohort_map[0] = cohort->parent->cohorts[0];
					auto tmp = gWindow->cohort_map.find(cohort->dep_parent);
					if (tmp != gWindow->cohort_map.end()) {
						tmp->second->addChild(cohort->dep_self);
					}
					cohort->type |= CT_DEP_DONE;
				}
			}
		}
	}

	gWindow->dep_map.clear();
	gWindow->dep_window.clear();
}

void GrammarApplicator::reflowRelationWindow(uint32_t max) {
	if (!max && !input_eof && !gWindow->next.empty() && gWindow->next.back()->cohorts.size() > 1) {
		max = gWindow->next.back()->cohorts[0]->global_number;
	}

	Cohort* cohort = gWindow->current->cohorts[1];
	while (cohort->prev) {
		cohort = cohort->prev;
	}

	for (; cohort; cohort = cohort->next) {
		if (max && cohort->global_number >= max) {
			break;
		}

		for (auto rel = cohort->relations_input.begin(); rel != cohort->relations_input.end();) {
			auto newrel = ss_u32sv.get();

			for (auto target : rel->second) {
				auto it = gWindow->relation_map.find(target);
				if (it != gWindow->relation_map.end()) {
					cohort->relations[rel->first].insert(it->second);
				}
				else {
					newrel->insert(target);
				}
			}

			// Defer missing relations for later
			if (newrel->empty()) {
				rel = cohort->relations_input.erase(rel);
			}
			else {
				rel->second = newrel;
				++rel;
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
	reading.mapping = nullptr;
	reading.tags_string.clear();

	insert_if_exists(reading.parent->possible_sets, grammar->sets_any);

	Reading::tags_list_t tlist;
	tlist.swap(reading.tags_list);

	for (auto tter : tlist) {
		addTagToReading(reading, tter, false);
	}

	reading.rehash();
}

Tag* GrammarApplicator::generateVarstringTag(const Tag* tag) {
	static thread_local UnicodeString tmp;
	tmp.remove();
	tmp.append(tag->tag.data(), SI32(tag->tag.size()));
	bool did_something = false;

	// Convert %[UuLl] and $1-9 markers to control codes to avoid having combined %$1 accidentally match %L or have $s in data be filled
	constexpr UStringView raw[] = { STR_VSu_raw, STR_VSU_raw, STR_VSl_raw, STR_VSL_raw, STR_VS1_raw, STR_VS2_raw, STR_VS3_raw, STR_VS4_raw, STR_VS5_raw, STR_VS6_raw, STR_VS7_raw, STR_VS8_raw, STR_VS9_raw };
	constexpr UStringView x01[] = { STR_VSu, STR_VSU, STR_VSl, STR_VSL, STR_VS1, STR_VS2, STR_VS3, STR_VS4, STR_VS5, STR_VS6, STR_VS7, STR_VS8, STR_VS9 };
	for (size_t i = 0; i < 13; ++i) {
		findAndReplace(tmp, raw[i].data(), x01[i].data());
	}

	// Replace unified sets with their matching tags
	if (tag->vs_sets) {
		for (size_t i = 0; i < tag->vs_sets->size(); ++i) {
			auto tags = ss_taglist.get();
			getTagList(*(*tag->vs_sets)[i], tags);
			static thread_local UString rpl;
			rpl.clear();
			// If there are multiple tags, such as from CompositeTags, put _ between them
			foreach (iter, *tags) {
				rpl += (*iter)->tag;
				if (std::distance(iter, iter_end) > 1) {
					rpl += '_';
				}
			}
			findAndReplace(tmp, (*tag->vs_names)[i].data(), rpl.data());
			did_something = true;
		}
	}

	// Replace $1-$9 with their respective match groups
	constexpr UStringView grp[] = { STR_VS1, STR_VS2, STR_VS3, STR_VS4, STR_VS5, STR_VS6, STR_VS7, STR_VS8, STR_VS9 };
	for (size_t i = 0; i < context_stack.back().regexgrp_ct && i < 9; ++i) {
		findAndReplace(tmp, grp[i].data(), USV((*context_stack.back().regexgrps)[i]));
		did_something = true;
	}

	// Handle %U %u %L %l markers.
	bool found;
	do {
		found = false;
		int32_t pos = -1, mpos = -1;
		if ((pos = tmp.lastIndexOf(STR_VSu.data(), SI32(STR_VSu.size()), 0)) != -1) {
			found = true;
			mpos = std::max(mpos, pos);
		}
		if ((pos = tmp.lastIndexOf(STR_VSU.data(), SI32(STR_VSU.size()), mpos)) != -1) {
			found = true;
			mpos = std::max(mpos, pos);
		}
		if ((pos = tmp.lastIndexOf(STR_VSl.data(), SI32(STR_VSl.size()), mpos)) != -1) {
			found = true;
			mpos = std::max(mpos, pos);
		}
		if ((pos = tmp.lastIndexOf(STR_VSL.data(), SI32(STR_VSL.size()), mpos)) != -1) {
			found = true;
			mpos = std::max(mpos, pos);
		}
		if (found && mpos != -1) {
			UChar mode = tmp[mpos + 1];
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

	const UChar* nt = tmp.getTerminatedBuffer();
	if (!did_something && nt == tag->tag) {
		u_fprintf(ux_stderr, "Warning: Unable to generate from tag '%S'! Possibly missing KEEPORDER and/or capturing regex from grammar on line %u before input line %u.\n", tag->tag.data(), grammar->lines, numLines);
		u_fflush(ux_stderr);
	}
	return addTag(nt, true);
}

uint32_t GrammarApplicator::addTagToReading(Reading& reading, uint32_t utag, bool rehash) {
	Tag* tag = grammar->single_tags.find(utag)->second;
	return addTagToReading(reading, tag, rehash);
}

uint32_t GrammarApplicator::addTagToReading(Reading& reading, Tag* tag, bool rehash) {
	if (tag->type & T_VARSTRING) {
		tag = generateVarstringTag(tag);
	}

	auto it = grammar->sets_by_tag.find(tag->hash);
	if (it != grammar->sets_by_tag.end()) {
		reading.parent->possible_sets.resize(std::max(reading.parent->possible_sets.size(), it->second.size()));
		reading.parent->possible_sets |= it->second;
	}
	reading.tags.insert(tag->hash);
	reading.tags_list.push_back(tag->hash);
	reading.tags_bloom.insert(tag->hash);
	// ToDo: Remove for real ordered mode
	if (ordered) {
		if (!reading.tags_string.empty()) {
			reading.tags_string += ' ';
		}
		reading.tags_string += tag->tag;
		reading.tags_string_hash = hash_value(reading.tags_string);
	}
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
	if (tag->type & (T_TEXTUAL | T_WORDFORM | T_BASEFORM)) {
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
	if (parse_dep && (tag->type & T_DEPENDENCY) && !(reading.parent->type & CT_DEP_DONE)) {
		reading.parent->dep_self = tag->dep_self;
		reading.parent->dep_parent = tag->dep_parent;
		if (tag->dep_parent == tag->dep_self) {
			reading.parent->dep_parent = DEP_NO_PARENT;
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
		reading.parent->setRelated();
	}
	if (!(tag->type & T_SPECIAL)) {
		reading.tags_plain.insert(tag->hash);
		reading.tags_plain_bloom.insert(tag->hash);
	}
	if (rehash) {
		reading.rehash();
	}

	if (grammar->has_bag_of_tags) {
		Reading& bot = reading.parent->parent->bag_of_tags;
		bot.tags.insert(tag->hash);
		bot.tags_list.push_back(tag->hash);
		bot.tags_bloom.insert(tag->hash);

		if (tag->type & (T_TEXTUAL | T_WORDFORM | T_BASEFORM)) {
			bot.tags_textual.insert(tag->hash);
			bot.tags_textual_bloom.insert(tag->hash);
		}
		if (tag->type & T_NUMERICAL) {
			bot.tags_numerical[tag->hash] = tag;
		}
		if (!reading.baseform && (tag->type & T_BASEFORM)) {
			bot.baseform = tag->hash;
		}
		if (!(tag->type & T_SPECIAL)) {
			bot.tags_plain.insert(tag->hash);
			bot.tags_plain_bloom.insert(tag->hash);
		}
		if (rehash) {
			bot.rehash();
		}
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
		reading.mapping = nullptr;
	}
	if (utag == reading.baseform) {
		reading.baseform = 0;
	}
	reading.rehash();
	reading.parent->type &= ~CT_NUM_CURRENT;
}

void GrammarApplicator::delTagFromReading(Reading& reading, Tag* tag) {
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
	for (auto it = mappings.begin(); it != mappings.end();) {
		Tag*& tag = *it;
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

	Tag* tag = mappings.back();
	mappings.pop_back();
	size_t i = mappings.size();
	for (auto ttag : mappings) {
		// To avoid duplicating needlessly many times, check for a similar reading in the cohort that's already got this mapping
		bool found = false;
		for (auto itr : cohort.readings) {
			if (itr->hash_plain == reading.hash_plain && itr->mapping && itr->mapping->hash == ttag->hash) {
				found = true;
				break;
			}
		}
		if (found) {
			continue;
		}
		Reading* nr = alloc_reading(reading);
		nr->mapped = mapped;
		nr->number = UI32(reading.number - i--);
		uint32_t mp = addTagToReading(*nr, ttag);
		if (mp != ttag->hash) {
			nr->mapping = grammar->single_tags.find(mp)->second;
		}
		else {
			nr->mapping = ttag;
		}
		cohort.appendReading(nr);
		numReadings++;
	}

	reading.mapped = mapped;
	uint32_t mp = addTagToReading(reading, tag);
	if (mp != tag->hash) {
		reading.mapping = grammar->single_tags.find(mp)->second;
	}
	else {
		reading.mapping = tag;
	}
}

void GrammarApplicator::splitAllMappings(all_mappings_t& all_mappings, Cohort& cohort, bool mapped) {
	if (all_mappings.empty()) {
		return;
	}
	static thread_local ReadingList readings;
	readings = cohort.readings;
	for (auto reading : readings) {
		auto iter = all_mappings.find(reading);
		if (iter == all_mappings.end()) {
			continue;
		}
		splitMappings(iter->second, cohort, *reading, mapped);
	}
	std::sort(cohort.readings.begin(), cohort.readings.end(), Reading::cmp_number);
	if (!grammar->reopen_mappings.empty()) {
		for (auto reading : cohort.readings) {
			if (reading->mapping && grammar->reopen_mappings.count(reading->mapping->hash)) {
				reading->mapped = false;
			}
		}
	}
	all_mappings.clear();
}

void GrammarApplicator::mergeReadings(ReadingList& readings) {
	static thread_local bc::flat_map<uint32_t, std::pair<uint32_t, Reading*>> mapped;
	mapped.clear();
	mapped.reserve(readings.size());
	static thread_local bc::flat_map<uint32_t, ReadingList> mlist;
	mlist.clear();
	mlist.reserve(readings.size());

	for (auto r : readings) {
		uint32_t hp = r->hash_plain, hplain = r->hash_plain;
		if (ordered) {
			hp = hplain = r->tags_string_hash;
		}
		uint32_t nm = 0;
		if (trace) {
			for (auto iter_hb : r->hit_by) {
				hp = hash_value(iter_hb, hp);
			}
		}
		if (r->mapping) {
			++nm;
		}
		Reading* sub = r->next;
		while (sub) {
			if (ordered) {
				hp = hash_value(sub->tags_string_hash, hp);
				hplain = hash_value(sub->tags_string_hash, hplain);
			}
			else {
				hp = hash_value(sub->hash_plain, hp);
				hplain = hash_value(sub->hash_plain, hplain);
			}
			if (trace) {
				for (auto iter_hb : sub->hit_by) {
					hp = hash_value(iter_hb, hp);
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
		mlist[hp + nm].push_back(r);
	}

	if (mlist.size() == readings.size()) {
		return;
	}

	readings.clear();
	static thread_local std::vector<Reading*> order;
	order.clear();

	for (auto& miter : mlist) {
		const ReadingList& clist = miter.second;
		Reading* nr = alloc_reading(*(clist.front()));
		if (nr->mapping) {
			erase(nr->tags_list, nr->mapping->hash);
		}
		for (auto iter1 : clist) {
			if (iter1->mapping && std::find(nr->tags_list.begin(), nr->tags_list.end(), iter1->mapping->hash) == nr->tags_list.end()) {
				nr->tags_list.push_back(iter1->mapping->hash);
			}
			free_reading(iter1);
		}
		order.push_back(nr);
	}

	std::sort(order.begin(), order.end(), Reading::cmp_number);
	readings.insert(readings.begin(), order.begin(), order.end());
}

void GrammarApplicator::mergeMappings(Cohort& cohort) {
	mergeReadings(cohort.readings);
	if (trace) {
		mergeReadings(cohort.deleted);
		mergeReadings(cohort.delayed);
	}
}

Cohort* GrammarApplicator::delimitAt(SingleWindow& current, Cohort* cohort) {
	SingleWindow* nwin = nullptr;
	if (current.parent->current == &current) {
		nwin = current.parent->allocPushSingleWindow();
	}
	else {
		foreach (iter, current.parent->next) {
			if (*iter == &current) {
				nwin = current.parent->allocSingleWindow();
				current.parent->next.insert(++iter, nwin);
				break;
			}
		}
		if (!nwin) {
			foreach (iter, current.parent->previous) {
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

	std::swap(current.flush_after, nwin->flush_after);
	std::swap(current.text_post, nwin->text_post);
	nwin->has_enclosures = current.has_enclosures;

	Cohort* cCohort = alloc_cohort(nwin);
	cCohort->global_number = current.parent->cohort_counter++;
	cCohort->wordform = tag_begin;

	Reading* cReading = alloc_reading(cCohort);
	cReading->baseform = begintag;
	insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
	addTagToReading(*cReading, begintag);

	cCohort->appendReading(cReading);
	nwin->appendCohort(cCohort);

	auto lc = cohort->local_number;
	auto nc = std::find(current.all_cohorts.begin() + lc, current.all_cohorts.end(), cohort);
	++nc;
	auto from = nc;
	for (; nc != current.all_cohorts.end(); ++nc) {
		(*nc)->parent = nwin;
		if ((*nc)->type & (CT_ENCLOSED | CT_REMOVED | CT_IGNORED)) {
			nwin->all_cohorts.push_back(*nc);
		}
		else {
			nwin->appendCohort(*nc);
		}
	}
	current.cohorts.erase(current.cohorts.begin() + lc + 1, current.cohorts.end());
	current.all_cohorts.erase(from, current.all_cohorts.end());

	cohort = current.cohorts.back();
	for (auto reading : cohort->readings) {
		addTagToReading(*reading, endtag);
	}
	gWindow->rebuildCohortLinks();

	return cohort;
}

void GrammarApplicator::reflowTextuals_Reading(Reading& r) {
	if (r.next) {
		reflowTextuals_Reading(*r.next);
	}
	for (auto it : r.tags) {
		Tag* tag = grammar->single_tags.find(it)->second;
		if (tag->type & T_TEXTUAL) {
			r.tags_textual.insert(it);
			r.tags_textual_bloom.insert(it);
		}
	}
}

void GrammarApplicator::reflowTextuals_Cohort(Cohort& c) {
	for (auto it : c.readings) {
		reflowTextuals_Reading(*it);
	}
	for (auto it : c.deleted) {
		reflowTextuals_Reading(*it);
	}
	for (auto it : c.ignored) {
		reflowTextuals_Reading(*it);
	}
	for (auto it : c.delayed) {
		reflowTextuals_Reading(*it);
	}
}

void GrammarApplicator::reflowTextuals_SingleWindow(SingleWindow& sw) {
	for (auto it : sw.all_cohorts) {
		reflowTextuals_Cohort(*it);
	}
}

void GrammarApplicator::reflowTextuals() {
	for (auto swit : gWindow->previous) {
		reflowTextuals_SingleWindow(*swit);
	}
	reflowTextuals_SingleWindow(*gWindow->current);
	for (auto swit : gWindow->next) {
		reflowTextuals_SingleWindow(*swit);
	}
}
}
