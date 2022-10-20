// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        class SelectedReplica : public Serialization::FabricSerializable
        {
        public:
            SelectedReplica();
            SelectedReplica(FABRIC_REPLICA_ID && replicaId, std::shared_ptr<Management::FaultAnalysisService::SelectedPartition> && selectedPartition) 
                : replicaOrInstanceId_(std::move(replicaId)), selectedPartition_(std::move(selectedPartition)) {};
            SelectedReplica(SelectedReplica const & other);
            SelectedReplica & operator = (SelectedReplica const & other);
            bool operator == (SelectedReplica const & rhs) const;
            bool operator != (SelectedReplica const & rhs) const;

            SelectedReplica(SelectedReplica && other);
            SelectedReplica & operator = (SelectedReplica && other);

            __declspec(property(get = get_ReplicaOrInstanceId)) FABRIC_INSTANCE_ID ReplicaOrInstanceId;
            FABRIC_INSTANCE_ID get_ReplicaOrInstanceId() const { return replicaOrInstanceId_; }

            __declspec(property(get = get_Partition)) std::shared_ptr<Management::FaultAnalysisService::SelectedPartition> const & Partition;
            std::shared_ptr<Management::FaultAnalysisService::SelectedPartition> const & get_Partition() const { return selectedPartition_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_SELECTED_REPLICA const & selectedReplica);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SELECTED_REPLICA &) const;

            FABRIC_FIELDS_02(replicaOrInstanceId_, selectedPartition_);

        private:
            FABRIC_REPLICA_ID replicaOrInstanceId_;
            std::shared_ptr<Management::FaultAnalysisService::SelectedPartition> selectedPartition_;
        };
    }
}
