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

#include "Window.h"

using namespace CG3;

Window::Window(GrammarApplicator *p) {
	parent = p;
	current = 0;
	window_span = 0;
	window_counter = 0;
	cohort_counter = 1;
}

Window::~Window() {
	std::list<SingleWindow*>::iterator iter;
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

void Window::pushSingleWindow(SingleWindow *swindow) {
	window_counter++;
	swindow->number = window_counter;
	swindow->parent = this;
	if (!next.empty()) {
		swindow->next = next.front();
		next.front()->previous = swindow;
	}
	if (current) {
		swindow->previous = current;
		current->next = swindow;
	}
	next.push_front(swindow);
}

void Window::appendSingleWindow(SingleWindow *swindow) {
	window_counter++;
	swindow->number = window_counter;
	swindow->parent = this;
	if (!next.empty()) {
		swindow->previous = next.back();
		next.back()->next = swindow;
	}
	next.push_back(swindow);
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
