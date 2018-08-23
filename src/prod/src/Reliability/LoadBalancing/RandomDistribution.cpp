// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RandomDistribution.h"

#include <algorithm>
#include <random>

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

RandomDistribution::RandomDistribution(uint32_t seed)
{
    lambda_ = 1;
    seed_ = seed;
}

DistributionMap RandomDistribution::GenerateDistribution(
    uint numberOfSamples,
    RandomDistribution::Enum distribution,
    int lowerBound,
    int upperBound)
{
    if (distribution == RandomDistribution::Enum::Gaussian)
    {
        return GenerateGaussianDistribution(numberOfSamples, lowerBound, upperBound);
    }
    else if (distribution == RandomDistribution::Enum::Uniform)
    {
        return GenerateUniformDistribution(numberOfSamples, lowerBound, upperBound);
    }
    else if (distribution == RandomDistribution::Enum::Exponential)
    {
        return GenerateExponentialDistribution(numberOfSamples, lowerBound, upperBound);
    }
    else
    {
        return GenerateAllOnesDistribution(numberOfSamples);
    }
}

uint RandomDistribution::GetNumberOfSamples(DistributionMap const& distribution)
{
    uint sum = 0;
    for (auto it : distribution)
    {
        sum += it.second;
    }
    return sum;
}

DistributionMap RandomDistribution::GenerateGaussianDistribution(
    uint numberOfSamples,
    int lowerBound,
    int upperBound)
{
    DistributionMap result;
    int range = upperBound - lowerBound + 1;
    mt19937 generator(seed_);
    int mean = (lowerBound + upperBound) / 2;
    int stddev = static_cast<int>(GAUSSIAN_SAMPLE_PERCENTAGE * range + 1);
    normal_distribution<double> distribution(mean, stddev);
    auto rand = bind(distribution, generator);

    vector<double> temp(numberOfSamples);
    generate(temp.begin(), temp.end(), rand);

    for (int i = 0; i < temp.size(); i++)
    {
        int number = (int)temp[i];
        if (number < lowerBound)
        {
            number = lowerBound;
        }
        else if (number > upperBound)
        {
            number = upperBound;
        }
        result[number]++;
    }
    return result;
}

DistributionMap RandomDistribution::GenerateUniformDistribution(
    uint numberOfSamples,
    int lowerBound,
    int upperBound)
{
    DistributionMap result;
    int range = upperBound - lowerBound + 1;
    int div = numberOfSamples / range;
    int mod = numberOfSamples % range;
    for (int i = lowerBound; i <= upperBound; i++)
    {
        result[i] = div;
        if (mod > 0)
        {
            result[i]++;
            mod--;
        }
    }
    return result;
}

DistributionMap RandomDistribution::GenerateExponentialDistribution(
    uint numberOfSamples,
    int lowerBound,
    int upperBound)
{
    DistributionMap result;
    int range = upperBound - lowerBound + 1;
    mt19937 generator(seed_);
    exponential_distribution<double> distribution(lambda_);
    auto rand = bind(distribution, generator);

    vector<double> temp(numberOfSamples);
    generate(temp.begin(), temp.end(), rand);

    double maxValue = *max_element(temp.begin(), temp.end());
    for (int i = 0; i < temp.size(); i++)
    {
        double normalized = temp[i] / maxValue;
        int number = static_cast<int>(normalized * range + lowerBound);
        if (number < lowerBound)
        {
            number = lowerBound;
        }
        else if (number > upperBound)
        {
            number = upperBound;
        }
        result[number]++;
    }
    return result;
}

DistributionMap RandomDistribution::GenerateAllOnesDistribution(uint numberOfSamples)
{
    return GenerateUniformDistribution(numberOfSamples, 1, 1);
}
