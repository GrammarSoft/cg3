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

#include "Profiler.hpp"
#include "sorted_vector.hpp"
#include "stdafx.hpp"
#include "filesystem.hpp"
namespace fs = ::std::filesystem;
using namespace CG3;

int main(int argc, char* argv[]) {
	Profiler out;
	out.read(argv[2]);

	std::map<size_t, std::string_view> out_strings;
	for (auto& it : out.strings) {
		out_strings[it.second] = it.first;
	}

	for (int i = 3; i < argc; ++i) {
		Profiler in;
		in.read(argv[i]);

		std::map<size_t, std::string_view> strings;
		for (auto& it : in.strings) {
			strings[it.second] = it.first;
		}

		if (out_strings[0] != strings[0]) {
			throw std::runtime_error("Cannot merge database from different grammars!");
		}

		for (auto& it : in.rule_contexts) {
			out.rule_contexts[it.first] += it.second;
		}

		for (auto& it : in.entries) {
			auto& ie = it.second;
			auto& oe = out.entries[it.first];
			oe.num_match += ie.num_match;
			oe.num_fail += ie.num_fail;
			if (!oe.example_window && ie.example_window) {
				auto id = out.addString(strings[ie.example_window]);
				oe.example_window = id;
			}
		}
	}

	out.write(argv[1]);
}
