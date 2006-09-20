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

Window::Window() {
	current = 0;
	num_windows = 0;
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
}

void Window::shuffleWindowsDown() {
	if (!previous.empty() && previous.size() >= num_windows) {
		delete previous.front();
		previous.pop_front();
	}

	if (current) {
		previous.push_back(current);
	}

	if (!next.empty()) {
		current = next.front();
		next.pop_front();
	}
}
