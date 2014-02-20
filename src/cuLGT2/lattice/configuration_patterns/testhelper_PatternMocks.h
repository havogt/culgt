/**
 * test_PatternMocks.h
 *
 *  Created on: Feb 20, 2014
 *      Author: vogt
 */

#ifndef TEST_PATTERNMOCKS_H_
#define TEST_PATTERNMOCKS_H_

template<int TNdim=4> class SiteTypeMock
{
public:
	static const int Ndim = TNdim;
	const int size;
	const int index;
	SiteTypeMock( int size, int index ) : size(size), index(index) {};
	SiteTypeMock( int index ) : size(0), index(index) {};
	int getIndex() const
 	{
		return index;
	}
	int getSize() const
	{
		return size;
	}
};


template<int mySize> class ParamTypeMock
{
public:
	static const int SIZE = mySize;
};




#endif /* TEST_PATTERNMOCKS_H_ */
