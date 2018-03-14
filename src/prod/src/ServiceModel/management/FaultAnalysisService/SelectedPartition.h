// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        class SelectedPartition : public Serialization::FabricSerializable, public Common::IFabricJsonSerializable
        {
        public:
            SelectedPartition();
            SelectedPartition(Common::NamingUri const & serviceName, Common::Guid const partitionId): serviceName_(serviceName), partitionId_(partitionId) {};
            SelectedPartition(SelectedPartition const & other);
            SelectedPartition & operator = (SelectedPartition const & other);
            bool operator == (SelectedPartition const & rhs) const;
            bool operator != (SelectedPartition const & rhs) const;

            SelectedPartition(SelectedPartition && other);
            SelectedPartition & operator = (SelectedPartition && other);

            __declspec(property(get = get_ServiceName)) Common::NamingUri const& ServiceName;
            Common::NamingUri const& get_ServiceName() const { return serviceName_; }

            __declspec(property(get = get_PartitionId)) Common::Guid const& PartitionId;
            Common::Guid const& get_PartitionId() const { return partitionId_; }

            Common::ErrorCode FromPublicApi(__in FABRIC_SELECTED_PARTITION const & selectedPartition);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SELECTED_PARTITION &) const;

            FABRIC_FIELDS_02(serviceName_, partitionId_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceName, serviceName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionId, partitionId_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            Common::NamingUri serviceName_;
            Common::Guid partitionId_;
        };
    }
}
