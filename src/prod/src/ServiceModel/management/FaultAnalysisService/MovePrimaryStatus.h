// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class MovePrimaryStatus : public Serialization::FabricSerializable
        {
        public:
            MovePrimaryStatus();

            MovePrimaryStatus(MovePrimaryStatus &&);

            MovePrimaryStatus & operator=(MovePrimaryStatus &&);

            MovePrimaryStatus(
                std::shared_ptr<Management::FaultAnalysisService::PrimaryMoveResult> && primaryMoveResult);

            __declspec(property(get = get_MovePrimaryResult)) std::shared_ptr<Management::FaultAnalysisService::PrimaryMoveResult> const & ResultSPtr;
            std::shared_ptr<Management::FaultAnalysisService::PrimaryMoveResult> const & get_MovePrimaryResult() const { return primaryMoveResultSPtr_; }

            std::shared_ptr<Management::FaultAnalysisService::PrimaryMoveResult> const & GetMovePrimaryResult();

            FABRIC_FIELDS_01(
                primaryMoveResultSPtr_);

        private:
            std::shared_ptr<Management::FaultAnalysisService::PrimaryMoveResult> primaryMoveResultSPtr_;
        };
    }
}
