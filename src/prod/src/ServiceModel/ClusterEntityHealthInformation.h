// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ClusterEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(ClusterEntityHealthInformation)

    public:
        ClusterEntityHealthInformation();

        ClusterEntityHealthInformation(ClusterEntityHealthInformation && other);
        ClusterEntityHealthInformation & operator = (ClusterEntityHealthInformation && other);

        std::wstring const & get_EntityId() const;
        
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        FABRIC_FIELDS_01(kind_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
        END_DYNAMIC_SIZE_ESTIMATION()
    };
    
    DEFINE_HEALTH_ENTITY_ACTIVATOR(ClusterEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_CLUSTER)
}
