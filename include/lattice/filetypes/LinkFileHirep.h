/**
 */

#ifndef LINKFILEHIREP_H_
#define LINKFILEHIREP_H_

#include <string>
#include <fstream>
#include <sstream>
#include "LinkFile.h"
#include "lattice/LocalLink.h"
#include "lattice/GlobalLink.h"
#include "lattice/parameterization_types/SUNRealFull.h"
#include "common/culgt_typedefs.h"
#include "common/culgt_compat.h"

namespace culgt
{

template<typename MemoryConfigurationPattern> class LinkFileHirep: public LinkFile<MemoryConfigurationPattern>
{
private:
	static const lat_dim_t memoryNdim = MemoryConfigurationPattern::SITETYPE::NDIM;
	typedef typename MemoryConfigurationPattern::PARAMTYPE::REALTYPE REALTYPE;

	int ndim;
	int nc;
	double plaquette;
	int size[memoryNdim];
	int sizeOfReal;

	int32_t readInt()
	{
		int32_t temp;
		LinkFile<MemoryConfigurationPattern>::file.read( (char*)&temp, sizeof(int32_t) );
		return  __builtin_bswap32( temp );
	}

	double readDouble()
	{
		int64_t temp;
		LinkFile<MemoryConfigurationPattern>::file.read( (char*)&temp, sizeof(int64_t) );
		temp =  __builtin_bswap64( temp );
		double* result = reinterpret_cast<double*>( &temp );
		return *result;
	}

	void writeInt( int32_t out )
	{
		int32_t temp = __builtin_bswap32( out );
		LinkFile<MemoryConfigurationPattern>::file.write( (char*)&temp, sizeof(int32_t) );
	}

	void writeDouble( double out )
	{
		int64_t* temp = reinterpret_cast<int64_t*>(&out);
		int64_t result = __builtin_bswap64( *temp );
		LinkFile<MemoryConfigurationPattern>::file.write( (char*)&result, sizeof(int64_t) );
	}

	void readSize()
	{
		for( int i = 0; i < memoryNdim; i++ )
		{
			size[i] = readInt();
		}
	}


	void writeSize()
	{
		for( int i = 0; i < memoryNdim; i++ )
		{
			writeInt( size[i] );
		}
	}

public:
	typedef LinkFile<MemoryConfigurationPattern> super;

	LinkFileHirep(){};
	LinkFileHirep( const int size[memoryNdim], ReinterpretReal reinterpret = STANDARD ) : LinkFile<MemoryConfigurationPattern>( size, reinterpret )
	{
		ndim=4;
		sizeOfReal = sizeof( double );
	}
	LinkFileHirep( const LatticeDimension<memoryNdim> size, ReinterpretReal reinterpret = STANDARD ) : LinkFile<MemoryConfigurationPattern>( size, reinterpret )
	{
		ndim=4;
		sizeOfReal = sizeof( double );
	}

	virtual void saveImplementation() CULGT_OVERRIDE
	{
		saveHeader();
		saveBody();
	}

	virtual void loadImplementation() CULGT_OVERRIDE
	{
		if( memoryNdim != 4 ) super::throwException( "Only Ndim = 4 supported", memoryNdim, 4 );
		loadHeader();
		verify();
		loadBody();
	}

	void loadHeader()
	{
		nc = readInt();
		readSize();
		plaquette = readDouble();
		std::cout << std::endl << "Plaquette: " << plaquette << std::endl;
	}

	void saveHeader()
	{
		writeInt( nc );
		writeSize();
		writeDouble(plaquette);
	}

	LocalLink<SUNRealFull<MemoryConfigurationPattern::PARAMTYPE::NC,REALTYPE> > getNextLink()
	{
		typedef SUNRealFull<MemoryConfigurationPattern::PARAMTYPE::NC,REALTYPE> LocalLinkParamType;
		typedef LocalLink<LocalLinkParamType> LocalLink;
		LocalLink link;

		for( int i = 0; i < LocalLinkParamType::SIZE; i++ )
		{
			typename LocalLinkParamType::TYPE value;
			value = (typename LocalLinkParamType::TYPE) readDouble();

			link.set( i, value );
		}
		return link;
	}

	void writeNextLink( LocalLink<SUNRealFull<MemoryConfigurationPattern::PARAMTYPE::NC,REALTYPE> > link )
	{
		typedef SUNRealFull<MemoryConfigurationPattern::PARAMTYPE::NC,REALTYPE> LocalLinkParamType;

		for( int i = 0; i < LocalLinkParamType::SIZE; i++ )
		{
			typename LocalLinkParamType::TYPE value = link.get( i );
			writeDouble( (double) value );
		}
	}


	void loadBody()
	{
		for( int i = 0; i < this->getLatticeDimension().getSize(); i++ )
		{
			for( int mu = 0; mu < memoryNdim; mu++ )
			{
				typename MemoryConfigurationPattern::SITETYPE site( this->getLatticeDimension(), NULL );
				site.setIndexFromNonParitySplitOrder( i );

				GlobalLink<MemoryConfigurationPattern> dest( this->getPointerToU(), site, mu );

				LocalLink<SUNRealFull<MemoryConfigurationPattern::PARAMTYPE::NC,REALTYPE> > src;
				src = getNextLink();

				dest = src;
			}
		}
	}

	void saveBody()
	{
		for( int i = 0; i < this->getLatticeDimension().getSize(); i++ )
		{
			for( int mu = 0; mu < memoryNdim; mu++ )
			{
				typename MemoryConfigurationPattern::SITETYPE site( this->getLatticeDimension(), NULL );
				site.setIndexFromNonParitySplitOrder( i );

				GlobalLink<MemoryConfigurationPattern> src( this->getPointerToU(), site, mu );

				LocalLink<SUNRealFull<MemoryConfigurationPattern::PARAMTYPE::NC,REALTYPE> > dest;

				dest = src;

				writeNextLink( dest );
			}
		}
	}

	void verify()
	{
		if( MemoryConfigurationPattern::PARAMTYPE::NC != nc )
		{
			super::throwException( "Wrong gauge group", MemoryConfigurationPattern::PARAMTYPE::NC, nc );
		}


		short mySizeOfReal;
		if( super::reinterpretReal == FLOAT ) mySizeOfReal = sizeof( float );
		else if( super::reinterpretReal == DOUBLE ) mySizeOfReal = sizeof( double );
		else mySizeOfReal = sizeof( typename MemoryConfigurationPattern::PARAMTYPE::REALTYPE );

		if( mySizeOfReal != sizeOfReal )
		{
			std::cout << "CONVERTING DOUBLE TO FLOAT! YOU ARE USING SP IN THIS APPLICATION!" << std::endl;
		}

		for( int i = 0; i < memoryNdim; i++ )
		{
			if( this->getLatticeDimension().getDimension(i) != size[i] )
			{
				std::stringstream msg;
				msg << "Wrong lattice size in ";
				msg << i;
				msg << " direction";
				super::throwException( msg.str(), this->getLatticeDimension().getDimension(i), size[i] );
			}
		}
	}

	void loadData()
	{

	}

	short getNdim() const
	{
		return 4;
	}

	short getNc() const
	{
		return nc;
	}

	short getSize( int i ) const
	{
		return size[i];
	}

	short getSizeOfReal() const
	{
		return sizeOfReal;
	}

};

}

#endif /* LINKFILE_H_ */

