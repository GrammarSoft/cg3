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

#include "Grammar.h"
#include "Strings.h"
#include "ContextualTest.h"
#include "Anchor.h"

namespace CG3 {

Grammar::Grammar() {
	has_dep = false;
	has_encl_final = false;
	is_binary = false;
	lines = 0;
	total_time = 0;
	delimiters = 0;
	soft_delimiters = 0;
	tag_any = 0;
	sets_any = 0;
	rules_any = 0;
	mapping_prefix = '@';
	ux_stderr = 0;
	ux_stdout = 0;
}

Grammar::~Grammar() {
	foreach (std::vector<Set*>, sets_list, iter_set, iter_set_end) {
		destroySet(*iter_set);
	}

	foreach (SetSet, sets_all, rsets, rsets_end) {
		delete (*rsets);
	}

	std::map<uint32_t, Anchor*>::iterator iter_anc;
	for (iter_anc = anchor_by_line.begin() ; iter_anc != anchor_by_line.end() ; iter_anc++) {
		if (iter_anc->second) {
			delete iter_anc->second;
		}
	}
	
	stdext::hash_map<uint32_t, CompositeTag*>::iterator iter_ctag;
	for (iter_ctag = tags.begin() ; iter_ctag != tags.end() ; iter_ctag++) {
		if (iter_ctag->second) {
			delete iter_ctag->second;
		}
	}

	Taguint32HashMap::iterator iter_stag;
	for (iter_stag = single_tags.begin() ; iter_stag != single_tags.end() ; iter_stag++) {
		if (iter_stag->second) {
			delete iter_stag->second;
		}
	}

	foreach (RuleVector, rules, iter_rules1, iter_rules1_end) {
		if (*iter_rules1) {
			delete *iter_rules1;
		}
	}

	foreach (RuleVector, before_sections, iter_rules2, iter_rules2_end) {
		if (*iter_rules2) {
			delete *iter_rules2;
		}
	}

	foreach (RuleVector, after_sections, iter_rules3, iter_rules3_end) {
		if (*iter_rules3) {
			delete *iter_rules3;
		}
	}

	foreach (RuleVector, null_section, iter_rules4, iter_rules4_end) {
		if (*iter_rules4) {
			delete *iter_rules4;
		}
	}

	foreach (std::vector<ContextualTest*>, template_list, tmpls, tmpls_end) {
		delete (*tmpls);
	}
}

void Grammar::addPreferredTarget(UChar *to) {
	Tag *tag = allocateTag(to);
	preferred_targets.push_back(tag->hash);
}
void Grammar::addSet(Set *to) {
	if (!delimiters && u_strcmp(to->name.c_str(), stringbits[S_DELIMITSET].getTerminatedBuffer()) == 0) {
		delimiters = to;
	}
	else if (!soft_delimiters && u_strcmp(to->name.c_str(), stringbits[S_SOFTDELIMITSET].getTerminatedBuffer()) == 0) {
		soft_delimiters = to;
	}
	if (to->name[0] == 'T' && to->name[1] == ':') {
		u_fprintf(ux_stderr, "Warning: Set name %S looks like a misattempt of template usage on line %u.\n", to->name.c_str(), to->line);
	}
	uint32_t chash = to->rehash();
	if (to->name[0] != '_' || to->name[1] != 'G' || to->name[2] != '_') {
		uint32_t nhash = hash_sdbm_uchar(to->name.c_str());
		if (set_name_seeds.find(nhash) != set_name_seeds.end()) {
			nhash += set_name_seeds[nhash];
		}
		if (sets_by_name.find(nhash) == sets_by_name.end()) {
			sets_by_name[nhash] = chash;
		}
		else if (chash != sets_by_contents.find(sets_by_name.find(nhash)->second)->second->hash) {
			Set *a = sets_by_contents.find(sets_by_name.find(nhash)->second)->second;
			if (a->name == to->name) {
				u_fprintf(ux_stderr, "Error: Set %S already defined at line %u. Redefinition attempted at line %u!\n", a->name.c_str(), a->line, to->line);
				CG3Quit(1);
			}
			else {
				for (uint32_t seed=0 ; seed<1000 ; seed++) {
					if (sets_by_name.find(nhash+seed) == sets_by_name.end()) {
						u_fprintf(ux_stderr, "Warning: Set %S got hash seed %u.\n", to->name.c_str(), seed);
						u_fflush(ux_stderr);
						set_name_seeds[nhash] = seed;
						sets_by_name[nhash+seed] = chash;
						break;
					}
				}
			}
		}
	}
	if (sets_by_contents.find(chash) == sets_by_contents.end()) {
		sets_by_contents[chash] = to;
	}
	else {
		Set *a = sets_by_contents.find(chash)->second;
		if (a->is_special != to->is_special || a->is_tag_unified != to->is_tag_unified || a->is_child_unified != to->is_child_unified
		|| a->is_set_unified != to->is_set_unified
		|| a->set_ops.size() != to->set_ops.size() || a->sets.size() != to->sets.size()
		|| a->single_tags.size() != to->single_tags.size() || a->tags.size() != to->tags.size()) {
			u_fprintf(ux_stderr, "Error: Content hash collision between set %S line %u and %S line %u!\n", a->name.c_str(), a->line, to->name.c_str(), to->line);
			CG3Quit(1);
		}
	}
}
Set *Grammar::getSet(uint32_t which) {
	Set *retval = 0;
	if (sets_by_contents.find(which) != sets_by_contents.end()) {
		retval = sets_by_contents[which];
	}
	else if (sets_by_name.find(which) != sets_by_name.end()) {
		if (set_name_seeds.find(which) != set_name_seeds.end()) {
			retval = getSet(sets_by_name[which+set_name_seeds[which]]);
		}
		else {
			retval = getSet(sets_by_name[which]);
		}
	}
	return retval;
}

Set *Grammar::allocateSet() {
	Set *ns = new Set;
	sets_all.insert(ns);
	return ns;
}
void Grammar::destroySet(Set *set) {
	sets_all.erase(set);
	delete set;
}
void Grammar::addSetToList(Set *s) {
	if (s->number == 0) {
		if (sets_list.empty() || sets_list.at(0) != s) {
			if (!s->sets.empty()) {
				foreach (uint32Vector, s->sets, sit, sit_end) {
					addSetToList(getSet(*sit));
				}
			}
			sets_list.push_back(s);
			s->number = (uint32_t)sets_list.size()-1;
		}
	}
}

CompositeTag *Grammar::addCompositeTag(CompositeTag *tag) {
	if (tag && tag->tags.size()) {
		tag->rehash();
		if (tags.find(tag->hash) != tags.end()) {
			uint32_t ct = tag->hash;
			delete tag;
			tag = tags[ct];
		}
		else {
			tags[tag->hash] = tag;
			tags_list.push_back(tag);
			tag->number = (uint32_t)tags_list.size()-1;
		}
	}
	else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty composite tag to grammar on line %u!\n", lines);
		CG3Quit(1);
	}
	return tags[tag->hash];
}
CompositeTag *Grammar::addCompositeTagToSet(Set *set, CompositeTag *tag) {
	if (tag && tag->tags.size()) {
		if (tag->tags.size() == 1) {
			Tag *rtag = *(tag->tags.begin());
			addTagToSet(rtag, set);
			delete tag;
			tag = 0;
		}
		else {
			tag = addCompositeTag(tag);
			set->tags_set.insert(tag->hash);
			set->tags.insert(tag);
			if (tag->is_special) {
				set->is_special = true;
			}
		}
	}
	else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty composite tag to grammar and set on line %u!\n", lines);
		CG3Quit(1);
	}
	return tag;
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
void Grammar::addRule(Rule *rule) {
	if (rule_by_line.find(rule->line) != rule_by_line.end()) {
		u_fprintf(ux_stderr, "Error: Multiple rules defined on line %u - cannot currently handle multiple rules per line!\n", rule->line);
		CG3Quit(1);
	}
	rule_by_line[rule->line] = rule;
}
void Grammar::destroyRule(Rule *rule) {
	delete rule;
}

Tag *Grammar::allocateTag() {
	return new Tag;
}
Tag *Grammar::allocateTag(const UChar *txt, bool raw) {
	Taguint32HashMap::iterator it;
	uint32_t thash = hash_sdbm_uchar(txt);
	if ((it = single_tags.find(thash)) != single_tags.end() && it->second->tag && u_strcmp(it->second->tag, txt) == 0) {
		return it->second;
	}

	Tag *tag = new Tag();
	if (raw) {
		tag->parseTagRaw(txt);
	}
	else {
		tag->parseTag(txt, ux_stderr);
	}
	tag->in_grammar = true;
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
			if (seed) {
				u_fprintf(ux_stderr, "Warning: Tag %S got hash seed %u.\n", txt, seed);
				u_fflush(ux_stderr);
			}
			tag->seed = seed;
			hash = tag->rehash();
			single_tags_list.push_back(tag);
			tag->number = (uint32_t)single_tags_list.size()-1;
			single_tags[hash] = tag;
			break;
		}
	}
	return single_tags[hash];
}
void Grammar::addTagToCompositeTag(Tag *simpletag, CompositeTag *tag) {
	if (simpletag && simpletag->tag) {
		tag->addTag(simpletag);
	}
	else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty tag to grammar and composite tag on line %u!\n", lines);
		CG3Quit(1);
	}
}
void Grammar::addTagToSet(Tag *rtag, Set *set) {
	set->tags_set.insert(rtag->hash);
	set->single_tags.insert(rtag);
	set->single_tags_hash.insert(rtag->hash);

	if (rtag->type & T_ANY) {
		set->match_any = true;
	}
	if (rtag->is_special) {
		set->is_special = true;
	}
	if (rtag->type & T_FAILFAST) {
		set->ff_tags.insert(rtag);
		set->ff_tags_hash.insert(rtag->hash);
	}
}
void Grammar::destroyTag(Tag *tag) {
	delete tag;
}

ContextualTest *Grammar::allocateContextualTest() {
	return new ContextualTest;
}
void Grammar::addContextualTest(ContextualTest *test, const UChar *name) {
	uint32_t cn = hash_sdbm_uchar(name);
	if (templates.find(cn) != templates.end()) {
		u_fprintf(ux_stderr, "Error: Redefinition attempt for template '%S' on line %u!\n", name, lines);
		CG3Quit(1);
	}
	templates[cn] = test;
	template_list.push_back(test);
}

void Grammar::addAnchor(const UChar *to, uint32_t line) {
	uint32_t ah = hash_sdbm_uchar(to);
	if (anchor_by_hash.find(ah) != anchor_by_hash.end()) {
		u_fprintf(ux_stderr, "Error: Redefinition attempt for anchor '%S' on line %u!\n", to, line);
		CG3Quit(1);
	}
	Anchor *anc = new Anchor;
	anc->setName(to);
	anc->line = line;
	anchor_by_hash[ah] = line;
	anchor_by_line[line] = anc;
}

void Grammar::resetStatistics() {
	total_time = 0;
	for (uint32_t j=0;j<rules.size();j++) {
		rules[j]->resetStatistics();
	}
}

void Grammar::renameAllRules() {
	foreach (RuleByLineHashMap, rule_by_line, iter_rule, iter_rule_end) {
		Rule *r = iter_rule->second;
		gbuffers[0][0] = 0;
		u_sprintf(&gbuffers[0][0], "L%u", r->line);
		r->setName(&gbuffers[0][0]);
	}
};

void Grammar::reindex(bool unused_sets) {
	set_alias.clear();
	sets_by_name.clear();
	rules.clear();
	before_sections.clear();
	after_sections.clear();
	null_section.clear();
	sections.clear();
	sets_list.clear();
	sets_any = 0;
	rules_any = 0;

	foreach (Setuint32HashMap, sets_by_contents, dset, dset_end) {
		dset->second->is_used = false;
		dset->second->number = 0;
	}

	RuleByLineMap rule_by_line;
	rule_by_line.insert(this->rule_by_line.begin(), this->rule_by_line.end());
	foreach (RuleByLineMap, rule_by_line, iter_rule, iter_rule_end) {
		Set *s = 0;
		s = getSet(iter_rule->second->target);
		s->markUsed(*this);
		if (iter_rule->second->dep_target) {
			iter_rule->second->dep_target->markUsed(*this);
		}
		ContextualTest *test = iter_rule->second->dep_test_head;
		while (test) {
			test->markUsed(*this);
			test = test->next;
		}
		test = iter_rule->second->test_head;
		while (test) {
			test->markUsed(*this);
			test = test->next;
		}
	}
	if (delimiters) {
		delimiters->markUsed(*this);
	}
	if (soft_delimiters) {
		soft_delimiters->markUsed(*this);
	}

	// This is only necessary due to binary grammars.
	// Sets used in unused templates may otherwise crash the loading of a binary grammar.
	foreach (std::vector<ContextualTest*>, template_list, tmpls, tmpls_end) {
		(*tmpls)->markUsed(*this);
	}

	if (unused_sets) {
		u_fprintf(ux_stdout, "Unused sets:\n");
		foreach (Setuint32HashMap, sets_by_contents, rset, rset_end) {
			if (!rset->second->is_used && !rset->second->name.empty()) {
				if (rset->second->name[0] != '_' || rset->second->name[1] != 'G' || rset->second->name[2] != '_') {
					u_fprintf(ux_stdout, "Line %u set %S\n", rset->second->line, rset->second->name.c_str());
				}
			}
		}
		u_fprintf(ux_stdout, "End of unused sets.\n");
		u_fflush(ux_stdout);
	}

	/*
	// ToDo: Actually destroying the data still needs more work to determine what is safe to kill off.

	foreach (Setuint32HashMap, sets_by_contents, rset, rset_end) {
		if (!rset->second->is_used) {
			destroySet(rset->second);
			rset->second = 0;
		}
	}

	sets_by_contents.clear();
	foreach (std::set<Set*>, sets_all, sall, sall_end) {
		sets_by_contents[(*sall)->hash] = *sall;
	}

	std::vector<CompositeTag*> ctmp;
	foreach (std::vector<CompositeTag*>, tags_list, citer, citer_end) {
		if ((*citer)->is_used) {
			ctmp.push_back(*citer);
		}
		else {
			destroyCompositeTag(*citer);
			*citer = 0;
		}
	}
	tags_list.clear();
	tags.clear();
	foreach (std::vector<CompositeTag*>, ctmp, cniter, cniter_end) {
		addCompositeTag(*cniter);
	}

	std::vector<Tag*> stmp;
	foreach (std::vector<Tag*>, single_tags_list, siter, siter_end) {
		if ((*siter)->is_used) {
			stmp.push_back(*siter);
		}
		else {
			destroyTag(*siter);
			*siter = 0;
		}
	}
	single_tags_list.clear();
	single_tags.clear();
	foreach (std::vector<Tag*>, stmp, sniter, sniter_end) {
		addTag(*sniter);
	}

	//*/

	// Stuff below this line is not optional...

	size_t num_sets = 0, num_lists = 0;
	size_t num_is = 0, num_it = 0;
	size_t max_is = 0, max_it = 0;
	std::map<size_t,size_t> cnt_is, cnt_it;

	foreach (Setuint32HashMap, sets_by_contents, tset, tset_end) {
		if (tset->second->is_used) {
			addSetToList(tset->second);
			if (tset->second->sets.empty()) {
				++num_lists;
				num_it += tset->second->tags_set.size();
				max_it = std::max(max_it, tset->second->tags_set.size());
				cnt_it[tset->second->tags_set.size()]++;
			}
			else {
				++num_sets;
				num_is += tset->second->sets.size();
				max_is = std::max(max_is, tset->second->sets.size());
				cnt_is[tset->second->sets.size()]++;
			}
		}
	}

	Taguint32HashMap::iterator iter_tags;
	for (iter_tags = single_tags.begin() ; iter_tags != single_tags.end() ; iter_tags++) {
		Tag *tag = iter_tags->second;
		if (tag->tag[0] == mapping_prefix) {
			tag->type |= T_MAPPING;
		}
		else {
			tag->type &= ~T_MAPPING;
		}
	}

	foreach (Setuint32HashMap, sets_by_contents, iter_sets, iter_sets_end) {
		if (iter_sets->second->is_used) {
			iter_sets->second->reindex(*this);
			indexSets(iter_sets->first, iter_sets->second);
		}
	}

	uint32Set sects;

	for (iter_rule = rule_by_line.begin() ; iter_rule != rule_by_line.end() ; iter_rule++) {
		if (iter_rule->second->section == -1) {
			before_sections.push_back(iter_rule->second);
		}
		else if (iter_rule->second->section == -2) {
			after_sections.push_back(iter_rule->second);
		}
		else if (iter_rule->second->section == -3) {
			null_section.push_back(iter_rule->second);
		}
		else {
			sects.insert(iter_rule->second->section);
			rules.push_back(iter_rule->second);
		}
		if (iter_rule->second->target) {
			indexSetToRule(iter_rule->second->line, getSet(iter_rule->second->target));
			rules_by_set[iter_rule->second->target].insert(iter_rule->first);
		}
		else {
			u_fprintf(ux_stderr, "Warning: Rule on line %u had no target.\n", iter_rule->second->line);
			u_fflush(ux_stderr);
		}
	}

	sections.insert(sections.end(), sects.begin(), sects.end());

	if (sets_by_tag.find(tag_any) != sets_by_tag.end()) {
		sets_any = &sets_by_tag[tag_any];
	}
	if (rules_by_tag.find(tag_any) != rules_by_tag.end()) {
		rules_any = &rules_by_tag[tag_any];
	}
}

void Grammar::indexSetToRule(uint32_t r, Set *s) {
	if (s->is_special || s->is_tag_unified) {
		indexTagToRule(tag_any, r);
		return;
	}
	if (s->sets.empty()) {
		TagHashSet::const_iterator tomp_iter;
		for (tomp_iter = s->single_tags.begin() ; tomp_iter != s->single_tags.end() ; tomp_iter++) {
			indexTagToRule((*tomp_iter)->hash, r);
		}
		CompositeTagHashSet::const_iterator comp_iter;
		for (comp_iter = s->tags.begin() ; comp_iter != s->tags.end() ; comp_iter++) {
			CompositeTag *curcomptag = *comp_iter;
			if (curcomptag->tags.size() == 1) {
				indexTagToRule((*(curcomptag->tags.begin()))->hash, r);
			}
			else {
				TagSet::const_iterator tag_iter;
				for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
					indexTagToRule((*tag_iter)->hash, r);
				}
			}
		}
	}
	else {
		for (uint32_t i=0;i<s->sets.size();i++) {
			Set *set = sets_by_contents.find(s->sets.at(i))->second;
			indexSetToRule(r, set);
		}
	}
}

void Grammar::indexTagToRule(uint32_t t, uint32_t r) {
	rules_by_tag[t].insert(r);
}

void Grammar::indexSets(uint32_t r, Set *s) {
	if (s->is_special || s->is_tag_unified) {
		indexTagToSet(tag_any, r);
		return;
	}
	if (s->sets.empty()) {
		TagHashSet::const_iterator tomp_iter;
		for (tomp_iter = s->single_tags.begin() ; tomp_iter != s->single_tags.end() ; tomp_iter++) {
			indexTagToSet((*tomp_iter)->hash, r);
		}
		CompositeTagHashSet::const_iterator comp_iter;
		for (comp_iter = s->tags.begin() ; comp_iter != s->tags.end() ; comp_iter++) {
			CompositeTag *curcomptag = *comp_iter;
			if (curcomptag->tags.size() == 1) {
				indexTagToSet((*(curcomptag->tags.begin()))->hash, r);
			}
			else {
				TagSet::const_iterator tag_iter;
				for (tag_iter = curcomptag->tags_set.begin() ; tag_iter != curcomptag->tags_set.end() ; tag_iter++) {
					indexTagToSet((*tag_iter)->hash, r);
				}
			}
		}
	}
	else {
		for (uint32_t i=0;i<s->sets.size();i++) {
			Set *set = sets_by_contents.find(s->sets.at(i))->second;
			indexSets(r, set);
		}
	}
}

void Grammar::indexTagToSet(uint32_t t, uint32_t r) {
	sets_by_tag[t].insert(r);
}

}
