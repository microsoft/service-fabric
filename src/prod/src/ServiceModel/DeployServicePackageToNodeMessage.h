// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployServicePackageToNodeMessage
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        struct PackageSharingPair : public Serialization::FabricSerializable, public Common::IFabricJsonSerializable
        {
            std::wstring SharedPackageName;
            FABRIC_PACKAGE_SHARING_POLICY_SCOPE PackageSharingScope;

            FABRIC_FIELDS_02(SharedPackageName, PackageSharingScope)
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::SharedPackageName, SharedPackageName)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::PackageSharingScope, PackageSharingScope)
            END_JSON_SERIALIZABLE_PROPERTIES()
        };

    public:
        DeployServicePackageToNodeMessage(); 
        DeployServicePackageToNodeMessage(
            std::wstring const & ServiceManifest,
            std::wstring const & applicationTypeName,
            std::wstring const & applicationTypeVersion,
            std::wstring const & nodeName,
            std::vector<PackageSharingPair> const & packageSharingPolicies);

        DeployServicePackageToNodeMessage(DeployServicePackageToNodeMessage && other);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        __declspec(property(get = get_ServiceManifestName))  std::wstring const  & ServiceManifestName;
        std::wstring const& get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ApplicationTypeName))  std::wstring const  & ApplicationTypeName;
        std::wstring const& get_ApplicationTypeName() const { return applicationTypeName_; }

        __declspec(property(get = get_ApplicationTypeVersion))  std::wstring const  & ApplicationTypeVersion;
        std::wstring const& get_ApplicationTypeVersion() const { return applicationTypeVersion_; }

        __declspec(property(get = get_NodeName))  std::wstring const  & NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }

        __declspec(property(get = get_PackageSharingPolicies))  std::vector<PackageSharingPair> const  & PackageSharingPolicies;
        std::vector<PackageSharingPair> const& get_PackageSharingPolicies() const { return packageSharingPolicies_; }

        DeployServicePackageToNodeMessage const & operator= (DeployServicePackageToNodeMessage && other);

        Common::ErrorCode GetSharingPoliciesFromRest(__out std::wstring & result);

        FABRIC_FIELDS_05(serviceManifestName_, applicationTypeName_, applicationTypeVersion_, nodeName_, packageSharingPolicies_)

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeName, applicationTypeName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeVersion, applicationTypeVersion_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PackageSharingPolicy, packageSharingPolicies_)
        END_JSON_SERIALIZABLE_PROPERTIES()     


    private:
        std::wstring serviceManifestName_;
        std::wstring applicationTypeName_;
        std::wstring applicationTypeVersion_;
        std::wstring nodeName_;
        std::vector<PackageSharingPair> packageSharingPolicies_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::DeployServicePackageToNodeMessage::PackageSharingPair);
