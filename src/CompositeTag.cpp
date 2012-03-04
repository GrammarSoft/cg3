/*
* Copyright (C) 2007-2012, GrammarSoft ApS
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
#include "Tag.h"

namespace CG3 {

CompositeTag::CompositeTag() :
is_used(false),
is_special(false),
hash(0),
number(0)
{
	// Nothing in the actual body...
}

void CompositeTag::addTag(Tag *tag) {
	tags.push_back(tag);
	tags_set.insert(tag);
	if (tag->type & T_SPECIAL) {
		is_special = true;
	}
}

uint32_t CompositeTag::rehash() {
	uint32_t retval = 0;
	foreach (TagList, tags, iter, iter_end) {
		retval = hash_sdbm_uint32_t((*iter)->hash, retval);
	}
	hash = retval;
	return retval;
}

void CompositeTag::markUsed() {
	is_used = true;
	foreach (TagList, tags, itag, itag_end) {
		(*itag)->markUsed();
	}
}

}
