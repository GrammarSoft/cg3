/*
* Copyright (C) 2007, GrammarSoft Aps
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

#ifndef __RECYCLER_H
#define __RECYCLER_H

#include "stdafx.h"
#include "Cohort.h"
#include "Reading.h"

namespace CG3 {

	class Recycler {
	public:
		static Recycler *instance();
		static void cleanup();

		void trim();

		Cohort *new_Cohort(SingleWindow*);
		void delete_Cohort(Cohort*);

		Reading *new_Reading(Cohort*);
		void delete_Reading(Reading*);

		uint32Set *new_uint32Set();
		void delete_uint32Set(uint32Set*);

		uint32HashSet *new_uint32HashSet();
		void delete_uint32HashSet(uint32HashSet*);

	protected:
		Recycler();
		~Recycler();

	private:
		static Recycler *gRecycler;

		uint32_t ACohorts, DCohorts;
		std::vector<Cohort*> Cohorts;

		uint32_t AReadings, DReadings;
		std::vector<Reading*> Readings;

		uint32_t Auint32Sets, Duint32Sets;
		std::vector<uint32Set*> uint32Sets;

		uint32_t Auint32HashSets, Duint32HashSets;
		std::vector<uint32HashSet*> uint32HashSets;
	};

}

#endif
