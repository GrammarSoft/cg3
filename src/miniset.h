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
#ifndef __MINISET_H
#define __MINISET_H
#include <vector>
#include <algorithm>

namespace CG3 {

template<typename T>
class miniset {
private:
	typedef typename std::vector<T> Cont;
	Cont elements;

public:
	typedef typename Cont::iterator iterator;
	typedef typename Cont::const_iterator const_iterator;
	typedef T value_type;
	typedef T key_type;

	bool insert(T t) {
		iterator it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it == elements.end() || *it != t) {
			elements.insert(it, t);
			return true;
		}
		return false;
	}

	bool erase(T t) {
		iterator it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it != elements.end() && *it == t) {
			elements.erase(it);
			return true;
		}
		return false;
	}

	const_iterator find(T t) const {
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

	void clear() {
		elements.clear();
	}
};

typedef miniset<uint32_t> uint32MiniSet;

}

#endif
