/**
 */

#ifndef COMPLEX_HXX_
#define COMPLEX_HXX_

#include "../cudacommon/cuda_host_device.h"
#include "../common/culgt_typedefs.h"
#include "../common/mathfunction_overload.h"
#include <math.h>
#include <fstream>

namespace culgt
{
template <typename T> class Complex
{
public:
	union{
		typename Real2<T>::VECTORTYPE e;
		struct{
			T x;
			T y;
		};
	};

	CUDA_HOST_DEVICE inline Complex<T>& operator=( const Complex<T> a )
	{
		this->x = a.x;
		this->y = a.y;
		return *this;
	}

	CUDA_HOST_DEVICE inline volatile Complex<T>& operator=( const Complex<T> a ) volatile
	{
		this->x = a.x;
		this->y = a.y;
		return *this;
	}

	CUDA_HOST_DEVICE inline Complex(const T x, const T y)
	{
		this->x = x;
		this->y = y;
	}

	CUDA_HOST_DEVICE inline Complex(const float x )
	{
		this->x = (T)x;
		this->y = 0;
	}
	CUDA_HOST_DEVICE inline Complex(const double x )
	{
		this->x = (T)x;
		this->y = 0;
	}

	CUDA_HOST_DEVICE inline Complex( const Complex<T>& a )
	{
		this->x = a.x;
		this->y = a.y;
	}

	CUDA_HOST_DEVICE inline Complex( volatile const Complex<T>& a )
	{
		this->x = a.x;
		this->y = a.y;
	}

	CUDA_HOST_DEVICE inline Complex()
	{
		this->x = 0;
		this->y = 0;
	}

	CUDA_HOST_DEVICE inline T abs()
	{
		return sqrt( x*x + y*y );
	}

	CUDA_HOST_DEVICE inline T phase()
	{
		return atan2( y, x );
	}

	CUDA_HOST_DEVICE inline T abs_squared()
	{
		return ( x*x + y*y );
	}

	CUDA_HOST_DEVICE inline Complex<T> conj()
	{
		return Complex<T>(x,-y);
	}

	CUDA_HOST_DEVICE inline Complex<T>& operator+=( const Complex<T> a )
	{
		x += a.x;
		y += a.y;
		return *this;
	}

	CUDA_HOST_DEVICE inline Complex<T>& operator+=( const T a )
	{
		x += a;
		return *this;
	}

	CUDA_HOST_DEVICE inline Complex<T>& operator-=( const Complex<T> a )
	{
		x -= a.x;
		y -= a.y;
		return *this;
	}

	CUDA_HOST_DEVICE inline Complex<T>& operator-=( const T a )
	{
		x -= a;
		return *this;
	}

	CUDA_HOST_DEVICE inline Complex<T>& operator*=( const Complex<T> a )
	{
		T re = x;
		T im = y;
		x = re*a.x - im*a.y;
		y = re*a.y + im*a.x;
		return *this;
	}

	CUDA_HOST_DEVICE inline Complex<T>& operator*=( const T a )
	{
		x *= a;
		y *= a;
		return *this;
	}

	CUDA_HOST_DEVICE inline Complex<T>& operator/=( const T a )
	{
		x /= a;
		y /= a;
		return *this;
	}

	CUDA_HOST_DEVICE inline Complex<T>& operator/=( const Complex<T> a )
	{
		T re = x;
		T im = y;
		T abs = a.x*a.x+a.y*a.y;
		x = (re*a.x + im*a.y)/abs;
		y = (-re*a.y + im*a.x)/abs;

		return *this;
	}

	static CUDA_HOST_DEVICE inline Complex<T> I()
	{
		return Complex<T>(0,1);
	}

};


template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator+( Complex<T> a, Complex<T> b )
{
	Complex<T> c = a;
	return c+=b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator+( Complex<T> a, float b )
{
	Complex<T> c = a;
	return c+=(T)b;
}
template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator+( Complex<T> a, double b )
{
	Complex<T> c = a;
	return c+=(T)b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator+( float a, Complex<T> b )
{
	Complex<T> c((T)a);
	return c+=b;
}
template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator+( double a, Complex<T> b )
{
	Complex<T> c((T)a);
	return c+=b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator-( Complex<T> a, Complex<T> b )
{
	Complex<T> c = a;
	return c-=b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator-( Complex<T> a, T b )
{
	Complex<T> c = a;
	return c-=b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator-( T a, Complex<T> b )
{
	Complex<T> c(a);
	return c-=b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator*( Complex<T> a, Complex<T> b )
{
	Complex<T> c = a;
	return c*=b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator/( Complex<T> a, Complex<T> b )
{
	Complex<T> c = a;
	return c/=b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator*( Complex<T> a, float b )
{
	Complex<T> c = a;
	return c*=(T)b;
}
template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator*( Complex<T> a, double b )
{
	Complex<T> c = a;
	return c*=(T)b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator*( float a, Complex<T> b )
{
	Complex<T> c = b; // we commuted arguments to use *=(T) which is simpler
	return c*=(T)a;
}
template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator*( double a, Complex<T> b )
{
	Complex<T> c = b; // we commuted arguments to use *=(T) which is simpler
	return c*=(T)a;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator/( Complex<T> a, float b )
{
	Complex<T> c = a;
	return c/=(T)b;
}
template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator/( Complex<T> a, double b )
{
	Complex<T> c = a;
	return c/=(T)b;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator/( float a, Complex<T> b )
{
	Complex<T> c((T)a,0);
	return c/=b;
}
template<typename T> CUDA_HOST_DEVICE inline Complex<T> operator/( double a, Complex<T> b )
{
	Complex<T> c((T)a,0);
	return c/=b;
}

template<typename T> CUDA_HOST_DEVICE inline T fabs( Complex<T> b )
{
	return b.abs();
}

/**
 * principal root
 */
template<typename T> CUDA_HOST_DEVICE inline Complex<T> sqrt( Complex<T> b )
{
	T tmp = b.abs();
	return Complex<T>( ::sqrt(.5*(b.x+tmp)), ((b.y>0)?(1.):(-1.))*::sqrt(.5*(-b.x+tmp)) );
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> csqrt( T b )
{
	if( b >=  0. ) return Complex<T>( ::sqrt(b), 0 );
	else return Complex<T>(0,::sqrt(-b));
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> log( Complex<T> b )
{
	return Complex<T>( ::log( b.abs() ), b.phase() );
}


/**
 * acos and asin are rather experimental...
 * They may end up in an infinite loop which will be indicated by a memory read error in cuda execution. Did not happen in may codes...
 * If you have the time to think carefully about a fail proof version of that, you are very welcome!
 * (or implement the boost way after checking license)
 */
template<typename T> CUDA_HOST_DEVICE inline Complex<T> asin( Complex<T> b, bool calledFromAcos = false )
{
	Complex<double> db( b.x, b.y );
	Complex<double> sqrttemp = sqrt( (double)1. - db*db );

	Complex<T> logtemp =  Complex<T>::I()*b + Complex<T>(sqrttemp.x,sqrttemp.y);

	if( logtemp.x != (T)0 ||  logtemp.y != 0 )
	{
		logtemp = log( logtemp );
		return Complex<T>( logtemp.y, -logtemp.x );
	}
	else
	{
//		if( calledFromAcos )
//		{
//			printf( "%f/t %f", b.x, b.y );
//		}
		Complex<T> result = acos(b, true)*(-1.f)+::acos(-1.f)/2.f;
		return result;
	}

//	Complex<T> result;
//	T arg = ::sqrt( (b.x*b.x+b.y*b.y-1.)*(b.x*b.x+b.y*b.y-1.) + 4.*b.y*b.y) ;
//	T abs_sq = (b.x*b.x+b.y*b.y);
//	result.x = sign(b.x)/2.*::acos( arg - abs_sq);
//	result.y = sign(b.y)/2.*::acosh( arg + abs_sq);
//	return result;
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> acos( Complex<T> b, bool calledFromAsin = false )
{
	Complex<double> db( b.x, b.y );
	Complex<double> sqrttemp = sqrt( (double)1. - db*db );

	Complex<T> logtemp =  b + Complex<T>::I()*Complex<T>(sqrttemp.x,sqrttemp.y);
	if( logtemp.x != (T)0 ||  logtemp.y != 0 )
	{
		logtemp = log( logtemp );
		return Complex<T>( logtemp.y, -logtemp.x );
	}
	else
	{
//		if( calledFromAsin )
//		{
//			printf( "%f/t %f", b.x, b.y );
//		}
		Complex<T> result = asin(b, true)*(-1.f)+::acos(-1.f)/2.f;
		return result;
	}
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> cos( Complex<T> b )
{
	return Complex<T>( ::cos(b.x)*::cosh(b.y), -::sin(b.x)*::sinh(b.y) );
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> norm_squared( Complex<T> b )
{
	return b.abs_squared();
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> conj( Complex<T> b )
{
	return b.conj();
}

template<typename T> CUDA_HOST_DEVICE inline Complex<T> exp( Complex<T> b )
{
	T temp = ::exp( b.x );
	return Complex<T>( temp*::cos( b.y ), temp*::sin( b.y ) );
}


template<typename T> inline std::ostream& operator<<(std::ostream& out, Complex<T> t)
{
    out << t.x << "+i*" << t.y;
    return out;
}


}

#endif /* COMPLEX_HXX_ */
