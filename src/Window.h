/*
* Copyright (C) 2007-2010, GrammarSoft ApS
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

#pragma once
#ifndef __WINDOW_H
#define __WINDOW_H

#include "stdafx.h"

namespace CG3 {
	class GrammarApplicator;
	class Cohort;
	class SingleWindow;

	class Window {
	public:
		GrammarApplicator *parent;
		uint32_t cohort_counter;
		uint32_t window_counter;
		uint32_t window_span;

		std::map<uint32_t, Cohort*> cohort_map;
		uint32HashMap dep_map;
		std::map<uint32_t, Cohort*> dep_window;

		std::list<SingleWindow*> previous;
		SingleWindow *current;
		std::list<SingleWindow*> next;

		Window(GrammarApplicator *p);
		~Window();

		SingleWindow *allocPushSingleWindow();
		SingleWindow *allocAppendSingleWindow();
		void shuffleWindowsDown();
	};

}

#endif
