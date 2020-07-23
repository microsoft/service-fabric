// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LBSimulator
{
    class LoadDistribution
    {
    public:
        static LoadDistribution Parse(std::wstring const& input, std::wstring const& delimiter);

        LoadDistribution(
            wchar_t distribution = 'U',
            double i1 = 0,
            double i2 = 0,
            double i3 = 0
            );

        uint GetRandom(Common::Random & rand) const;

    private:
        double RandomNumUniform(Common::Random & rand) const;

        double RandomNumPowerLaw(Common::Random & rand) const;

        wchar_t distributionType_;
        double input1_;
        double input2_;
        double input3_;
    };
}
