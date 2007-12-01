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
#ifndef __RECYCLER_H
#define __RECYCLER_H

#include "stdafx.h"

namespace CG3 {

	class Recycler {
	public:
		static Recycler *instance();
		static void cleanup();

		uint32Set *new_uint32Set();
		void delete_uint32Set(uint32Set*);

	protected:
		Recycler();
		~Recycler();

	private:
		static Recycler *gRecycler;

		uint32_t Auint32Sets, Duint32Sets;
		std::vector<uint32Set*> uint32Sets;
	};

}

#endif
