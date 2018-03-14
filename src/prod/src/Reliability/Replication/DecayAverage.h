// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        //
        //  This class implements a running average calculation using a decay factor
        //  The decay factor is expected to be >= 0 and <= 1
        // 
        //  This class is NOT thread safe and must be protected by a higher level lock
        //
        class DecayAverage
        {
            DENY_COPY(DecayAverage)

        public:
            DecayAverage(
                double decayFactor,
                Common::TimeSpan decayInterval);

            DecayAverage(DecayAverage && other);

            __declspec (property(get=get_Value)) Common::TimeSpan Value;
            Common::TimeSpan get_Value() const;
 
            void Update(Common::TimeSpan const & currentValue);

            void UpdateDecayFactor(
                double decayFactor,
                Common::TimeSpan const & decayInterval);

            void Reset();

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

            std::wstring ToString() const;

        private:
            double decayFactor_;
            Common::TimeSpan decayInterval_;
            Common::StopwatchTime lastUpdatedTime_;
            uint lastValueMilliSeconds_;
            double weightedSumMilliSeconds_;
            double sumOfWeightMilliSeconds_;
        };
    }
}
