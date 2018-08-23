//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Client
{
    class ChaosScheduleDescriptionResult
        : public Api::IChaosScheduleDescriptionResult
        , public Common::ComponentRoot
    {
        DENY_COPY(ChaosScheduleDescriptionResult);

    public:
        ChaosScheduleDescriptionResult(
            __in std::shared_ptr<Management::FaultAnalysisService::ChaosScheduleDescription> &&chaosScheduleDescription);

        std::shared_ptr<Management::FaultAnalysisService::ChaosScheduleDescription> const & GetChaosSchedule();

    private:
        std::shared_ptr<Management::FaultAnalysisService::ChaosScheduleDescription> chaosScheduleDescription_;
    };
}
