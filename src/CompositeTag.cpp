/*
* Copyright (C) 2007, GrammarSoft Aps
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

#include "CompositeTag.h"

using namespace CG3;

CompositeTag::CompositeTag() {
	hash = 0;
}

CompositeTag::~CompositeTag() {
}

void CompositeTag::addTag(uint32_t tag) {
	tags.insert(tag);
	tags_set.insert(tag);
}

uint32_t CompositeTag::rehash() {
	uint32_t retval = 0;
	uint32Set::iterator iter;
	for (iter = tags_set.begin() ; iter != tags_set.end() ; iter++) {
		retval = hash_sdbm_uint32_t(*iter, retval);
	}
	hash = retval;
	return retval;
}
