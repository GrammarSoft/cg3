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
#include "Recycler.h"

using namespace CG3;

Recycler *Recycler::gRecycler = 0;

Recycler::Recycler() {
	Auint32Sets = Duint32Sets = 0;
	for (uint32_t i=0;i<500;i++) {
		uint32Set *t = new uint32Set;
		uint32Sets.push_back(t);
	}
}

Recycler::~Recycler() {
	while (!uint32Sets.empty()) {
		delete uint32Sets.back();
		uint32Sets.pop_back();
	}
}

Recycler *Recycler::instance() {
	if (!gRecycler) {
		gRecycler = new Recycler();
	}
	return gRecycler;
}

void Recycler::cleanup() {
	delete gRecycler;
	gRecycler = 0;
}

uint32Set *Recycler::new_uint32Set() {
	Auint32Sets++;
	if (!uint32Sets.empty()) {
		uint32Set *t = uint32Sets.back();
		uint32Sets.pop_back();
		return t;
	}
	return new uint32Set;
}

void Recycler::delete_uint32Set(uint32Set *t) {
	Duint32Sets++;
	t->clear();
	uint32Sets.push_back(t);
}
