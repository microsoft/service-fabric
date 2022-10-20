// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedNetworkCodePackageQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        DeployedNetworkCodePackageQueryResult();

        DeployedNetworkCodePackageQueryResult(
            std::wstring const & applicationName,
            std::wstring const & networkName,
            std::wstring const & codePackageName,
            std::wstring const & codePackageVersion,
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId,
            std::wstring const & containerId,
            std::wstring const & containerAddress);

        DeployedNetworkCodePackageQueryResult(DeployedNetworkCodePackageQueryResult && other);

        DeployedNetworkCodePackageQueryResult const & operator = (DeployedNetworkCodePackageQueryResult && other);

        ~DeployedNetworkCodePackageQueryResult() = default;

        Common::ErrorCode FromPublicApi(__in FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_ITEM const & deployedNetworkCodePackageQueryResult);

        std::wstring ToString() const;

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_ITEM & publicDeployedNetworkCodePackageQueryResult) const;

        __declspec(property(get = get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get = get_NetworkName)) std::wstring const & NetworkName;
        std::wstring const & get_NetworkName() const { return networkName_; }

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        std::wstring const & get_CodePackageName() const { return codePackageName_; }

        __declspec(property(get = get_CodePackageVersion)) std::wstring const & CodePackageVersion;
        std::wstring const & get_CodePackageVersion() const { return codePackageVersion_; }

        __declspec(property(get = get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        __declspec(property(get = get_ContainerAddress)) std::wstring const & ContainerAddress;
        std::wstring const & get_ContainerAddress() const { return containerAddress_; }

        __declspec(property(get = get_ContainerId)) std::wstring const & ContainerId;
        std::wstring const & get_ContainerId() const { return containerId_; }

        FABRIC_FIELDS_08(
            applicationName_, 
            networkName_,
            codePackageName_,
            codePackageVersion_,
            serviceManifestName_,
            servicePackageActivationId_,
            containerAddress_,
            containerId_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)        
            SERIALIZABLE_PROPERTY(Constants::NetworkName, networkName_)
            SERIALIZABLE_PROPERTY(Constants::CodePackageName, codePackageName_)
            SERIALIZABLE_PROPERTY(Constants::CodePackageVersion, codePackageVersion_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationId, servicePackageActivationId_)
            SERIALIZABLE_PROPERTY(Constants::ContainerAddress, containerAddress_)
            SERIALIZABLE_PROPERTY(Constants::ContainerId, containerId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationName_;
        std::wstring networkName_;
        std::wstring codePackageName_;
        std::wstring codePackageVersion_;
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        std::wstring containerAddress_;
        std::wstring containerId_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(DeployedNetworkCodePackageList, DeployedNetworkCodePackageQueryResult)
}
