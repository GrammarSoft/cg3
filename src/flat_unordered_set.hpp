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
#ifndef c6d28b7452ec699b_FLAT_UNORDERED_SET_HPP
#define c6d28b7452ec699b_FLAT_UNORDERED_SET_HPP
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstdint>

namespace CG3 {

template<typename T, T res_empty = T(-1), T res_del = T(-1) - 1>
class flat_unordered_set {
public:
	class const_iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
	private:
		friend class flat_unordered_set;
		const flat_unordered_set* fus;
		size_t i;

	public:
		typedef T reference;

		const_iterator()
		  : fus(0)
		  , i(0)
		{
		}

		const_iterator(const flat_unordered_set& fus, size_t i = 0)
		  : fus(&fus)
		  , i(i)
		{
		}

		const_iterator& operator=(const const_iterator& o) {
			fus = o.fus;
			i = o.i;
			return *this;
		}

		const_iterator& operator++() {
			for (++i; i < fus->capacity(); ++i) {
				if (fus->elements[i] != res_empty && fus->elements[i] != res_del) {
					break;
				}
			}
			if (i >= fus->capacity()) {
				fus = 0;
				i = 0;
			}
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator tmp(*this);
			operator++();
			return tmp;
		}

		const_iterator& operator--() {
			if (i == 0) {
				fus = 0;
				i = 0;
			}
			else {
				for (--i; i > 0; --i) {
					if (fus->elements[i] != res_empty && fus->elements[i] != res_del) {
						break;
					}
				}
			}
			return *this;
		}

		bool operator==(const const_iterator& o) const {
			return fus == o.fus && i == o.i;
		}

		bool operator!=(const const_iterator& o) const {
			return !(*this == o);
		}

		T operator*() const {
			return fus->elements[i];
		}
	};

	typedef const_iterator iterator;
	typedef typename std::vector<T> container;
	typedef typename container::size_type size_type;
	typedef T value_type;
	typedef T key_type;
	enum {
		DEFAULT_CAP = static_cast<size_type>(16u),
	};

	flat_unordered_set()
	  : size_(0)
	{
	}

	void insert(T t) {
		assert(t != res_empty && t != res_del && "Value cannot be res_empty or res_del!");

		if ((size_ + 1) * 3 / 2 >= capacity() / 2) {
			reserve(std::max(static_cast<size_type>(DEFAULT_CAP), capacity() * 2));
		}
		size_t max = capacity() - 1;
		size_t spot = hash_value(t) & max;
		while (elements[spot] != res_empty && elements[spot] != t) {
			spot = (spot + 5) & max;
		}
		if (elements[spot] != t) {
			elements[spot] = t;
			++size_;
		}
	}

	template<typename It>
	void insert(It b, It e) {
		size_t d = std::distance(b, e);
		size_t c = capacity();
		while ((size_ + d) * 3 / 2 >= c / 2) {
			c = std::max(static_cast<size_type>(DEFAULT_CAP), c * 2);
		}
		if (c != capacity()) {
			reserve(c);
		}

		for (; b != e; ++b) {
			insert(*b);
		}
	}

	void erase(T t) {
		assert(t != res_empty && t != res_del && "Value cannot be res_empty or res_del!");

		if (size_ == 0) {
			return;
		}
		size_t max = capacity() - 1;
		size_t spot = hash_value(t) & max;
		while (elements[spot] != res_empty && elements[spot] != t) {
			spot = (spot + 5) & max;
		}
		if (elements[spot] == t) {
			elements[spot] = res_del;
			--size_;
		}
	}

	const_iterator erase(const_iterator it) {
		elements[it.i] = res_del;
		++it;
		--size_;
		return it;
	}

	const_iterator find(T t) const {
		assert(t != res_empty && t != res_del && "Value cannot be res_empty or res_del!");

		const_iterator it;

		if (size_) {
			size_t max = capacity() - 1;
			size_t spot = hash_value(t) & max;
			while (elements[spot] != res_empty && elements[spot] != t) {
				spot = (spot + 5) & max;
			}
			if (elements[spot] == t) {
				it.fus = this;
				it.i = spot;
			}
		}

		return it;
	}

	size_t count(T t) const {
		return (find(t) != end());
	}

	const_iterator begin() const {
		if (size_ == 0) {
			return end();
		}
		for (size_t i = 0, ie = capacity(); i < ie; ++i) {
			if (elements[i] != res_empty && elements[i] != res_del) {
				return const_iterator(*this, i);
			}
		}
		return end();
	}

	const_iterator end() const {
		return const_iterator();
	}

	size_type size() const {
		return size_;
	}

	size_type capacity() const {
		return elements.size();
	}

	void reserve(size_type n) {
		if (size_ == 0) {
			elements.resize(n, res_empty);
			return;
		}

		static container vals;
		vals.resize(0);
		vals.reserve(size_);
		for (size_type i = 0, ie = capacity(); i < ie; ++i) {
			if (elements[i] != res_empty && elements[i] != res_del) {
				vals.push_back(elements[i]);
			}
		}

		clear(n);
		size_ = vals.size();
		size_t max = capacity() - 1;
		for (size_type i = 0, ie = vals.size(); i < ie; ++i) {
			size_t spot = hash_value(vals[i]) & max;
			while (elements[spot] != res_empty && elements[spot] != vals[i]) {
				spot = (spot + 5) & max;
			}
			elements[spot] = vals[i];
		}
	}

	bool empty() const {
		return (size_ == 0);
	}

	template<typename It>
	void assign(It b, It e) {
		clear();
		insert(b, e);
	}

	void swap(flat_unordered_set& other) {
		std::swap(size_, other.size_);
		elements.swap(other.elements);
	}

	void clear(size_type n = 0) {
		size_ = elements.size();
		elements.resize(0);
		elements.resize(std::max(size_, n), res_empty);
		size_ = 0;
	}

	container& get() {
		return elements;
	}

private:
	size_type size_;
	container elements;

	T hash_value(T t) const {
		return (t << 8) | ((t >> 8) & 0xFF);
	}

	friend class const_iterator;
};

typedef flat_unordered_set<uint32_t> uint32FlatHashSet;
}

#endif
