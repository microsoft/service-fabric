//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Client
{
    class ChaosEventsSegmentResult
        : public Api::IChaosEventsSegmentResult
        , public Common::ComponentRoot
    {
        DENY_COPY(ChaosEventsSegmentResult);

    public:
        ChaosEventsSegmentResult(
            __in std::shared_ptr<Management::FaultAnalysisService::ChaosEventsSegment> &&chaosEventsSegment);

        std::shared_ptr<Management::FaultAnalysisService::ChaosEventsSegment> const & GetChaosEvents();

    private:
        std::shared_ptr<Management::FaultAnalysisService::ChaosEventsSegment> chaosEventsSegment_;
    };
}
