/*
* Copyright (C) 2007-2024, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
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
#ifndef c6d28b7452ec699b_PROFILER_HPP
#define c6d28b7452ec699b_PROFILER_HPP

#include "stdafx.hpp"
#include <string>
#include <string_view>
#include <map>
#include <sstream>

namespace CG3 {

enum : uint8_t {
	ET_RULE = 0,
	ET_CONTEXT = 1,
};

struct Profiler {
	std::map<std::string, size_t, std::less<>> strings;
	std::map<size_t, size_t> grammars;
	size_t grammar_ast = 0;
	std::stringstream buf;

	struct Key {
		uint8_t type = ET_RULE;
		uint32_t id = 0;

		bool operator<(const Key& o) const {
			if (type == o.type) {
				return id < o.id;
			}
			return type < o.type;
		}
	};

	struct Entry {
		uint8_t type = ET_RULE;
		uint32_t grammar = 0;
		size_t b = 0;
		size_t e = 0;
		size_t num_match = 0;
		size_t num_fail = 0;
		size_t example_window = 0;
	};
	std::map<Key, Entry> entries;
	std::map<std::pair<uint32_t, uint32_t>, size_t> rule_contexts;

	size_t addString(std::string_view str) {
		auto it = strings.find(str);
		if (it != strings.end()) {
			return it->second;
		}
		auto sz = strings.size() + 1;
		strings.emplace(std::string(str), sz);
		return sz;
	}

	uint32_t addGrammar(std::string_view fname, std::string_view grammar) {
		auto f = addString(fname);
		auto g = addString(grammar);
		grammars[f] = g;
		return UI32(g);
	}

	void addRule(uint32_t n, uint32_t g, size_t b, size_t e) {
		Key k{ET_RULE, n};
		entries.emplace(k, Entry{ET_RULE, g, b, e});
	}

	void addContext(uint32_t c, uint32_t g, size_t b, size_t e) {
		Key k{ ET_CONTEXT, c };
		if (entries.count(k) == 0) {
			entries.emplace(k, Entry{ET_CONTEXT, g, b, e});
		}
	}

	void write(const char* fname);

	void read(const char* fname);
};

}

#endif
