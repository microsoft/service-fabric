// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IMovePrimaryResult
    {
    public:
        virtual ~IMovePrimaryResult() {};

        virtual std::shared_ptr<Management::FaultAnalysisService::PrimaryMoveResult> const & GetMovePrimaryResult() = 0;
    };
}
