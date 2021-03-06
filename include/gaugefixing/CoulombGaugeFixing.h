/**
 *  Created on: Mar 19, 2014
 *      Author: vogt
 */

#ifndef COULOMBGAUGEFIXING_H_
#define COULOMBGAUGEFIXING_H_
#include "gaugefixing_thread_types.h"
#include <assert.h>
#include "lattice/parameterization_types/SU2Vector4.h"
#include "lattice/SubgroupIterator.h"
#include "common/culgt_typedefs.h"
#include "util/timer/Chronotimer.h"
#include <iostream>
#include "util/reduction/Reduction.h"
#include "cudacommon/cuda_error.h"
#include "GaugeFixing8Threads.h"
#include "GaugeFixing4Threads.h"
#include "algorithms/OrUpdate.h"
#include "algorithms/SaUpdate.h"
#include "gaugetypes/LandauCoulombGaugeType.h"
#include "RunInfo.h"
#include "GaugeStats.h"
#include <string>
#include "lattice/GlobalLink.h"
#include "util/rng/PhiloxWrapper.h"

#include "GaugeSettings.h"

#include "RandomGaugeTrafo.h"

#include "GaugeFixingSaOr.h"
#include <boost/mpl/assert.hpp>
#include "TimesliceGaugeFixingOverrelaxation.h"
#include "TimesliceGaugeFixingSimulatedAnnealing.h"

using std::string;


namespace culgt
{



namespace CoulombGaugefixingKernel
{

template<typename GlobalLinkType, typename LocalLinkType>  __global__ void generateGaugeQualityPerSite( typename GlobalLinkType::PARAMTYPE::TYPE* U, LatticeDimension<GlobalLinkType::PATTERNTYPE::SITETYPE::NDIM> dim, lat_index_t* nn, double *dGff, double *dA )
{
	typename GlobalLinkType::PATTERNTYPE::SITETYPE site( dim, nn );

	lat_index_t index = blockIdx.x * blockDim.x + threadIdx.x;

	LocalLinkType Sum;
	Sum.zero();

	double gff = 0;
	for( int mu = 1; mu < 4; mu++ )
	{
		LocalLinkType temp;

		site.setIndex( index );
		GlobalLinkType globalLink( U, site, mu );
		temp = globalLink;
		Sum += temp;
		gff += temp.reTrace();

		site.setNeighbour(mu,false);
		GlobalLinkType globDw( U, site, mu );
		temp = globDw;
		Sum -= temp;
	}

	Sum -= Sum.trace()/(LocalLinkType::PARAMTYPE::REALTYPE)(GlobalLinkType::PARAMTYPE::NC);

	LocalLinkType SumHerm;
	SumHerm = Sum;
	SumHerm.hermitian();

	Sum -= SumHerm;

	dA[index] = Sum.normFrobeniusSquared();
	dGff[index] = gff;
}

template<typename GlobalLinkType, typename LocalLinkType>  __global__ void generateGaugeQualityPerSiteLogarithmic( typename GlobalLinkType::PARAMTYPE::TYPE* U, LatticeDimension<GlobalLinkType::PATTERNTYPE::SITETYPE::NDIM> dim, lat_index_t* nn, double *dGff, double *dA )
{
	// only for SU(2)
//	BOOST_MPL_ASSERT_RELATION( LocalLinkType::PARAMTYPE::NC, ==, 2 );

	typename GlobalLinkType::PATTERNTYPE::SITETYPE site( dim, nn );

	lat_index_t index = blockIdx.x * blockDim.x + threadIdx.x;

	typename LocalLinkType::PARAMTYPE::REALTYPE A[3] = {0,0,0};

	typedef LocalLink<SUNRealFull<LocalLinkType::PARAMTYPE::NC,typename LocalLinkType::PARAMTYPE::REALTYPE> > LOCALLINKREALFULL;


	double gff = 0;
	for( int mu = 1; mu < 4; mu++ )
	{
		LOCALLINKREALFULL temp;

		site.setIndex( index );
		GlobalLinkType globalLink( U, site, mu );
		temp = globalLink;
		typename LocalLinkType::PARAMTYPE::REALTYPE norm = ::sqrt( temp.get(1)*temp.get(1) + temp.get(2)*temp.get(2) + temp.get(3)*temp.get(3) );
		for( int i = 0; i < 3; i++ )
//			A[i] += temp.get(i+1)/norm*sin(norm/2);
			A[i] += 2*temp.get(i+1)*::acos(temp.get(0))/norm;

		gff += 2. - ::acos( temp.reTrace() / 2. )*::acos( temp.reTrace() / 2. );

		site.setNeighbour(mu,false);
		GlobalLinkType globDw( U, site, mu );
		temp = globDw;
		norm = ::sqrt( temp.get(1)*temp.get(1) + temp.get(2)*temp.get(2) + temp.get(3)*temp.get(3) );
		for( int i = 0; i < 3; i++ )
//			A[i] -= temp.get(i+1)/norm*sin(norm/2);
			A[i] -= 2*temp.get(i+1)*::acos(temp.get(0))/norm;
	}

	dA[index] = ::sqrt( A[0]*A[0] +  A[1]*A[1]+ A[2]*A[2]); // check this
	dGff[index] = gff;
}

}


template<typename PatternType, typename LocalLinkType> class CoulombGaugefixing: public GaugeFixingSaOr
{
public:
	typedef GlobalLink<PatternType,true> GlobalLinkType;
	typedef typename PatternType::PARAMTYPE::TYPE T;
	typedef typename PatternType::PARAMTYPE::REALTYPE REALT;
	COPY_GLOBALLINKTYPE( GlobalLinkType, GlobalLinkType2, 1 );

	CoulombGaugefixing( T* Ut, T* UtDown, LatticeDimension<GlobalLinkType::PATTERNTYPE::SITETYPE::NDIM> dim, long seed ) : GaugeFixingSaOr( dim.getSize() ), dimTimeslice(dim), overrelaxation( &this->Ut, &this->UtDown, dim, seed, 1.5 ), microcanonical( &this->Ut, &this->UtDown, dim, seed ), simulatedAnnealing( &this->Ut, &this->UtDown, dim, seed, 1. ), seed(seed), reducer(dimTimeslice.getSize())
	{
		setTimeslice( Ut, UtDown );
	}

	void setTimeslice( T* Ut, T* UtDown )
	{
		GlobalLinkType::unbindTexture();
		this->Ut = Ut;
		GlobalLinkType::bindTexture( Ut, GlobalLinkType::getArraySize( dimTimeslice ) );
		GlobalLinkType2::unbindTexture();
		this->UtDown = UtDown;
		GlobalLinkType2::bindTexture( UtDown, GlobalLinkType2::getArraySize( dimTimeslice ) );
	}

	GaugeStats getGaugeStats( GaugeFieldDefinition definition = GAUGEFIELD_STANDARD )
	{
		KernelSetup<GlobalLinkType::PATTERNTYPE::SITETYPE::NDIM> setupNoSplit( dimTimeslice, false );
		if( definition == GAUGEFIELD_STANDARD )
		{
			CoulombGaugefixingKernel::generateGaugeQualityPerSite<GlobalLinkType,LocalLinkType><<<setupNoSplit.getGridSize(),setupNoSplit.getBlockSize()>>>( Ut, dimTimeslice, SiteNeighbourTableManager<typename GlobalLinkType::PATTERNTYPE::SITETYPE>::getDevicePointer( dimTimeslice ), dGff, dA );
		}
		else
		{
			if( GlobalLinkType::PATTERNTYPE::PARAMTYPE::NC == 2 )
			{
				CoulombGaugefixingKernel::generateGaugeQualityPerSiteLogarithmic<GlobalLinkType,LocalLinkType><<<setupNoSplit.getGridSize(),setupNoSplit.getBlockSize()>>>( Ut, dimTimeslice, SiteNeighbourTableManager<typename GlobalLinkType::PATTERNTYPE::SITETYPE>::getDevicePointer( dimTimeslice ), dGff, dA );
			}
			else
			{
				assert( false );
			}
		}
		CUDA_LAST_ERROR( "generateGaugeQualityPerSite" );


//		double dAAvg = reducer.reduceAll( dA )/(double)dimTimeslice.getSize()/(double)(GlobalLinkType::PATTERNTYPE::PARAMTYPE::NC);
		double dAAvg = reducer.reduceAllMax( dA )/(double)(GlobalLinkType::PATTERNTYPE::PARAMTYPE::NC);
		double dGffAvg = reducer.reduceAll( dGff )/(double)dimTimeslice.getSize()/(double)((GlobalLinkType::PATTERNTYPE::SITETYPE::NDIM-1)*GlobalLinkType::PATTERNTYPE::PARAMTYPE::NC);

		return GaugeStats( dGffAvg, dAAvg );
	}

	void runOverrelaxation( float orParameter )
	{
		overrelaxation.setOrParameter( orParameter );
		overrelaxation.run();
	}

	RunInfo computeRunInfoOverrelaxation( double time, long iter )
	{
		return RunInfo::makeRunInfo<GlobalLinkType,LocalLinkType,LandauCoulombGaugeType<COULOMB> >( dimTimeslice.getSize(), time, iter, OrUpdate<typename LocalLinkType::PARAMTYPE::REALTYPE>::Flops );
	}

//	void runCornell( float alpha, int id = -1 )
//	{
//		cornell.setAlpha( alpha );
//		cornell.run( id );
//	}

	void runMicrocanonical()
	{
		microcanonical.run();
	}

	void runSimulatedAnnealing( float temperature )
	{
		simulatedAnnealing.setTemperature( temperature );
		simulatedAnnealing.run();
	}

	RunInfo computeRunInfoSimulatedAnnealing( double time, long iterSa, long iterMicro )
	{
		return RunInfo::makeRunInfo<GlobalLinkType,LocalLinkType,LandauCoulombGaugeType<COULOMB> >( dimTimeslice.getSize(), time, iterSa, SaUpdate<typename LocalLinkType::PARAMTYPE::REALTYPE>::Flops, iterMicro, MicroUpdate<typename LocalLinkType::PARAMTYPE::REALTYPE>::Flops );
	}

	void tuneOverrelaxation( float orParameter = 1.5, int iter = 1000 )
	{
		overrelaxation.setOrParameter( orParameter );
		overrelaxation.tune( iter );
	}

//	template<typename RNG> void cornellAutoTune( float alpha = .5, int iter = 1000 )
//	{
//		cornell.setAlpha( alpha );
//		cornell.tune( iter );
//	}

	void tuneSimulatedAnnealing( float temperature = 1.0, int iter = 1000 )
	{
		simulatedAnnealing.setTemperature( temperature );
		simulatedAnnealing.tune( iter );
	}

	void tuneMicrocanonical( int iter = 1000 )
	{
		microcanonical.tune( iter );
	}

	void randomTrafo()
	{
		RandomGaugeTrafo<PatternType,LocalLinkType>::randomTrafo( Ut, UtDown, dimTimeslice, seed );
	}

	void reproject()
	{
		GaugeConfigurationCudaHelper<T>::template reproject<typename GlobalLinkType::PATTERNTYPE,LocalLinkType, PhiloxWrapper<REALT> >( Ut, dimTimeslice, seed, PhiloxWrapper<REALT>::getNextCounter() );
		GaugeConfigurationCudaHelper<T>::template reproject<typename GlobalLinkType::PATTERNTYPE,LocalLinkType, PhiloxWrapper<REALT> >( UtDown, dimTimeslice, seed, PhiloxWrapper<REALT>::getNextCounter() );
	}

	void allocateCopyMemory()
	{
		GaugeConfigurationCudaHelper<T>::allocateMemory( &UtBest, GlobalLinkType::getArraySize(dimTimeslice) );
		GaugeConfigurationCudaHelper<T>::allocateMemory( &UtDownBest, GlobalLinkType::getArraySize(dimTimeslice) );
		GaugeConfigurationCudaHelper<T>::allocateMemory( &UtClean, GlobalLinkType::getArraySize(dimTimeslice) );
		GaugeConfigurationCudaHelper<T>::allocateMemory( &UtDownClean, GlobalLinkType::getArraySize(dimTimeslice) );
	}

	void freeCopyMemory()
	{
		GaugeConfigurationCudaHelper<T>::freeMemory( UtBest );
		GaugeConfigurationCudaHelper<T>::freeMemory( UtDownBest );
		GaugeConfigurationCudaHelper<T>::freeMemory( UtClean );
		GaugeConfigurationCudaHelper<T>::freeMemory( UtDownClean );
	}

	void saveCopy()
	{
		CUDA_SAFE_CALL( cudaMemcpy( UtBest, Ut, GlobalLinkType::getArraySize(dimTimeslice)*sizeof(T), cudaMemcpyDeviceToDevice ), "cudaMemcpy in saveCopy (device to device)" );
		CUDA_SAFE_CALL( cudaMemcpy( UtDownBest, UtDown, GlobalLinkType::getArraySize(dimTimeslice)*sizeof(T), cudaMemcpyDeviceToDevice ), "cudaMemcpy in saveCopy (device to device)" );
	}

	void writeBackCopy()
	{
		CUDA_SAFE_CALL( cudaMemcpy( Ut, UtBest, GlobalLinkType::getArraySize(dimTimeslice)*sizeof(T), cudaMemcpyDeviceToDevice ), "cudaMemcpy in saveCopy (device to device)" );
		CUDA_SAFE_CALL( cudaMemcpy( UtDown, UtDownBest, GlobalLinkType::getArraySize(dimTimeslice)*sizeof(T), cudaMemcpyDeviceToDevice ), "cudaMemcpy in saveCopy (device to device)" );
	}

	void storeCleanCopy()
	{
		CUDA_SAFE_CALL( cudaMemcpy( UtClean, Ut, GlobalLinkType::getArraySize(dimTimeslice)*sizeof(T), cudaMemcpyDeviceToDevice ), "cudaMemcpy in saveCopy (device to device)" );
		CUDA_SAFE_CALL( cudaMemcpy( UtDownClean, UtDown, GlobalLinkType::getArraySize(dimTimeslice)*sizeof(T), cudaMemcpyDeviceToDevice ), "cudaMemcpy in saveCopy (device to device)" );
	}

	void takeCleanCopy()
	{
		CUDA_SAFE_CALL( cudaMemcpy( Ut, UtClean, GlobalLinkType::getArraySize(dimTimeslice)*sizeof(T), cudaMemcpyDeviceToDevice ), "cudaMemcpy in saveCopy (device to device)" );
		CUDA_SAFE_CALL( cudaMemcpy( UtDown, UtDownClean, GlobalLinkType::getArraySize(dimTimeslice)*sizeof(T), cudaMemcpyDeviceToDevice ), "cudaMemcpy in saveCopy (device to device)" );
	}

private:
	LatticeDimension<GlobalLinkType::PATTERNTYPE::SITETYPE::NDIM> dimTimeslice;
	T* Ut;
	T* UtDown;

	T* UtBest;
	T* UtDownBest;
	T* UtClean;
	T* UtDownClean;

	Reduction<double> reducer;

	TimesliceGaugeFixingOverrelaxation<PatternType,LocalLinkType,LandauCoulombGaugeType<COULOMB> > overrelaxation;
	TimesliceGaugeFixingOverrelaxation<PatternType,LocalLinkType,LandauCoulombGaugeType<COULOMB>, true > microcanonical;
	TimesliceGaugeFixingSimulatedAnnealing<PatternType,LocalLinkType,LandauCoulombGaugeType<COULOMB> > simulatedAnnealing;

	long seed;
};


}

#endif /* COULOMBGAUGEFIXING_H_ */
