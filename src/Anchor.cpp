/*
* Copyright (C) 2007-2011, GrammarSoft ApS
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

#include "Anchor.h"
#include "Strings.h"

namespace CG3 {

Anchor::Anchor() :
line(0)
{
	// Nothing in the actual body...
}

void Anchor::setName(uint32_t to) {
	if (!to) {
		to = static_cast<uint32_t>(rand());
	}
	size_t n = sprintf(&cbuffers[0][0], "_A_%u_%u_", line, to);
	name.clear();
	name.reserve(n);
	for (size_t i=0 ; i<n ; ++i) {
		name.push_back(cbuffers[0][i]);
	}
}
void Anchor::setName(const UChar *to) {
	if (to) {
		name = to;
	}
	else {
		setName(static_cast<uint32_t>(rand()));
	}
}

}
