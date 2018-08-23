// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(ApplicationEntityHealthInformation)

    public:
        ApplicationEntityHealthInformation();

        ApplicationEntityHealthInformation(
            std::wstring const & applicationName,
            FABRIC_INSTANCE_ID applicationInstanceId);

        ApplicationEntityHealthInformation(ApplicationEntityHealthInformation && other) = default;
        ApplicationEntityHealthInformation & operator = (ApplicationEntityHealthInformation && other) = default;
        
        __declspec(property(get=get_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_ApplicationInstanceId)) FABRIC_INSTANCE_ID ApplicationInstanceId;
        FABRIC_INSTANCE_ID get_ApplicationInstanceId() const { return applicationInstanceId_; }

        std::wstring const& get_EntityId() const { return applicationName_; }
        DEFINE_INSTANCE_FIELD(applicationInstanceId_)       

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        FABRIC_FIELDS_03(kind_, applicationName_, applicationInstanceId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        FABRIC_INSTANCE_ID applicationInstanceId_;
    };

    DEFINE_HEALTH_ENTITY_ACTIVATOR(ApplicationEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_APPLICATION)
}
