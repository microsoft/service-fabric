// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class MoveSecondaryStatus : public Serialization::FabricSerializable
        {
        public:
            MoveSecondaryStatus();

            MoveSecondaryStatus(MoveSecondaryStatus &&);

            MoveSecondaryStatus & operator=(MoveSecondaryStatus &&);

            MoveSecondaryStatus(
                std::shared_ptr<Management::FaultAnalysisService::SecondaryMoveResult> && secondaryMoveResult);

            __declspec(property(get = get_MoveSecondaryResult)) std::shared_ptr<Management::FaultAnalysisService::SecondaryMoveResult> const & ResultSPtr;
            std::shared_ptr<Management::FaultAnalysisService::SecondaryMoveResult> const & get_MoveSecondaryResult() const { return secondaryMoveResultSPtr_; }

            std::shared_ptr<Management::FaultAnalysisService::SecondaryMoveResult> const & GetMoveSecondaryResult();

            FABRIC_FIELDS_01(
                secondaryMoveResultSPtr_);

        private:
            std::shared_ptr<Management::FaultAnalysisService::SecondaryMoveResult> secondaryMoveResultSPtr_;
        };
    }
}
