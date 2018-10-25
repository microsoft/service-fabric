//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once 

namespace Api
{
    class IChaosDescriptionResult
    {
    public:
        virtual ~IChaosDescriptionResult() {};

        virtual std::shared_ptr<Management::FaultAnalysisService::ChaosDescription> const & GetChaos() = 0;
    };
}
