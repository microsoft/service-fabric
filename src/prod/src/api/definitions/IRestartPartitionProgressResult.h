// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IRestartPartitionProgressResult
    {
    public:
        virtual ~IRestartPartitionProgressResult() {};

        virtual std::shared_ptr<Management::FaultAnalysisService::RestartPartitionProgress> const & GetProgress() = 0;
    };
}
