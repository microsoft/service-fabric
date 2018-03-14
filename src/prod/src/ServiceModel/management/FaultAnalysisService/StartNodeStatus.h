// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StartNodeStatus: public Serialization::FabricSerializable
        {
        public:
            StartNodeStatus();

            StartNodeStatus(StartNodeStatus &&);

            StartNodeStatus & operator=(StartNodeStatus &&);

            StartNodeStatus(
                std::shared_ptr<Management::FaultAnalysisService::NodeResult> && nodeResult);

            __declspec(property(get = get_NodeResult)) std::shared_ptr<Management::FaultAnalysisService::NodeResult> const & Result;
            std::shared_ptr<Management::FaultAnalysisService::NodeResult> const & get_NodeResult() const { return nodeResult_; }

            std::shared_ptr<Management::FaultAnalysisService::NodeResult> const & GetNodeResult();

            Common::ErrorCode FromPublicApi(__in FABRIC_START_NODE_STATUS const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_START_NODE_STATUS &) const;
            
            FABRIC_FIELDS_01(
                nodeResult_);

        private:
            std::shared_ptr<Management::FaultAnalysisService::NodeResult> nodeResult_;
        };
    }
}
