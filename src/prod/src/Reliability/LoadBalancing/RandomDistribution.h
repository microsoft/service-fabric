// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        // <Key, Value> map where the key signifies the label
        // and the value signifies the number of generated samples with the label K
        // Example: [1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3] would be the equivalent of
        // the following distribution map {<1, 4>, <2, 7>, <3, 1>}
        typedef std::map<int, uint> DistributionMap;

        // What percentage of the range to take as the mean in Gaussian distribution
        // 99.7% of data under normal curve falls within [mean - 3*stddev, mean + 3*stddev]
        // So reverse engineer that, to cover the entire smaple range [A, B]
        // Mean = (B-A)/2, StdDev = (B-A)/6, or about 16% of the entire sample
        const double GAUSSIAN_SAMPLE_PERCENTAGE = 0.16;

        class RandomDistribution
        {
        public:
            RandomDistribution(uint32_t seed);

            enum Enum
            {
                Gaussian = 0,
                Uniform = 1,
                Exponential = 2,
                AllOnes = 3
            };
            DistributionMap GenerateDistribution(uint numberOfSamples, RandomDistribution::Enum distribution, int lowerBound = 1, int upperBound = 1);

            __declspec(property(get = get_Lambda, put = set_Lambda)) double Lambda;
            double get_Lambda() { return lambda_; }
            void set_Lambda(double lambda) { lambda_ = lambda; }

            __declspec(property(get = get_Seed, put = set_Seed)) uint32_t Seed;
            uint32_t get_Seed() { return seed_; }
            void set_Seed(uint32_t seed) { seed_ = seed; }

            uint GetNumberOfSamples(DistributionMap const& distribution);

        private:
            DistributionMap GenerateGaussianDistribution(uint numberOfSamples, int lowerBound, int upperBound);
            DistributionMap GenerateUniformDistribution(uint numberOfSamples, int lowerBound, int upperBound);
            DistributionMap GenerateExponentialDistribution(uint numberOfSamples, int lowerBound, int upperBound);
            DistributionMap GenerateAllOnesDistribution(uint numberOfSamples);

            // Rate of decline of an exponential distribution - higher number means steeper curve.
            double lambda_;

            // Seed that the generator uses.
            uint32_t seed_;
        };
    }
}
