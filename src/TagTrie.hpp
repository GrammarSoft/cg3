/*
* Copyright (C) 2007-2024, GrammarSoft ApS
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

#pragma once
#ifndef c6d28b7452ec699b_TAGTRIE_H
#define c6d28b7452ec699b_TAGTRIE_H

#include "stdafx.hpp"
#include "Tag.hpp"

namespace CG3 {
struct trie_node_t;
typedef bc::flat_map<Tag*, trie_node_t, compare_Tag> trie_t;

struct trie_node_t {
	bool terminal = false;
	std::unique_ptr<trie_t> trie;
};

inline bool trie_insert(trie_t& trie, const TagVector& tv, size_t w = 0) {
	trie_node_t& node = trie[tv[w]];
	if (node.terminal) {
		return false;
	}
	if (w < tv.size() - 1) {
		if (!node.trie) {
			node.trie.reset(new trie_t);
			//std::cerr << "new Trie" << std::endl;
		}
		return trie_insert(*node.trie, tv, w + 1);
	}
	node.terminal = true;
	node.trie.reset();
	return true;
}

inline std::unique_ptr<trie_t> _trie_copy_helper(const trie_t& trie) {
	auto nt = std::make_unique<trie_t>();
	for (auto& p : trie) {
		(*nt)[p.first].terminal = p.second.terminal;
		if (p.second.trie) {
			(*nt)[p.first].trie = _trie_copy_helper(*p.second.trie);
		}
	}
	return nt;
}

inline trie_t trie_copy(const trie_t& trie) {
	trie_t nt;
	for (auto& p : trie) {
		nt[p.first].terminal = p.second.terminal;
		if (p.second.trie) {
			nt[p.first].trie = _trie_copy_helper(*p.second.trie);
		}
	}
	return nt;
}

inline void trie_delete(trie_t& trie) {
	for (auto& p : trie) {
		if (p.second.trie) {
			trie_delete(*p.second.trie);
			p.second.trie.reset();
		}
	}
}

inline bool trie_singular(const trie_t& trie) {
	if (trie.size() != 1) {
		return false;
	}
	const trie_node_t& node = trie.begin()->second;
	if (node.terminal) {
		return true;
	}
	if (node.trie) {
		return trie_singular(*node.trie);
	}
	return false;
}

inline uint32_t trie_rehash(const trie_t& trie) {
	uint32_t retval = 0;
	for (auto& kv : trie) {
		retval = hash_value(kv.first->hash, retval);
		if (kv.second.trie) {
			retval = hash_value(trie_rehash(*kv.second.trie), retval);
		}
	}
	return retval;
}

inline void trie_markused(trie_t& trie) {
	for (auto& kv : trie) {
		kv.first->markUsed();
		if (kv.second.trie) {
			trie_markused(*kv.second.trie);
		}
	}
}

inline bool trie_hasType(trie_t& trie, uint32_t type) {
	for (auto& kv : trie) {
		if (kv.first->type & type) {
			return true;
		}
		if (kv.second.trie && trie_hasType(*kv.second.trie, type)) {
			return true;
		}
	}
	return false;
}

inline void trie_getTagList(const trie_t& trie, TagList& theTags) {
	for (auto& kv : trie) {
		theTags.push_back(kv.first);
		if (kv.second.trie) {
			trie_getTagList(*kv.second.trie, theTags);
		}
	}
}

inline bool trie_getTagList(const trie_t& trie, TagList& theTags, const void* node) {
	for (auto& kv : trie) {
		theTags.push_back(kv.first);
		if (node == &kv) {
			return true;
		}
		if (kv.second.trie && trie_getTagList(*kv.second.trie, theTags, node)) {
			return true;
		}
		theTags.pop_back();
	}
	return false;
}

/*
inline void trie_getTagList(const trie_t& trie, TagVector& theTags) {
	for (auto& kv : trie) {
		theTags.push_back(kv.first);
		if (kv.second.trie) {
			trie_getTagList(*kv.second.trie, theTags);
		}
	}
}
//*/

inline TagVector trie_getTagList(const trie_t& trie) {
	TagVector theTags;
	for (auto& kv : trie) {
		theTags.push_back(kv.first);
		if (kv.second.trie) {
			trie_getTagList(*kv.second.trie, theTags);
		}
	}
	return theTags;
}

inline void trie_getTags(const trie_t& trie, TagVectorSet& rv, TagVector& tv) {
	for (auto& kv : trie) {
		tv.push_back(kv.first);
		if (kv.second.terminal) {
			std::sort(tv.begin(), tv.end(), compare_Tag());
			rv.insert(tv);
			tv.pop_back();
			continue;
		}
		if (kv.second.trie) {
			trie_getTags(*kv.second.trie, rv, tv);
		}
	}
}

inline TagVectorSet trie_getTags(const trie_t& trie) {
	TagVectorSet rv;
	for (auto& kv : trie) {
		TagVector tv;
		tv.push_back(kv.first);
		if (kv.second.terminal) {
			std::sort(tv.begin(), tv.end(), compare_Tag());
			rv.insert(tv);
			tv.pop_back();
			continue;
		}
		if (kv.second.trie) {
			trie_getTags(*kv.second.trie, rv, tv);
		}
	}
	return rv;
}

inline void trie_getTagsOrdered(const trie_t& trie, TagVectorSet& rv, TagVector& tv) {
	for (auto& kv : trie) {
		tv.push_back(kv.first);
		if (kv.second.terminal) {
			rv.insert(tv);
			tv.pop_back();
			continue;
		}
		if (kv.second.trie) {
			trie_getTagsOrdered(*kv.second.trie, rv, tv);
		}
	}
}

inline TagVectorSet trie_getTagsOrdered(const trie_t& trie) {
	TagVectorSet rv;
	for (auto& kv : trie) {
		TagVector tv;
		tv.push_back(kv.first);
		if (kv.second.terminal) {
			rv.insert(tv);
			tv.pop_back();
			continue;
		}
		if (kv.second.trie) {
			trie_getTagsOrdered(*kv.second.trie, rv, tv);
		}
	}
	return rv;
}

inline void trie_serialize(const trie_t& trie, std::ostream& out) {
	for (auto& kv : trie) {
		writeSwapped<uint32_t>(out, kv.first->number);
		writeSwapped<uint8_t>(out, kv.second.terminal);
		if (kv.second.trie) {
			writeSwapped<uint32_t>(out, UI32(kv.second.trie->size()));
			trie_serialize(*kv.second.trie, out);
		}
		else {
			writeSwapped<uint32_t>(out, 0);
		}
	}
}
}

#endif
