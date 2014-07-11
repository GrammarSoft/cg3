/*
* Copyright (C) 2007-2014, GrammarSoft ApS
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

#include "Grammar.hpp"
#include "Strings.hpp"
#include "ContextualTest.hpp"

namespace CG3 {

Grammar::Grammar() :
ux_stderr(0),
ux_stdout(0),
has_dep(false),
has_relations(false),
has_encl_final(false),
is_binary(false),
sub_readings_ltr(false),
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
	
	stdext::hash_map<uint32_t, CompositeTag*>::iterator iter_ctag;
	for (iter_ctag = tags.begin() ; iter_ctag != tags.end() ; iter_ctag++) {
		if (iter_ctag->second) {
			delete iter_ctag->second;
		}
	}

	Taguint32HashMap::iterator iter_stag;
	for (iter_stag = single_tags.begin() ; iter_stag != single_tags.end() ; ++iter_stag) {
		if (iter_stag->second) {
			delete iter_stag->second;
		}
	}

	foreach (RuleVector, rule_by_number, iter_rules, iter_rules_end) {
		delete *iter_rules;
	}

	foreach (ContextVector, contexts_list, it, it_end) {
		delete *it;
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

	if (!to->sets.empty() && !(to->type & (ST_TAG_UNIFY|ST_CHILD_UNIFY|ST_SET_UNIFY))) {
		bool all_tags = true;
		for (size_t i=0 ; i<to->sets.size() ; ++i) {
			if (i > 0 && to->set_ops[i-1] != S_OR) {
				all_tags = false;
				break;
			}
			Set *s = getSet(to->sets[i]);
			if (!s->sets.empty() || s->tags_list.size() != 1) {
				all_tags = false;
				break;
			}
		}

		if (all_tags) {
			for (size_t i=0 ; i<to->sets.size() ; ++i) {
				Set *s = getSet(to->sets[i]);
				for (size_t j=0 ; j<s->tags_list.size() ; ++j) {
					if (s->tags_list[j].which == ANYTAG_TAG) {
						addTagToSet(s->tags_list[j].getTag(), to);
					}
					else {
						addCompositeTagToSet(to, s->tags_list[j].getCompositeTag());
					}
				}
			}
			to->sets.clear();
			to->set_ops.clear();

			to->reindex(*this);
			if (verbosity_level > 1 && (to->name[0] != '_' || to->name[1] != 'G' || to->name[2] != '_')) {
				u_fprintf(ux_stderr, "Info: SET %S on line %u changed to a LIST.\n", to->name.c_str(), to->line);
				u_fflush(ux_stderr);
			}
		}
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

		boost_foreach (Tag *iter, to->ff_tags) {
			for (AnyTagVector::iterator ater = positive->tags_list.begin() ; ater != positive->tags_list.end() ; ) {
				if (ater->hash() == iter->hash) {
					ater = positive->tags_list.erase(ater);
				}
				else {
					++ater;
				}
			}
			positive->single_tags.erase(iter);
			positive->single_tags_hash.erase(iter->hash);
			UString str = iter->toUString(true);
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
	for (; to->name[0] != '_' || to->name[1] != 'G' || to->name[2] != '_' ;) {
		uint32_t nhash = hash_value(to->name.c_str());
		if (sets_by_name.find(nhash) != sets_by_name.end()) {
			Set *a = sets_by_contents.find(sets_by_name.find(nhash)->second)->second;
			if (a == to || a->hash == to->hash) {
				break;
			}
		}
		if (set_name_seeds.find(to->name) != set_name_seeds.end()) {
			nhash += set_name_seeds[to->name];
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
						set_name_seeds[to->name] = seed;
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
				u_fprintf(ux_stderr, "Error: Content hash collision between set %S on line %u and %S on line %u!\n", a->name.c_str(), a->line, to->name.c_str(), to->line);
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
		uint32FlatHashMap::const_iterator iter = sets_by_name.find(which);
		if (iter != sets_by_name.end()) {
			Setuint32HashMap::const_iterator citer = sets_by_contents.find(iter->second);
			if (citer != sets_by_contents.end()) {
				set_name_seeds_t::const_iterator iter2 = set_name_seeds.find(citer->second->name);
				if (iter2 != set_name_seeds.end()) {
					return getSet(iter->second + iter2->second);
				}
				else {
					return citer->second;
				}
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
		if (sets_list.empty() || sets_list[0] != s) {
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
	uint32_t sh = hash_value(name);

	if (ux_isSetOp(name) != S_IGNORE) {
		u_fprintf(ux_stderr, "Error: Found set operator '%S' where set name expected on line %u!\n", name, lines);
		CG3Quit(1);
	}

	if ((
	(name[0] == '$' && name[1] == '$')
	|| (name[0] == '&' && name[1] == '&')
	) && name[2]) {
		const UChar *wname = &(name[2]);
		uint32_t wrap = hash_value(wname);
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
			if (tags[tag->hash] != tag) {
				uint32_t ct = tag->hash;
				delete tag;
				tag = tags[ct];
			}
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
	rule->number = rule_by_number.size();
	rule_by_number.push_back(rule);
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
	if (!raw && ux_isSetOp(txt) != S_IGNORE) {
		u_fprintf(ux_stderr, "Warning: Tag '%S' on line %u looks like a set operator. Maybe you meant to do SET instead of LIST?\n", txt, lines);
		u_fflush(ux_stderr);
	}
	Taguint32HashMap::iterator it;
	uint32_t thash = hash_value(txt);
	if ((it = single_tags.find(thash)) != single_tags.end() && !it->second->tag.empty() && u_strcmp(it->second->tag.c_str(), txt) == 0) {
		return it->second;
	}

	Tag *tag = new Tag();
	if (raw) {
		tag->parseTagRaw(txt, this);
	}
	else {
		tag->parseTag(txt, ux_stderr, this);
	}
	tag->type |= T_GRAMMAR;
	uint32_t hash = tag->rehash();
	for (uint32_t seed = 0; seed < 10000; seed++) {
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

ContextualTest *Grammar::addContextualTest(ContextualTest *t) {
	if (t == 0) {
		return 0;
	}
	t->rehash();

	t->linked = addContextualTest(t->linked);
	foreach (ContextList, t->ors, it, it_end) {
		*it = addContextualTest(*it);
	}

	for (uint32_t seed = 0; seed < 1000; ++seed) {
		contexts_t::iterator cit = contexts.find(t->hash+seed);
		if (cit == contexts.end()) {
			contexts[t->hash+seed] = t;
			t->hash += seed;
			t->seed = seed;
			contexts_list.push_back(t);
			t->number = contexts_list.size();
			if (verbosity_level > 1 && seed) {
				u_fprintf(ux_stderr, "Warning: Context on line %u got hash seed %u.\n", t->line, seed);
				u_fflush(ux_stderr);
			}
			break;
		}
		if (t == cit->second) {
			break;
		}
		if (*t == *cit->second) {
			delete t;
			t = cit->second;
			break;
		}
	}
	return t;
}

void Grammar::addTemplate(ContextualTest *test, const UChar *name) {
	uint32_t cn = hash_value(name);
	if (templates.find(cn) != templates.end()) {
		u_fprintf(ux_stderr, "Error: Redefinition attempt for template '%S' on line %u!\n", name, lines);
		CG3Quit(1);
	}
	templates[cn] = test;
	template_list.push_back(test);
}

void Grammar::addAnchor(const UChar *to, uint32_t at, bool primary) {
	uint32_t ah = allocateTag(to, true)->hash;
	uint32FlatHashMap::iterator it = anchors.find(ah);
	if (primary && it != anchors.end()) {
		u_fprintf(ux_stderr, "Error: Redefinition attempt for anchor '%S' on line %u!\n", to, lines);
		CG3Quit(1);
	}
	if (at > rule_by_number.size()) {
		u_fprintf(ux_stderr, "Warning: No corresponding rule available for anchor '%S' on line %u!\n", to, lines);
		at = rule_by_number.size();
	}
	if (it == anchors.end()) {
		anchors[ah] = at;
	}
}

void Grammar::resetStatistics() {
	total_time = 0;
	for (uint32_t j=0;j<rules.size();j++) {
		rules[j]->resetStatistics();
	}
}

void Grammar::renameAllRules() {
	foreach (RuleVector, rule_by_number, iter_rule, iter_rule_end) {
		Rule *r = *iter_rule;
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
		uint32_t sh = hash_value(*sset);
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
			uint32_t nhash = hash_value(to->name);
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
							set_name_seeds[to->name] = seed;
							sets_by_name[nhash+seed] = chash;
							break;
						}
					}
				}
			}
		}
	}

	foreach (TagVector, single_tags_list, iter, iter_end) {
		if ((*iter)->regexp && (*iter)->tag[0] == '/') {
			regex_tags.insert((*iter)->regexp);
		}
		if (((*iter)->type & T_CASE_INSENSITIVE) && (*iter)->tag[0] == '/') {
			icase_tags.insert((*iter));
		}
		if (!(*iter)->vs_sets) {
			continue;
		}
		foreach (SetVector, *(*iter)->vs_sets, sit, sit_end) {
			(*sit)->markUsed(*this);
		}
	}

	foreach (TagVector, single_tags_list, titer, titer_end) {
		if ((*titer)->type & T_TEXTUAL) {
			continue;
		}
		foreach (Grammar::regex_tags_t, regex_tags, iter, iter_end) {
			UErrorCode status = U_ZERO_ERROR;
			uregex_setText(*iter, (*titer)->tag.c_str(), (*titer)->tag.length(), &status);
			if (status == U_ZERO_ERROR) {
				if (uregex_find(*iter, -1, &status)) {
					(*titer)->type |= T_TEXTUAL;
				}
			}
		}
		foreach (Grammar::icase_tags_t, icase_tags, iter, iter_end) {
			UErrorCode status = U_ZERO_ERROR;
			if (u_strCaseCompare((*titer)->tag.c_str(), (*titer)->tag.length(), (*iter)->tag.c_str(), (*iter)->tag.length(), U_FOLD_CASE_DEFAULT, &status) == 0) {
				(*titer)->type |= T_TEXTUAL;
			}
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: u_strCaseCompare() returned %s - cannot continue!\n", u_errorName(status));
				CG3Quit(1);
			}
		}
	}

#ifdef TR_RATIO
	boost::unordered_map<std::pair<uint32_t, uint32_t>, size_t> unique_targets;
	size_t max_target = 0;
#endif
	foreach (RuleVector, rule_by_number, iter_rule, iter_rule_end) {
		if ((*iter_rule)->wordform) {
			wf_rules.push_back(*iter_rule);
		}
		Set *s = 0;
		s = getSet((*iter_rule)->target);
		s->markUsed(*this);
		if ((*iter_rule)->childset1) {
			s = getSet((*iter_rule)->childset1);
			s->markUsed(*this);
		}
		if ((*iter_rule)->childset2) {
			s = getSet((*iter_rule)->childset2);
			s->markUsed(*this);
		}
		if ((*iter_rule)->maplist) {
			(*iter_rule)->maplist->markUsed(*this);
		}
		if ((*iter_rule)->sublist) {
			(*iter_rule)->sublist->markUsed(*this);
		}
		if ((*iter_rule)->dep_target) {
			(*iter_rule)->dep_target->markUsed(*this);
		}
		foreach (ContextList, (*iter_rule)->tests, it, it_end) {
			(*it)->markUsed(*this);
		}
		foreach (ContextList, (*iter_rule)->dep_tests, it, it_end) {
			(*it)->markUsed(*this);
		}
#ifdef TR_RATIO
		size_t& ut = unique_targets[std::make_pair((*iter_rule)->wordform, (*iter_rule)->target)];
		++ut;
		max_target = std::max(max_target, ut);
#endif
	}

#ifdef TR_RATIO
	double avg = rule_by_number.size() / static_cast<double>(unique_targets.size());
	double vsum = 0.0;
	for (const auto& v : unique_targets) {
		vsum += std::pow(v.second - avg, 2.0);
	}
	double var = vsum / unique_targets.size();

	u_fprintf(ux_stderr, "Target:Rule ratios: %u rules, %u unique targets, %u max rules, %f average, %f variance, %f stddev\n", rule_by_number.size(), unique_targets.size(), max_target, avg, var, std::sqrt(var));
	u_fflush(ux_stderr);
#endif

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
	for (iter_tags = single_tags.begin() ; iter_tags != single_tags.end() ; ++iter_tags) {
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

	foreach (RuleVector, rule_by_number, iter_rule, iter_rule_end) {
		if ((*iter_rule)->section == -1) {
			before_sections.push_back(*iter_rule);
		}
		else if ((*iter_rule)->section == -2) {
			after_sections.push_back(*iter_rule);
		}
		else if ((*iter_rule)->section == -3) {
			null_section.push_back(*iter_rule);
		}
		else {
			sects.insert((*iter_rule)->section);
			rules.push_back(*iter_rule);
		}
		if ((*iter_rule)->target) {
			indexSetToRule((*iter_rule)->number, getSet((*iter_rule)->target));
			rules_by_set[(*iter_rule)->target].insert((*iter_rule)->number);
		}
		else {
			u_fprintf(ux_stderr, "Warning: Rule on line %u had no target.\n", (*iter_rule)->line);
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
		boost_foreach (Tag *tomp_iter, s->single_tags) {
			indexTagToRule(tomp_iter->hash, r);
		}
		boost_foreach (CompositeTag *curcomptag, s->tags) {
			if (curcomptag->tags.size() == 1) {
				// ToDo: If this is ever run, something has gone wrong...
				indexTagToRule((*(curcomptag->tags.begin()))->hash, r);
			}
			else {
				const_foreach (CompositeTag::tags_t, curcomptag->tags, tag_iter, tag_iter_end) {
					indexTagToRule((*tag_iter)->hash, r);
				}
			}
		}
	}
	else {
		for (uint32_t i=0;i<s->sets.size();i++) {
			Set *set = sets_by_contents.find(s->sets[i])->second;
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
		boost_foreach (Tag *tomp_iter, s->single_tags) {
			indexTagToSet(tomp_iter->hash, r);
		}
		boost_foreach (CompositeTag *curcomptag, s->tags) {
			if (curcomptag->tags.size() == 1) {
				// ToDo: If this is ever run, something has gone wrong...
				indexTagToSet((*(curcomptag->tags.begin()))->hash, r);
			}
			else {
				const_foreach (CompositeTag::tags_t, curcomptag->tags, tag_iter, tag_iter_end) {
					indexTagToSet((*tag_iter)->hash, r);
				}
			}
		}
	}
	else {
		for (uint32_t i=0;i<s->sets.size();i++) {
			Set *set = sets_by_contents.find(s->sets[i])->second;
			indexSets(r, set);
		}
	}
}

void Grammar::indexTagToSet(uint32_t t, uint32_t r) {
	sets_by_tag[t].insert(r);
}

}
