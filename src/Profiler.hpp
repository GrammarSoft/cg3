/*
* Copyright (C) 2007-2023, GrammarSoft ApS
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

struct Profiler {
	std::map<std::string, size_t, std::less<>> strings;
	std::map<size_t, size_t> grammars;
	size_t grammar_ast = 0;
	std::stringstream buf;

	struct Entry {
		size_t grammar = 0;
		size_t b = 0;
		size_t e = 0;
		size_t num_match = 0;
		size_t num_fail = 0;
		size_t example_window = 0;
		size_t example_target = 0;
	};
	std::map<uint32_t, Entry> rules;
	std::map<uint32_t, Entry> contexts;
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

	void addRule(std::string_view fname, size_t b, size_t e) {
		auto str = addString(fname);
		auto sz = UI32(rules.size() + 1);
		rules.emplace(sz, Entry{str, b, e});
	}

	void addContext(uint32_t c, std::string_view fname, size_t b, size_t e) {
		if (contexts.count(c) == 0) {
			auto str = addString(fname);
			contexts.emplace(c, Entry{str, b, e});
		}
	}

	void write(const char* fname);
};

}

#endif
