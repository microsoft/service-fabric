//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once 

namespace Api
{
    class IChaosScheduleDescriptionResult
    {
    public:
        virtual ~IChaosScheduleDescriptionResult() {};

        virtual std::shared_ptr<Management::FaultAnalysisService::ChaosScheduleDescription> const & GetChaosSchedule() = 0;
    };
}
