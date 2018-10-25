// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures.Concurrent
{
    using System;

    /// <summary>
    /// A thread-safe random number generator.
    /// Uses a global random for generating seed and thread local random for thread-safe random number generation.
    /// RandomGen2 from https://blogs.msdn.microsoft.com/pfxteam/2009/02/19/getting-random-numbers-in-a-thread-safe-way/
    /// </summary>
    internal static class RandomGenerator
    {
        /// <summary>
        /// Global seed generator.
        /// </summary>
        private static Random globalSeedRandom = new Random();

        /// <summary>
        /// Thread local random number generator.
        /// </summary>
        [ThreadStatic]
        private static Random localRandom;

        /// <summary>
        /// Returns a nonnegative random number.
        /// </summary>
        /// <returns>
        /// A 32-bit signed integer greater than or equal to zero and less than System.Int32.MaxValue.
        /// </returns>
        public static int Next()
        {
            Random snapLocalRandom = localRandom;

            // If thread does not have a Random, create a random.
            if (snapLocalRandom == null)
            {
                int seed;
                lock (globalSeedRandom)
                {
                    seed = globalSeedRandom.Next();
                }

                localRandom = snapLocalRandom = new Random(seed);
            }

            return snapLocalRandom.Next();
        }

        /// <summary>
        /// Returns a random number within a specified range.
        /// </summary>
        /// <param name="minValue"> The inclusive lower bound of the random number returned.</param>
        /// <param name="maxValue">
        /// The exclusive upper bound of the random number returned. maxValue must be greater than or equal to minValue.
        /// </param>
        /// <returns>
        /// A 32-bit signed integer greater than or equal to minValue and less than maxValue;
        /// that is, the range of return values includes minValue but not maxValue. If minValue
        /// equals maxValue, minValue is returned.
        /// </returns>
        public static int Next(int minValue, int maxValue)
        {
            Random snapLocalRandom = localRandom;

            // If thread does not have a Random, create a random.
            if (snapLocalRandom == null)
            {
                int seed;
                lock (globalSeedRandom)
                {
                    seed = globalSeedRandom.Next();
                }

                localRandom = snapLocalRandom = new Random(seed);
            }

            return snapLocalRandom.Next(minValue, maxValue);
        }

        /// <summary>
        /// Returns a random number between 0.0 and 1.0.
        /// </summary>
        /// <returns>
        /// A double-precision floating point number greater than or equal to 0.0, and less than 1.0.
        /// </returns>
        public static double NextDouble()
        {
            Random snapLocalRandom = localRandom;

            // If thread does not have a Random, create a random.
            if (snapLocalRandom == null)
            {
                int seed;
                lock (globalSeedRandom)
                {
                    seed = globalSeedRandom.Next();
                }

                localRandom = snapLocalRandom = new Random(seed);
            }

            return snapLocalRandom.NextDouble();
        }
    }
}