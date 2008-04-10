/*
* Copyright (C) 2007, GrammarSoft ApS
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
