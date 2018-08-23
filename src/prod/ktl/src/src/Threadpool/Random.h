#pragma once

#include <math.h>
#include <limits.h>
#include "MinPal.h"

namespace KtlThreadpool{

    class Random
    {
    private:
        // Private Constants
        static const int MBIG   = INT_MAX;
        static const int MSEED  = 161803398;
        static const int MZ     = 0;


        // Member Variables
        int inext;
        int inextp;
        int SeedArray[56];

        bool initialized;

    public:
        // Constructors
        Random()
        {
            initialized = false;
        }

        void Init()
        {
            LARGE_INTEGER time;

            if (!QueryPerformanceCounter(&time)) {
                time.QuadPart = GetTickCount();
            }

            Init((int)time.u.LowPart ^ GetCurrentThreadId() ^ GetCurrentProcessId());
        }

        void Init(int Seed)
        {
            int ii;
            int mj, mk;

            //Initialize our Seed array. This algorithm comes from Numerical Recipes in C (2nd Ed.)
            mj = MSEED - abs(Seed);
            SeedArray[55] = mj;
            mk = 1;
            for (int i = 1; i < 55; i++) {  //Apparently the range [1..55] is special (Knuth)
                ii = (21 * i) % 55;
                SeedArray[ii] = mk;
                mk = mj - mk;
                if (mk < 0) mk += MBIG;
                mj = SeedArray[ii];
            }
            for (int k = 1; k < 5; k++) {
                for (int i = 1; i < 56; i++) {
                    SeedArray[i] -= SeedArray[1 + (i + 30) % 55];
                    if (SeedArray[i] < 0) SeedArray[i] += MBIG;
                }
            }
            inext = 0;
            inextp = 21;
            Seed = 1;

            initialized = true;
        }

        bool IsInitialized()
        {
            return initialized;
        }

    private:
        // Package Private Methods
        // Sample: returns a new random number [0..1) and reSeed the Seed array.
        double Sample()
        {
            //Including this division at the end gives us significantly improved random number distribution.
            return (InternalSample()*(1.0/MBIG));
        }

        int InternalSample()
        {
            int retVal;
            int locINext = inext;
            int locINextp = inextp;

            if (++locINext >= 56) locINext = 1;
            if (++locINextp >= 56) locINextp = 1;

            retVal = SeedArray[locINext] - SeedArray[locINextp];

            if (retVal == MBIG) retVal--;
            if (retVal < 0) retVal += MBIG;

            SeedArray[locINext] = retVal;

            inext = locINext;
            inextp = locINextp;

            return retVal;
        }

        double GetSampleForLargeRange()
        {
            // The distribution of double value returned by Sample
            // is not distributed well enough for a large range.
            // If we use Sample for a range [Int32.MinValue..Int32.MaxValue)
            // We will end up getting even numbers only.

            int result = InternalSample();
            // Note we can't use addition here. The distribution will be bad if we do that.
            bool negative = (InternalSample() % 2 == 0);  // decide the sign based on second sample
            if( negative) {
                result = -result;
            }
            double d = result;
            d += (INT_MAX - 1); // get a number in range [0 .. 2 * Int32MaxValue - 1)
            d /= 2 * (unsigned int)INT_MAX - 1;
            return d;
        }

    public:
        // Public Instance Methods
        // Next: returns an int [0..Int32.MaxValue)
        int Next()
        {
            _ASSERTE(initialized);
            return InternalSample();
        }


        // Next: returns an int [minvalue..maxvalue)
        int Next(int minValue, int maxValue)
        {
            _ASSERTE(initialized);
            _ASSERTE(minValue < maxValue);

            LONGLONG range = (LONGLONG)maxValue-minValue;
            double result;

            if( range <= (LONGLONG)INT_MAX)
                result = (Sample() * range) + minValue;
            else
                result = (GetSampleForLargeRange() * range) + minValue;

            _ASSERTE(result >= minValue && result < maxValue);
            return (int)result;
        }

        // Next: returns an int [0..maxValue)
        int Next(int maxValue)
        {
            _ASSERTE(initialized);
            double result = Sample()*maxValue;
            _ASSERTE(result >= 0 && result < maxValue);
            return (int)result;
        }


        // Next: returns a double [0..1)
        double NextDouble()
        {
            _ASSERTE(initialized);
            double result = Sample();
            _ASSERTE(result >= 0 && result < 1);
            return result;
        }


        // NextBytes: Fills the byte array with random bytes [0..0x7f].  The entire array is filled.
        void NextBytes(BYTE buffer[], int length)
        {
            _ASSERTE(initialized);
            for (int i = 0; i < length; i++) {
                buffer[i] = (BYTE)(InternalSample() % (256));
            }
        }
    };
}
