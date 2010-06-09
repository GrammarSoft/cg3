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

#pragma once
#ifndef __SORTED_VECTOR_HPP
#define __SORTED_VECTOR_HPP
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdint.h> // C99 or C++0x or C++ TR1 will have this header. ToDo: Change to <cstdint> when C++0x broader support gets under way.

namespace CG3 {

template<typename T>
class sorted_vector {
private:
	typedef typename std::vector<T> Cont;
	typedef typename Cont::iterator iterator;
	Cont elements;

public:
	typedef typename Cont::const_iterator const_iterator;
	typedef typename Cont::size_type size_type;
	typedef T value_type;
	typedef T key_type;

	#ifdef CG_TRACE_OBJECTS
	sorted_vector() {
		std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << std::endl;
	}

	~sorted_vector() {
		std::cerr << "OBJECT: " << __PRETTY_FUNCTION__ << ": " << elements.size() << std::endl;
	}
	#endif

	bool insert(T t) {
		if (elements.empty()) {
			elements.push_back(t);
			return true;
		}
		else if (elements.back() < t) {
			elements.push_back(t);
			return true;
		}
		else if (t < elements.front()) {
			elements.insert(elements.begin(), t);
			return true;
		}
		iterator it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it == elements.end() || *it != t) {
			elements.insert(it, t);
			return true;
		}
		return false;
	}

	bool push_back(T t) {
		return insert(t);
	}

	bool erase(T t) {
		if (elements.empty()) {
			return false;
		}
		else if (elements.back() < t) {
			return false;
		}
		else if (t < elements.front()) {
			return false;
		}
		iterator it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it != elements.end() && *it == t) {
			elements.erase(it);
			return true;
		}
		return false;
	}

	const_iterator find(T t) const {
		if (elements.empty()) {
			return elements.end();
		}
		else if (elements.back() < t) {
			return elements.end();
		}
		else if (t < elements.front()) {
			return elements.end();
		}
		const_iterator it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it != elements.end() && *it != t) {
			return elements.end();
		}
		return it;
	}

	const_iterator begin() const {
		return elements.begin();
	}

	const_iterator end() const {
		return elements.end();
	}

	size_type size() const {
		return elements.size();
	}

	bool empty() const {
		return elements.empty();
	}

	void clear() {
		elements.clear();
	}
};

typedef sorted_vector<uint32_t> uint32SortedVector;

}

#endif
