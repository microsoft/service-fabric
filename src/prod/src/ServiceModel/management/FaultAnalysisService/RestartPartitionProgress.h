// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class RestartPartitionProgress : public Serialization::FabricSerializable
        {
        public:

            RestartPartitionProgress();

            RestartPartitionProgress(RestartPartitionProgress &&);

            RestartPartitionProgress & operator=(RestartPartitionProgress &&);

            RestartPartitionProgress(
                TestCommandProgressState::Enum state,
                std::shared_ptr<Management::FaultAnalysisService::RestartPartitionResult> && result);

            __declspec(property(get = get_State)) TestCommandProgressState::Enum State;
            __declspec(property(get = get_Result)) std::shared_ptr<Management::FaultAnalysisService::RestartPartitionResult> const & Result;

            TestCommandProgressState::Enum const & get_State() const { return state_; }
            std::shared_ptr<Management::FaultAnalysisService::RestartPartitionResult> const & get_Result() const { return result_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_PARTITION_RESTART_PROGRESS const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_PARTITION_RESTART_PROGRESS &) const;

            FABRIC_FIELDS_02(
                state_,
                result_);

        private:
            TestCommandProgressState::Enum state_;
            std::shared_ptr<Management::FaultAnalysisService::RestartPartitionResult> result_;
        };
    }
}
