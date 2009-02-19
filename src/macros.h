/*
* Copyright (C) 2007, GrammarSoft ApS
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

#ifndef __MACROS_H
#define __MACROS_H

#define index_matches(index, entry) ((index).find(entry) != (index).end())

#define foreach(type, container, iter, iter_end) \
	type::iterator iter; \
	type::iterator iter_end = (container).end(); \
	for (iter = (container).begin() ; iter != iter_end ; iter++) 

#define const_foreach(type, container, iter, iter_end) \
	type::const_iterator iter; \
	type::const_iterator iter_end = (container).end(); \
	for (iter = (container).begin() ; iter != iter_end ; iter++) 

#define reverse_foreach(type, container, iter, iter_end) \
	type::reverse_iterator iter; \
	type::reverse_iterator iter_end = (container).rend(); \
	for (iter = (container).rbegin() ; iter != iter_end ; iter++) 

#define reverse_const_foreach(type, container, iter, iter_end) \
	type::reverse_const_iterator iter; \
	type::reverse_const_iterator iter_end = (container).rend(); \
	for (iter = (container).rbegin() ; iter != iter_end ; iter++) 

#endif
