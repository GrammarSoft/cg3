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
#include "Grammar.h"

using namespace CG3;

Grammar::Grammar() {
	has_dep = false;
	last_modified = 0;
	name = 0;
	lines = 0;
	curline = 0;
	delimiters = 0;
	soft_delimiters = 0;
	tag_any = 0;
	mapping_prefix = '@';
	srand((uint32_t)time(0));
}

Grammar::~Grammar() {
	if (name) {
		delete[] name;
	}
	
	preferred_targets.clear();
	
	sections.clear();
	
	stdext::hash_map<uint32_t, Set*>::iterator iter_set;
	for (iter_set = sets_by_contents.begin() ; iter_set != sets_by_contents.end() ; iter_set++) {
		if (iter_set->second) {
			delete iter_set->second;
		}
	}
	sets_by_contents.clear();
	sets_by_name.clear();

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

	stdext::hash_map<uint32_t, Tag*>::iterator iter_stag;
	for (iter_stag = single_tags.begin() ; iter_stag != single_tags.end() ; iter_stag++) {
		if (iter_stag->second) {
			delete iter_stag->second;
		}
	}
	single_tags.clear();

	std::vector<Rule*>::iterator iter_rules;
	for (iter_rules = rules.begin() ; iter_rules != rules.end() ; iter_rules++) {
		if (*iter_rules) {
			delete *iter_rules;
		}
	}
	rules.clear();

	for (iter_rules = before_sections.begin() ; iter_rules != before_sections.end() ; iter_rules++) {
		if (*iter_rules) {
			delete *iter_rules;
		}
	}
	before_sections.clear();

	for (iter_rules = after_sections.begin() ; iter_rules != after_sections.end() ; iter_rules++) {
		if (*iter_rules) {
			delete *iter_rules;
		}
	}
	after_sections.clear();

	set_alias.clear();

	stdext::hash_map<uint32_t,uint32HashSet*>::iterator xrule;
	for (xrule = rules_by_tag.begin() ; xrule != rules_by_tag.end() ; xrule++) {
		xrule->second->clear();
		delete xrule->second;
		xrule->second = 0;
	}
	rules_by_tag.clear();

	stdext::hash_map<uint32_t,uint32HashSet*>::iterator xset;
	for (xset = sets_by_tag.begin() ; xset != sets_by_tag.end() ; xset++) {
		xset->second->clear();
		delete xset->second;
		xset->second = 0;
	}
	sets_by_tag.clear();
}

void Grammar::addPreferredTarget(UChar *to) {
	Tag *tag = new Tag();
	tag->parseTag(to, ux_stderr);
	tag->rehash();
	addTag(tag);
	preferred_targets.push_back(tag->hash);
}
void Grammar::addSet(Set *to) {
	assert(to);
	uint32_t nhash = hash_sdbm_uchar(to->name, 0);
	uint32_t chash = to->rehash();
	if (sets_by_name.find(nhash) == sets_by_name.end()) {
		sets_by_name[nhash] = chash;
	}
	if (sets_by_contents.find(chash) == sets_by_contents.end()) {
		sets_by_contents[chash] = to;
	}
}
Set *Grammar::getSet(uint32_t which) {
	Set *retval = 0;
	if (sets_by_contents.find(which) != sets_by_contents.end()) {
		retval = sets_by_contents[which];
	}
	else if (sets_by_name.find(which) != sets_by_name.end()) {
		retval = getSet(sets_by_name[which]);
	}
	return retval;
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
		if (tag->tags.size() == 1) {
			Tag *rtag = single_tags[*(tag->tags.begin())];
			set->tags_set.insert(rtag->hash);
			set->single_tags.insert(rtag->hash);

			if (rtag->type & T_ANY) {
				set->match_any = true;
			}
			//*
			delete tag;
			tag = 0;
			//*/
		} else {
			addCompositeTag(tag);
			set->tags_set.insert(tag->hash);
			set->tags.insert(tag->hash);
		}
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty composite tag to grammar and set!\n");
	}
}
CompositeTag *Grammar::allocateCompositeTag() {
	return new CompositeTag;
}
void Grammar::destroyCompositeTag(CompositeTag *tag) {
	delete tag;
}

Rule *Grammar::allocateRule() {
	return new Rule;
}
void Grammar::addRule(Rule *rule, std::vector<Rule*> *where) {
	rule_by_line[rule->line] = rule;
	where->push_back(rule);
}
void Grammar::destroyRule(Rule *rule) {
	delete rule;
}

Tag *Grammar::allocateTag(const UChar *tag) {
	Tag *fresh = new Tag;
	fresh->parseTag(tag, ux_stderr);
	return fresh;
}
void Grammar::addTag(Tag *simpletag) {
	simpletag->in_grammar = true;
	if (simpletag && simpletag->tag) {
		simpletag->rehash();
		if (single_tags.find(simpletag->hash) != single_tags.end()) {
			if (u_strcmp(single_tags[simpletag->hash]->tag, simpletag->tag) != 0) {
				u_fprintf(ux_stderr, "Warning: Hash collision between %S and %S!\n", single_tags[simpletag->hash]->tag, simpletag->tag);
			}
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

void Grammar::setName(const char *to) {
	name = new UChar[strlen(to)+1];
	u_uastrcpy(name, to);
}

void Grammar::setName(const UChar *to) {
	name = new UChar[u_strlen(to)+1];
	u_strcpy(name, to);
}

void Grammar::trim() {
	set_alias.clear();
	sets_by_name.clear();

	stdext::hash_map<uint32_t, Tag*>::iterator iter_tags;
	for (iter_tags = single_tags.begin() ; iter_tags != single_tags.end() ; iter_tags++) {
		if (!iter_tags->second->type) {
			Tag *tag = iter_tags->second;
			tag->type &= ~T_TEXTUAL;
		}
	}
}

void Grammar::reindex() {
	stdext::hash_map<uint32_t, Tag*>::iterator iter_tags;
	for (iter_tags = single_tags.begin() ; iter_tags != single_tags.end() ; iter_tags++) {
		Tag *tag = iter_tags->second;
		if (tag->tag[0] == mapping_prefix) {
			tag->type |= T_MAPPING;
		}
		else {
			tag->type &= ~T_MAPPING;
		}
	}

	stdext::hash_map<uint32_t, Set*>::iterator iter_sets;
	for (iter_sets = sets_by_contents.begin() ; iter_sets != sets_by_contents.end() ; iter_sets++) {
		iter_sets->second->reindex(this);
		indexSets(iter_sets->first, iter_sets->second);
	}

	std::map<uint32_t, Rule*>::iterator iter_rule;
	for (iter_rule = rule_by_line.begin() ; iter_rule != rule_by_line.end() ; iter_rule++) {
		if (iter_rule->second->target) {
			indexSetToRule(iter_rule->second->line, getSet(iter_rule->second->target));
		}
	}
}

void Grammar::indexSetToRule(uint32_t r, Set *s) {
	if (s->is_special) {
		indexTagToRule(tag_any, r);
		return;
	}
	if (s->sets.empty()) {
		stdext::hash_set<uint32_t>::const_iterator comp_iter;
		for (comp_iter = s->single_tags.begin() ; comp_iter != s->single_tags.end() ; comp_iter++) {
			indexTagToRule(*comp_iter, r);
		}
		for (comp_iter = s->tags.begin() ; comp_iter != s->tags.end() ; comp_iter++) {
			if (tags.find(*comp_iter) != tags.end()) {
				CompositeTag *curcomptag = tags.find(*comp_iter)->second;
				if (curcomptag->tags.size() == 1) {
					indexTagToRule(*(curcomptag->tags.begin()), r);
				} else {
					std::set<uint32_t>::const_iterator tag_iter;
					for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
						indexTagToRule(*tag_iter, r);
					}
				}
			}
		}
	} else if (!s->sets.empty()) {
		for (uint32_t i=0;i<s->sets.size();i++) {
			Set *set = sets_by_contents.find(s->sets.at(i))->second;
			indexSetToRule(r, set);
		}
	}
}

void Grammar::indexTagToRule(uint32_t t, uint32_t r) {
	if (rules_by_tag.find(t) == rules_by_tag.end()) {
		std::pair<uint32_t,uint32HashSet*> p;
		p.first = t;
		p.second = new uint32HashSet;
		rules_by_tag.insert(p);
	}
	rules_by_tag.find(t)->second->insert(r);
}

void Grammar::indexSets(uint32_t r, Set *s) {
	if (s->is_special) {
		indexTagToSet(tag_any, r);
		return;
	}
	if (s->sets.empty()) {
		stdext::hash_set<uint32_t>::const_iterator comp_iter;
		for (comp_iter = s->single_tags.begin() ; comp_iter != s->single_tags.end() ; comp_iter++) {
			indexTagToSet(*comp_iter, r);
		}
		for (comp_iter = s->tags.begin() ; comp_iter != s->tags.end() ; comp_iter++) {
			if (tags.find(*comp_iter) != tags.end()) {
				CompositeTag *curcomptag = tags.find(*comp_iter)->second;
				if (curcomptag->tags.size() == 1) {
					indexTagToSet(*(curcomptag->tags.begin()), r);
				} else {
					std::set<uint32_t>::const_iterator tag_iter;
					for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
						indexTagToSet(*tag_iter, r);
					}
				}
			}
		}
	} else if (!s->sets.empty()) {
		for (uint32_t i=0;i<s->sets.size();i++) {
			Set *set = sets_by_contents.find(s->sets.at(i))->second;
			indexSets(r, set);
		}
	}
}

void Grammar::indexTagToSet(uint32_t t, uint32_t r) {
	if (sets_by_tag.find(t) == sets_by_tag.end()) {
		std::pair<uint32_t,uint32HashSet*> p;
		p.first = t;
		p.second = new uint32HashSet;
		sets_by_tag.insert(p);
	}
	sets_by_tag.find(t)->second->insert(r);
}
