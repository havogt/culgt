/*
 * Overrelaxation.hxx
 *
 *  Created on: May 10, 2012
 *      Author: vogt
 */

#ifndef ORUPDATE_HXX_
#define ORUPDATE_HXX_

#include "../GlobalConstants.h"
#include "../../lattice/datatype/datatypes.h"

class OrUpdate
{
public:
	__device__ inline OrUpdate();
	__device__ inline OrUpdate( float param );
	__device__ inline void calculateUpdate( volatile Real (&shA)[4*NSB], short id );
	__device__ inline void setParameter( float param );
	__device__ inline float getParameter();
private:
	float orParameter;
};

__device__ OrUpdate::OrUpdate()
{
}

__device__ OrUpdate::OrUpdate( float param ) : orParameter(param)
{
}

__device__ void OrUpdate::calculateUpdate( volatile Real (&shA)[4*NSB], short id )
{
#ifdef USE_DP_ORUPDATE
	double ai_sq = shA[id+NSB]*shA[id+NSB]+shA[id+2*NSB]*shA[id+2*NSB]+shA[id+3*NSB]*shA[id+3*NSB];
	double a0_sq = shA[id]*shA[id];

	double b=((double)orParameter*a0_sq+ai_sq)/(a0_sq+ai_sq);
	double c=rsqrt(a0_sq+b*b*ai_sq); // even in DP calculation the rsqrt has no effect on the stability of GFF

	shA[id]*=c;
	shA[id+NSB]*=b*c;
	shA[id+2*NSB]*=b*c;
	shA[id+3*NSB]*=b*c;
#else
	Real ai_sq = shA[id+NSB]*shA[id+NSB]+shA[id+2*NSB]*shA[id+2*NSB]+shA[id+3*NSB]*shA[id+3*NSB];
	Real a0_sq = shA[id]*shA[id];

	Real b=(orParameter*a0_sq+ai_sq)/(a0_sq+ai_sq);
	Real c=rsqrt(a0_sq+b*b*ai_sq);

	shA[id]*=c;
	shA[id+NSB]*=b*c;
	shA[id+2*NSB]*=b*c;
	shA[id+3*NSB]*=b*c;
	// 22 flops
#endif
}

__device__ void OrUpdate::setParameter( float param )
{
	orParameter = param;
}

__device__ float OrUpdate::getParameter()
{
	return orParameter;
}
#endif /* ORUPDATE_HXX_ */
