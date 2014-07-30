/**
 * GaugeConfigurationIteratingApplication.h
 *
 *  Created on: Apr 15, 2014
 *      Author: vogt
 */

#ifndef GAUGECONFIGURATIONITERATINGAPPLICATION_H_
#define GAUGECONFIGURATIONITERATINGAPPLICATION_H_

#include "../lattice/GaugeConfiguration.h"
#include "../lattice/LinkFile.h"
#include "../lattice/LatticeDimension.h"
#include "FileIterator.h"
#include "ProgramOptions.h"

namespace culgt
{


template<typename PatternType, typename LinkFileType> class GaugeConfigurationIteratingApplication
{
public:
	GaugeConfigurationIteratingApplication( const LatticeDimension<PatternType::SITETYPE::Ndim> dimension, FileIterator fileiterator, ProgramOptions* programOptions ): dimension(dimension), configuration(dimension), fileiterator(fileiterator), programOptions(programOptions)
	{
		configuration.allocateMemory();
	}

	virtual ~GaugeConfigurationIteratingApplication()
	{
		configuration.freeMemory();
	}

	virtual void iterate() = 0;
	virtual void setup() = 0;
	virtual void teardown() = 0;

//	virtual void addProgramOptions() = 0;

	void run()
	{
		setup();
		for( fileiterator.reset(); fileiterator.hasElement(); fileiterator.next() )
		{
			iterate();
		}
		teardown();
	}

	bool load()
	{
		linkFile->setFilename( fileiterator.getFilename() );
		std::cout << fileiterator.getFilename() ;
		try
		{
			linkFile->load( configuration.getHostPointer() );
			std::cout << " loaded!" << std::endl;
			return true;
		}
		catch( FileNotFoundException& e )
		{
			std::cout << " not found! Skipping..." << std::endl;
			return false;
		}
	}

	bool loadToDevice()
	{
		bool isLoaded = load();
		if( isLoaded )
		{
			configuration.copyToDevice();
			std::cout << "Copied to device!" << std::endl;
		}
		return isLoaded;
	}

	void save( string appendix )
	{
		linkFile->setFilename( fileiterator.getFilename( appendix ) );
		std::cout << fileiterator.getFilename( appendix ) << std::endl;
		linkFile->save( configuration.getHostPointer() );
	}

	void saveFromDevice( string appendix )
	{
		configuration.copyToHost();
		save( appendix );
	}

	template<typename ConcreteApplicationType> static ConcreteApplicationType* init( const int argc, const char* argv[] )
	{
		// do the command line interpretation here.
		ProgramOptions* po = new ProgramOptions();
//		ConcreteApplicationType::addProgramOptions( po );
		po->parseOptions( argc, argv, true );

		int fileNumberEnd = po->getFileNumberStart()+(po->getNConf()-1)*po->getFileNumberStep();
		FileIterator fileiterator( po->getFileBasename(), po->getFileEnding(), po->getFileNumberformat(), po->getFileNumberStart(), fileNumberEnd );


		LatticeDimension<PatternType::SITETYPE::Ndim> dimtest(po->getNt(),po->getNx(),po->getNy(),po->getNz());

		// TODO in principle we could read the sizes from the gaugeconfig file!
		APP = new ConcreteApplicationType( LatticeDimension<PatternType::SITETYPE::Ndim>(po->getNt(),po->getNx(),po->getNy(),po->getNz()), fileiterator, po );


		APP->linkFile = new LinkFileType( APP->dimension, po->getReinterpretReal() );
//		APP->linkFile.setReinterpretReal( po->getReinterpretReal() );

		// parse again for options that where added in the constructor
		po->parseOptions( argc, argv );

		return dynamic_cast<ConcreteApplicationType*>(APP);
	}

	static void destroy()
	{
//		delete APP->programOptions;
//		delete APP->linkFile;
		delete APP;
	}

	LinkFileType& getLinkFile()
	{
		return *linkFile;
	}

	const LatticeDimension<PatternType::SITETYPE::Ndim>& getDimension() const
	{
		return dimension;
	}

	template<typename ConcreteApplicationType> static void main( const int argc, const char* argv[] )
	{
		init<ConcreteApplicationType>( argc, argv );
		APP->run();
		APP->destroy();
	}

protected:
	LatticeDimension<PatternType::SITETYPE::Ndim> dimension;
	LinkFileType* linkFile;
	GaugeConfiguration<PatternType> configuration;
	FileIterator fileiterator;
	ProgramOptions* programOptions;

private:
	static GaugeConfigurationIteratingApplication<PatternType, LinkFileType>* APP;
};

template<typename PatternType, typename LinkFileType> GaugeConfigurationIteratingApplication<PatternType,LinkFileType>* GaugeConfigurationIteratingApplication<PatternType,LinkFileType>::APP = NULL;

}

#endif /* GAUGECONFIGURATIONITERATINGAPPLICATION_H_ */
