// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if defined(PLATFORM_UNIX)
extern int rand_r(unsigned int *seedp);
#define RandomFunction(p) rand_r(p)
#define RANDOM_MAX RAND_MAX
#else
#define RandomFunction(p) RtlRandomEx(p)
#define RANDOM_MAX (MAXLONG - 1)
#endif

// A thread-safe random number generator.
// Uses a global random for generating seed and thread local random for thread-safe random number generation.

class RandomGenerator
{
public:
    RandomGenerator() : localSeedRandom_(RANDOM_MAX + 1)
    {
    }

    double NextDouble()
    {
        if (localSeedRandom_ > RANDOM_MAX)
        {
            localSeedRandom_ = RandomFunction(&globalSeedRandom_);
        }

        return ((double) RandomFunction(&localSeedRandom_)) / RANDOM_MAX;
    }

    double Next()
    {
        if (localSeedRandom_ > RANDOM_MAX)
        {
            localSeedRandom_ = RandomFunction(&globalSeedRandom_);
        }

        return RandomFunction(&localSeedRandom_);
    }

private:
    ULONG localSeedRandom_;

private:
    static ULONG globalSeedRandom_;
};
