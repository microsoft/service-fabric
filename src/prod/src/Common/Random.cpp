// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/Random.h"
#include "Common/Environment.h"

namespace Common
{

    __volatile LONG Random::StaticSeed = static_cast<LONG>(Stopwatch::Now().Ticks / 10000);

    Random::Random()
    {
        Reseed( InterlockedIncrement( &StaticSeed ));
    }

    Random::Random( int Seed )
    {
        Reseed( Seed );
    }

    void Random::Reseed( int Seed )
    {
        int ii;
        int mj, mk;

        initialSeed = Seed;

        // Initialize our Seed array.
        // This algorithm comes from Numerical Recipes in C (2nd Ed.)
        mj = MSEED - std::abs( Seed );
        SeedArray[ 55 ] = mj;
        mk = 1;

        // Apparently the range [1..55] is special (Knuth) and so we're wasting the 0'th position.
        for ( int i = 1; i < 55; i++ )
        {  
            ii = ( 21 * i ) % 55;
            SeedArray[ ii ] = mk;
            mk = mj - mk;
            if ( mk < 0 )
                mk += MBIG;
            mj = SeedArray[ ii ];
        }

        for ( int k = 1; k < 5; k++ )
        {
            for ( int i = 1; i < 56; i++ )
            {
                SeedArray[ i ] -= SeedArray[ 1 + ( i + 30 ) % 55 ];
                if ( SeedArray[ i ] < 0 )
                    SeedArray[ i ] += MBIG;
            }
        }

        inext = 0;
        inextp = 21;
    }

    int Random::Next()
    {
        return InternalSample();
    }

    int Random::Next( int minValue, int maxValue )
    {
        //argument_valid( minValue, ( minValue < = maxValue ) );

        int64 range = ( int64 ) maxValue - minValue;

        if ( range <= ( int64 ) Int32_MaxValue )
            return( int )( Sample() * range ) + minValue;
        else
            return( int )(( int64 )( GetSampleForLargeRange() * range ) + minValue );
    }

    int Random::Next( int maxValue )
    {
        //argument_valid( maxValue, maxValue >= 0 );

        return( int )( Sample() * maxValue );
    }

    byte Random::NextByte()
    {
        return( byte )( Sample() * 256 );
    }

    double Random::NextDouble()
    { 
        return Sample();
    }

    void Random::NextBytes( std::vector< byte >& buffer )
    {
        for ( size_t i = 0; i < buffer.size(); ++i )
        {
            buffer[ i ] = byte( Next( Byte_MaxValue ));
        }
    }

    double Random::Sample()
    {
        //Including this division at the end gives us significantly improved
        //random number distribution.
        return InternalSample() * ( 1.0 / MBIG );
    }

    int Random::InternalSample()
    {
        int retVal;
        int locINext = inext;
        int locINextp = inextp;

        if ( ++locINext >= 56 )
            locINext = 1;
        if ( ++locINextp >= 56 )
            locINextp = 1;

        retVal = SeedArray[ locINext ] - SeedArray[ locINextp ];

        if ( retVal < 0 )
            retVal += MBIG;

        SeedArray[ locINext ] = retVal;

        inext = locINext;
        inextp = locINextp;

        return retVal;
    }

    /// < remarks >
    /// The distribution of double value returned by Sample 
    /// is not distributed well enough for a large range.
    /// If we use Sample for a range [Int32.MinValue..Int32.MaxValue)
    /// We will end up getting even numbers only.
    /// < /remarks >
    double Random::GetSampleForLargeRange()
    {
        int result = InternalSample();

        // Note we can't use addition here. The distribution will be bad if we do that.
        bool negative = ( InternalSample() % 2 == 0 );  // decide the sign based on second sample

        if ( negative )
            result = -result;

        double d = result;
        d += ( Int32_MaxValue - 1 ); // get a number in range [0 .. 2 * Int32MaxValue - 1)
        d /= 2 * ( unsigned )Int32_MaxValue - 1  ;              
        return d;
    }
}
