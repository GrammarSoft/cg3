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
#include "stdafx.h"
#include "Grammar.h"

using namespace CG3;

Grammar::Grammar() {
	last_modified = 0;
	name = 0;
	lines = 0;
	curline = 0;
	delimiters = 0;
	vislcg_compat_mode = false;
	srand((uint32_t)time(0));
}

Grammar::~Grammar() {
	if (name) {
		delete name;
	}
	
	std::vector<UChar*>::iterator iter;
	for (iter = preferred_targets.begin() ; iter != preferred_targets.end() ; iter++) {
		if (*iter) {
			delete *iter;
		}
	}
	preferred_targets.clear();
	
	sections.clear();
	
	stdext::hash_map<uint32_t, Set*>::iterator iter_set;
	for (iter_set = sets.begin() ; iter_set != sets.end() ; iter_set++) {
		if (iter_set->second) {
			delete iter_set->second;
		}
	}
	sets.clear();
/*
	for (iter_set = uniqsets.begin() ; iter_set != uniqsets.end() ; iter_set++) {
		if (iter_set->second) {
			delete iter_set->second;
		}
	}
	uniqsets.clear();
//*/
	std::map<uint32_t, Anchor*>::iterator iter_anc;
	for (iter_anc = anchors.begin() ; iter_anc != anchors.end() ; iter_anc++) {
		if (iter_anc->second) {
			delete iter_anc->second;
		}
	}
	anchors.clear();
	
	stdext::hash_map<uint32_t, CompositeTag*>::iterator iter_ctag;
	for (iter_ctag = tags.begin() ; iter_ctag != tags.end() ; iter_ctag++) {
		if (iter_ctag->second) {
			delete iter_ctag->second;
		}
	}
	tags.clear();

	std::vector<Rule*>::iterator iter_rules;
	for (iter_rules = rules.begin() ; iter_rules != rules.end() ; iter_rules++) {
		if (*iter_rules) {
			delete *iter_rules;
		}
	}
	rules.clear();
}

void Grammar::addPreferredTarget(UChar *to) {
	UChar *pf = new UChar[u_strlen(to)+1];
	u_strcpy(pf, to);
	preferred_targets.push_back(pf);
}
void Grammar::addSet(Set *to) {
	const UChar *sname = to->getName();
	uint32_t hash = hash_sdbm_uchar(to->name, 0);
	if (sets.find(hash) == sets.end()) {
		sets[hash] = to;
	}
	else if (!(sname[0] == '_' && sname[1] == 'G' && sname[2] == '_')) {
		u_fprintf(ux_stderr, "Warning: Set %S already existed.\n", to->name);
	}
}
void Grammar::addUniqSet(Set *to) {
	uint32_t hash = to->rehash();
	if (to && to->tags.size() && uniqsets.find(hash) == uniqsets.end()) {
		uniqsets[hash] = to;
	}
}
Set *Grammar::getSet(uint32_t which) {
	if (sets.find(which) != sets.end()) {
		return sets[which];
	}
	return 0;
}

Set *Grammar::allocateSet() {
	return new Set;
}
void Grammar::destroySet(Set *set) {
	delete set;
}

void Grammar::addCompositeTag(CompositeTag *tag) {
	if (tag && tag->tags.size()) {
		tag->rehash();
		if (tags.find(tag->hash) != tags.end()) {
			delete tags[tag->hash];
		}
		tags[tag->hash] = tag;
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty composite tag to grammar!\n");
	}
}
void Grammar::addCompositeTagToSet(Set *set, CompositeTag *tag) {
	if (tag && tag->tags.size()) {
		addCompositeTag(tag);
		set->addCompositeTag(tag->hash);
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty composite tag to grammar and set!\n");
	}
}
CompositeTag *Grammar::allocateCompositeTag() {
	return new CompositeTag;
}
CompositeTag *Grammar::duplicateCompositeTag(CompositeTag *tag) {
	if (tag && tag->tags.size()) {
		CompositeTag *tmp = new CompositeTag;
		std::map<uint32_t, uint32_t>::iterator iter;
		for (iter = tag->tags_map.begin() ; iter != tag->tags_map.end() ; iter++) {
			tmp->addTag(iter->second);
		}
		return tmp;
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to duplicate an empty composite tag!\n");
	}
	return 0;
}
void Grammar::destroyCompositeTag(CompositeTag *tag) {
	delete tag;
}

Rule *Grammar::allocateRule() {
	return new Rule;
}
void Grammar::addRule(Rule *rule) {
	rules.push_back(rule);
}
void Grammar::destroyRule(Rule *rule) {
	delete rule;
}

Tag *Grammar::allocateTag(const UChar *tag) {
	Tag *fresh = new Tag;
	fresh->parseTag(tag);
	return fresh;
}
Tag *Grammar::duplicateTag(uint32_t tag) {
	if (tag && single_tags.find(tag) != single_tags.end() && single_tags[tag]->tag) {
		Tag *fresh = new Tag;
		fresh->duplicateTag(single_tags[tag]);
		return fresh;
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to duplicate an empty tag!\n");
	}
	return 0;
}
void Grammar::addTag(Tag *simpletag) {
	if (simpletag && simpletag->tag) {
		simpletag->rehash();
		if (single_tags.find(simpletag->hash) != single_tags.end()) {
			delete single_tags[simpletag->hash];
		}
		single_tags[simpletag->hash] = simpletag;
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty tag to grammar!\n");
	}
}
void Grammar::addTagToCompositeTag(Tag *simpletag, CompositeTag *tag) {
	if (simpletag && simpletag->tag) {
		addTag(simpletag);
		tag->addTag(simpletag->hash);
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty tag to grammar and composite tag!\n");
	}
}
void Grammar::destroyTag(Tag *tag) {
	delete tag;
}

void Grammar::addAnchor(const UChar *to, uint32_t line) {
	uint32_t ah = hash_sdbm_uchar(to, 0);
	if (anchors.find(ah) != anchors.end()) {
		u_fprintf(ux_stderr, "Warning: Anchor '%S' redefined on line %u!\n", to, curline);
		delete anchors[ah];
		anchors.erase(ah);
	}
	Anchor *anc = new Anchor;
	anc->setName(to);
	anc->line = line;
	anchors[ah] = anc;
}

void Grammar::addAnchor(const UChar *to) {
	addAnchor(to, (uint32_t)(rules.size()+1));
}

void Grammar::manipulateSet(uint32_t set_a, int op, uint32_t set_b, uint32_t result) {
/*
	return;
/*/
	if (op <= S_IGNORE || op >= STRINGS_COUNT) {
		u_fprintf(ux_stderr, "Error: Invalid set operation on line %u!\n", curline);
		return;
	}
	if (sets.find(set_a) == sets.end()) {
		u_fprintf(ux_stderr, "Error: Invalid left operand for set operation on line %u!\n", curline);
		return;
	}
	if (sets.find(set_b) == sets.end()) {
		u_fprintf(ux_stderr, "Error: Invalid right operand for set operation on line %u!\n", curline);
		return;
	}
	if (!result) {
		u_fprintf(ux_stderr, "Error: Invalid target for set operation on line %u!\n", curline);
		return;
	}

	stdext::hash_map<uint32_t, uint32_t> result_tags;
	std::map<uint32_t, uint32_t> result_map;
	switch (op) {
		case S_OR:
		{
			result_tags.insert(sets[set_a]->tags.begin(), sets[set_a]->tags.end());
			result_map.insert(sets[set_a]->tags.begin(), sets[set_a]->tags.end());
			result_tags.insert(sets[set_b]->tags.begin(), sets[set_b]->tags.end());
			result_map.insert(sets[set_b]->tags.begin(), sets[set_b]->tags.end());
			break;
		}
		case S_FAILFAST:
		{
			stdext::hash_map<uint32_t, uint32_t>::iterator iter;
			for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
				if (sets[set_b]->tags.find(iter->first) == sets[set_b]->tags.end()) {
					result_tags[iter->first] = iter->second;
					result_map[iter->first] = iter->second;
				}
			}
			for (iter = sets[set_b]->tags.begin() ; iter != sets[set_b]->tags.end() ; iter++) {
				CompositeTag *tmp = allocateCompositeTag();
				std::map<uint32_t, uint32_t>::iterator iter_tag;
				for (iter_tag = tags[iter->second]->tags_map.begin() ; iter_tag != tags[iter->second]->tags_map.end() ; iter_tag++) {
					Tag *tmptag = duplicateTag(iter_tag->second);
					tmptag->failfast = !(tmptag->failfast);
					tmptag->rehash();
					addTagToCompositeTag(tmptag, tmp);
				}
				tmp->rehash();
				addCompositeTag(tmp);
				result_tags[tmp->hash] = tmp->hash;
				result_map[tmp->hash] = tmp->hash;
			}
			break;
		}
		case S_NOT:
		{
			stdext::hash_map<uint32_t, uint32_t>::iterator iter;
			for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
				if (sets[set_b]->tags.find(iter->first) == sets[set_b]->tags.end()) {
					result_tags[iter->first] = iter->second;
					result_map[iter->first] = iter->second;
				}
			}
			for (iter = sets[set_b]->tags.begin() ; iter != sets[set_b]->tags.end() ; iter++) {
				CompositeTag *tmp = allocateCompositeTag();
				std::map<uint32_t, uint32_t>::iterator iter_tag;
				for (iter_tag = tags[iter->second]->tags_map.begin() ; iter_tag != tags[iter->second]->tags_map.end() ; iter_tag++) {
					Tag *tmptag = duplicateTag(iter_tag->second);
					tmptag->negative = !(tmptag->negative);
					tmptag->rehash();
					addTagToCompositeTag(tmptag, tmp);
				}
				tmp->rehash();
				addCompositeTag(tmp);
				result_tags[tmp->hash] = tmp->hash;
				result_map[tmp->hash] = tmp->hash;
			}
			break;
		}
		case S_MINUS:
		{
			stdext::hash_map<uint32_t, uint32_t>::iterator iter;
			for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
				if (sets[set_b]->tags.find(iter->first) == sets[set_b]->tags.end()) {
					result_tags[iter->first] = iter->second;
					result_map[iter->first] = iter->second;
				}
			}
			break;
		}
		case S_MULTIPLY:
		case S_PLUS:
		{
			stdext::hash_map<uint32_t, uint32_t>::iterator iter_a;
			for (iter_a = sets[set_a]->tags.begin() ; iter_a != sets[set_a]->tags.end() ; iter_a++) {
				stdext::hash_map<uint32_t, uint32_t>::iterator iter_b;
				for (iter_b = sets[set_b]->tags.begin() ; iter_b != sets[set_b]->tags.end() ; iter_b++) {
					CompositeTag *tag_r = allocateCompositeTag();

					stdext::hash_map<uint32_t, uint32_t>::iterator iter_t;
					for (iter_t = tags[iter_a->second]->tags.begin() ; iter_t != tags[iter_a->second]->tags.end() ; iter_t++) {
						tag_r->addTag(iter_t->second);
					}

					for (iter_t = tags[iter_b->second]->tags.begin() ; iter_t != tags[iter_b->second]->tags.end() ; iter_t++) {
						tag_r->addTag(iter_t->second);
					}
					tag_r->rehash();
					addCompositeTag(tag_r);
					result_tags[tag_r->hash] = tag_r->hash;
					result_map[tag_r->hash] = tag_r->hash;
				}
			}
			break;
		}
		default:
		{
			u_fprintf(ux_stderr, "Error: Invalid set operation %u between %S and %S!\n", op, sets[set_a]->getName(), sets[set_b]->getName());
			break;
		}
	}
	sets[result]->tags.clear();
	sets[result]->tags.swap(result_tags);

	sets[result]->tags_map.clear();
	sets[result]->tags_map.swap(result_map);
//*/
}

void Grammar::setName(const char *to) {
	name = new UChar[strlen(to)+1];
	u_uastrcpy(name, to);
}

void Grammar::setName(const UChar *to) {
	name = new UChar[u_strlen(to)+1];
	u_strcpy(name, to);
}

void Grammar::printRule(UFILE *to, Rule *rule) {
	if (rule->wordform) {
		u_fprintf(to, "%S ", rule->wordform);
	}

	u_fprintf(to, "%S", keywords[rule->type]);

	if (rule->name && !(rule->name[0] == '_' && rule->name[1] == 'R' && rule->name[2] == '_')) {
		u_fprintf(to, ":%S", rule->name);
	}
	u_fprintf(to, " ");

	if (rule->subst_target) {
		u_fprintf(to, "%S ", uniqsets[rule->subst_target]->name);
	}

	if (rule->maplist.size()) {
		std::list<uint32_t>::iterator iter;
		u_fprintf(to, "(");
		for (iter = rule->maplist.begin() ; iter != rule->maplist.end() ; iter++) {
			single_tags[*iter]->print(to);
			u_fprintf(to, " ");
		}
		u_fprintf(to, ") ");
	}

	if (rule->target) {
		u_fprintf(to, "%S ", uniqsets[rule->target]->name);
	}

	if (rule->tests.size()) {
		std::list<ContextualTest*>::iterator iter;
		for (iter = rule->tests.begin() ; iter != rule->tests.end() ; iter++) {
			u_fprintf(to, "(");
			printContextualTest(to, *iter);
			u_fprintf(to, ") ");
		}
	}
}

void Grammar::printContextualTest(UFILE *to, ContextualTest *test) {
	if (test->absolute) {
		u_fprintf(to, "@");
	}
	if (test->scanall) {
		u_fprintf(to, "**");
	}
	else if (test->scanfirst) {
		u_fprintf(to, "*");
	}

	u_fprintf(to, "%d", test->offset);

	if (test->careful) {
		u_fprintf(to, "C");
	}
	if (test->span_windows) {
		u_fprintf(to, "W");
	}

	u_fprintf(to, " ");

	if (test->target) {
		u_fprintf(to, "%S ", uniqsets[test->target]->name);
	}
	if (test->barrier) {
		u_fprintf(to, "BARRIER %S ", uniqsets[test->barrier]->name);
	}

	if (test->linked) {
		u_fprintf(to, "LINK ");
		printContextualTest(to, test->linked);
	}
}
