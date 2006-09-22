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
#include "uextras.h"

using namespace CG3;
using namespace CG3::Strings;

bool GrammarApplicator::doesTagMatchSet(const uint32_t tag, const uint32_t set) {
	bool retval = false;

	stdext::hash_map<uint32_t, Set*>::const_iterator iter = grammar->sets_by_contents.find(set);
	if (iter != grammar->sets_by_contents.end()) {
		const Set *theset = iter->second;

		CompositeTag *ctag = new CompositeTag();
		ctag->addTag(tag);
		ctag->rehash();

		if (theset->tags.find(ctag->hash) != theset->tags.end()) {
			retval = true;
		}
		delete ctag;
	}
	return retval;
}

bool GrammarApplicator::doesSetMatchReading(const Reading *reading, const uint32_t set) {
	bool retval = false;

	if (index_reading_tags_yes.find(reading->hash_tags) != index_reading_tags_yes.end()) {
		Index *index = index_reading_tags_yes[reading->hash_tags];
		if (index->values.find(set) != index->values.end()) {
			cache_hits++;
			return true;
		}
	}
	if (index_reading_tags_no.find(reading->hash_tags) != index_reading_tags_no.end()) {
		Index *index = index_reading_tags_no[reading->hash_tags];
		if (index->values.find(set) != index->values.end()) {
			cache_hits++;
			return false;
		}
	}
	if (index_reading_yes.find(reading->hash) != index_reading_yes.end()) {
		Index *index = index_reading_yes[reading->hash];
		if (index->values.find(set) != index->values.end()) {
			cache_hits++;
			return true;
		}
	}
	if (index_reading_no.find(reading->hash) != index_reading_no.end()) {
		Index *index = index_reading_no[reading->hash];
		if (index->values.find(set) != index->values.end()) {
			cache_hits++;
			return false;
		}
	}

	cache_miss++;
	bool used_special = false;

	stdext::hash_map<uint32_t, Set*>::const_iterator iter = grammar->sets_by_contents.find(set);
	if (iter != grammar->sets_by_contents.end()) {
		const Set *theset = iter->second;
		if (!theset->tags.empty()) {
			stdext::hash_map<uint32_t, uint32_t>::const_iterator ster;
			for (ster = theset->tags.begin() ; ster != theset->tags.end() ; ster++) {
				bool match = true;
				bool failfast = true;
				bool special = false;
				const CompositeTag *ctag = grammar->tags.find(ster->second)->second;

				stdext::hash_map<uint32_t, uint32_t>::const_iterator cter;
				for (cter = ctag->tags.begin() ; cter != ctag->tags.end() ; cter++) {
					const Tag *tag = grammar->single_tags.find(cter->second)->second;
					if (!(tag->features & F_FAILFAST)) {
						failfast = false;
					}
					if (tag->type & (T_WORDFORM|T_BASEFORM)) {
						special = true;
					}
					if (reading->tags.find(cter->second) == reading->tags.end()) {
						match = false;
						if (tag->features & F_NEGATIVE) {
							match = true;
						}
					}
					else {
						if (tag->features & F_NEGATIVE) {
							match = false;
						}
					}
					if (!match) {
						break;
					}
				}
				if (match) {
					used_special = special;
					if (failfast) {
						retval = false;
					}
					else {
						retval = true;
					}
					break;
				}
			}
		} else {
			used_special = true;
			uint32_t size = (uint32_t)theset->sets.size();
			if (size == 1) {
				retval = doesSetMatchReading(reading, theset->sets.at(0));
			} else {
				bool *matches = new bool[size];
				for (uint32_t i=0;i<size;i++) {
					matches[i] = doesSetMatchReading(reading, theset->sets.at(i));
					retval = matches[i];
					if (retval) {
						break;
					}
				}
				delete matches;
			}
		}
	}

	if (retval) {
		if (used_special) {
			if (index_reading_yes.find(reading->hash) == index_reading_yes.end()) {
				index_reading_yes[reading->hash] = new Index();
			}
			index_reading_yes[reading->hash]->values[set] = set;
		}
		else {
			if (index_reading_tags_yes.find(reading->hash_tags) == index_reading_tags_yes.end()) {
				index_reading_tags_yes[reading->hash_tags] = new Index();
			}
			index_reading_tags_yes[reading->hash_tags]->values[set] = set;
		}
	}
	else {
		if (used_special) {
			if (index_reading_no.find(reading->hash) == index_reading_no.end()) {
				index_reading_no[reading->hash] = new Index();
			}
			index_reading_no[reading->hash]->values[set] = set;
		}
		else {
			if (index_reading_tags_no.find(reading->hash_tags) == index_reading_tags_no.end()) {
				index_reading_tags_no[reading->hash_tags] = new Index();
			}
			index_reading_tags_no[reading->hash_tags]->values[set] = set;
		}
	}

	return retval;
}

bool GrammarApplicator::doesSetMatchCohortNormal(const Cohort *cohort, const uint32_t set) {
	bool retval = false;
	std::vector<Reading*>::const_iterator iter;
	for (iter = cohort->readings.begin() ; iter != cohort->readings.end() ; iter++) {
		Reading *reading = *iter;
		if (!reading->deleted) {
			if (doesSetMatchReading(reading, set)) {
				retval = true;
				break;
			}
		}
	}
	return retval;
}

bool GrammarApplicator::doesSetMatchCohortCareful(const Cohort *cohort, const uint32_t set) {
	bool retval = true;
	std::vector<Reading*>::const_iterator iter;
	for (iter = cohort->readings.begin() ; iter != cohort->readings.end() ; iter++) {
		Reading *reading = *iter;
		if (!reading->deleted) {
			if (!doesSetMatchReading(reading, set)) {
				retval = false;
				break;
			}
		}
	}
	return retval;
}
