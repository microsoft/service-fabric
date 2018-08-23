// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ComposeDeploymentStatusQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
        DENY_COPY(ComposeDeploymentStatusQueryResult)

    public:
        ComposeDeploymentStatusQueryResult() = default;
        ComposeDeploymentStatusQueryResult(
            std::wstring const & deploymentName_,
            Common::NamingUri const & applicationName,
            ComposeDeploymentStatus::Enum dockerComposeDeploymentStatus,
            std::wstring const & statusDetails);

        ComposeDeploymentStatusQueryResult(ComposeDeploymentStatusQueryResult && other)= default;
        ComposeDeploymentStatusQueryResult & operator = (ComposeDeploymentStatusQueryResult && other) = default;

        __declspec(property(get=get_DeploymentName)) std::wstring const & DeploymentName;
        std::wstring const & get_DeploymentName() const { return deploymentName_; }

        __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get = get_Status)) ComposeDeploymentStatus::Enum  const Status;
         const ComposeDeploymentStatus::Enum  get_Status() const { return dockerComposeDeploymentStatus_; }

        __declspec(property(get = get_StatusDescription)) std::wstring const & StatusDetails;
        std::wstring const & get_StatusDescription() const { return statusDetails_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM & publicQueryResult) const;

        void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        std::wstring CreateContinuationToken() const override;

        Common::ErrorCode FromPublicApi(__in FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM const & publicComposeQueryResult);

        FABRIC_FIELDS_04(
            applicationName_,
            dockerComposeDeploymentStatus_,
            statusDetails_,
            deploymentName_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::DeploymentName, deploymentName_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::ComposeDeploymentStatus, dockerComposeDeploymentStatus_)
            SERIALIZABLE_PROPERTY_IF(Constants::StatusDetails, statusDetails_, !statusDetails_.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(deploymentName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(dockerComposeDeploymentStatus_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(statusDetails_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring deploymentName_;
        Common::NamingUri applicationName_;
        ComposeDeploymentStatus::Enum dockerComposeDeploymentStatus_;
        std::wstring statusDetails_;
    };

    QUERY_JSON_LIST(ComposeDeploymentList, ComposeDeploymentStatusQueryResult)
}
