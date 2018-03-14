// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class InvokeDataLossProgress : public Serialization::FabricSerializable
        {
        public:
            InvokeDataLossProgress();

            InvokeDataLossProgress(InvokeDataLossProgress &&);

            InvokeDataLossProgress & operator=(InvokeDataLossProgress &&);

            InvokeDataLossProgress(
                TestCommandProgressState::Enum state,
                std::shared_ptr<Management::FaultAnalysisService::InvokeDataLossResult> && result);

            __declspec(property(get=get_State)) TestCommandProgressState::Enum State;
            __declspec(property(get=get_Result)) std::shared_ptr<Management::FaultAnalysisService::InvokeDataLossResult> const & Result;

            TestCommandProgressState::Enum const & get_State() const { return state_; }
            std::shared_ptr<Management::FaultAnalysisService::InvokeDataLossResult> const & get_Result() const { return result_; }
            
            Common::ErrorCode FromPublicApi(__in FABRIC_PARTITION_DATA_LOSS_PROGRESS const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_PARTITION_DATA_LOSS_PROGRESS &) const;

            FABRIC_FIELDS_02(
                state_,
                result_);

        private:
            TestCommandProgressState::Enum state_;
            std::shared_ptr<Management::FaultAnalysisService::InvokeDataLossResult> result_;
        };
    }
}
