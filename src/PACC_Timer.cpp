/*
 *  Portable Agile C++ Classes (PACC)
 *  Copyright (C) 2004 by Marc Parizeau
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
 * \file PACC/Util/Timer.cpp
 * \brief Class methods for the portable timer.
 * \author Marc Parizeau, Laboratoire de vision et syst&egrave;mes num&eacute;riques, Universit&eacute; Laval
 * $Revision: 1.25 $
 * $Date: 2007/01/23 21:28:09 $
 */

#include "PACC_Timer.h"
#include "PACC_Assert.h"

#ifdef WIN32
#include <windows.h>
#include <stdexcept>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

using namespace std;
using namespace PACC;

double Timer::mPeriod = 0;

/*! 
Under Windows, this method always calls the QueryPerformanceFrequency function to determine the count period of the hardware time-stamp. 

Under Unix, with hardware mode activated, when using the gcc compiler on an i386 family processor (Pentium or AMD), or on a PowerPC family processor, this method calibrates the hardware time-stamp counter using the standard gettimeofday function. This calibration is conducted by measuring a delay of approximatly \c inDelay micro-seconds, and the count period is averaged over \c inTimes runs. In all other cases, or when the hardware mode is deactivated, the count period is fixed at 1 micro-second (the maximum resolution of gettimeofday).

This method is called automatically by the class constructor. Under normal circumstances, the user should not concern himself with calibration. 
 */
void Timer::calibrateCountPeriod(unsigned int inDelay, unsigned int inTimes)
{
	inDelay = inDelay; // Just to foil the warning. Compiler will optimize this away.
	inTimes = inTimes; // Just to foil the warning. Compiler will optimize this away.
#ifdef WIN32
	// use the windows counter
	LARGE_INTEGER lFrequency;
	PACC_AssertM(QueryPerformanceFrequency(&lFrequency), "Timer::Timer() no performance counter on this processor!");
	mPeriod = 1. / lFrequency.QuadPart;
#else
	if(mHardware) {
#if defined (__GNUG__) && (defined (__i386__) || defined (__ppc__))
		double lPeriod = 0;
		// calibrate by matching the time-stamps with the micro-seconds of gettimeofday
		for(unsigned int i = 0; i < inTimes; ++ i) {
			timeval lStartTime, lTime;
			::gettimeofday(&lStartTime, 0);
			unsigned long long lStartCount = getCount();
			::usleep(inDelay);
			::gettimeofday(&lTime, 0);
			unsigned long long lCount = getCount() - lStartCount;
			lTime.tv_sec -= lStartTime.tv_sec;
			lTime.tv_usec -= lStartTime.tv_usec;
			// dismiss the first run of the loop
			if(i != 0) lPeriod += (lTime.tv_sec + lTime.tv_usec*0.000001)/lCount;
		}
		mPeriod = lPeriod/(inTimes-1);
#else
		// use the microseconds of gettimeofday
		mPeriod = 0.000001;
#endif
	} else {
		// use the microseconds of gettimeofday
		mPeriod = 0.000001;
	}
#endif
}

/*! 
This method returns the highest resolution count available for this timer. Under Windows, it returns the hardware performance counter value as provided by the QueryPerformanceCounter method. 

Under Unix, when using the gcc compiler on an i386 family processor (Pentium or AMD), or on a PowerPC family processor, this method can also return the hardware performance counter value using in-lined assembly code. On all other platforms, or when the hardware mode is deactivated, it returns a count based on the standard gettimeofday function (in micro-seconds).
 */
PACC_TimeStamp Timer::getCount(void) const
{
	PACC_TimeStamp lCount = 0;
#ifdef WIN32
	LARGE_INTEGER lCurrent;
	QueryPerformanceCounter(&lCurrent);
	lCount = lCurrent.QuadPart;
#else
	if(mHardware) {
#if defined (__GNUG__) && defined (__i386__)
		__asm__ volatile("rdtsc" : "=A" (lCount));
#else
#if defined (__GNUG__) && defined (__ppc__)
		register unsigned int lLow;
		register unsigned int lHigh1;
		register unsigned int lHigh2;
		do {
			// make sure that high bits have not changed
			__asm__ volatile ( "mftbu %0" : "=r" (lHigh1) );
			__asm__ volatile ( "mftb  %0" : "=r" (lLow) );
			__asm__ volatile ( "mftbu %0" : "=r" (lHigh2) );
		} while(lHigh1 != lHigh2);
		// transfer to lCount
		unsigned int *lPtr = (unsigned int*) &lCount;
		*lPtr++ = lHigh1; *lPtr = lLow;
#else
		timeval lCurrent;
		::gettimeofday(&lCurrent, 0);
		lCount = (unsigned long long)lCurrent.tv_sec*1000000 + lCurrent.tv_usec;
#endif
#endif
	} else {
		timeval lCurrent;
		::gettimeofday(&lCurrent, 0);
		lCount = (unsigned long long)lCurrent.tv_sec*1000000 + lCurrent.tv_usec;
	}
#endif
	return lCount;
}
