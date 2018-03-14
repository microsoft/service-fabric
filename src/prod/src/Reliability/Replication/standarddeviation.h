// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class StandardDeviation 
        {
        public:
            StandardDeviation();

            __declspec (property(get = get_Average)) Common::TimeSpan Average;
            Common::TimeSpan get_Average() const
            {
                auto avg = (count_ == 0) ? Common::TimeSpan::Zero : Common::TimeSpan::FromMilliseconds(milliSecondsSum_ / count_); 
                return avg;
            }

            __declspec (property(get = get_StdDev)) Common::TimeSpan StdDev;
            Common::TimeSpan get_StdDev() const;

            void Add(Common::TimeSpan const & value);
            void Clear();
        private:
            int count_;
            double milliSecondsSum_;
            double squaredMilliSecondsSum_;
        };
    }
}
