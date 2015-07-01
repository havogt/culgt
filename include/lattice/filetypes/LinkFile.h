/**
 * LinkFile.h
 *
 *  Created on: Feb 27, 2014
 *      Author: vogt
 */

#ifndef LINKFILE_H_
#define LINKFILE_H_

#include <string>
#include <fstream>
#include "lattice/LatticeDimension.h"
#include "common/culgt_typedefs.h"
#include "common/culgt_compat.h"
#include "filetype_typedefs.h"

namespace culgt
{




class IOException: public std::exception
{
public:
	~IOException() throw() {};
	IOException( std::string msg ) : msg(msg){};
	virtual const char* what() const throw()
	{
		return msg.c_str();
	}
protected:
	std::string msg;
};

class LinkFileException: public IOException
{
public:
	LinkFileException( std::string msg ) : IOException(msg){};
};

class FileNotFoundException: IOException
{
public:
	FileNotFoundException( std::string filename ) : IOException( "File " + filename+ " not found!" ) {};
};



template<typename MemoryConfigurationPattern> class LinkFile
{
public:
	virtual ~LinkFile() {};


	LinkFile() : reinterpretReal(STANDARD), fileIsOpen( false ), filenameIsSet( false )
	{
	};

	LinkFile( ReinterpretReal reinterpret ) : reinterpretReal(reinterpret), fileIsOpen( false ), filenameIsSet( false )
	{
	};

	LinkFile( const int size[MemoryConfigurationPattern::SITETYPE::NDIM], ReinterpretReal reinterpret ) :reinterpretReal(reinterpret), fileIsOpen( false ), filenameIsSet( false ), latticeDimension( size )
	{
	};

	LinkFile( const LatticeDimension<MemoryConfigurationPattern::SITETYPE::NDIM> size, ReinterpretReal reinterpret ) : reinterpretReal(reinterpret),fileIsOpen( false ), filenameIsSet( false ), latticeDimension( size )
	{
	};

	virtual void load( typename MemoryConfigurationPattern::PARAMTYPE::TYPE* U ) CULGT_FINAL
	{
		this->U = U;
		this->openFile();
		this->loadImplementation();
		this->closeFile();
	};

	virtual void save( typename MemoryConfigurationPattern::PARAMTYPE::TYPE* U ) CULGT_FINAL
	{
		this->U = U;
		this->openFileWrite();
		this->saveImplementation();
		this->closeFile();
	};

	void openFileWrite()
	{
		if( !filenameIsSet )
		{
			throw IOException( "Filename not set." );
		}

		file.open( filename.c_str(), std::ios::out | std::ios::binary);

		if( file.good() )
		{
			fileIsOpen = true;
		}
		else
		{
			throw FileNotFoundException( filename );
		}
	};

	void openFile()
	{
		if( !filenameIsSet )
		{
			throw IOException( "Filename not set." );
		}

		file.open( filename.c_str(), std::ios::in | std::ios::binary);

		if( file.good() )
		{
			fileIsOpen = true;
		}
		else
		{
			throw FileNotFoundException( filename );
		}
	};
	void closeFile()
	{
		if( fileIsOpen )
		{
			file.close();
		}
	}
	const std::string& getFilename() const
	{
		return filename;
	}

	void setFilename(const std::string& filename)
	{
		this->filename = filename;
		filenameIsSet = true;
	}

	void setPointerToU( typename MemoryConfigurationPattern::PARAMTYPE::TYPE* U )
	{
		this->U = U;
	}

	typename MemoryConfigurationPattern::PARAMTYPE::TYPE* getPointerToU() const
	{
		return U;
	}

	LatticeDimension<MemoryConfigurationPattern::SITETYPE::NDIM> getLatticeDimension() const
	{
		return latticeDimension;
	}

protected:
	std::fstream file;
	ReinterpretReal reinterpretReal;
	virtual void loadImplementation(){};
	virtual void saveImplementation(){};

	void throwException( std::string msg, int expected, int inFile )
	{
		std::stringstream message;
		message << msg;
		message << ": Expected ";
		message << expected;
		message << " in file, got ";
		message << inFile;
		throw LinkFileException( message.str() );
	}

private:
	bool fileIsOpen;
	bool filenameIsSet;
	LatticeDimension<MemoryConfigurationPattern::SITETYPE::NDIM> latticeDimension;
	std::string filename;
	typename MemoryConfigurationPattern::PARAMTYPE::TYPE* U;
};

}

#endif /* LINKFILE_H_ */


