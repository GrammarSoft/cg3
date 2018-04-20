/*
* Copyright (C) 2007-2018, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
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

#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Cohort.hpp"
#include "GrammarApplicator.hpp"

namespace CG3 {

Window::Window(GrammarApplicator* p)
  : parent(p)
  , cohort_counter(1)
  , window_counter(0)
  , window_span(0)
  , current(0)
{
}

Window::~Window() {
	SingleWindowCont::iterator iter;
	for (iter = previous.begin(); iter != previous.end(); iter++) {
		delete *iter;
	}

	delete current;
	current = 0;

	for (iter = next.begin(); iter != next.end(); iter++) {
		delete *iter;
	}
}

SingleWindow* Window::allocSingleWindow() {
	SingleWindow* swindow = alloc_swindow(this);
	window_counter++;
	swindow->number = window_counter;
	return swindow;
}

SingleWindow* Window::allocPushSingleWindow() {
	SingleWindow* swindow = alloc_swindow(this);
	window_counter++;
	swindow->number = window_counter;
	if (!next.empty()) {
		swindow->next = next.front();
		next.front()->previous = swindow;
	}
	if (current) {
		swindow->previous = current;
		current->next = swindow;
	}
	next.insert(next.begin(), swindow);
	return swindow;
}

SingleWindow* Window::allocAppendSingleWindow() {
	SingleWindow* swindow = alloc_swindow(this);
	window_counter++;
	swindow->number = window_counter;
	if (!next.empty()) {
		swindow->previous = next.back();
		next.back()->next = swindow;
	}
	next.push_back(swindow);
	return swindow;
}

void Window::shuffleWindowsDown() {
	if (current) {
		current->variables_set = parent->variables;
		current->variables_rem.clear();
		previous.push_back(current);
		current = 0;
	}

	if (!next.empty()) {
		current = next.front();
		next.erase(next.begin());
	}
}

void Window::rebuildSingleWindowLinks() {
	SingleWindow* sWindow = 0;

	for (auto iter : previous) {
		iter->previous = sWindow;
		if (sWindow) {
			sWindow->next = iter;
		}
		sWindow = iter;
	}

	if (current) {
		current->previous = sWindow;
		if (sWindow) {
			sWindow->next = current;
		}
		sWindow = current;
	}

	for (auto iter : next) {
		iter->previous = sWindow;
		if (sWindow) {
			sWindow->next = iter;
		}
		sWindow = iter;
	}

	if (sWindow) {
		sWindow->next = 0;
	}
}

void Window::rebuildCohortLinks() {
	SingleWindow* sWindow = 0;
	if (!previous.empty()) {
		sWindow = previous.front();
	}
	else if (current) {
		sWindow = current;
	}
	else if (!next.empty()) {
		sWindow = next.front();
	}

	Cohort* prev = 0;
	while (sWindow) {
		for (auto citer : sWindow->cohorts) {
			citer->prev = prev;
			citer->next = 0;
			if (prev) {
				prev->next = citer;
			}
			prev = citer;
		}
		sWindow = sWindow->next;
	}
}
}
