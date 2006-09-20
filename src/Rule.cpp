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
#include "stdafx.h"
#include <unicode/ustring.h>
#include "Strings.h"
#include "Rule.h"
#include "ContextualTest.h"

using namespace CG3;
using namespace CG3::Strings;

Rule::Rule() {
	name = 0;
	subst_target = 0;
	target = 0;
	line = 0;
	wordform = 0;
	type = K_IGNORE;
}

Rule::~Rule() {
	delete name;
	std::list<ContextualTest*>::iterator iter;
	for (iter = tests.begin() ; iter != tests.end() ; iter++) {
		delete (*iter);
	}
	tests.clear();
}

void Rule::setName(uint32_t to) {
	if (!to) {
		to = (uint32_t)rand();
	}
	name = new UChar[32];
	memset(name, 0, 32);
	u_sprintf(name, "_R_%u_%u_", line, to);
}
void Rule::setName(const UChar *to) {
	if (to) {
		name = new UChar[u_strlen(to)+1];
		u_strcpy(name, to);
	} else {
		setName((uint32_t)rand());
	}
}

ContextualTest *Rule::allocateContextualTest() {
	return new ContextualTest;
}

void Rule::addContextualTest(ContextualTest *to) {
	tests.push_back(to);
}

void Rule::destroyContextualTest(ContextualTest *to) {
	tests.remove(to);
	delete to;
}
