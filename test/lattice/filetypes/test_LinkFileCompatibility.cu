/**
 * FIXME: should not need cuda (needs for calculating plaquette)
 */

#include "gmock/gmock.h"
#include "lattice/GaugeConfiguration.h"
#include "lattice/parameterization_types/SU3Vector4.h"
#include "lattice/parameterization_types/ParameterizationMediatorSU3_Vector4_Real18.h"
#include "lattice/configuration_patterns/GPUPatternParityPriority.h"
#include "lattice/filetypes/LinkFileILDG.h"
#include "lattice/filetypes/LinkFileVogt.h"
#include "lattice/filetypes/LinkFileNERSC.h"
#include "observables/PlaquetteAverage.h"

using namespace culgt;
using namespace ::testing;

template<int N> class LinkFileCompatibilitySU3Template: public Test
{
public:
	typedef float REAL;
	typedef SU3Vector4<REAL> PARAMTYPE;
	typedef SiteIndex<4,FULL_SPLIT> SITE;
	typedef GPUPatternParityPriority<SITE,PARAMTYPE> PATTERNTYPE;
	typedef LocalLink<SUNRealFull<3,REAL> > LOCALLINK;

	LatticeDimension<4> dim;
	GaugeConfiguration<PATTERNTYPE> config;
	GaugeConfiguration<PATTERNTYPE> config2;
	PlaquetteAverage<PATTERNTYPE,LOCALLINK>* plaquetteCalculator;

	const double plaquetteValue = 5.948501558951e-01;

	void SetUp()
	{
		config.allocateMemory();
		config2.allocateMemory();
		plaquetteCalculator = new PlaquetteAverage<PATTERNTYPE,LOCALLINK>( config.getDevicePointer(), dim );
	}
	void TearDown()
	{
		config.freeMemory();
		config2.freeMemory();
		delete plaquetteCalculator;
	}

	REAL calcPlaquetteOnConfig()
	{
		return plaquetteCalculator->getPlaquette();
	}

	LinkFileCompatibilitySU3Template(): dim(N,N,N,N), config( dim ), config2( dim )
	{
	}
};

typedef LinkFileCompatibilitySU3Template<4> LinkFileCompatibilitySU3;

TEST_F( LinkFileCompatibilitySU3, CheckILDGPlaquette )
{
	LinkFileILDG<PATTERNTYPE> linkfile( dim );
	linkfile.setFilename( "lat.sample.l4444.ildg" );

	config.loadFile( linkfile );
	config.copyToDevice();

	ASSERT_FLOAT_EQ( plaquetteValue, calcPlaquetteOnConfig() );
}

TEST_F( LinkFileCompatibilitySU3, CheckVogtPlaquette )
{
	LinkFileVogt<PATTERNTYPE> linkfile( dim );
	linkfile.setFilename( "lat.sample.l4444.vogt" );

	config.loadFile( linkfile );
	config.copyToDevice();

	ASSERT_FLOAT_EQ( plaquetteValue, calcPlaquetteOnConfig() );
}

TEST_F( LinkFileCompatibilitySU3, CheckNERSCPlaquette )
{
	LinkFileNERSC<PATTERNTYPE> linkfile( dim );
	linkfile.setFilename( "lat.sample.l4444.nersc" );

	config.loadFile( linkfile );
	config.copyToDevice();

	ASSERT_FLOAT_EQ( plaquetteValue, calcPlaquetteOnConfig() );
}

TEST_F( LinkFileCompatibilitySU3, VogtWriteReadFromILDG )
{
	LinkFileILDG<PATTERNTYPE> linkfileIn( dim );
	linkfileIn.setFilename( "lat.sample.l4444.ildg" );
	config2.loadFile( linkfileIn );


	LinkFileVogt<PATTERNTYPE> linkfileOut( dim );
	linkfileOut.setFilename( "tempILDG2Vogt.vogt" );
	config2.saveFile( linkfileOut );

	LinkFileVogt<PATTERNTYPE> linkfileCheck( dim );
	linkfileCheck.setFilename( "tempILDG2Vogt.vogt" );
	config.loadFile( linkfileCheck );
	config.copyToDevice();

	ASSERT_FLOAT_EQ( plaquetteValue, calcPlaquetteOnConfig() );
}

TEST_F( LinkFileCompatibilitySU3, NERSCWriteReadFromVogt )
{
	LinkFileVogt<PATTERNTYPE> linkfileIn( dim );
	linkfileIn.setFilename( "lat.sample.l4444.vogt" );
	config2.loadFile( linkfileIn );


	LinkFileNERSC<PATTERNTYPE> linkfileOut( dim );
	linkfileOut.setFilename( "tempVogt2NERSC.nersc" );
	config2.saveFile( linkfileOut );

	LinkFileNERSC<PATTERNTYPE> linkfileCheck( dim );
	linkfileCheck.setFilename( "tempVogt2NERSC.nersc" );
	config.loadFile( linkfileCheck );
	config.copyToDevice();

	ASSERT_FLOAT_EQ( plaquetteValue, calcPlaquetteOnConfig() );
}

//TEST_F( LinkFileCompatibilitySU3, ILDGWriteReadFromNERSC )
//{
//	LinkFileNERSC<PATTERNTYPE> linkfileIn( dim );
//	linkfileIn.setFilename( "lat.sample.l4444.nersc" );
//	config2.loadFile( linkfileIn );
//
//
//	LinkFileILDG<PATTERNTYPE> linkfileOut( dim );
//	linkfileOut.setFilename( "tempNERSC2ILDG.ildg" );
//	config2.saveFile( linkfileOut );
//
//	LinkFileILDG<PATTERNTYPE> linkfileCheck( dim );
//	linkfileCheck.setFilename( "tempNERSC2ILDG.ildg" );
//	config.loadFile( linkfileCheck );
//	config.copyToDevice();
//
//	ASSERT_FLOAT_EQ( plaquetteValue, calcPlaquetteOnConfig() );
//}

typedef LinkFileCompatibilitySU3Template<5> LinkFileCompatibilitySU3WrongSize;

TEST_F( LinkFileCompatibilitySU3WrongSize, ILDGThrowsException )
{
	LinkFileILDG<PATTERNTYPE> linkfile( dim );
	linkfile.setFilename( "lat.sample.l4444.ildg" );

	ASSERT_THROW( config.loadFile( linkfile ), LinkFileException );
}

TEST_F( LinkFileCompatibilitySU3WrongSize, VogtThrowsException )
{
	LinkFileVogt<PATTERNTYPE> linkfile( dim );
	linkfile.setFilename( "lat.sample.l4444.vogt" );

	ASSERT_THROW( config.loadFile( linkfile ), LinkFileException );
}

TEST_F( LinkFileCompatibilitySU3WrongSize, NERSCThrowsException )
{
	LinkFileNERSC<PATTERNTYPE> linkfile( dim );
	linkfile.setFilename( "lat.sample.l4444.nersc" );

	ASSERT_THROW( config.loadFile( linkfile ), LinkFileException );
}


