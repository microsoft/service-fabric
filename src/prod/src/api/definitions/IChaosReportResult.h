// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IChaosReportResult
    {
    public:
        virtual ~IChaosReportResult() {};

        virtual std::shared_ptr<Management::FaultAnalysisService::ChaosReport> const & GetChaosReport() = 0;
    };
}
