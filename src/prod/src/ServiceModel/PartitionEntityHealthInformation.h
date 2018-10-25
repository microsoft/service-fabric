// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class PartitionEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(PartitionEntityHealthInformation)
    public:
        PartitionEntityHealthInformation();

        explicit PartitionEntityHealthInformation(Common::Guid const& partitionId);

        PartitionEntityHealthInformation(PartitionEntityHealthInformation && other) = default;
        PartitionEntityHealthInformation & operator = (PartitionEntityHealthInformation && other) = default;
        
         __declspec(property(get=get_PartitionId)) Common::Guid const& PartitionId;
        Common::Guid const& get_PartitionId() const { return partitionId_; }

        std::wstring const& get_EntityId() const;
        
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        FABRIC_FIELDS_02(kind_, partitionId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::Guid partitionId_;
    };

    DEFINE_HEALTH_ENTITY_ACTIVATOR(PartitionEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_PARTITION)
}
