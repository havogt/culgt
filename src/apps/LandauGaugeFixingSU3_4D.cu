/*
 * test_gaugefixing.cpp
 *
 *  Created on: Apr 18, 2012
 *      Author: vogt
 */

#include <iostream>
#include <math.h>
#include <sstream>
#include <malloc.h>
#include "../lattice/gaugefixing/GaugeFixingSubgroupStep.hxx"
#include "../lattice/gaugefixing/GaugeFixingStats.hxx"
#include "../lattice/gaugefixing/overrelaxation/OrUpdate.hxx"
#include "../lattice/access_pattern/StandardPattern.hxx"
#include "../lattice/access_pattern/GpuCoulombPattern.hxx"
#include "../lattice/access_pattern/GpuLandauPattern.hxx"
#include "../lattice/SiteCoord.hxx"
#include "../lattice/SiteIndex.hxx"
#include "../lattice/Link.hxx"
#include "../lattice/SU3.hxx"
#include "../lattice/Matrix.hxx"
#include "../lattice/LinkFile.hxx"
#include "../lattice/gaugefixing/overrelaxation/OrSubgroupStep.hxx"
#include "../util/timer/Chronotimer.h"
#include "../lattice/filetypes/FileMDP.hxx"
#include "../lattice/filetypes/FileVogt.hxx"

using namespace std;

const lat_dim_t Ndim = 4;
const short Nc = 3;

#ifdef _X_
const lat_coord_t Nx = _X_;
#else
#error "Define X (the lattice size in x-direction)"
#endif
#ifdef _Y_
const lat_coord_t Ny = _Y_;
#else
const lat_coord_t Ny = _X_;
bool warnY = true; // TODO print the warning
#endif
#ifdef _Z_
const lat_coord_t Nz = _Z_;
#else
const lat_coord_t Nz = _X_;
bool warnZ = true;
#endif
#ifdef _T_
const lat_coord_t Nt = _T_;
#else
#error "Define T (the lattice size in t-direction)"
#endif

const lat_coord_t size[Ndim] = {Nt,Nx,Ny,Nz};
const int arraySize = Nt*Nx*Ny*Nz*Ndim*Nc*Nc*2;
const int timesliceArraySize = Nx*Ny*Nz*Ndim*Nc*Nc*2;

typedef StandardPattern<SiteCoord<Ndim,false>,Ndim,Nc> Standard;
typedef GpuLandauPattern< SiteCoord<Ndim,true>,Ndim,Nc> Gpu;


typedef Link<Gpu,SiteCoord<Ndim,true>,Ndim,Nc> TLink;


__device__ inline Real cuFabs( Real a )
{
	return (a>0)?(a):(-a);
}

void initNeighbourTable( lat_index_t* nnt )
{
	const lat_coord_t size[Ndim] = {Nt,Nx,Ny,Nz};
	SiteIndex<4,true> s(size);
	s.calculateNeighbourTable( nnt );
}


__global__ void projectSU3( Real* U )
{
	const lat_coord_t size[Ndim] = {Nt,Nx,Ny,Nz};
	SiteCoord<4,true> s(size);
	int site = blockIdx.x * blockDim.x + threadIdx.x;

	s.setLatticeIndex( site );

	for( int mu = 0; mu < 4; mu++ )
	{
		TLink linkUp( U, s, mu );
		SU3<TLink> globUp( linkUp );

		globUp.projectSU3();
	}
}


__global__ void __launch_bounds__(256,4) orStep( Real* U, lat_index_t* nn, bool parity, float orParameter )
{
	typedef GpuLandauPattern< SiteIndex<Ndim,true>,Ndim,Nc> GpuIndex;
	typedef Link<GpuIndex,SiteIndex<Ndim,true>,Ndim,Nc> TLinkIndex;

	const lat_coord_t size[Ndim] = {Nt,Nx,Ny,Nz};
	SiteIndex<4,true> s(size);
	s.nn = nn;

	const bool updown = threadIdx.x / 128;
	const short mu = (threadIdx.x % 128) / 32;
	const short id = (threadIdx.x % 128) % 32;

	int site = blockIdx.x * blockDim.x/8 + id;
	if( parity == 1 ) site += s.getLatticeSize()/2;

	s.setLatticeIndex( site );
	if( updown==1 )
	{
		s.setNeighbour(mu,false);
	}

//	if(id == 0) printf("bin in or\n");

	Matrix<complex,Nc> locMat;
	SU3<Matrix<complex,Nc> > locU(locMat);

	TLinkIndex link( U, s, mu );

	SU3<TLinkIndex> globU( link );

	// make link local
	locU.assignWithoutThirdLine(globU);
	locU.reconstructThirdLine();

	// define the update algorithm
	OrUpdate overrelax( orParameter );
	GaugeFixingSubgroupStep<SU3<Matrix<complex,Nc> >, OrUpdate, LANDAU> subgroupStep( &locU, overrelax, id, mu, updown );

	// do the subgroup iteration
	SU3<Matrix<complex,Nc> >::perSubgroup( subgroupStep );

	// project back
	//globU.projectSU3withoutThirdRow();

	// copy link back
	globU.assignWithoutThirdLine(locU);
}




Real calculatePolyakovLoopAverage( Real *U )
{
	Matrix<complex,3> tempMat;
	SU3<Matrix<complex,3> > temp( tempMat );
	Matrix<complex,3> temp2Mat;
	SU3<Matrix<complex,3> > temp2( temp2Mat );

	SiteCoord<Ndim,true> s( size );

	complex result(0,0);

	for( s[1] = 0; s[1] < s.size[1]; s[1]++ )
	{
		for( s[2] = 0; s[2] < s.size[2]; s[2]++ )
		{
			for( s[3] = 0; s[3] < s.size[3]; s[3]++ )
			{
				temp.identity();
				temp2.zero();

				for( s[0] = 0; s[0] < s.size[0]; s[0]++ )
				{

					TLink link( U, s, 0 );
					SU3<TLink> globU( link );

					temp2 = temp2 + temp*globU;

					temp = temp2;
					temp2.zero();
				}
				result += temp.trace();
			}
		}
	}

	return sqrt(result.x*result.x+result.y*result.y) / (Real)(s.getLatticeSizeTimeslice()*Nc);
}






int main(int argc, char* argv[])
{

	cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, 0);

	printf("\nDevice %d: \"%s\"\n", 0, deviceProp.name);
	printf("CUDA Capability Major/Minor version number:    %d.%d\n\n", deviceProp.major, deviceProp.minor);

	Chronotimer allTimer;
	allTimer.reset();

	SiteCoord<4,true> s(size);
	LinkFile<FileMDP, Standard, Gpu, SiteCoord<4,true> > lf;


	// allocate Memory
	// host memory for configuration
	Real* U = (Real*)malloc( arraySize*sizeof(Real) );

	// device memory for configuration
	Real* dU;
	cudaMalloc( &dU, arraySize*sizeof(Real) );

	// host memory for the neighbour table
	lat_index_t* nn = (lat_index_t*)malloc( s.getLatticeSize()*(2*(Ndim))*sizeof(lat_index_t) );

	// device memory for the timeslice neighbour table
	lat_index_t *dNn;
	cudaMalloc( &dNn, s.getLatticeSize()*(2*(Ndim))*sizeof( lat_index_t ) );



	// initialise the timeslice neighbour table
	initNeighbourTable( nn );
	// copy neighbour table to device
	cudaMemcpy( dNn, nn, s.getLatticeSize()*(2*(Ndim))*sizeof( lat_index_t ), cudaMemcpyHostToDevice );

	int threadsPerBlock = 32*8; // 32 sites are updated within a block (8 threads are needed per site)
	int numBlocks = s.getLatticeSize()/2/32; // // half of the lattice sites (a parity) are updated in a kernel call

	allTimer.start();

	cudaFuncSetCacheConfig( orStep, cudaFuncCachePreferL1 );
	
	// instantiate GaugeFixingStats object
	GaugeFixingStats gaugeStats( dU, LANDAU, s.getLatticeSize(), 1.0e-6 );


	double totalKernelTime = 0;

	for( int i = 0; i < 1; i++ )
	{

		stringstream filename(stringstream::out);
		filename << "/data/msk/config_n32t32beta6105_sp" << setw( 4 ) << setfill( '0' ) << i << ".vogt";
//		filename << "/home/vogt/configs/STUDIENARBEIT/N32/config_n32t32beta570_sp" << setw( 4 ) << setfill( '0' ) << i << ".vogt";


		bool loadOk = lf.load( s, filename.str(), U );

		if( !loadOk )
		{
			cout << "Error while loading. Trying next file." << endl;
			break;
		}
		else
		{
			cout << "File loaded." << endl;
		}
//		Real polBefore = calculatePolyakovLoopAverage( U );

		// copying configuration t ...
		cudaMemcpy( dU, U, arraySize*sizeof(Real), cudaMemcpyHostToDevice );

		// check the current gauge quality
		gaugeStats.generateGaugeQuality();
		printf( "gff: %1.10f\t\tdA: %1.10f\n", gaugeStats.getCurrentGff(), gaugeStats.getCurrentA() );

		float orParameter = 1.7;

		Chronotimer kernelTimer;
		kernelTimer.reset();
		kernelTimer.start();
		for( int i = 0; i < 5000; i++ )
		{
			orStep<<<numBlocks,threadsPerBlock>>>(dU, dNn, 0, orParameter );
			orStep<<<numBlocks,threadsPerBlock>>>(dU, dNn, 1, orParameter );

//			if( i % 100 == 0 )
//			{
//				projectSU3<<<numBlocks*2,32>>>( dU );
		// check the current gauge quality
			gaugeStats.generateGaugeQuality();
			printf( "gff: %1.10f\t\tdA: %e\n", gaugeStats.getCurrentGff(), gaugeStats.getCurrentA() );


//			}
		}
		cudaThreadSynchronize();
		kernelTimer.stop();
		cout << "kernel time for config: " << kernelTimer.getTime() << " s"<< endl;
		totalKernelTime += kernelTimer.getTime();
		cudaMemcpy( U, dU, arraySize*sizeof(Real), cudaMemcpyDeviceToHost );

//		cout << "Polyakov loop: " << polBefore << " - " << calculatePolyakovLoopAverage( U ) << endl;
	}

	allTimer.stop();
	cout << "total time: " << allTimer.getTime() << " s" << endl;
	cout << "total kernel time: " << totalKernelTime << " s" << endl;

	cout << (double)((long)2253*(long)s.getLatticeSize()*(long)5000)/totalKernelTime/1.0e9 << " GFlops at "
				<< (double)((long)192*(long)s.getLatticeSize()*(long)(5000)*(long)sizeof(Real))/totalKernelTime/1.0e9 << "GB/s memory throughput." << endl;

}
