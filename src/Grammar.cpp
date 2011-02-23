/*
* Copyright (C) 2007-2011, GrammarSoft ApS
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

Grammar::Grammar() :
ux_stderr(0),
ux_stdout(0),
has_dep(false),
has_encl_final(false),
is_binary(false),
grammar_size(0),
mapping_prefix('@'),
lines(0),
verbosity_level(0),
total_time(0),
rules_any(0),
sets_any(0),
delimiters(0),
soft_delimiters(0),
tag_any(0)
{
	// Nothing in the actual body...
}

Grammar::~Grammar() {
	foreach (std::vector<Set*>, sets_list, iter_set, iter_set_end) {
		destroySet(*iter_set);
	}

	foreach (SetSet, sets_all, rsets, rsets_end) {
		delete *rsets;
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
		delete *tmpls;
	}
}

void Grammar::addPreferredTarget(UChar *to) {
	Tag *tag = allocateTag(to);
	preferred_targets.push_back(tag->hash);
}

void Grammar::addSet(Set *& to) {
	if (!delimiters && u_strcmp(to->name.c_str(), stringbits[S_DELIMITSET].getTerminatedBuffer()) == 0) {
		delimiters = to;
	}
	else if (!soft_delimiters && u_strcmp(to->name.c_str(), stringbits[S_SOFTDELIMITSET].getTerminatedBuffer()) == 0) {
		soft_delimiters = to;
	}
	if (verbosity_level > 0 && to->name[0] == 'T' && to->name[1] == ':') {
		u_fprintf(ux_stderr, "Warning: Set name %S looks like a misattempt of template usage on line %u.\n", to->name.c_str(), to->line);
	}

	// If there are failfast tags, and if they don't comprise the whole of the set, split the set into Positive - Negative
	if (!to->ff_tags.empty() && to->ff_tags.size() < to->tags_list.size()) {
		Set *positive = allocateSet(to);
		Set *negative = allocateSet();

		UString str;
		str = stringbits[S_GPREFIX].getTerminatedBuffer();
		str += to->name;
		str += '_';
		str += stringbits[S_POSITIVE].getTerminatedBuffer();
		positive->setName(str);
		str = stringbits[S_GPREFIX].getTerminatedBuffer();
		str += to->name;
		str += '_';
		str += stringbits[S_NEGATIVE].getTerminatedBuffer();
		negative->setName(str);

		TagSet tags;
		foreach (TagHashSet, to->ff_tags, iter, iter_end) {
			for (AnyTagVector::iterator ater = positive->tags_list.begin() ; ater != positive->tags_list.end() ; ) {
				if (ater->hash() == (*iter)->hash) {
					ater = positive->tags_list.erase(ater);
				}
				else {
					++ater;
				}
			}
			positive->single_tags.erase(*iter);
			positive->single_tags_hash.erase((*iter)->hash);
			UString str = (*iter)->toUString(true);
			str.erase(str.find('^'), 1);
			Tag *tag = allocateTag(str.c_str());
			addTagToSet(tag, negative);
		}
		positive->ff_tags.clear();

		positive->reindex(*this);
		negative->reindex(*this);
		addSet(positive);
		addSet(negative);

		to->tags.clear();
		to->tags_list.clear();
		to->single_tags.clear();
		to->single_tags_hash.clear();
		to->ff_tags.clear();

		to->sets.push_back(positive->hash);
		to->sets.push_back(negative->hash);
		to->set_ops.push_back(S_MINUS);

		to->reindex(*this);
		if (verbosity_level > 1) {
			u_fprintf(ux_stderr, "Info: LIST %S on line %u was split into two sets.\n", to->name.c_str(), to->line);
			u_fflush(ux_stderr);
		}
	}

	uint32_t chash = to->rehash();
	for (;;) {
		uint32_t nhash = hash_sdbm_uchar(to->name.c_str());
		if (sets_by_name.find(nhash) != sets_by_name.end()) {
			Set *a = sets_by_contents.find(sets_by_name.find(nhash)->second)->second;
			if (a == to || a->hash == to->hash) {
				break;
			}
		}
		if (set_name_seeds.find(nhash) != set_name_seeds.end()) {
			nhash += set_name_seeds[nhash];
		}
		if (sets_by_name.find(nhash) == sets_by_name.end()) {
			sets_by_name[nhash] = chash;
			break;
		}
		else if (chash != sets_by_contents.find(sets_by_name.find(nhash)->second)->second->hash) {
			Set *a = sets_by_contents.find(sets_by_name.find(nhash)->second)->second;
			if (a->name == to->name) {
				u_fprintf(ux_stderr, "Error: Set %S already defined at line %u. Redefinition attempted at line %u!\n", a->name.c_str(), a->line, to->line);
				CG3Quit(1);
			}
			else {
				for (uint32_t seed=0 ; seed<1000 ; ++seed) {
					if (sets_by_name.find(nhash+seed) == sets_by_name.end()) {
						if (verbosity_level > 0 && (to->name[0] != '_' || to->name[1] != 'G' || to->name[2] != '_')) {
							u_fprintf(ux_stderr, "Warning: Set %S got hash seed %u.\n", to->name.c_str(), seed);
							u_fflush(ux_stderr);
						}
						set_name_seeds[nhash] = seed;
						sets_by_name[nhash+seed] = chash;
						break;
					}
				}
			}
		}
		break;
	}
	if (sets_by_contents.find(chash) == sets_by_contents.end()) {
		sets_by_contents[chash] = to;
	}
	else {
		Set *a = sets_by_contents.find(chash)->second;
		if (a != to) {
			if ((a->type & (ST_SPECIAL|ST_TAG_UNIFY|ST_CHILD_UNIFY|ST_SET_UNIFY)) != (to->type & (ST_SPECIAL|ST_TAG_UNIFY|ST_CHILD_UNIFY|ST_SET_UNIFY))
			|| a->set_ops.size() != to->set_ops.size() || a->sets.size() != to->sets.size()
			|| a->single_tags.size() != to->single_tags.size() || a->tags.size() != to->tags.size()) {
				u_fprintf(ux_stderr, "Error: Content hash collision between set %S line %u and %S line %u!\n", a->name.c_str(), a->line, to->name.c_str(), to->line);
				CG3Quit(1);
			}
			destroySet(to);
		}
	}
	to = sets_by_contents[chash];
}

Set *Grammar::getSet(uint32_t which) const {
	Setuint32HashMap::const_iterator iter = sets_by_contents.find(which);
	if (iter != sets_by_contents.end()) {
		return iter->second;
	}
	else {
		uint32HashMap::const_iterator iter = sets_by_name.find(which);
		if (iter != sets_by_name.end()) {
			uint32HashMap::const_iterator iter2 = set_name_seeds.find(which);
			if (iter2 != set_name_seeds.end()) {
				return getSet(iter->second + iter2->second);
			}
			else {
				return getSet(iter->second);
			}
		}
	}
	return 0;
}

Set *Grammar::allocateSet(Set *from) {
	Set *ns = 0;
	if (from) {
		ns = new Set(*from);
	}
	else {
		ns = new Set;
	}
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

Set *Grammar::parseSet(const UChar *name) {
	uint32_t sh = hash_sdbm_uchar(name);

	if (ux_isSetOp(name) != S_IGNORE) {
		u_fprintf(ux_stderr, "Error: Found set operator '%S' where set name expected on line %u!\n", name, lines);
		CG3Quit(1);
	}

	if ((
	(name[0] == '$' && name[1] == '$')
	|| (name[0] == '&' && name[1] == '&')
	) && name[2]) {
		const UChar *wname = &(name[2]);
		uint32_t wrap = hash_sdbm_uchar(wname);
		Set *wtmp = getSet(wrap);
		if (!wtmp) {
			u_fprintf(ux_stderr, "Error: Attempted to reference undefined set '%S' on line %u!\n", wname, lines);
			CG3Quit(1);
		}
		Set *tmp = getSet(sh);
		if (!tmp) {
			Set *ns = allocateSet();
			ns->line = lines;
			ns->setName(name);
			ns->sets.push_back(wtmp->hash);
			if (name[0] == '$' && name[1] == '$') {
				ns->type |= ST_TAG_UNIFY;
			}
			else if (name[0] == '&' && name[1] == '&') {
				ns->type |= ST_SET_UNIFY;
			}
			addSet(ns);
		}
	}
	if (set_alias.find(sh) != set_alias.end()) {
		sh = set_alias[sh];
	}
	Set *tmp = getSet(sh);
	if (!tmp) {
		u_fprintf(ux_stderr, "Error: Attempted to reference undefined set '%S' on line %u!\n", name, lines);
		CG3Quit(1);
	}
	return tmp;
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
			set->tags_list.push_back(tag);
			set->tags.insert(tag);
			if (tag->is_special) {
				set->type |= ST_SPECIAL;
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
	if (txt[0] == '(') {
		u_fprintf(ux_stderr, "Error: Tag '%S' cannot start with ( on line %u! Possible extra opening ( or missing closing ) to the left. If you really meant it, escape it as \\(.\n", txt, lines);
		CG3Quit(1);
	}
	Taguint32HashMap::iterator it;
	uint32_t thash = hash_sdbm_uchar(txt);
	if ((it = single_tags.find(thash)) != single_tags.end() && !it->second->tag.empty() && u_strcmp(it->second->tag.c_str(), txt) == 0) {
		return it->second;
	}

	Tag *tag = new Tag();
	if (raw) {
		tag->parseTagRaw(txt);
	}
	else {
		tag->parseTag(txt, ux_stderr, this);
	}
	tag->type |= T_GRAMMAR;
	uint32_t hash = tag->rehash();
	uint32_t seed = 0;
	for ( ; seed < 10000 ; seed++) {
		uint32_t ih = hash + seed;
		if ((it = single_tags.find(ih)) != single_tags.end()) {
			Tag *t = it->second;
			if (t->tag == tag->tag) {
				hash += seed;
				delete tag;
				break;
			}
		}
		else {
			if (verbosity_level > 0 && seed) {
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
	if (simpletag && !simpletag->tag.empty()) {
		tag->addTag(simpletag);
	}
	else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty tag to grammar and composite tag on line %u!\n", lines);
		CG3Quit(1);
	}
}

void Grammar::addTagToSet(Tag *rtag, Set *set) {
	set->tags_list.push_back(rtag);
	set->single_tags.insert(rtag);
	set->single_tags_hash.insert(rtag->hash);

	if (rtag->type & T_ANY) {
		set->type |= ST_ANY;
	}
	if (rtag->type & T_SPECIAL) {
		set->type |= ST_SPECIAL;
	}
	if (rtag->type & T_FAILFAST) {
		set->ff_tags.insert(rtag);
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
	foreach (Setuint32HashMap, sets_by_contents, dset, dset_end) {
		dset->second->type &= ~ST_USED;
		dset->second->number = 0;
	}

	foreach (static_sets_t, static_sets, sset, sset_end) {
		uint32_t sh = hash_sdbm_uchar(*sset);
		if (set_alias.find(sh) != set_alias.end()) {
			u_fprintf(ux_stderr, "Error: Static set %S is an alias; only real sets may be made static!\n", (*sset).c_str());
			CG3Quit(1);
		}
		Set *s = getSet(sh);
		if (!s) {
			if (verbosity_level > 0) {
				u_fprintf(ux_stderr, "Warning: Set %S was not defined, so cannot make it static.\n", (*sset).c_str());
			}
			continue;
		}
		if (s->name != *sset) {
			s->setName(*sset);
		}
		s->markUsed(*this);
		s->type |= ST_STATIC;
	}

	set_alias.clear();
	sets_by_name.clear();
	rules.clear();
	before_sections.clear();
	after_sections.clear();
	null_section.clear();
	sections.clear();
	sets_list.clear();
	set_name_seeds.clear();
	sets_any = 0;
	rules_any = 0;

	foreach (Setuint32HashMap, sets_by_contents, dset, dset_end) {
		Set *to = dset->second;
		if (to->type & ST_STATIC) {
			uint32_t nhash = hash_sdbm_uchar(to->name);
			const uint32_t chash = to->hash;

			if (sets_by_name.find(nhash) == sets_by_name.end()) {
				sets_by_name[nhash] = chash;
			}
			else if (chash != sets_by_contents.find(sets_by_name.find(nhash)->second)->second->hash) {
				Set *a = sets_by_contents.find(sets_by_name.find(nhash)->second)->second;
				if (a->name == to->name) {
					u_fprintf(ux_stderr, "Error: Static set %S already defined. Redefinition attempted!\n", a->name.c_str());
					CG3Quit(1);
				}
				else {
					for (uint32_t seed=0 ; seed<1000 ; ++seed) {
						if (sets_by_name.find(nhash+seed) == sets_by_name.end()) {
							if (verbosity_level > 0) {
								u_fprintf(ux_stderr, "Warning: Static set %S got hash seed %u.\n", to->name.c_str(), seed);
								u_fflush(ux_stderr);
							}
							set_name_seeds[nhash] = seed;
							sets_by_name[nhash+seed] = chash;
							break;
						}
					}
				}
			}
		}
	}

	foreach (TagVector, single_tags_list, iter, iter_end) {
		if (!(*iter)->vs_sets) {
			continue;
		}
		foreach (SetVector, *(*iter)->vs_sets, sit, sit_end) {
			(*sit)->markUsed(*this);
		}
	}

	RuleByLineMap rule_by_line;
	rule_by_line.insert(this->rule_by_line.begin(), this->rule_by_line.end());
	foreach (RuleByLineMap, rule_by_line, iter_rule, iter_rule_end) {
		Set *s = 0;
		s = getSet(iter_rule->second->target);
		s->markUsed(*this);
		if (iter_rule->second->childset1) {
			s = getSet(iter_rule->second->childset1);
			s->markUsed(*this);
		}
		if (iter_rule->second->childset2) {
			s = getSet(iter_rule->second->childset2);
			s->markUsed(*this);
		}
		if (iter_rule->second->maplist) {
			iter_rule->second->maplist->markUsed(*this);
		}
		if (iter_rule->second->sublist) {
			iter_rule->second->sublist->markUsed(*this);
		}
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
			if (!(rset->second->type & ST_USED) && !rset->second->name.empty()) {
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
		if (tset->second->type & ST_USED) {
			addSetToList(tset->second);
			if (tset->second->sets.empty()) {
				++num_lists;
				num_it += tset->second->tags_list.size();
				max_it = std::max(max_it, tset->second->tags_list.size());
				cnt_it[tset->second->tags_list.size()]++;
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
		if (iter_sets->second->type & ST_USED) {
			iter_sets->second->reindex(*this);
			indexSets(iter_sets->first, iter_sets->second);
		}
	}

	uint32Set sects;

	foreach (RuleByLineMap, rule_by_line, iter_rule, iter_rule_end) {
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
	if (s->type & (ST_SPECIAL|ST_TAG_UNIFY)) {
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
				// ToDo: If this is ever run, something has gone wrong...
				indexTagToRule((*(curcomptag->tags.begin()))->hash, r);
			}
			else {
				const_foreach (TagList, curcomptag->tags, tag_iter, tag_iter_end) {
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
	if (s->type & (ST_SPECIAL|ST_TAG_UNIFY)) {
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
				// ToDo: If this is ever run, something has gone wrong...
				indexTagToSet((*(curcomptag->tags.begin()))->hash, r);
			}
			else {
				const_foreach (TagList, curcomptag->tags, tag_iter, tag_iter_end) {
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
