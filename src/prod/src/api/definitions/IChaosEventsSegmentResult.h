//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Api
{
    class IChaosEventsSegmentResult
    {
    public:
        virtual ~IChaosEventsSegmentResult() {};

        virtual std::shared_ptr<Management::FaultAnalysisService::ChaosEventsSegment> const & GetChaosEvents() = 0;
    };
}
