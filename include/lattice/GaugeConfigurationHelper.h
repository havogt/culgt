/**
 * GaugeConfigurationCudaHelper.h
 *
 *  Created on: Feb 26, 2014
 *      Author: vogt
 */

#ifndef GAUGECONFIGURATIONHELPER_H_
#define GAUGECONFIGURATIONHELPER_H_

#include "../cudacommon/cuda_error.h"
#include "LatticeDimension.h"
#include "GlobalLink.h"
#include "LocalLink.h"

namespace culgt{

template<typename Pattern> class GaugeConfigurationHelper
{
public:
	typedef typename Pattern::PARAMTYPE::TYPE TYPE;
	typedef typename Pattern::SITETYPE SITE;
	static void setCold( TYPE* pointerToPointer, LatticeDimension<Pattern::SITETYPE::NDIM> );
};

template<typename Pattern> void GaugeConfigurationHelper<Pattern>::setCold( TYPE* U, LatticeDimension<Pattern::SITETYPE::NDIM> dim )
{
	LocalLink<typename Pattern::PARAMTYPE> locLink;
	locLink.identity();
	SITE site( dim, NULL );
	for( int i = 0; i < dim.getSize(); i++ )
	{
		for( int mu = 0; mu < SITE::NDIM; mu++ )
		{
			site.setIndex( i );

			GlobalLink<Pattern> globLink( U, site, mu );

			globLink = locLink;
		}
	}
}

}
#endif /* GAUGECONFIGURATIONHELPER_H_ */
