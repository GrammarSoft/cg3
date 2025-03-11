/*
* Copyright (C) 2007-2025, GrammarSoft ApS
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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_POOL_HPP
#define c6d28b7452ec699b_POOL_HPP
#include "sorted_vector.hpp"
#include <stdexcept>

namespace CG3 {

template<typename T>
struct pool {
	using pool_t = sorted_vector<T*>;
	pool_t p;

	~pool() {
		for (auto it : p) {
			delete it;
		}
	}

	auto get() {
		typename pool_t::value_type var = nullptr;
		if (!p.empty()) {
			var = p.back();
			p.pop_back();
		}

		#ifdef CG_TRACE_OBJECTS
		std::cerr << "OBJECT: " << VOIDP(var) << " " << __PRETTY_FUNCTION__ << std::endl;
		#endif

		return var;
	}

	void put(T* t) {
		#ifdef CG_TRACE_OBJECTS
		std::cerr << "OBJECT: " << VOIDP(t) << " " << __PRETTY_FUNCTION__ << std::endl;
		#endif

		t->clear();
		auto ins = p.insert(t);
		if (ins.second == false) {
			// Enable to debug memory access errors
			//throw std::runtime_error("Pool cannot insert item twice!");
		}
	}
};

}

#endif
