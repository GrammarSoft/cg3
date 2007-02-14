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
 * \file PACC/Util/Timer.hpp
 * \brief Class definition for the portable timer.
 * \author Marc Parizeau, Laboratoire de vision et syst&egrave;mes num&eacute;riques, Universit&eacute; Laval
 * $Revision: 1.19 $
 * $Date: 2006/11/21 17:01:17 $
 */

#ifndef PACC_Timer_hpp
#define PACC_Timer_hpp

namespace PACC {

	/*! \brief Portable timer class.
	\author Marc Parizeau, Laboratoire de vision et syst&egrave;mes num&eacute;riques, Universit&eacute; Laval
	\ingroup Util
	
	This class implements a simple stopwatch timer that is always running. Method Timer::getValue can be used to return the current timer value in seconds. Method Timer::reset resets this value to 0. 
	
	By default, if possible, the class uses the CPU's high resolution hardware time-stamp to measure time. Under Windows, this always translates to a call to the QueryPerformanceCounter method. Under Unix, when using the gcc compiler, inlined assembly code is used to retrieve the hardware counter on the following platforms:
	- Pentium family (i386)
	- PowerPC family (ppc)
	.
	Otherwise, the class uses the Unix gettimeofday method to retrieve a somewhat lower resolution time stamp (max resolution is micro-seconds).
	
	The current high resolution time stamp can be retrieved using method Timer::getCount. The time period associated with a single count increment is platform dependent. Its value can be fetch with method Timer::getCountPeriod.
	
	\attention Hardware time-stamp counters may be dependent on the CPU clock frequency. Under Unix, the hardware time-stamp frequency needs to be evaluated using a calibration procedure based on method gettimeofday. If applicable, this procedure lasts about 0.1 sec when using default parameters (see Timer::calibrateCountPeriod).   
	*/
	class Timer {
	 public:
		/*! \brief Construct a timer and reset its value. 
		
		On supported Unix platforms, argument \c inHardware=true (default) enables the use of the CPU's high resolution hardware time-stamp. Argument \c inHardware=false forces the use of a somewhat lower resolution count based on the Unix gettimeofday method. Note that the use of the hardware counter under Unix requires a preliminary calibration procedure (see Timer::calibrateCountPeriod).
		
		Under Windows, timer values are always computed using functions QueryPerformanceCounter and QueryPerformanceFrequency.
		*/
		Timer(bool inHardware=true) : mHardware(inHardware) {
			if(mPeriod == 0) calibrateCountPeriod(); 
			reset();
		}

		//! Calibrate the count period.
		void calibrateCountPeriod(unsigned int inDelay=10000, unsigned int inTimes=10);

		//! Return the current high resolution count.
		unsigned long long getCount(void) const;

		//! Return the time period of a single count increment (in seconds).
		double getCountPeriod(void) const {return mPeriod;}

		//! Return the current timer value in seconds.
		double getValue(void) const {return (getCount()-mCount)*mPeriod;}

		//! reset the timer value (to 0 second).
		void reset(void) {mCount = getCount();}

	 protected:
		bool mHardware; //!< Specifies whether hardware time-stamps should be used.
		unsigned long long mCount; //!< Count value at last reset.
		static double mPeriod; //!< Time period of a single count in seconds.
	};

} // end of PACC namespace

#endif // PACC_Timer_hpp
