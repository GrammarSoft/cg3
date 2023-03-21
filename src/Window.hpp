/*
* Copyright (C) 2007-2023, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this progam.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_WINDOW_H
#define c6d28b7452ec699b_WINDOW_H

#include "stdafx.hpp"

namespace CG3 {
class GrammarApplicator;
class Cohort;
class SingleWindow;

typedef std::vector<SingleWindow*> SingleWindowCont;

class Window {
public:
	GrammarApplicator* parent = nullptr;
	uint32_t cohort_counter = 0;
	uint32_t window_counter = 0;
	uint32_t window_span = 0;

	std::map<uint32_t, Cohort*> cohort_map;
	uint32FlatHashMap dep_map;
	std::map<uint32_t, Cohort*> dep_window;
	uint32FlatHashMap relation_map;

	SingleWindowCont previous;
	SingleWindow* current = nullptr;
	SingleWindowCont next;

	Window(GrammarApplicator* p);
	~Window();

	SingleWindow* allocSingleWindow();
	SingleWindow* allocPushSingleWindow();
	SingleWindow* allocAppendSingleWindow();
	SingleWindow* back();
	void shuffleWindowsDown();
	void rebuildSingleWindowLinks();
	void rebuildCohortLinks();
};
}

#endif
