// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <limits>

namespace Common
{
#define Int32_MaxValue INT_MAX
#define Byte_MaxValue UCHAR_MAX
    /// gorn: shamelessly stolen from clr\bcl\system\random.cs

    /// < author > jroxe < /author >
    /// < date > July 8, 1998 < /date >
    class Random
    {

        const static int MBIG =  Int32_MaxValue; // without constexpr std::numeric_limits< int32 >::max();
        const static int MSEED = 161803398;
        const static int MZ = 0;

        //
        // Member Variables
        //
        int inext, inextp;
        int SeedArray[56];
        int initialSeed;
        static __volatile LONG StaticSeed;

    public:

        __declspec( property( get=get_InitialSeed )) int InitialSeed;
        int get_InitialSeed() const { return initialSeed; }

        Random();
        Random( int Seed );

        void Reseed( int Seed );

        /// <returns> An int [0..Int32.MaxValue) </returns>
        virtual int Next();

        /// <returns> An int [minvalue..maxValue) </returns>
        virtual int Next( int minValue, int maxValue );

        /// <returns> An int [0..maxValue) </returns>
        virtual int Next( int maxValue );

        virtual byte NextByte();

        /// <returns> A double [0..1) </returns>
        virtual double NextDouble();

        virtual void NextBytes( std::vector< byte >& buffer );

    protected:

        /// <returns> Return a new random number [0..1) and reSeed the Seed array. </returns>
        virtual double Sample();

    private:

        int InternalSample();

        /// <remarks>
        /// The distribution of double value returned by Sample 
        /// is not distributed well enough for a large range.
        /// If we use Sample for a range [Int32.MinValue..Int32.MaxValue)
        /// We will end up getting even numbers only.
        /// </remarks>
        double GetSampleForLargeRange();
    };
}
