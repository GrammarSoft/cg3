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
#ifndef __GOOGLE_WRAPPER_HPP
#define __GOOGLE_WRAPPER_HPP
#include <limits>
#include <utility>
#include <google/dense_hash_set>
#include <google/dense_hash_map>

namespace CG3 {

template<typename T>
class dense_hash_set {
private:
	typedef typename google::dense_hash_set<T> Cont;
	Cont elements;

public:
	typedef typename Cont::iterator iterator;
	typedef typename Cont::const_iterator const_iterator;
	typedef typename Cont::size_type size_type;
	typedef T value_type;
	typedef T key_type;

	dense_hash_set() {
		elements.set_empty_key(std::numeric_limits<T>::max());
		elements.set_deleted_key(std::numeric_limits<T>::max()-1);
	}

	void insert(T t) {
		elements.insert(t);
	}

	void insert(iterator first, iterator second) {
		elements.insert(first, second);
	}

	void erase(T t) {
		elements.erase(t);
	}

	const_iterator find(T t) const {
		return elements.find(t);
	}

	const_iterator begin() const {
		return elements.begin();
	}

	const_iterator end() const {
		return elements.end();
	}

	size_type size() const {
		return elements.size();
	}

	bool empty() const {
		return elements.empty();
	}

	void clear() {
		elements.clear();
	}
};

template<typename T, typename Y>
class dense_hash_map {
private:
	typedef typename google::dense_hash_map<T,Y> Cont;
	Cont elements;

public:
	typedef typename Cont::iterator iterator;
	typedef typename Cont::const_iterator const_iterator;
	typedef typename Cont::size_type size_type;
	typedef Y value_type;
	typedef T key_type;

	dense_hash_map() {
		elements.set_empty_key(std::numeric_limits<T>::max());
		elements.set_deleted_key(std::numeric_limits<T>::max()-1);
	}

	Y& operator[](size_type i) {
		return elements[i];
	}

	void insert(std::pair<T,Y> v) {
		elements.insert(v);
	}

	void erase(const T& t) {
		elements.erase(t);
	}

	void erase(const iterator& it) {
		elements.erase(it);
	}

	const_iterator find(T t) const {
		return elements.find(t);
	}

	const_iterator begin() const {
		return elements.begin();
	}

	const_iterator end() const {
		return elements.end();
	}

	iterator find(T t) {
		return elements.find(t);
	}

	iterator begin() {
		return elements.begin();
	}

	iterator end() {
		return elements.end();
	}

	size_type size() const {
		return elements.size();
	}

	bool empty() const {
		return elements.empty();
	}

	void clear() {
		elements.clear();
	}
};

}

#endif
