/*
* Copyright (C) 2007-2011, GrammarSoft ApS
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

#include "Window.h"
#include "SingleWindow.h"
#include "Cohort.h"

namespace CG3 {

Window::Window(GrammarApplicator *p) {
	parent = p;
	current = 0;
	window_span = 0;
	window_counter = 0;
	cohort_counter = 1;
}

Window::~Window() {
	SingleWindowCont::iterator iter;
	for (iter = previous.begin() ; iter != previous.end() ; iter++) {
		delete *iter;
	}

	if (current) {
		delete current;
		current = 0;
	}

	for (iter = next.begin() ; iter != next.end() ; iter++) {
		delete *iter;
	}
}

SingleWindow *Window::allocSingleWindow() {
	SingleWindow *swindow = new SingleWindow(this);
	window_counter++;
	swindow->number = window_counter;
	return swindow;
}

SingleWindow *Window::allocPushSingleWindow() {
	SingleWindow *swindow = new SingleWindow(this);
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
	next.push_front(swindow);
	return swindow;
}

SingleWindow *Window::allocAppendSingleWindow() {
	SingleWindow *swindow = new SingleWindow(this);
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
		previous.push_back(current);
		current = 0;
	}

	if (!next.empty()) {
		current = next.front();
		next.pop_front();
	}
}

void Window::rebuildSingleWindowLinks() {
	SingleWindow *sWindow = 0;

	foreach (SingleWindowCont, previous, iter, iter_end) {
		(*iter)->previous = sWindow;
		if (sWindow) {
			sWindow->next = *iter;
		}
		sWindow = *iter;
	}

	if (current) {
		current->previous = sWindow;
		if (sWindow) {
			sWindow->next = current;
		}
		sWindow = current;
	}

	foreach (SingleWindowCont, next, iter, iter_end) {
		(*iter)->previous = sWindow;
		if (sWindow) {
			sWindow->next = *iter;
		}
		sWindow = *iter;
	}

	if (sWindow) {
		sWindow->next = 0;
	}
}

void Window::rebuildCohortLinks() {
	SingleWindow *sWindow = 0;
	if (!previous.empty()) {
		sWindow = previous.front();
	}
	else if (current) {
		sWindow = current;
	}
	else if (!next.empty()) {
		sWindow = next.front();
	}

	Cohort *prev = 0;
	while (sWindow) {
		foreach (CohortVector, sWindow->cohorts, citer, citer_end) {
			(*citer)->prev = prev;
			(*citer)->next = 0;
			if (prev) {
				prev->next = *citer;
			}
			prev = *citer;
		}
		sWindow = sWindow->next;
	}
}

}
