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

GrammarApplicator::GrammarApplicator() {
	grammar = 0;
}

GrammarApplicator::~GrammarApplicator() {
	grammar = 0;
}

void GrammarApplicator::setGrammar(Grammar *res) {
	grammar = res;
}

uint32_t GrammarApplicator::addTag(UChar *txt) {
	Tag *tag = new Tag();
	tag->parseTag(txt);
	uint32_t hash = tag->rehash();
	if (single_tags.find(hash) == single_tags.end()) {
		single_tags[hash] = tag;
	} else {
		delete tag;
	}
	return hash;
}
