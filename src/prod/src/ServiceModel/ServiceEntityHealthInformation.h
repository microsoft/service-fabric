// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(ServiceEntityHealthInformation)

    public:
        ServiceEntityHealthInformation();

        ServiceEntityHealthInformation(
            std::wstring const & serviceName,
            FABRIC_INSTANCE_ID instanceId);

        ServiceEntityHealthInformation(ServiceEntityHealthInformation && other) = default;
        ServiceEntityHealthInformation & operator = (ServiceEntityHealthInformation && other) = default;
        
        __declspec(property(get=get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

        std::wstring const & get_EntityId() const { return serviceName_; }
        DEFINE_INSTANCE_FIELD(instanceId_)
        
        __declspec(property(get=get_InstanceId)) FABRIC_INSTANCE_ID  InstanceId;
        FABRIC_INSTANCE_ID get_InstanceId() const { return instanceId_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        FABRIC_FIELDS_03(kind_, serviceName_, instanceId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring serviceName_;
        FABRIC_INSTANCE_ID instanceId_;
    };

    DEFINE_HEALTH_ENTITY_ACTIVATOR(ServiceEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_SERVICE)
}
