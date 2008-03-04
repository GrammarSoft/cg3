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
#include "Rule.h"

using namespace CG3;
using namespace CG3::Strings;

Rule::Rule() {
	name = 0;
	target = 0;
	dep_target = 0;
	line = 0;
	wordform = 0;
	num_fail = 0;
	num_match = 0;
	total_time = 0;
	varname = 0;
	varvalue = 0;
	weight = 0.0;
	quality = 0.0;
	type = K_IGNORE;
}

Rule::~Rule() {
	delete[] name;
	std::list<ContextualTest*>::iterator iter;
	for (iter = tests.begin() ; iter != tests.end() ; iter++) {
		delete (*iter);
	}

	for (iter = dep_tests.begin() ; iter != dep_tests.end() ; iter++) {
		delete (*iter);
	}

	delete dep_target;
}

void Rule::setName(const UChar *to) {
	if (to) {
		name = new UChar[u_strlen(to)+1];
		u_strcpy(name, to);
	}
}

ContextualTest *Rule::allocateContextualTest() {
	return new ContextualTest;
}

void Rule::addContextualTest(ContextualTest *to, std::list<ContextualTest*> *thelist) {
	thelist->push_back(to);
}

void Rule::resetStatistics() {
	std::list<ContextualTest*>::iterator iter;
	for (iter = tests.begin() ; iter != tests.end() ; iter++) {
		(*iter)->resetStatistics();
	}
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}

bool Rule::cmp_quality(Rule *a, Rule *b) {
	return a->total_time > b->total_time;
}
