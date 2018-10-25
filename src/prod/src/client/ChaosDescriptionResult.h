//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Client
{
    class ChaosDescriptionResult
        : public Api::IChaosDescriptionResult
        , public Common::ComponentRoot
    {
        DENY_COPY(ChaosDescriptionResult);

    public:
        ChaosDescriptionResult(
            __in std::shared_ptr<Management::FaultAnalysisService::ChaosDescription> &&chaosDescription);

        std::shared_ptr<Management::FaultAnalysisService::ChaosDescription> const & GetChaos();

    private:
        std::shared_ptr<Management::FaultAnalysisService::ChaosDescription> chaosDescription_;
    };
}
