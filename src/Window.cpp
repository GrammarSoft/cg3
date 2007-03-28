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
	previous.clear();

	if (current) {
		delete current;
	}

	for (iter = next.begin() ; iter != next.end() ; iter++) {
		delete *iter;
	}
	next.clear();

	cohort_map.clear();
	dep_map.clear();
	dep_window.clear();
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
