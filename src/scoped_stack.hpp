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
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_SCOPED_STACK_HPP
#define c6d28b7452ec699b_SCOPED_STACK_HPP
#include <vector>

namespace CG3 {

template<typename C>
struct scoped_stack {
	struct proxy {
		proxy(scoped_stack* ss)
		  : z(ss->z++)
		  , ss(ss)
		{
			if (ss->cs.size() < ss->z) {
				ss->cs.resize(ss->z);
			}
		}

		~proxy() {
			ss->cs[z].clear();
			--ss->z;
		}

		C* operator->() {
			return &ss->cs[z];
		}

		C& operator*() {
			return ss->cs[z];
		}

		operator C&() {
			return ss->cs[z];
		}

	private:
		size_t z;
		scoped_stack* ss;
	};

	scoped_stack()
	  : z(0)
	{}

	proxy get() {
		return proxy(this);
	}

private:
	friend struct proxy;
	size_t z;
	std::vector<C> cs;
};
}

#endif
