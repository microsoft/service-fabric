// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverThrottle
        {
        public:
            FailoverThrottle(Common::ConfigEntry<Common::TimeSpan> & throttleTime);

            bool IsThrottling();
            FailoverThrottle(FailoverThrottle const&) = delete;
            void operator=(FailoverThrottle const&) = delete;
        private:
            Common::ConfigEntry<Common::TimeSpan> & throttleTime_;
            Common::Stopwatch stopwatch_;
            bool isFirstCall_;
        };
    }
}
