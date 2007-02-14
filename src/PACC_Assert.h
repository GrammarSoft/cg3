/*
 *  Portable Agile C++ Classes (PACC)
 *  Copyright (C) 2001-2004 by Marc Parizeau
 *  http://manitou.gel.ulaval.ca/~parizeau/PACC
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Contact:
 *  Laboratoire de Vision et Systemes Numeriques
 *  Departement de genie electrique et de genie informatique
 *  Universite Laval, Quebec, Canada, G1K 7P4
 *  http://vision.gel.ulaval.ca
 *
 */

/*!
 *  \file   PACC/Util/Assert.hpp
 *  \brief  Assert macro.
 *  \author Marc Parizeau
 *  $Revision: 1.17 $
 *  $Date: 2006/09/15 18:13:09 $
 
 This macro can be used to assert the validity of a boolean condition 
 (first argument: \c COND). If the  expression is true nothing happens. Otherwise,
 an error message (second argument: \c MESSAGE) is outputted with the file name and line
 number where the macro was executed, and the program exits. On Unix, the 
 message is send to the standard error stream (std::cerr). On windows, a popup 
 (message box) is created with the error message.
 */
 
#ifndef PACC_Assert_hpp
#define PACC_Assert_hpp

#ifndef PACC_NDEBUG
#ifdef WIN32
/////// for Windows, pop a message box ///////
#include <windows.h>
#include <sstream>
#ifndef UNICODE
#define PACC_AssertM(COND,MESSAGE) \
if(!(COND)) { \
	std::ostringstream lStream; \
	lStream << __FILE__ << ":" << __LINE__ << "\n" << MESSAGE; \
	::MessageBox(NULL, lStream.str().c_str(), "PACC assert error!", MB_ICONERROR | MB_OK); \
	exit(-1); \
}
#else
#define PACC_AssertM(COND,MESSAGE) \
if(!(COND)) { \
	std::wostringstream lStream; \
	lStream << __FILE__ << ":" << __LINE__ << "\n" << MESSAGE; \
	::MessageBox(NULL, lStream.str().c_str(), L"PACC assert error!", MB_ICONERROR | MB_OK); \
	exit(-1); \
}
#endif

#else
/////// otherwise, output message to console ///////
#include <iostream>
#define PACC_AssertM(COND,MESSAGE) \
if(!(COND)) { \
	std::cerr << "\n***** PACC assert failed *****\nin "; \
	std::cerr << __FILE__ << ":" << __LINE__ << "\n" << MESSAGE; \
	std::cerr << "\n******************************" << endl; \
	exit(-1); \
}
#endif // WIN32

#else // PACC_NDEBUG
#define PACC_AssertM(COND,MESSAGE)
#endif

#endif // PACC_Assert_hpp
