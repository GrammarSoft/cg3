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
#ifndef __BLOOMISH_HPP
#define __BLOOMISH_HPP
#include <algorithm>
#include <stdint.h> // C99 or C++0x or C++ TR1 will have this header. ToDo: Change to <cstdint> when C++0x broader support gets under way.

namespace CG3 {

template<typename Cont>
class bloomish {
private:
	Cont value[4];

public:
	bloomish() {
		std::fill(value, value+4, 0);
	}

	bloomish(const bloomish<Cont>& other) {
		std::copy(other.value, other.value+4, &value[0]);
	}

	void insert(const Cont& v) {
		if (v & 4) {
			value[3] |= v;
		}
		else if (v & 2) {
			value[2] |= v;
		}
		else if (v & 1) {
			value[1] |= v;
		}
		else {
			value[0] |= v;
		}
	}

	bool matches(const Cont& v) const {
		if (v & 4) {
			return (value[3] & v) == v;
		}
		else if (v & 2) {
			return (value[2] & v) == v;
		}
		else if (v & 1) {
			return (value[1] & v) == v;
		}
		return (value[0] & v) == v;
	}
};

typedef bloomish<uint32_t> uint32Bloomish;

}

#endif
