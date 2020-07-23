// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "LoadDistribution.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

LoadDistribution LoadDistribution::Parse(std::wstring const& input, std::wstring const& delimiter)
{
    StringCollection loadStr;
    StringUtility::Split(input, loadStr, delimiter);
    StringUtility::TrimWhitespaces(loadStr[0]);
    ASSERT_IF(loadStr.size() < 3, "Invalid load distribution {0}", loadStr);

    wchar_t dist = loadStr[0].c_str()[0];
    double i1 = Double_Parse(loadStr[1]);
    double i2 = Double_Parse(loadStr[2]);
    double i3(0);
    if (loadStr.size() > 3)
        i3 = Double_Parse(loadStr[3]);

    return LoadDistribution(dist, i1, i2, i3);
}

LoadDistribution::LoadDistribution(wchar_t distribution, double i1, double i2, double i3)
    : distributionType_(distribution), input1_(i1), input2_(i2), input3_(i3)
{
}

uint LoadDistribution::GetRandom(Random & rand) const
{
    double result;
    if (distributionType_ == 'U' || distributionType_ == 'u')
        result = RandomNumUniform(rand);
    else if (distributionType_ == 'P' || distributionType_ == 'p')
        result = RandomNumPowerLaw(rand);
    else
        Assert::CodingError("Unsupported distribution type: {0}", distributionType_);
            
    ASSERT_IF(
        result < 0.0 || result > static_cast<double>(UINT_MAX), 
        "Generated random number out of range (uint): {0}", result);

    return static_cast<uint>(ceil(result));
}

double LoadDistribution::RandomNumUniform(Random & rand) const
{
    return rand.NextDouble() * (input2_ - input1_) + input1_;
}

double LoadDistribution::RandomNumPowerLaw(Random & rand) const
{
    double value = max(rand.NextDouble(), 0.1);
    return input2_ * pow(value, -input1_) + input3_;
}
