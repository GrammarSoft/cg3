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
	jumpstart = 0;
	jumpend = 0;
	weight = 0.0;
	quality = 0.0;
	type = K_IGNORE;
	flags = 0;
	test_head = 0;
	dep_test_head = 0;
}

Rule::~Rule() {
	delete[] name;

	while (test_head) {
		ContextualTest *t = test_head->next;
		delete test_head;
		test_head = t;
	}
	while (dep_test_head) {
		ContextualTest *t = dep_test_head->next;
		delete dep_test_head;
		dep_test_head = t;
	}

	delete dep_target;
}

void Rule::setName(const UChar *to) {
	if (name) {
		delete[] name;
	}
	if (to) {
		name = new UChar[u_strlen(to)+1];
		u_strcpy(name, to);
	}
}

ContextualTest *Rule::allocateContextualTest() {
	return new ContextualTest;
}

void Rule::addContextualTest(ContextualTest *to, ContextualTest **head) {
	if (*head) {
		(*head)->prev = to;
		to->next = *head;
	}
	*head = to;
}

void Rule::resetStatistics() {
	ContextualTest *t = test_head;
	while (t) {
		t->resetStatistics();
		t = t->next;
	}
	t = dep_test_head;
	while (t) {
		t->resetStatistics();
		t = t->next;
	}
	num_fail = 0;
	num_match = 0;
	total_time = 0;
}

bool Rule::cmp_quality(const Rule *a, const Rule *b) {
	return a->total_time > b->total_time;
}
