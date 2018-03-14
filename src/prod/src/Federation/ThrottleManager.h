// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class ThrottleManager
    {
    public:
        ThrottleManager(int threshold, Common::TimeSpan delayLowerBound, Common::TimeSpan delayUpperBound);

        Common::TimeSpan CheckDelay() const;
        void Add();

    private:
        int threshold_;
        Common::TimeSpan delayLowerBound_;
        Common::TimeSpan delayUpperBound_;
        int continuousCount_;
        Common::DateTime lastActivity_;
        Common::TimeSpan delay_;
    };
}
