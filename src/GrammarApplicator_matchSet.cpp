/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */

#include "GrammarApplicator.h"
#include "uextras.h"

using namespace CG3;
using namespace CG3::Strings;

bool GrammarApplicator::doesTagMatchSet(uint32_t tag, uint32_t set) {
	bool retval = false;
	Tag *thetag = single_tags[tag];

	stdext::hash_map<uint32_t, Set*>::const_iterator iter = grammar->uniqsets.find(set);
	if (iter != grammar->uniqsets.end()) {
		const Set *theset = iter->second;

		CompositeTag *ctag = new CompositeTag();
		ctag->addTag(tag);
		ctag->rehash();

		if (theset->tags.find(ctag->hash) != theset->tags.end()) {
			retval = true;
		}
		delete ctag;
	}
	return retval;
}
