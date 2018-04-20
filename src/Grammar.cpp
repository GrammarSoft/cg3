/*
* Copyright (C) 2007-2018, GrammarSoft ApS
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

Grammar::Grammar()
  : ux_stderr(0)
  , ux_stdout(0)
  , has_dep(false)
  , has_bag_of_tags(false)
  , has_relations(false)
  , has_encl_final(false)
  , has_protect(false)
  , is_binary(false)
  , sub_readings_ltr(false)
  , grammar_size(0)
  , mapping_prefix('@')
  , lines(0)
  , verbosity_level(0)
  , total_time(0)
  , rules_any(0)
  , sets_any(0)
  , delimiters(0)
  , soft_delimiters(0)
  , tag_any(0)
{
	// Nothing in the actual body...
}

Grammar::~Grammar() {
	for (auto iter_set : sets_list) {
		destroySet(iter_set);
	}

	for (auto rsets : sets_all) {
		delete rsets;
	}

	Taguint32HashMap::iterator iter_stag;
	for (iter_stag = single_tags.begin(); iter_stag != single_tags.end(); ++iter_stag) {
		if (iter_stag->second) {
			delete iter_stag->second;
		}
	}

	for (auto iter_rules : rule_by_number) {
		delete iter_rules;
	}

	for (auto cntx : contexts) {
		delete cntx.second;
	}
}

void Grammar::addSet(Set*& to) {
	if (!delimiters && u_strcmp(to->name.c_str(), stringbits[S_DELIMITSET].getTerminatedBuffer()) == 0) {
		delimiters = to;
	}
	else if (!soft_delimiters && u_strcmp(to->name.c_str(), stringbits[S_SOFTDELIMITSET].getTerminatedBuffer()) == 0) {
		soft_delimiters = to;
	}
	if (verbosity_level > 0 && to->name[0] == 'T' && to->name[1] == ':') {
		u_fprintf(ux_stderr, "Warning: Set name %S looks like a misattempt of template usage on line %u.\n", to->name.c_str(), to->line);
	}

	if (!to->sets.empty() && !(to->type & (ST_TAG_UNIFY | ST_CHILD_UNIFY | ST_SET_UNIFY))) {
		bool all_tags = true;
		for (size_t i = 0; i < to->sets.size(); ++i) {
			if (i > 0 && to->set_ops[i - 1] != S_OR) {
				all_tags = false;
				break;
			}
			Set* s = getSet(to->sets[i]);
			if (!s->sets.empty()) {
				all_tags = false;
				break;
			}
			if (!s->trie.empty() && !s->trie_special.empty()) {
				all_tags = false;
				break;
			}
			if (s->getNonEmpty().size() != 1 || !trie_singular(s->getNonEmpty())) {
				all_tags = false;
				break;
			}
		}

		if (all_tags) {
			for (size_t i = 0; i < to->sets.size(); ++i) {
				Set* s = getSet(to->sets[i]);
				maybe_used_sets.insert(s);
				TagVector tv = trie_getTagList(s->getNonEmpty());
				if (tv.size() == 1) {
					addTagToSet(tv[0], to);
				}
				else {
					bool special = false;
					for (auto tag : tv) {
						if (tag->type & T_SPECIAL) {
							special = true;
							break;
						}
					}
					if (special) {
						trie_insert(to->trie_special, tv);
					}
					else {
						trie_insert(to->trie, tv);
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
	if (!to->ff_tags.empty() && to->ff_tags.size() < (to->trie.size() + to->trie_special.size())) {
		Set* positive = allocateSet();
		Set* negative = allocateSet();

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

		positive->trie.swap(to->trie);
		positive->trie_special.swap(to->trie_special);

		for (auto iter : to->ff_tags) {
			auto pit = positive->trie_special.find(iter);
			if (pit != positive->trie_special.end()) {
				if (pit->second.terminal) {
					if (pit->second.trie) {
						trie_delete(*pit->second.trie);
					}
					positive->trie_special.erase(pit);
				}
			}
			Tag* tag = new Tag(*iter);
			tag->type &= ~T_FAILFAST;
			tag = addTag(tag);
			addTagToSet(tag, negative);
		}

		positive->reindex(*this);
		negative->reindex(*this);
		addSet(positive);
		addSet(negative);

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
	for (; to->name[0] != '_' || to->name[1] != 'G' || to->name[2] != '_';) {
		uint32_t nhash = hash_value(to->name.c_str());
		if (sets_by_name.find(nhash) != sets_by_name.end()) {
			Set* a = sets_by_contents.find(sets_by_name.find(nhash)->second)->second;
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
			Set* a = sets_by_contents.find(sets_by_name.find(nhash)->second)->second;
			if (a->name == to->name) {
				u_fprintf(ux_stderr, "Error: Set %S already defined at line %u. Redefinition attempted at line %u!\n", a->name.c_str(), a->line, to->line);
				CG3Quit(1);
			}
			else {
				for (uint32_t seed = 0; seed < 1000; ++seed) {
					if (sets_by_name.find(nhash + seed) == sets_by_name.end()) {
						if (verbosity_level > 0 && (to->name[0] != '_' || to->name[1] != 'G' || to->name[2] != '_')) {
							u_fprintf(ux_stderr, "Warning: Set %S got hash seed %u.\n", to->name.c_str(), seed);
							u_fflush(ux_stderr);
						}
						set_name_seeds[to->name] = seed;
						sets_by_name[nhash + seed] = chash;
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
		Set* a = sets_by_contents.find(chash)->second;
		if (a != to) {
			a->reindex(*this);
			to->reindex(*this);
			if ((a->type & (ST_SPECIAL | ST_TAG_UNIFY | ST_CHILD_UNIFY | ST_SET_UNIFY)) != (to->type & (ST_SPECIAL | ST_TAG_UNIFY | ST_CHILD_UNIFY | ST_SET_UNIFY)) || a->set_ops.size() != to->set_ops.size() || a->sets.size() != to->sets.size() || a->trie.size() != to->trie.size() || a->trie_special.size() != to->trie_special.size()) {
				u_fprintf(ux_stderr, "Error: Content hash collision between set %S on line %u and %S on line %u!\n", a->name.c_str(), a->line, to->name.c_str(), to->line);
				CG3Quit(1);
			}
			destroySet(to);
		}
	}
	to = sets_by_contents[chash];
}

Set* Grammar::getSet(uint32_t which) const {
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

Set* Grammar::allocateSet() {
	Set* ns = new Set;
	sets_all.insert(ns);
	return ns;
}

void Grammar::destroySet(Set* set) {
	sets_all.erase(set);
	delete set;
}

void Grammar::addSetToList(Set* s) {
	if (s->number == 0) {
		if (sets_list.empty() || sets_list[0] != s) {
			if (!s->sets.empty()) {
				for (auto sit : s->sets) {
					addSetToList(getSet(sit));
				}
			}
			sets_list.push_back(s);
			s->number = (uint32_t)sets_list.size() - 1;
		}
	}
}

void Grammar::allocateDummySet() {
	Set* set_c = allocateSet();
	set_c->line = 0;
	set_c->setName(stringbits[S_IGNORE].getTerminatedBuffer());
	Tag* t = allocateTag(stringbits[S_IGNORE].getTerminatedBuffer());
	addTagToSet(t, set_c);
	addSet(set_c);
	set_c->number = std::numeric_limits<uint32_t>::max();
	sets_list.insert(sets_list.begin(), set_c);
}

uint32_t Grammar::removeNumericTags(uint32_t s) {
	Set* set = getSet(s);
	if (!set->sets.empty()) {
		bool did = false;
		auto sets = set->sets;
		for (size_t i = 0; i < sets.size(); ++i) {
			uint32_t ns = removeNumericTags(sets[i]);
			if (ns == 0) {
				set = getSet(sets[i]);
				u_fprintf(ux_stderr, "Error: Removing numeric tags for branch resulted in set %S on line %u being empty!\n", set->name.c_str(), set->line);
				CG3Quit(1);
			}
			if (ns != sets[i]) {
				sets[i] = ns;
				did = true;
			}
		}
		if (did) {
			Set* ns = allocateSet();
			ns->type = set->type;
			ns->line = set->line;
			ns->name = stringbits[S_GPREFIX].getTerminatedBuffer();
			ns->name += set->name;
			ns->name += '_';
			ns->name += 'B';
			ns->name += '_';
			ns->sets = sets;
			ns->set_ops = set->set_ops;
			addSet(ns);
			set = ns;
		}
	}
	else {
		bool did = false;
		std::map<TagVector, bool> ntags;
		TagVector tags;
		const trie_t* tries[2] = { &set->trie, &set->trie_special };
		for (size_t i = 0; i < 2; ++i) {
			if (tries[i]->empty()) {
				continue;
			}
			auto ctags = trie_getTags(*tries[i]);
			for (auto it = ctags.begin(); it != ctags.end(); ++it) {
				bool special = false;
				tags.clear();
				fill_tagvector(*it, tags, did, special);
				if (!tags.empty()) {
					ntags[tags] = special;
				}
			}
		}

		if (!set->ff_tags.empty()) {
			bool special = false;
			tags.clear();
			fill_tagvector(set->ff_tags, tags, did, special);
			if (!tags.empty()) {
				ntags[tags] = special;
			}
		}

		if (did) {
			if (ntags.empty()) {
				tags.clear();
				tags.push_back(single_tags[tag_any]);
				ntags[tags] = true;
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Set %S was empty and replaced with the * set in the C branch on line %u.\n", set->name.c_str(), set->line);
					u_fflush(ux_stderr);
				}
			}
			Set* ns = allocateSet();
			ns->type = set->type;
			ns->line = set->line;
			ns->name = stringbits[S_GPREFIX].getTerminatedBuffer();
			ns->name += set->name;
			ns->name += '_';
			ns->name += 'B';
			ns->name += '_';

			for (auto it = ntags.begin(); it != ntags.end(); ++it) {
				if (it->second) {
					if (it->first.size() == 1 && (it->first[0]->type & T_FAILFAST)) {
						ns->ff_tags.insert(it->first[0]);
					}
					else {
						trie_insert(ns->trie_special, it->first);
					}
				}
				else {
					trie_insert(ns->trie, it->first);
				}
			}
			addSet(ns);
			set = ns;
		}
	}
	return set->hash;
}

void Grammar::getTags(const Set& set, std::set<TagVector>& rv) {
	// ToDo: getTags() ought to account for other operators than OR
	for (auto s : set.sets) {
		getTags(*getSet(s), rv);
	}
	TagVector tv;
	trie_getTags(set.trie, rv, tv);
	tv.clear();
	trie_getTags(set.trie_special, rv, tv);
}

Rule* Grammar::allocateRule() {
	return new Rule;
}

void Grammar::addRule(Rule* rule) {
	rule->number = rule_by_number.size();
	rule_by_number.push_back(rule);
}

void Grammar::destroyRule(Rule* rule) {
	delete rule;
}

Tag* Grammar::allocateTag() {
	return new Tag;
}

Tag* Grammar::allocateTag(const UChar* txt) {
	if (txt[0] == 0) {
		u_fprintf(ux_stderr, "Error: Empty tag on line %u! Forgot to fill in a ()?\n", lines);
		CG3Quit(1);
	}
	if (txt[0] == '(') {
		u_fprintf(ux_stderr, "Error: Tag '%S' cannot start with ( on line %u! Possible extra opening ( or missing closing ) to the left. If you really meant it, escape it as \\(.\n", txt, lines);
		CG3Quit(1);
	}
	Taguint32HashMap::iterator it;
	uint32_t thash = hash_value(txt);
	if ((it = single_tags.find(thash)) != single_tags.end() && !it->second->tag.empty() && u_strcmp(it->second->tag.c_str(), txt) == 0) {
		return it->second;
	}

	Tag* tag = new Tag();
	tag->parseTagRaw(txt, this);
	return addTag(tag);
}

Tag* Grammar::addTag(Tag* tag) {
	tag->type |= T_GRAMMAR;
	uint32_t hash = tag->rehash();
	for (uint32_t seed = 0; seed < 10000; seed++) {
		uint32_t ih = hash + seed;
		Taguint32HashMap::iterator it;
		if ((it = single_tags.find(ih)) != single_tags.end()) {
			Tag* t = it->second;
			if (t == tag) {
				return tag;
			}
			if (t->tag == tag->tag) {
				hash += seed;
				delete tag;
				break;
			}
		}
		else {
			if (verbosity_level > 0 && seed) {
				u_fprintf(ux_stderr, "Warning: Tag %S got hash seed %u.\n", tag->tag.c_str(), seed);
				u_fflush(ux_stderr);
			}
			tag->seed = seed;
			hash = tag->rehash();
			single_tags_list.push_back(tag);
			tag->number = (uint32_t)single_tags_list.size() - 1;
			single_tags[hash] = tag;
			break;
		}
	}
	return single_tags[hash];
}

void Grammar::addTagToSet(Tag* rtag, Set* set) {
	if (rtag->type & T_ANY) {
		set->type |= ST_ANY;
	}
	if (rtag->type & T_FAILFAST) {
		set->ff_tags.insert(rtag);
	}
	if (rtag->type & T_SPECIAL) {
		set->type |= ST_SPECIAL;
		set->trie_special[rtag].terminal = true;
	}
	else {
		set->trie[rtag].terminal = true;
	}
}

void Grammar::destroyTag(Tag* tag) {
	delete tag;
}

ContextualTest* Grammar::allocateContextualTest() {
	return new ContextualTest;
}

ContextualTest* Grammar::addContextualTest(ContextualTest* t) {
	if (t == 0) {
		return 0;
	}
	t->rehash();

	t->linked = addContextualTest(t->linked);
	for (auto& it : t->ors) {
		it = addContextualTest(it);
	}

	for (uint32_t seed = 0; seed < 1000; ++seed) {
		contexts_t::iterator cit = contexts.find(t->hash + seed);
		if (cit == contexts.end()) {
			contexts[t->hash + seed] = t;
			t->hash += seed;
			t->seed = seed;
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

void Grammar::addTemplate(ContextualTest* test, const UChar* name) {
	uint32_t cn = hash_value(name);
	if (templates.find(cn) != templates.end()) {
		u_fprintf(ux_stderr, "Error: Redefinition attempt for template '%S' on line %u!\n", name, lines);
		CG3Quit(1);
	}
	templates[cn] = test;
}

void Grammar::addAnchor(const UChar* to, uint32_t at, bool primary) {
	uint32_t ah = allocateTag(to)->hash;
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
	for (uint32_t j = 0; j < rules.size(); j++) {
		rules[j]->resetStatistics();
	}
}

void Grammar::renameAllRules() {
	for (auto r : rule_by_number) {
		gbuffers[0][0] = 0;
		u_sprintf(&gbuffers[0][0], "L%u", r->line);
		r->setName(&gbuffers[0][0]);
	}
}

void Grammar::reindex(bool unused_sets, bool used_tags) {
	for (auto dset : sets_by_contents) {
		if (dset.second->number == std::numeric_limits<uint32_t>::max()) {
			dset.second->type |= ST_USED;
			continue;
		}
		if (!(dset.second->type & ST_STATIC)) {
			dset.second->type &= ~ST_USED;
		}
		dset.second->number = 0;
	}

	for (auto sset : static_sets) {
		uint32_t sh = hash_value(sset);
		if (set_alias.find(sh) != set_alias.end()) {
			u_fprintf(ux_stderr, "Error: Static set %S is an alias; only real sets may be made static!\n", sset.c_str());
			CG3Quit(1);
		}
		Set* s = getSet(sh);
		if (!s) {
			if (verbosity_level > 0) {
				u_fprintf(ux_stderr, "Warning: Set %S was not defined, so cannot make it static.\n", sset.c_str());
			}
			continue;
		}
		if (s->name != sset) {
			s->setName(sset);
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
	if (!is_binary) {
		sets_list.resize(1);
		sets_list[0]->number = 0;
	}
	set_name_seeds.clear();
	sets_any = 0;
	rules_any = 0;

	for (auto iter : single_tags_list) {
		if (iter->regexp && iter->tag[0] != '"' && iter->tag[0] != '<') {
			regex_tags.insert(iter->regexp);
		}
		if ((iter->type & T_CASE_INSENSITIVE) && iter->tag[0] != '"' && iter->tag[0] != '<') {
			icase_tags.insert(iter);
		}
		if (is_binary) {
			continue;
		}
		if (!iter->vs_sets) {
			continue;
		}
		for (auto sit : *iter->vs_sets) {
			sit->markUsed(*this);
		}
	}

	for (auto titer : single_tags_list) {
		if (titer->type & T_TEXTUAL) {
			continue;
		}
		for (auto iter : regex_tags) {
			UErrorCode status = U_ZERO_ERROR;
			uregex_setText(iter, titer->tag.c_str(), titer->tag.size(), &status);
			if (status == U_ZERO_ERROR) {
				if (uregex_find(iter, -1, &status)) {
					titer->type |= T_TEXTUAL;
				}
			}
		}
		for (auto iter : icase_tags) {
			UErrorCode status = U_ZERO_ERROR;
			if (u_strCaseCompare(titer->tag.c_str(), titer->tag.size(), iter->tag.c_str(), iter->tag.size(), U_FOLD_CASE_DEFAULT, &status) == 0) {
				titer->type |= T_TEXTUAL;
			}
			if (status != U_ZERO_ERROR) {
				u_fprintf(ux_stderr, "Error: u_strCaseCompare() returned %s - cannot continue!\n", u_errorName(status));
				CG3Quit(1);
			}
		}
	}

	for (auto it = parentheses.begin(); it != parentheses.end(); ++it) {
		single_tags[it->first]->markUsed();
		single_tags[it->second]->markUsed();
	}

	for (auto it = preferred_targets.begin(); it != preferred_targets.end(); ++it) {
		single_tags[*it]->markUsed();
	}

	for (auto rule : rule_by_number) {
		if (rule->wordform) {
			wf_rules.push_back(rule);
		}
		if (rule->type == K_PROTECT) {
			has_protect = true;
		}
		if (is_binary) {
			continue;
		}
		Set* s = 0;
		s = getSet(rule->target);
		s->markUsed(*this);
		if (rule->childset1) {
			s = getSet(rule->childset1);
			s->markUsed(*this);
		}
		if (rule->childset2) {
			s = getSet(rule->childset2);
			s->markUsed(*this);
		}
		if (rule->maplist) {
			rule->maplist->markUsed(*this);
		}
		if (rule->sublist) {
			rule->sublist->markUsed(*this);
		}
		if (rule->dep_target) {
			rule->dep_target->markUsed(*this);
		}
		for (auto it : rule->tests) {
			it->markUsed(*this);
		}
		for (auto it : rule->dep_tests) {
			it->markUsed(*this);
		}
	}

	if (!is_binary) {
		if (delimiters) {
			delimiters->markUsed(*this);
		}
		if (soft_delimiters) {
			soft_delimiters->markUsed(*this);
		}

		contexts_t tosave;
		for (auto cntx = contexts.begin(); cntx != contexts.end(); ++cntx) {
			if (cntx->second->is_used) {
				tosave[cntx->first] = cntx->second;
				continue;
			}
			delete cntx->second;
		}
		contexts.swap(tosave);
	}

	if (unused_sets) {
		u_fprintf(ux_stdout, "Unused sets:\n");
		for (auto rset : sets_by_contents) {
			if (!(rset.second->type & ST_USED) && !rset.second->name.empty() && maybe_used_sets.count(rset.second) == 0) {
				if (rset.second->name[0] != '_' || rset.second->name[1] != 'G' || rset.second->name[2] != '_') {
					u_fprintf(ux_stdout, "Line %u set %S\n", rset.second->line, rset.second->name.c_str());
				}
			}
		}
		u_fprintf(ux_stdout, "End of unused sets.\n");
		u_fflush(ux_stdout);
	}

	// Stuff below this line is not optional...

	for (auto tset : sets_by_contents) {
		if (tset.second->type & ST_USED) {
			addSetToList(tset.second);
		}
	}

	for (auto iter_tags : single_tags) {
		Tag* tag = iter_tags.second;
		if (tag->tag[0] == mapping_prefix) {
			tag->type |= T_MAPPING;
		}
		else {
			tag->type &= ~T_MAPPING;
		}
	}

	if (!is_binary) {
		for (auto set : sets_list) {
			set->reindex(*this);
		}
		for (auto set : sets_list) {
			setAdjustSets(set);
		}
	}
	for (auto set : sets_list) {
		indexSets(set->number, set);
	}

	uint32SortedVector sects;

	for (auto rule : rule_by_number) {
		if (rule->section == -1) {
			before_sections.push_back(rule);
		}
		else if (rule->section == -2) {
			after_sections.push_back(rule);
		}
		else if (rule->section == -3) {
			null_section.push_back(rule);
		}
		else {
			sects.insert(rule->section);
			rules.push_back(rule);
		}
		if (rule->target) {
			Set* set = 0;
			if (is_binary) {
				set = sets_list[rule->target];
			}
			else {
				set = sets_by_contents.find(rule->target)->second;
				rule->target = set->number;
			}
			indexSetToRule(rule->number, set);
			rules_by_set[rule->target].insert(rule->number);
		}
		else {
			u_fprintf(ux_stderr, "Warning: Rule on line %u had no target.\n", rule->line);
			u_fflush(ux_stderr);
		}
		if ((rule->maplist && (rule->maplist->type & ST_CHILD_UNIFY)) || (rule->sublist && (rule->sublist->type & ST_CHILD_UNIFY))) {
			rule->flags |= FL_CAPTURE_UNIF;
		}
		if (is_binary) {
			continue;
		}
		if (rule->childset1) {
			Set* set = sets_by_contents.find(rule->childset1)->second;
			rule->childset1 = set->number;
		}
		if (rule->childset2) {
			Set* set = sets_by_contents.find(rule->childset2)->second;
			rule->childset2 = set->number;
		}
		if (rule->dep_target) {
			contextAdjustTarget(rule->dep_target);
		}
		for (auto test : rule->tests) {
			contextAdjustTarget(test);
		}
		for (auto test : rule->dep_tests) {
			contextAdjustTarget(test);
		}
	}

	if (!sects.empty()) {
		for (uint32_t i = 0; i <= sects.back(); ++i) {
			sections.push_back(i);
		}
	}

	if (sets_by_tag.find(tag_any) != sets_by_tag.end()) {
		sets_any = &sets_by_tag[tag_any];
	}
	if (rules_by_tag.find(tag_any) != rules_by_tag.end()) {
		rules_any = &rules_by_tag[tag_any];
	}

	sets_by_contents.clear();

	for (auto to : sets_list) {
		if (to->type & ST_STATIC) {
			uint32_t nhash = hash_value(to->name);
			const uint32_t cnum = to->number;

			if (sets_by_name.find(nhash) == sets_by_name.end()) {
				sets_by_name[nhash] = cnum;
			}
			else if (cnum != sets_list[sets_by_name.find(nhash)->second]->number) {
				Set* a = sets_list[sets_by_name.find(nhash)->second];
				if (a->name == to->name) {
					u_fprintf(ux_stderr, "Error: Static set %S already defined. Redefinition attempted!\n", a->name.c_str());
					CG3Quit(1);
				}
				else {
					for (uint32_t seed = 0; seed < 1000; ++seed) {
						if (sets_by_name.find(nhash + seed) == sets_by_name.end()) {
							if (verbosity_level > 0) {
								u_fprintf(ux_stderr, "Warning: Static set %S got hash seed %u.\n", to->name.c_str(), seed);
								u_fflush(ux_stderr);
							}
							set_name_seeds[to->name] = seed;
							sets_by_name[nhash + seed] = cnum;
							break;
						}
					}
				}
			}
		}
	}

	// Gather knowledge of which sets have varstrings
	boost::dynamic_bitset<> sets_vstr(sets_list.size());
	bool did = true;
	while (did) {
		did = false;
		for (auto set : sets_list) {
			if (sets_vstr.test(set->number)) {
				continue;
			}
			for (auto iset : set->sets) {
				if (sets_vstr.test(iset)) {
					sets_vstr.set(set->number);
					did = true;
					break;
				}
			}
			if (trie_hasType(set->trie, T_VARSTRING) || trie_hasType(set->trie_special, T_VARSTRING)) {
				sets_vstr.set(set->number);
				did = true;
			}
		}
	}

	// Gather knowledge of which contexts use unification or varstrings
	bc::flat_set<ContextualTest*> nk;
	did = true;
	while (did) {
		did = false;

		for (auto cntx : contexts) {
			ContextualTest* t = cntx.second;

			if (nk.count(t)) {
				continue;
			}

			if (t->tmpl && nk.count(t->tmpl)) {
				//u_fprintf(ux_stderr, "added tmpl %u\n", t->line);
				did |= nk.insert(t).second;
				continue;
			}
			if (t->linked && nk.count(t->linked)) {
				//u_fprintf(ux_stderr, "added linked %u\n", t->line);
				did |= nk.insert(t).second;
				continue;
			}
			if (t->target && (sets_list[t->target]->type & MASK_ST_UNIFY)) {
				//u_fprintf(ux_stderr, "added target %u\n", t->line);
				did |= nk.insert(t).second;
				continue;
			}
			if (t->target && sets_vstr.test(t->target)) {
				//u_fprintf(ux_stderr, "added target %u\n", t->line);
				did |= nk.insert(t).second;
				continue;
			}
			if (t->barrier && (sets_list[t->barrier]->type & MASK_ST_UNIFY)) {
				//u_fprintf(ux_stderr, "added barrier %u\n", t->line);
				did |= nk.insert(t).second;
				continue;
			}
			if (t->barrier && sets_vstr.test(t->barrier)) {
				//u_fprintf(ux_stderr, "added target %u\n", t->line);
				did |= nk.insert(t).second;
				continue;
			}
			if (t->cbarrier && (sets_list[t->cbarrier]->type & MASK_ST_UNIFY)) {
				//u_fprintf(ux_stderr, "added cbarrier %u\n", t->line);
				did |= nk.insert(t).second;
				continue;
			}
			if (t->cbarrier && sets_vstr.test(t->cbarrier)) {
				//u_fprintf(ux_stderr, "added target %u\n", t->line);
				did |= nk.insert(t).second;
				continue;
			}
		}
	}

	for (auto r : rule_by_number) {
		// Determine whether this rule probably needs KEEPORDER
		if (r->flags & RF_KEEPORDER) {
			continue;
		}
		/* While this is a good indication that the unified set is used in the target, it is not 100%
		if (r->target && (sets_list[r->target]->type & MASK_ST_UNIFY)) {
			continue;
		}
		//*/
		bool needs = false;
		if (r->dep_target && nk.count(r->dep_target)) {
			needs = true;
		}
		for (auto cntx : r->tests) {
			if (nk.count(cntx)) {
				needs = true;
			}
		}
		for (auto cntx : r->dep_tests) {
			if (nk.count(cntx)) {
				needs = true;
			}
		}
		if (needs) {
			r->flags |= RF_KEEPORDER;
			/* We can do the whole thing fully automatically, so explicit KEEPORDER will eventually be deprecated
			if (verbosity_level) {
				u_fprintf(ux_stderr, "Warning: Rule on line %u probably needs KEEPORDER.\n", r->line);
				u_fflush(ux_stderr);
			}
			//*/
		}
	}

	if (used_tags) {
		for (auto iter_tags = single_tags.begin(); iter_tags != single_tags.end(); ++iter_tags) {
			Tag* tag = iter_tags->second;
			if (tag->type & T_USED) {
				UString tmp(tag->toUString(true));
				u_fprintf(ux_stdout, "%S\n", tmp.c_str());
			}
		}
		exit(0);
	}
}

inline void trie_indexToRule(const trie_t& trie, Grammar& grammar, uint32_t r) {
	for (auto& kv : trie) {
		grammar.indexTagToRule(kv.first->hash, r);
		if (kv.second.trie) {
			trie_indexToRule(*kv.second.trie, grammar, r);
		}
	}
}

void Grammar::indexSetToRule(uint32_t r, Set* s) {
	if (s->type & (ST_SPECIAL | ST_TAG_UNIFY)) {
		indexTagToRule(tag_any, r);
		return;
	}

	trie_indexToRule(s->trie, *this, r);
	trie_indexToRule(s->trie_special, *this, r);

	for (uint32_t i = 0; i < s->sets.size(); ++i) {
		Set* set = sets_list[s->sets[i]];
		indexSetToRule(r, set);
	}
}

void Grammar::indexTagToRule(uint32_t t, uint32_t r) {
	rules_by_tag[t].insert(r);
}

inline void trie_indexToSet(const trie_t& trie, Grammar& grammar, uint32_t r) {
	for (auto& kv : trie) {
		grammar.indexTagToSet(kv.first->hash, r);
		if (kv.second.trie) {
			trie_indexToSet(*kv.second.trie, grammar, r);
		}
	}
}

void Grammar::indexSets(uint32_t r, Set* s) {
	if (s->type & (ST_SPECIAL | ST_TAG_UNIFY)) {
		indexTagToSet(tag_any, r);
		return;
	}

	trie_indexToSet(s->trie, *this, r);
	trie_indexToSet(s->trie_special, *this, r);

	for (uint32_t i = 0; i < s->sets.size(); ++i) {
		Set* set = sets_list[s->sets[i]];
		indexSets(r, set);
	}
}

void Grammar::indexTagToSet(uint32_t t, uint32_t r) {
	if (sets_by_tag.find(t) == sets_by_tag.end()) {
		sets_by_tag[t].resize(sets_list.size());
	}
	sets_by_tag[t].set(r);
}

void Grammar::setAdjustSets(Set* s) {
	if (!(s->type & ST_USED)) {
		return;
	}
	s->type &= ~ST_USED;

	for (uint32_t i = 0; i < s->sets.size(); ++i) {
		Set* set = sets_by_contents.find(s->sets[i])->second;
		s->sets[i] = set->number;
		setAdjustSets(set);
	}
}

void Grammar::contextAdjustTarget(ContextualTest* test) {
	if (!test->is_used) {
		return;
	}
	test->is_used = false;

	if (test->target) {
		Set* set = sets_by_contents.find(test->target)->second;
		test->target = set->number;
	}
	if (test->barrier) {
		Set* set = sets_by_contents.find(test->barrier)->second;
		test->barrier = set->number;
	}
	if (test->cbarrier) {
		Set* set = sets_by_contents.find(test->cbarrier)->second;
		test->cbarrier = set->number;
	}

	for (auto tor : test->ors) {
		contextAdjustTarget(tor);
	}
	if (test->tmpl) {
		contextAdjustTarget(test->tmpl);
	}
	if (test->linked) {
		contextAdjustTarget(test->linked);
	}
}
}
