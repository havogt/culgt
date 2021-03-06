#include "gmock/gmock.h"
#include "lattice/LocalLink.h"
#include "lattice/parameterization_types/SU3Real12.h"
#include "lattice/parameterization_types/SUNRealFull.h"
#include "lattice/parameterization_types/ParameterizationMediatorSU3_Real12_Real18.h"
#include "lattice/parameterization_types/SU2Vector4.h"

using namespace culgt;
using namespace ::testing;




class ALocalLinkWithSU3Real12AndSU3Real18: public Test
{
public:
	LocalLink<SUNRealFull<3,float> > link18;
	LocalLink<SU3Real12<float> > link12_1;
	LocalLink<SU3Real12<float> > link12_2;

	void SetUp()
	{
		link18.zero();
		link12_1.zero();
		link12_2.zero();
	}
};

TEST_F( ALocalLinkWithSU3Real12AndSU3Real18, GetReturnsPreviouslySetValue )
{
	link18.set(1, 2.42);
	ASSERT_FLOAT_EQ( 2.42, link18.get(1) );
}

TEST_F( ALocalLinkWithSU3Real12AndSU3Real18, OperatorAssignCopiesForSameParamType )
{
	link12_1.set(1, 2.42);
	link12_2 = link12_1;
	ASSERT_FLOAT_EQ( 2.42, link12_2.get(1) );
}

TEST_F( ALocalLinkWithSU3Real12AndSU3Real18, OperatorAssignDoesNotUseReference )
{
	link12_1.set(1, 2.42);
	link12_2 = link12_1;
	link12_2.set(1, 1.3 );
	EXPECT_FLOAT_EQ( 1.3, link12_2.get(1) );
	ASSERT_FLOAT_EQ( 2.42, link12_1.get(1) );
}

TEST_F( ALocalLinkWithSU3Real12AndSU3Real18, OperatorAssignCopiesFromReal18ToReal12 )
{
	link18.set(1, 2.42);
	link12_1 = link18;
	ASSERT_FLOAT_EQ( 2.42, link12_1.get(1) );
}

TEST_F( ALocalLinkWithSU3Real12AndSU3Real18, OperatorAssignCopiesFromReal12ToReal18 )
{
	link12_1.set(1, 2.42);
	link18 = link12_1;
	ASSERT_FLOAT_EQ( 2.42, link18.get(1) );
}

TEST_F( ALocalLinkWithSU3Real12AndSU3Real18, OperatorAssignCopiesFromReal12ToReal18AndReconstructsThirdLine )
{
	// define a matrix that has (1 0 0) in first and (0 1 0) in second line: expect (0 0 1) in third line
	link12_1.set(0, 1.);
	link12_1.set(8, 1.);
	link18 = link12_1;
	ASSERT_FLOAT_EQ( 1., link18.get(16) );
}

class ASpecialLocalLinkWithSU3Real18: public Test
{
public:
	LocalLink<SUNRealFull<3,float> > A;
	LocalLink<SUNRealFull<3,float> > B;
	LocalLink<SUNRealFull<3,float> > Ahermitian;
	LocalLink<SUNRealFull<3,float> > ALeftSubgroupMult01;
	LocalLink<SUNRealFull<3,float> > ARightSubgroupMult01;
	LocalLink<SUNRealFull<3,float> > AplusA;

	LocalLink<SU2Vector4<float> > aSU2link;

	const float redet = 0.585224903024000;
	const float imdet = 0.185504451600000;
	const float retrace = 2.3660;
	const float imtrace = 2.3844;
	const float frobeniusNorm = 3.1442840;

	const Complex<float> trace;

	const float AtimesA_0_SP =  0.1127562523; // single precision operation

	ASpecialLocalLinkWithSU3Real18() : trace(2.3659999, 2.3843999)
	{
	}

	void assertFloatEqual( LocalLink<SUNRealFull<3,float> > a, LocalLink<SUNRealFull<3,float> > b )
	{
		for( int i = 0; i < 18; i++ )
		{
			ASSERT_FLOAT_EQ( a.get(i), b.get(i) );
		}
	}

	void SetUp()
	{
		/*
		 * set a 3x3 matrix A with
		 * det(A) =  0.585224903024000 - 0.185504451600000i
		 * trace(A) = 2.3660 + 2.3844i
		 * (A*A)_11 = 0.1127562523 (in SP,  0.11275629 in DP )
		 * A = [0.9649 + 0.7922i   0.9572 + 0.0357i   0.1419 + 0.6787i
		 * 		0.1576 + 0.9595i   0.4854 + 0.8491i   0.4218 + 0.7577i
		 * 		0.9706 + 0.6557i   0.8003 + 0.9340i   0.9157 + 0.7431i]
		 */
		A << 	0.9649, 0.7922,		0.9572, 0.0357,		0.1419, 0.6787,
				0.1576, 0.9595,		0.4854, 0.8491,		0.4218, 0.7577,
				0.9706, 0.6557,		0.8003, 0.9340,		0.9157, 0.7431;


		Ahermitian <<	0.9649, -0.7922,   0.1576, -0.9595,   0.9706, -0.6557,
						0.9572, -0.0357,   0.4854, -0.8491,   0.8003, -0.9340,
						0.1419, -0.6787,   0.4218, -0.7577,   0.9157, -0.7431;

		AplusA <<  1.9298000, + 1.5844001,  1.9144000, + 0.0714000, 0.2838000, + 1.3573999,
				  0.3152000, + 1.9190000,  0.9708000, + 1.6982000, 0.8436000, + 1.5154001,
				  1.9412000, + 1.3114001,  1.6006000, + 1.8680000, 1.8314000, + 1.4862000;

		float4 SU2linkData;
		SU2linkData.x = 0.5309;
		SU2linkData.w = 0.8038;
		SU2linkData.z = 0.2338;
		SU2linkData.y = 0.1323;
		aSU2link.set( 0, SU2linkData );


		/*
		 * 		| X X 0 |
		 * A * 	| X X 0 |
		 * 		| 0 0 2 |
		 */
		ARightSubgroupMult01 << -0.3530214, + 1.3144565,  0.6576587, - 0.4375715, 0.1419000, + 0.6787000,
				 -0.9133987, + 0.5017763,  0.8501104, + 0.3058043, 0.4218000, + 0.7577000,
				 -0.3224384, + 1.0157899,  1.3158057, + 0.1342926, 0.9157000, + 0.7431000;
		/*
		 * | X X 0 |
		 * | X X 0 | * A
		 * | 0 0 2 |
		 */
		ALeftSubgroupMult01 <<  -0.2145999, + 1.4413472,  0.4806324, + 1.0510885,-0.4718312, + 0.7073354,
				  0.5245142, + 0.3251596,  0.7116889, + 0.1789136, 0.7100046, - 0.0766865,
				  0.9706000, + 0.6557000,  0.8003000, + 0.9340000, 0.9157000, + 0.7431000;

//		A.set(0, 0.9649);
//		A.set(1, 0.7922);
//		A.set(2, 0.9572);
//		A.set(3, 0.0357);
//		A.set(4, 0.1419);
//		A.set(5, 0.6787);
//		A.set(6, 0.1576);
//		A.set(7, 0.9595);
//		A.set(8, 0.4854);
//		A.set(9, 0.8491);
//		A.set(10, 0.4218);
//		A.set(11, 0.7577);
//		A.set(12, 0.9706);
//		A.set(13, 0.6557);
//		A.set(14, 0.8003);
//		A.set(15, 0.9340);
//		A.set(16, 0.9157);
//		A.set(17, 0.7431);

	}
};

TEST_F( ASpecialLocalLinkWithSU3Real18, IdentityWorks )
{
	A.identity();

	ASSERT_FLOAT_EQ( 1., A.get(0) );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, ReDetWorks )
{
	float result = A.reDet();

	ASSERT_FLOAT_EQ( redet, result );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, ReTraceWorks )
{
	float result = A.reTrace();

	ASSERT_FLOAT_EQ( retrace, result );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, CompareEqualOperatorWorks )
{
	LocalLink<SUNRealFull<3,float> > linkEq;
	linkEq = A;

	LocalLink<SUNRealFull<3,float> > linkNotEq;
	linkNotEq.zero();

	ASSERT_TRUE( linkEq == A );
	ASSERT_FALSE( linkNotEq == A );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, CompareUnequalOperatorWorks )
{
	LocalLink<SUNRealFull<3,float> > linkNotEq;
	linkNotEq.zero();

	ASSERT_TRUE( linkNotEq != A );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, OperatorMultAssignWorks )
{
	LocalLink<SUNRealFull<3,float> > B;
	B = A;

	B *= A;

	ASSERT_FLOAT_EQ( B.get(0), AtimesA_0_SP );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, OperatorMultWorks )
{
	LocalLink<SUNRealFull<3,float> > B;
	B = A*A;

	ASSERT_FLOAT_EQ( B.get(0), AtimesA_0_SP );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, HermitianWorks )
{
	A.hermitian();

	bool isSame = (A == Ahermitian );

	ASSERT_TRUE( isSame );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, GetSU2SubgroupWorks )
{
	LocalLink<SU2Vector4<float> > subgroup;
	subgroup = A.getSU2Subgroup( 0, 2 );

	ASSERT_FLOAT_EQ( subgroup.get(0).x, A.get(0)+A.get(16) );
}


TEST_F( ASpecialLocalLinkWithSU3Real18, LeftSubgroupMultWorks )
{
	A.leftSubgroupMult( aSU2link, 0, 1 );

	assertFloatEqual( ALeftSubgroupMult01, A );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, RightSubgroupMultWorks )
{
	A.rightSubgroupMult( aSU2link, 0, 1 );

	assertFloatEqual( ARightSubgroupMult01, A );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, TraceWorks )
{
	Complex<float> result = A.trace();

	ASSERT_FLOAT_EQ( trace.x, result.x );
	ASSERT_FLOAT_EQ( trace.y, result.y );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, AddAssignWorks )
{
	A += A;

	assertFloatEqual( AplusA, A );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, SubtractAssignWorks )
{
	A -= A;

	assertFloatEqual( LocalLink<SUNRealFull<3,float> >::getZero(), A );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, FrobeniusNormWorks )
{
	float frobeniusNormSquared = frobeniusNorm*frobeniusNorm;
	ASSERT_FLOAT_EQ( frobeniusNormSquared, A.normFrobeniusSquared() );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, MultWithScalarWorks )
{
	float someVal = 0.5;
	float originalVal = A.get(5);

	A*=someVal;

	ASSERT_FLOAT_EQ( originalVal*someVal, A.get(5) );
}

TEST_F( ASpecialLocalLinkWithSU3Real18, ReprojectWorks )
{
	AplusA.reproject();

	ASSERT_FLOAT_EQ( 1., AplusA.reDet() );
}

TEST( ALocalLink, GetIdentityWorks )
{
	LocalLink<SUNRealFull<3,float> > link;
	link.identity();

	bool isSame = (link == LocalLink<SUNRealFull<3,float> >::getIdentity() );

	ASSERT_TRUE( isSame );
}

TEST( ALocalLink, OperatorStreamWorksToFillMatrix )
{
	LocalLink<SUNRealFull<3,float> > link;

	link << 1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16., 17., 18., 19.;

	ASSERT_FLOAT_EQ( 18., link.get(17) );
}







