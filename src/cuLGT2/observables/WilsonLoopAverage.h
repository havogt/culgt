/**
 *  Created on: Mar 14, 2014
 *      Author: vogt
 */

#ifndef WILSONLOOPAVERAGE_H_
#define WILSONLOOPAVERAGE_H_

#include "../lattice/LatticeDimension.h"
#include "../lattice/LocalLink.h"
#include "../lattice/parameterization_types/SUNRealFull.h"
#include "WilsonLoopAverageCudaHelper.h"
#include "../cuLGT1legacy/Reduction.hxx"

namespace culgt
{

template<typename PatternType, typename LocalLinkType = LocalLink<SUNRealFull<PatternType::PARAMTYPE::NC, typename PatternType::PARAMTYPE::TYPE> > > class WilsonLoopAverage
{
public:
	typedef typename PatternType::PARAMTYPE::TYPE T;
	typedef typename PatternType::PARAMTYPE::REALTYPE REALT;

	WilsonLoopAverage( T* U, LatticeDimension<PatternType::SITETYPE::Ndim> dim ) : dim(dim), U(U)
	{
		cudaMalloc( (void**)&devPtr, sizeof( REALT )*dim.getSize() );
	};

	void calculateWilsonLoops( int dir1, int length1, int dir2, int length2 )
	{
		WilsonLoopAverageCudaHelper<PatternType,LocalLinkType>::calculateWilsonLoops( U, devPtr, dim, dir1, length1, dir2, length2 );
	}

	REALT getWilsonLoop( int dir1, int length1, int dir2, int length2 )
	{
		calculateWilsonLoops( dir1, length1, dir2, length2 );
		Reduction<REALT> reducer(dim.getSize());

		return reducer.reduceAll( devPtr )/(REALT)dim.getSize();
	}

	void calculatePolyakovLoopCorrelators( int separationDir, int separation, int timeDir = 0 )
	{
		WilsonLoopAverageCudaHelper<PatternType,LocalLinkType>::calculatePolyakovLoopCorrelators( U, devPtr, dim, separationDir, separation, timeDir );
	}

	REALT getPolyakovLoopCorrelator( int separationDir, int separation, int timeDir = 0 )
	{
		calculatePolyakovLoopCorrelators( separationDir, separation, timeDir );
		Reduction<REALT> reducer(dim.getSize());

		return reducer.reduceAll( devPtr )/(REALT)dim.getSize();
	}

	void calculatePolyakovLoops( int timeDir = 0 )
	{
		WilsonLoopAverageCudaHelper<PatternType,LocalLinkType>::calculatePolyakovLoops( U, devPtr, dim, timeDir );
	}

	REALT getPolyakovLoop( int timeDir = 0 )
	{
		calculatePolyakovLoops( timeDir );
		Reduction<REALT> reducer(dim.getSize());

		return reducer.reduceAll( devPtr )/(REALT)dim.getSize();
	}



	REALT* getDevicePointer() const
	{
		return devPtr;
	}

	~WilsonLoopAverage()
	{
		cudaFree( devPtr );
	}


private:
	LatticeDimension<PatternType::SITETYPE::Ndim> dim;
	T* U;
	REALT* devPtr;
};

}

#endif /* WILSONLOOPAVERAGE_H_ */
