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

		if (theset->single_tags.find(tag) != theset->single_tags.end()) {
			retval = true;
		}
		else {
			CompositeTag *ctag = new CompositeTag();
			ctag->addTag(tag);
			ctag->rehash();

			if (theset->tags.find(ctag->hash) != theset->tags.end()) {
				retval = true;
			}
			delete ctag;
		}
	}
	return retval;
}

inline bool GrammarApplicator::__index_matches(const stdext::hash_map<uint32_t, Index*> *me, const uint32_t value, const uint32_t set) {
	if (me->find(value) != me->end()) {
		const Index *index = me->find(value)->second;
		if (index->values.find(set) != index->values.end()) {
			cache_hits++;
			return true;
		}
	}
	return false;
}

bool GrammarApplicator::doesSetMatchReading(const Reading *reading, const uint32_t set, bool bypass_index) {
	bool retval = false;

	assert(reading->hash != 0);

	if (reading->hash && reading->hash != 1) {
		if (!bypass_index && __index_matches(&index_reading_yes, reading->hash, set)) { return true; }
		if (__index_matches(&index_reading_no, reading->hash, set)) { return false; }
	}
	if (reading->hash_plain && reading->hash_plain != 1) {
		if (!bypass_index && __index_matches(&index_reading_plain_yes, reading->hash_plain, set)) { return true; }
		if (__index_matches(&index_reading_plain_no, reading->hash_plain, set)) { return false; }
	}
	/*
	if (reading->hash_tags) {
	if (__index_matches(&index_reading_tags_yes, reading->hash_tags, set)) { return true; }
	if (__index_matches(&index_reading_tags_no, reading->hash_tags, set)) { return false; }
	}
	//*/

	cache_miss++;
	bool used_special = false;
	bool only_plain = true;

	stdext::hash_map<uint32_t, Set*>::const_iterator iter = grammar->sets_by_contents.find(set);
	if (iter != grammar->sets_by_contents.end()) {
		const Set *theset = iter->second;
		if (theset->match_any) {
			retval = true;
		}
		else if (theset->sets.empty()) {
			bool set_special = false;
			bool set_plain = true;
			stdext::hash_map<uint32_t, uint32_t>::const_iterator ster;

			for (ster = theset->single_tags.begin() ; ster != theset->single_tags.end() ; ster++) {
				bool match = true;
				bool failfast = true;
				bool comp_special = false;
				bool comp_plain = true;

				const Tag *tag = grammar->single_tags.find(ster->second)->second;
				if (!(tag->type & T_FAILFAST)) {
					failfast = false;
				}
				if (tag->type & (T_WORDFORM|T_BASEFORM)) {
					comp_special = true;
					set_special = true;
				}
				if (tag->type) {
					comp_plain = false;
					set_plain = false;
				}
				if (tag->regexp) {
					match = !reading->tags_textual.empty();
					std::map<uint32_t, uint32_t>::const_iterator mter;
					for (mter = reading->tags_textual.begin() ; mter != reading->tags_textual.end() ; mter++) {
						// ToDo: Cache regexp and icase hits/misses
						const Tag *itag = single_tags.find(mter->second)->second;
						UErrorCode status = U_ZERO_ERROR;
						uregex_setText(tag->regexp, itag->tag, u_strlen(itag->tag), &status);
						if (status != U_ZERO_ERROR) {
							u_fprintf(ux_stderr, "Error: uregex_setText(MatchSet) returned %s - cannot continue!\n", u_errorName(status));
							exit(1);
						}
						status = U_ZERO_ERROR;
						match = (uregex_matches(tag->regexp, 0, &status) == TRUE);
						if (status != U_ZERO_ERROR) {
							u_fprintf(ux_stderr, "Error: uregex_matches(MatchSet) returned %s - cannot continue!\n", u_errorName(status));
							exit(1);
						}
						if (match) {
							break;
						}
					}
				}
				else if (reading->tags.find(ster->second) == reading->tags.end()) {
					match = false;
					if (tag->type & T_NEGATIVE) {
						match = true;
					}
				}
				else {
					if (tag->type & T_NEGATIVE) {
						match = false;
					}
				}
				if (match) {
					match_single++;
					used_special = comp_special;
					only_plain = comp_plain;
					if (failfast) {
						retval = false;
					}
					else {
						retval = true;
					}
					if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
						last_mapping_tag = tag->hash;
					}
					break;
				} else {
					used_special = set_special;
					only_plain = set_plain;
				}
			}

			if (!retval) {
				for (ster = theset->tags.begin() ; ster != theset->tags.end() ; ster++) {
					bool match = true;
					bool failfast = true;
					bool comp_special = false;
					bool comp_plain = true;
					const CompositeTag *ctag = grammar->tags.find(ster->second)->second;

					stdext::hash_map<uint32_t, uint32_t>::const_iterator cter;
					for (cter = ctag->tags.begin() ; cter != ctag->tags.end() ; cter++) {
						const Tag *tag = grammar->single_tags.find(cter->second)->second;
						if (!(tag->type & T_FAILFAST)) {
							failfast = false;
						}
						if (tag->type & (T_WORDFORM|T_BASEFORM)) {
							comp_special = true;
							set_special = true;
						}
						if (tag->type) {
							comp_plain = false;
							set_plain = false;
						}
						if (reading->tags.find(cter->second) == reading->tags.end()) {
							match = false;
							if (tag->type & T_NEGATIVE) {
								match = true;
							}
						}
						else {
							if (tag->type & T_NEGATIVE) {
								match = false;
							}
						}
						if (!match) {
							break;
						}
						if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
							last_mapping_tag = tag->hash;
						}
					}
					if (match) {
						match_comp++;
						used_special = comp_special;
						only_plain = comp_plain;
						if (failfast) {
							retval = false;
						}
						else {
							retval = true;
						}
						break;
					} else {
						last_mapping_tag = 0;
						used_special = set_special;
						only_plain = set_plain;
					}
				}
			}
		}
		else {
			used_special = true;
			only_plain = false;
			uint32_t size = (uint32_t)theset->sets.size();
			for (uint32_t i=0;i<size;i++) {
				bool match = doesSetMatchReading(reading, theset->sets.at(i), bypass_index);
				bool failfast = false;
				while (i < size-1 && theset->set_ops.at(i) != S_OR) {
					switch (theset->set_ops.at(i)) {
						case S_PLUS:
							if (match) {
								match = doesSetMatchReading(reading, theset->sets.at(i+1), bypass_index);
							}
							break;
						case S_FAILFAST:
							if (match) {
								if (doesSetMatchReading(reading, theset->sets.at(i+1), bypass_index)) {
									match = false;
									failfast = true;
								}
							}
							break;
						case S_MINUS:
							if (match) {
								if (doesSetMatchReading(reading, theset->sets.at(i+1), bypass_index)) {
									match = false;
								}
							}
							break;
						case S_NOT:
							if (!match) {
								if (!doesSetMatchReading(reading, theset->sets.at(i+1))) {
									match = true;
								}
							}
							break;
						default:
							break;
					}
					i++;
				}
				if (match) {
					match_sub++;
					retval = true;
					break;
				}
				if (failfast) {
					match_sub++;
					retval = false;
					break;
				}
			}
		}
	}

	if (retval) {
		if (only_plain && reading->hash_plain && reading->hash_plain != 1) {
			if (index_reading_plain_yes.find(reading->hash_plain) == index_reading_plain_yes.end()) {
				index_reading_plain_yes[reading->hash_plain] = new Index();
			}
			index_reading_plain_yes[reading->hash_plain]->values[set] = set;
		}
		/*
		else if (!used_special && reading->hash_tags) {
		if (index_reading_tags_yes.find(reading->hash_tags) == index_reading_tags_yes.end()) {
		index_reading_tags_yes[reading->hash_tags] = new Index();
		}
		index_reading_tags_yes[reading->hash_tags]->values[set] = set;
		}
		//*/
		if (reading->hash && reading->hash != 1) {
			if (index_reading_yes.find(reading->hash) == index_reading_yes.end()) {
				index_reading_yes[reading->hash] = new Index();
			}
			index_reading_yes[reading->hash]->values[set] = set;
		}
	}
	else {
		if (only_plain && reading->hash_plain && reading->hash_plain != 1) {
			if (index_reading_plain_no.find(reading->hash_plain) == index_reading_plain_no.end()) {
				index_reading_plain_no[reading->hash_plain] = new Index();
			}
			index_reading_plain_no[reading->hash_plain]->values[set] = set;
		}
		/*
		else if (!used_special && reading->hash_tags) {
		if (index_reading_tags_no.find(reading->hash_tags) == index_reading_tags_no.end()) {
		index_reading_tags_no[reading->hash_tags] = new Index();
		}
		index_reading_tags_no[reading->hash_tags]->values[set] = set;
		}
		//*/
		if (reading->hash && reading->hash != 1) {
			if (index_reading_no.find(reading->hash) == index_reading_no.end()) {
				index_reading_no[reading->hash] = new Index();
			}
			index_reading_no[reading->hash]->values[set] = set;
		}
	}

	return retval;
}

bool GrammarApplicator::doesSetMatchCohortNormal(const Cohort *cohort, const uint32_t set) {
	bool retval = false;
	std::list<Reading*>::const_iterator iter;
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
	std::list<Reading*>::const_iterator iter;
	for (iter = cohort->readings.begin() ; iter != cohort->readings.end() ; iter++) {
		Reading *reading = *iter;
		if (!reading->deleted) {
			last_mapping_tag = 0;
			const Set *theset = grammar->sets_by_contents.find(set)->second;
			if (!doesSetMatchReading(reading, set, theset->has_mappings)) {
				retval = false;
				break;
			}
			// A mapped tag must be the only mapped tag in the reading to be considered a Careful match
			if (last_mapping_tag && reading->tags_mapped.size() > 1) {
				retval = false;
				break;
			}
		}
	}
	return retval;
}

int32_t GrammarApplicator::doesSetMatchDependency(const SingleWindow *sWindow, const Cohort *current, const ContextualTest *test) {
	int32_t rv = -1;

	std::list<Reading*>::const_iterator iter;
	for (iter = current->readings.begin() ; iter != current->readings.end() ; iter++) {
		Reading *reading = *iter;
		if (!reading->deleted) {
			const std::set<uint32_t> *deps = 0;
			if (test->dep_child) {
				deps = &reading->dep_children;
			}
			else if (test->dep_parent) {
				deps = &reading->dep_parents;
			}
			else {
				deps = &reading->dep_siblings;
			}

			bool retval = false;
			std::set<uint32_t>::const_iterator dter;
			for (dter = deps->begin() ; dter != deps->end() ; dter++) {
				if (sWindow->dep_map.find(*dter) == sWindow->dep_map.end()) {
					u_fprintf(ux_stderr, "Warning: Dependency %u does not exist - ignoring.\n", *dter);
					continue;
				}
				uint32_t dep_pos = sWindow->dep_map.find(*dter)->second;
				Cohort *cohort = sWindow->cohorts[dep_pos];
				if (test->careful) {
					retval = doesSetMatchCohortCareful(cohort, test->target);
				}
				else {
					retval = doesSetMatchCohortNormal(cohort, test->target);
				}
				if (retval && !test->negative) {
					rv = dep_pos;
					break;
				}
				else if (retval && test->negative) {
					rv = 0;
					break;
				}
			}
			if (rv != -1) {
				break;
			}
		}
	}

	return rv;
}
