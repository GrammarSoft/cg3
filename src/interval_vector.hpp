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

#pragma once
#ifndef c6d28b7452ec699b_INTERVAL_VECTOR_HPP
#define c6d28b7452ec699b_INTERVAL_VECTOR_HPP
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace CG3 {

template<typename T>
class interval_vector {
private:
	struct interval {
		T lb;
		T ub;

		explicit interval(T lb = T())
		  : lb(lb)
		  , ub(lb)
		{
		}

		explicit interval(T lb, T ub)
		  : lb(lb)
		  , ub(ub)
		{
		}

		bool operator<(const interval& o) const {
			return (ub < o.lb);
		}

		bool operator<(const T& o) const {
			return (ub < o);
		}
	};
	typedef typename std::vector<interval> Cont;
	typedef typename Cont::iterator ContIter;
	typedef typename Cont::const_iterator ContConstIter;
	Cont elements;
	size_t _size;

public:
	class const_iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
	private:
		const Cont* elements;
		ContConstIter it;
		T t;

	public:
		typedef T reference;

		const_iterator()
		  : elements(0)
		  , t(T())
		{
		}

		const_iterator(const Cont& elements, ContConstIter it)
		  : elements(&elements)
		  , it(it)
		  , t(T())
		{
			if (it != elements.end()) {
				t = it->lb;
			}
		}

		const_iterator(const Cont& elements, ContConstIter it, T t)
		  : elements(&elements)
		  , it(it)
		  , t(t)
		{
		}

		const_iterator& operator=(const const_iterator& o) {
			it = o.it;
			t = o.t;
			return *this;
		}

		const_iterator& operator++() {
			if (it == elements->end()) {
				t = T();
				return *this;
			}
			if (t == it->ub) {
				++it;
				if (it == elements->end()) {
					t = T();
				}
				else {
					t = it->lb;
				}
			}
			else {
				++t;
			}
			return *this;
		}

		const_iterator& operator--() {
			if (it == elements->end() || t == it->lb) {
				if (it == elements->begin()) {
					t = T();
					it = elements->end();
				}
				else {
					--it;
					t = it->ub;
				}
			}
			else {
				--t;
			}
			return *this;
		}

		bool operator==(const const_iterator& o) const {
			return it == o.it && t == o.t;
		}

		bool operator!=(const const_iterator& o) const {
			return !(*this == o);
		}

		T operator*() const {
			return t;
		}
	};

	typedef const_iterator iterator;
	typedef size_t size_type;
	typedef T value_type;
	typedef T key_type;

	interval_vector()
	  : _size(0)
	{
	}

	template<typename Iter>
	interval_vector(Iter b, const Iter& e)
	  : _size(0)
	{
		for (; b != e; ++b) {
			insert(*b);
		}
	}

	bool insert(T t) {
		ContIter it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it != elements.end() && t >= it->lb && t <= it->ub) {
			return false;
		}
		ContIter pr = it - 1;
		if (it != elements.begin() && pr->ub + 1 == t) {
			++pr->ub;
			if (it != elements.end() && pr->ub + 1 == it->lb) {
				pr->ub = it->ub;
				elements.erase(it);
			}
		}
		else if (it != elements.end() && it->lb == t + 1) {
			--it->lb;
			if (it != elements.begin() && pr->ub + 1 == it->lb) {
				pr->ub = it->ub;
				elements.erase(it);
			}
		}
		else {
			elements.insert(it, interval(t));
		}
		++_size;
		return true;
	}

	template<typename It>
	void insert(It b, It e) {
		for (; b != e; ++b) {
			insert(*b);
		}
	}

	bool push_back(T t) {
		return insert(t);
	}

	bool erase(T t) {
		ContIter it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it == elements.end()) {
			return false;
		}
		if (it->ub < t || it->lb > t) {
			return false;
		}
		if (it->lb == t && it->ub == t) {
			elements.erase(it);
			--_size;
			return true;
		}
		if (it->ub == t) {
			--it->ub;
			--_size;
			return true;
		}
		if (it->lb == t) {
			++it->lb;
			--_size;
			return true;
		}
		if (it->lb < t && it->ub > t) {
			elements.insert(it + 1, interval(t + 1, it->ub));
			it->ub = t - 1;
			--_size;
			return true;
		}

		assert(false && "interval_vector.erase() should never reach this place...");
		return false;
	}

	const_iterator find(T t) const {
		ContConstIter it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it == elements.end()) {
			return end();
		}
		if (it->ub < t || it->lb > t) {
			return end();
		}
		return const_iterator(elements, it, t);
	}

	const_iterator lower_bound(T t) const {
		ContConstIter it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it == elements.end()) {
			return end();
		}
		if (it->ub < t || it->lb > t) {
			++it;
			if (it == elements.end()) {
				return end();
			}
			t = it->lb;
		}
		return const_iterator(elements, it, t);
	}

	bool contains(T t) const {
		ContConstIter it = std::lower_bound(elements.begin(), elements.end(), t);
		if (it == elements.end()) {
			return false;
		}
		if (it->ub < t || it->lb > t) {
			return false;
		}
		return true;
	}

	const_iterator begin() const {
		return const_iterator(elements, elements.begin());
	}

	const_iterator end() const {
		return const_iterator(elements, elements.end());
	}

	T front() const {
		return elements.front().lb;
	}

	T back() const {
		return elements.back().ub;
	}

	size_type size() const {
		return _size;
	}

	bool empty() const {
		return elements.empty();
	}

	void clear() {
		elements.clear();
		_size = 0;
	}

	interval_vector intersect(const interval_vector& o) const {
		interval_vector rv;
		if (!empty() && !o.empty()) {
			ContConstIter a = elements.begin();
			ContConstIter b = o.elements.begin();
			while (a != elements.end() && b != o.elements.end()) {
				while (a != elements.end() && b != o.elements.end() && a->ub < b->lb) {
					++a;
				}
				while (a != elements.end() && b != o.elements.end() && b->ub < a->lb) {
					++b;
				}
				while (a != elements.end() && b != o.elements.end() && a->ub >= b->lb && b->ub >= a->lb) {
					const T lb = std::max(a->lb, b->lb);
					const T ub = std::min(a->ub, b->ub);
					if (!rv.elements.empty() && rv.elements.back().ub + 1 == lb) {
						rv.elements.back().ub = ub;
					}
					else {
						rv.elements.push_back(interval(lb, ub));
					}
					rv._size += ub - lb + 1;
					if (a->ub < b->ub) {
						++a;
					}
					else {
						++b;
					}
				}
			}
		}
		return rv;
	}
};

typedef interval_vector<uint32_t> uint32IntervalVector;
}

#endif
