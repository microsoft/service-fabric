// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedApplicationEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(DeployedApplicationEntityHealthInformation)

    public:
        DeployedApplicationEntityHealthInformation();

        DeployedApplicationEntityHealthInformation(
            std::wstring const & applicationName,
            Common::LargeInteger const & nodeId,
            std::wstring const & nodeName,
            FABRIC_INSTANCE_ID applicationInstanceId);

        DeployedApplicationEntityHealthInformation(DeployedApplicationEntityHealthInformation && other) = default;
        DeployedApplicationEntityHealthInformation & operator = (DeployedApplicationEntityHealthInformation && other) = default;
        
        __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_NodeId)) Common::LargeInteger const & NodeId;
        Common::LargeInteger const & get_NodeId() const { return nodeId_; }

        __declspec(property(get=get_ApplicationInstanceId)) FABRIC_INSTANCE_ID ApplicationInstanceId;
        FABRIC_INSTANCE_ID get_ApplicationInstanceId() const { return applicationInstanceId_; }

        std::wstring const & get_EntityId() const;
        DEFINE_INSTANCE_FIELD(applicationInstanceId_)
    
        Common::ErrorCode GenerateNodeId();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        FABRIC_FIELDS_05(kind_, applicationName_, nodeId_, nodeName_, applicationInstanceId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        Common::LargeInteger nodeId_;
        std::wstring nodeName_;
        FABRIC_INSTANCE_ID applicationInstanceId_;
    };

    DEFINE_HEALTH_ENTITY_ACTIVATOR(DeployedApplicationEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION)
}
