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
#include "../lattice/gaugefixing/GaugeFixingStepSU2.hxx"
#include "../lattice/gaugefixing/GaugeFixingStats.hxx"
#include "../lattice/gaugefixing/overrelaxation/OrUpdate.hxx"
#include "../lattice/access_pattern/StandardPattern.hxx"
#include "../lattice/access_pattern/GpuCoulombPattern.hxx"
#include "../lattice/access_pattern/GpuLandauPattern.hxx"
#include "../lattice/SiteCoord.hxx"
#include "../lattice/SiteIndex.hxx"
#include "../lattice/Link.hxx"
#include "../lattice/Matrix.hxx"
#include "../lattice/SU2.hxx"
#include "../lattice/LinkFile.hxx"
#include "../lattice/gaugefixing/overrelaxation/OrSubgroupStep.hxx"
#include "../util/timer/Chronotimer.h"
#include "../lattice/filetypes/FileHeaderOnly.hxx"
#include "../lattice/filetypes/FilePlain.hxx"
#include "../lattice/filetypes/FileVogt.hxx"
#include "../lattice/filetypes/filetype_typedefs.h"
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>


using namespace std;

const lat_dim_t Ndim = 4;
const short Nc = 2;

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


// boost program options setup
boost::program_options::variables_map options_vm;
boost::program_options::options_description options_desc("Allowed options");

// parameters from command line or config file
int nconf;
long seed; // TODO check datatype
int orMaxIter;
int orCheckPrec;
float orParameter;
float orPrecision;
int reproject;
int saSteps;
float saMin;
float saMax;
int gaugeCopies;
string fileEnding;
string fileBasename;
int fileStartnumber;
int fileNumberformat;
string configFile;
bool noRandomTrafo;
FileType fileType;


// lattice setup
const lat_coord_t size[Ndim] = {Nt,Nx,Ny,Nz};
__constant__ lat_coord_t dSize[Ndim] = {Nt,Nx,Ny,Nz};
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


//__global__ void projectSU3( Real* U )
//{
//	const lat_coord_t size[Ndim] = {Nt,Nx,Ny,Nz};
//	SiteCoord<4,true> s(size);
//	int site = blockIdx.x * blockDim.x + threadIdx.x;
//
//	s.setLatticeIndex( site );
//
//	for( int mu = 0; mu < 4; mu++ )
//	{
//		TLink linkUp( U, s, mu );
//		SU3<TLink> globUp( linkUp );
//
//
//		Matrix<complex,Nc> locMat;
//		SU3<Matrix<complex,Nc> > locU(locMat);
//
//		locU.assignWithoutThirdLine(globUp);
//		locU.projectSU3withoutThirdRow();
//
//
//		globUp.assignWithoutThirdLine(locU);
//
////		globUp.projectSU3withoutThirdRow();
//	}
//}


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

//	Matrix<complex,Nc> locMat;
//	SU2<Matrix<complex,Nc> > locU(locMat);

	Quaternion<Real> locQuat;
//	SU2<Quaternion<Real> > locU(locQuat);

	TLinkIndex link( U, s, mu );
	SU2<TLinkIndex> globU( link );

	// make link local
//	locU.assignQuaternion(globU);

	complex a = globU.get(0,0);
	locQuat[0] = a.x;
	locQuat[3] = a.y;
	a = globU.get(0,1);
	locQuat[2] = a.x;
	locQuat[1] = a.y;


//	if( id == 0 && mu == 0 && updown == 0 ) printf( "%f\n", globU.get(0,0).x );

	// define the update algorithm
	OrUpdate overrelax( orParameter );
	GaugeFixingStepSU2<OrUpdate, LANDAU> TheUpdate( &locQuat, overrelax, id, mu, updown );

	TheUpdate.apply();



//	globU.assignQuaternion(locU);

	// reproject
	locQuat.projectSU2();

	// copy link back
	a.x = locQuat[0];
	a.y = locQuat[3];
	globU.set(0,0,a);
	a.x = locQuat[2];
	a.y = locQuat[1];
	globU.set(0,1,a);
}

//__global__ void calculatePlaquette( Real *U, lat_index_t* nn, double *dPlaquette )
//{
//	typedef GpuLandauPattern< SiteIndex<Ndim,true>,Ndim,Nc> GpuIndex;
//	typedef Link<GpuIndex,SiteIndex<Ndim,true>,Ndim,Nc> TLinkIndex;
//
//	int site = blockIdx.x * blockDim.x + threadIdx.x;
//
//	const lat_coord_t size[Ndim] = {Nt,Nx,Ny,Nz};
//	SiteIndex<4,true> s(size);
//	s.nn = nn;
//
//	Matrix<complex,Nc> matP;
//	SU3<Matrix<complex,Nc> > P(matP);
//
//	Matrix<complex,Nc> matTemp;
//	SU3<Matrix<complex,Nc> > temp(matTemp);
//
//
//	double localPlaquette = 0;
//
//
//	for( int mu = 0; mu < 4; mu++ )
//	{
//		for( int nu = mu+1; nu < 4; nu++)
//		{
//			P.identity();
//
//			{
//				s.setLatticeIndex( site );
//
//				TLinkIndex link( U, s, mu );
//				SU3<TLinkIndex> globU( link );
//
//				temp.assignWithoutThirdLine( globU );
//				temp.reconstructThirdLine();
//
//				P *= temp;
//			}
//
//			{
//				s.setNeighbour( mu, true );
//
//				TLinkIndex link( U, s, nu );
//				SU3<TLinkIndex> globU( link );
//				temp.assignWithoutThirdLine( globU );
//				temp.reconstructThirdLine();
//
//				P *= temp;
//			}
//
//			{
//				s.setLatticeIndex( site );
//				s.setNeighbour(nu, true );
//
//				TLinkIndex link( U, s, mu );
//				SU3<TLinkIndex> globU( link );
//				temp.assignWithoutThirdLine( globU );
//				temp.reconstructThirdLine();
//				temp.hermitian();
//
//				P *= temp;
//			}
//
//			{
//				s.setLatticeIndex( site );
//
//				TLinkIndex link( U, s, nu );
//				SU3<TLinkIndex> globU( link );
//				temp.assignWithoutThirdLine( globU );
//				temp.reconstructThirdLine();
//				temp.hermitian();
//
//				P *= temp;
//			}
//
//
//
//
//
//			localPlaquette += P.trace().x;
//		}
//	}
//
//	dPlaquette[site] = localPlaquette/6./3.;
////	dPlaquette[site] = 1;
////	dPlaquette[site] = 0;
//}
//
//__global__ void printPlaquette( double* dPlaquette )
//{
//	const lat_coord_t size[Ndim] = {Nx,Ny,Nz,Nt};
//	SiteCoord<4,true> s(size);
//
//	double plaquette = 0;
//	for( int i = 0; i < s.getLatticeSize(); i++ )
//	{
//		plaquette += dPlaquette[i];
//	}
//
//	printf( "\t%E\n", plaquette/double(s.getLatticeSize()) );
//
//}

//Real calculatePolyakovLoopAverage( Real *U )
//{
//	Matrix<complex,3> tempMat;
//	SU3<Matrix<complex,3> > temp( tempMat );
//	Matrix<complex,3> temp2Mat;
//	SU3<Matrix<complex,3> > temp2( temp2Mat );
//
//	SiteCoord<Ndim,true> s( size );
//
//	complex result(0,0);
//
//	for( s[1] = 0; s[1] < s.size[1]; s[1]++ )
//	{
//		for( s[2] = 0; s[2] < s.size[2]; s[2]++ )
//		{
//			for( s[3] = 0; s[3] < s.size[3]; s[3]++ )
//			{
//				temp.identity();
//				temp2.zero();
//
//				for( s[0] = 0; s[0] < s.size[0]; s[0]++ )
//				{
//
//					TLink link( U, s, 0 );
//					SU3<TLink> globU( link );
//
//					temp2 = temp2 + temp*globU;
//
//					temp = temp2;
//					temp2.zero();
//				}
//				result += temp.trace();
//			}
//		}
//	}
//
//	return sqrt(result.x*result.x+result.y*result.y) / (Real)(s.getLatticeSizeTimeslice()*Nc);
//}






int main(int argc, char* argv[])
{
	// read parameters (command line or given config file)
	options_desc.add_options()
		("help", "produce help message")
		("nconf,m", boost::program_options::value<int>(&nconf)->default_value(1), "how many files to gaugefix")
		("ormaxiter", boost::program_options::value<int>(&orMaxIter)->default_value(1000), "Max. number of OR iterations")
		("seed", boost::program_options::value<long>(&seed)->default_value(1), "RNG seed")
		("sasteps", boost::program_options::value<int>(&saSteps)->default_value(1000), "number of SA steps")
		("samin", boost::program_options::value<float>(&saMin)->default_value(.01), "min. SA temperature")
		("samax", boost::program_options::value<float>(&saMax)->default_value(.4), "max. SA temperature")
		("orparameter", boost::program_options::value<float>(&orParameter)->default_value(1.7), "OR parameter")
		("orprecision", boost::program_options::value<float>(&orPrecision)->default_value(1E-7), "OR precision (dmuAmu)")
		("orcheckprecision", boost::program_options::value<int>(&orCheckPrec)->default_value(100), "how often to check the gauge precision")
		("reproject", boost::program_options::value<int>(&reproject)->default_value(100), "reproject every arg-th step")
		("gaugecopies", boost::program_options::value<int>(&gaugeCopies)->default_value(1), "Number of gauge copies")
		("ending", boost::program_options::value<string>(&fileEnding)->default_value(".vogt"), "file ending to append to basename (default: .vogt)")
		("basename", boost::program_options::value<string>(&fileBasename), "file basename (part before numbering starts)")
		("startnumber", boost::program_options::value<int>(&fileStartnumber)->default_value(0), "file index number to start from (startnumber, ..., startnumber+nconf-1")
		("numberformat", boost::program_options::value<int>(&fileNumberformat)->default_value(1), "number format for file index: 1 = (0,1,2,...,10,11), 2 = (00,01,...), 3 = (000,001,...),...")
		("filetype", boost::program_options::value<FileType>(&fileType), "type of configuration (PLAIN, HEADERONLY, VOGT)")
		("config-file", boost::program_options::value<string>(&configFile), "config file (command line arguments overwrite config file settings)")

		("norandomtrafo", boost::program_options::value<bool>(&noRandomTrafo)->default_value(false), "no random gauge trafo" )
		;

	boost::program_options::positional_options_description options_p;
	options_p.add("config-file", -1);

	boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
			options(options_desc).positional(options_p).run(), options_vm);
	boost::program_options::notify(options_vm);

	ifstream cfg( configFile.c_str() );
	boost::program_options::store(boost::program_options::parse_config_file( cfg, options_desc), options_vm);
	boost::program_options::notify(options_vm);

	if (options_vm.count("help")) {
		cout << "Usage: " << argv[0] << " [options] [config-file]" << endl;
		cout << options_desc << "\n";
		return 1;
	}












	cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, 0);

	printf("\nDevice %d: \"%s\"\n", 0, deviceProp.name);
	printf("CUDA Capability Major/Minor version number:    %d.%d\n\n", deviceProp.major, deviceProp.minor);

	Chronotimer allTimer;
	allTimer.reset();

	SiteCoord<4,true> s(size);


	// TODO maybe we should choose the filetype on compile time
	LinkFile<FileHeaderOnly, Standard, Gpu, SiteCoord<4,true> > lfHeaderOnly;
	LinkFile<FileVogt, Standard, Gpu, SiteCoord<4,true> > lfVogt;
	LinkFile<FilePlain, Standard, Gpu, SiteCoord<4,true> > lfPlain;


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


	double* dPlaquette;
	cudaMalloc( &dPlaquette, s.getLatticeSize()*sizeof(double) );



	// initialise the timeslice neighbour table
	initNeighbourTable( nn );
	// copy neighbour table to device
	cudaMemcpy( dNn, nn, s.getLatticeSize()*(2*(Ndim))*sizeof( lat_index_t ), cudaMemcpyHostToDevice );

	int threadsPerBlock = 32*8; // 32 sites are updated within a block (8 threads are needed per site)
	int numBlocks = s.getLatticeSize()/2/32; // // half of the lattice sites (a parity) are updated in a kernel call

	allTimer.start();

	cudaFuncSetCacheConfig( orStep, cudaFuncCachePreferL1 );
	
	// instantiate GaugeFixingStats object
	lat_coord_t *devicePointerToSize;
	cudaGetSymbolAddress( (void**)&devicePointerToSize, "dSize" );
	GaugeFixingStats<Ndim,Nc,LANDAU> gaugeStats( dU, &size[0], devicePointerToSize );


	double totalKernelTime = 0;

	long totalStepNumber = 0;

	for( int i = fileStartnumber; i < fileStartnumber+nconf; i++ )
	{

		stringstream filename(stringstream::out);
		filename << fileBasename << setw( fileNumberformat ) << setfill( '0' ) << i << fileEnding;
//		filename << "/home/vogt/configs/STUDIENARBEIT/N32/config_n32t32beta570_sp" << setw( 4 ) << setfill( '0' ) << i << ".vogt";
		cout << "loading " << filename.str() << " as " << fileType << endl;

		bool loadOk;

		switch( fileType )
		{
		case VOGT:
			loadOk = lfVogt.load( s, filename.str(), U );
			break;
		case PLAIN:
			loadOk = lfPlain.load( s, filename.str(), U );
			break;
		case HEADERONLY:
			loadOk = lfHeaderOnly.load( s, filename.str(), U );
			break;
		default:
			cout << "Filetype not set to a known value. Exiting";
			exit(1);
		}

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

		// copying configuration ...
		cudaMemcpy( dU, U, arraySize*sizeof(Real), cudaMemcpyHostToDevice );

		// calculate and print the gauge quality
		printf( "i:\t\tgff:\t\tdA:\n");
		gaugeStats.generateGaugeQuality();

		Chronotimer kernelTimer;
		kernelTimer.reset();
		kernelTimer.start();
		for( int i = 0; i < orMaxIter; i++ )
		{
			orStep<<<numBlocks,threadsPerBlock>>>(dU, dNn, 0, orParameter );
			orStep<<<numBlocks,threadsPerBlock>>>(dU, dNn, 1, orParameter );


			if( i % orCheckPrec == 0 )
			{

		// check the current gauge quality
			gaugeStats.generateGaugeQuality();
			printf( "%d\t\t%1.10f\t\t%e\n", i, gaugeStats.getCurrentGff(), gaugeStats.getCurrentA() );

//			calculatePlaquette<<<s.getLatticeSize()/32,32>>>( dU, dNn, dPlaquette );
//			printPlaquette<<<1,1>>>(dPlaquette );

			if( gaugeStats.getCurrentA() < orPrecision ) break;
			}

			totalStepNumber++;
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

//	cout << (double)((long)2253*(long)s.getLatticeSize()*(long)totalStepNumber)/totalKernelTime/1.0e9 << " GFlops at "
//				<< (double)((long)192*(long)s.getLatticeSize()*(long)(totalStepNumber)*(long)sizeof(Real))/totalKernelTime/1.0e9 << "GB/s memory throughput." << endl;

}