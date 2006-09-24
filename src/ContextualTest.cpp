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
#include "ContextualTest.h"

using namespace CG3;
using namespace CG3::Strings;

ContextualTest::ContextualTest() {
	careful = false;
	negated = false;
	negative = false;
	scanfirst = false;
	scanall = false;
	absolute = false;
	span_windows = false;
	offset = 0;
	target = 0;
	barrier = 0;
	linked = 0;
	hash = 0;
	line = 0;
}

ContextualTest::~ContextualTest() {
	delete linked;
}

void ContextualTest::parsePosition(const UChar *pos) {
	if (u_strstr(pos, stringbits[S_ASTERIKTWO])) {
		scanall = true;
	}
	else if (u_strstr(pos, stringbits[S_ASTERIK])) {
		scanfirst = true;
	}
	if (u_strchr(pos, 'C')) {
		careful = true;
	}
	if (u_strchr(pos, 'W')) {
		span_windows = true;
	}
	if (u_strchr(pos, '@')) {
		absolute = true;
	}
	UChar tmp[8];
	u_sscanf(pos, "%[^0-9]%d", &tmp, &offset);
	if (u_strchr(pos, '-')) {
		offset = (-1) * abs(offset);
	}
}

ContextualTest *ContextualTest::allocateContextualTest() {
	return new ContextualTest;
}

void ContextualTest::destroyContextualTest(ContextualTest *to) {
	delete to;
}

uint32_t ContextualTest::rehash() {
	hash = 0;
	hash = hash_sdbm_uint32_t(hash, (uint32_t)careful);
	hash = hash_sdbm_uint32_t(hash, (uint32_t)negated);
	hash = hash_sdbm_uint32_t(hash, (uint32_t)negative);
	hash = hash_sdbm_uint32_t(hash, (uint32_t)scanfirst);
	hash = hash_sdbm_uint32_t(hash, (uint32_t)scanall);
	hash = hash_sdbm_uint32_t(hash, (uint32_t)absolute);
	hash = hash_sdbm_uint32_t(hash, (uint32_t)span_windows);
	hash = hash_sdbm_uint32_t(hash, target);
	hash = hash_sdbm_uint32_t(hash, barrier);
	hash = hash_sdbm_uint32_t(hash, abs(offset));
	if (offset < 0) {
		hash = hash_sdbm_uint32_t(hash, 5000);
	}
	if (linked) {
		hash = hash_sdbm_uint32_t(hash, linked->rehash());
	}
	assert(hash != 0);
	return hash;
}
