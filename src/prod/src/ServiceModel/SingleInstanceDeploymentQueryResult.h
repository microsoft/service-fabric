//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class SingleInstanceDeploymentQueryResult
        : public Serialization::FabricSerializable
        , public IPageContinuationToken
    {
    public:
        SingleInstanceDeploymentQueryResult() = default;
        SingleInstanceDeploymentQueryResult(SingleInstanceDeploymentQueryResult const &) = default;
        SingleInstanceDeploymentQueryResult(SingleInstanceDeploymentQueryResult &&) = default;
        SingleInstanceDeploymentQueryResult(
            std::wstring const & deploymentName,
            Common::NamingUri const & applicationUri,
            ServiceModel::SingleInstanceDeploymentStatus::Enum const status,
            std::wstring const & statusDetails);

        SingleInstanceDeploymentQueryResult & operator=(SingleInstanceDeploymentQueryResult const &);
        SingleInstanceDeploymentQueryResult & operator=(SingleInstanceDeploymentQueryResult && other);

        __declspec(property(get=get_DeploymentName)) std::wstring const & DeploymentName;
        std::wstring const & get_DeploymentName() const { return deploymentName_; }

        __declspec(property(get=get_ApplicationUri)) Common::NamingUri const & ApplicationUri;
        Common::NamingUri const & get_ApplicationUri() const { return applicationUri_; }

        __declspec(property(get=get_Status)) ServiceModel::SingleInstanceDeploymentStatus::Enum Status;
        ServiceModel::SingleInstanceDeploymentStatus::Enum get_Status() const { return status_; }

        __declspec(property(get=get_StatusDetails)) std::wstring const & StatusDetails;
        std::wstring const & get_StatusDetails() const { return statusDetails_; }

        std::wstring CreateContinuationToken() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::deploymentName, deploymentName_)
            SERIALIZABLE_PROPERTY(Constants::applicationUri, applicationUri_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::status, status_)
            SERIALIZABLE_PROPERTY_IF(Constants::statusDetails, statusDetails_, !statusDetails_.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

        FABRIC_FIELDS_04(
            deploymentName_,
            applicationUri_,
            status_,
            statusDetails_)

    protected:
        std::wstring deploymentName_;
        Common::NamingUri applicationUri_;
        ServiceModel::SingleInstanceDeploymentStatus::Enum status_;
        std::wstring statusDetails_;
    };
}
