// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Used for creating or updating scaleout parameters of an application.
    class CreateApplicationRequestMessageBody : public Serialization::FabricSerializable
    {
    public:
        CreateApplicationRequestMessageBody() {}

        CreateApplicationRequestMessageBody(
            Common::NamingUri const & appName,
            ServiceModel::ApplicationIdentifier const & appId,
            uint64 instanceId,
            ApplicationCapacityDescription const & appCapacity,
            ServiceModel::ServicePackageResourceGovernanceMap const& spRGDescription,
            ServiceModel::CodePackageContainersImagesMap const& cpContainersImages)
            : applicationName_(appName)
            , applicationId_(appId)
            , instanceId_(instanceId)
            , capacityDescription_(appCapacity)
            , resourceGovernanceDescription_(spRGDescription)
            , codePackageContainersImages_(cpContainersImages)
            , networks_()
        {}

        CreateApplicationRequestMessageBody(
            Common::NamingUri const & appName,
            ServiceModel::ApplicationIdentifier const & appId,
            uint64 instanceId,
            ApplicationCapacityDescription const & appCapacity,
            ServiceModel::ServicePackageResourceGovernanceMap const& spRGDescription,
            ServiceModel::CodePackageContainersImagesMap const& cpContainersImages,
            std::vector<std::wstring> const & networks)
            : applicationName_(appName)
            , applicationId_(appId)
            , instanceId_(instanceId)
            , capacityDescription_(appCapacity)
            , resourceGovernanceDescription_(spRGDescription)
            , codePackageContainersImages_(cpContainersImages)
            , networks_(networks)
        {}

        __declspec(property(get = get_ApplicationName, put = put_ApplicationName)) Common::NamingUri & ApplicationName;
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
        void put_ApplicationName(Common::NamingUri const & applicationName) { applicationName_ = applicationName; }

        __declspec(property(get = get_ApplicationId, put = put_ApplicationId)) ServiceModel::ApplicationIdentifier & ApplicationId;
        ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return applicationId_; }
        void put_ApplicationId(ServiceModel::ApplicationIdentifier const & applicationId) { applicationId_ = applicationId; }

        __declspec(property(get = get_InstanceId, put = put_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }
        void put_InstanceId(uint64 instanceId) { instanceId_ = instanceId; }

        __declspec(property(get = get_ApplicationCapacity, put = put_ApplicationCapacity)) ApplicationCapacityDescription const & ApplicationCapacity;
        ApplicationCapacityDescription const & get_ApplicationCapacity() const { return capacityDescription_; }
        void put_ApplicationCapacity(ApplicationCapacityDescription const& appCapacity) { capacityDescription_ = appCapacity; }

        __declspec(property (get = get_ResourceGovernanceDescription, put = put_ResourceGovernanceDescription))
            ServiceModel::ServicePackageResourceGovernanceMap ResourceGovernanceDescription;
        ServiceModel::ServicePackageResourceGovernanceMap const& get_ResourceGovernanceDescription() const { return  resourceGovernanceDescription_; }
        void put_ResourceGovernanceDescription(ServiceModel::ServicePackageResourceGovernanceMap const& desc) { resourceGovernanceDescription_ = desc; }

        __declspec(property (get = get_CodePackageContainersImages, put = put_CodePackageContainersImages))
            ServiceModel::CodePackageContainersImagesMap CodePackageContainersImages;
        ServiceModel::CodePackageContainersImagesMap const& get_CodePackageContainersImages() const { return  codePackageContainersImages_; }
        void put_CodePackageContainersImages(ServiceModel::CodePackageContainersImagesMap const& desc) { codePackageContainersImages_ = desc; }

        __declspec(property(get = get_Networks, put = put_Networks)) std::vector<std::wstring> const & Networks;
        std::vector<std::wstring> const & get_Networks() const { return networks_; }
        void put_Networks(std::vector<std::wstring> const & networks) { networks_ = networks; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("Name={0} InstanceId={1} Capacity={2} NetworkCount={3}",
                applicationName_,
                instanceId_,
                capacityDescription_,
                networks_.size());
        }

        FABRIC_FIELDS_07(
            applicationName_,
            applicationId_,
            instanceId_,
            capacityDescription_,
            resourceGovernanceDescription_,
            codePackageContainersImages_,
            networks_);

    private:
        Common::NamingUri applicationName_;
        ServiceModel::ApplicationIdentifier applicationId_;
        uint64 instanceId_;
        ApplicationCapacityDescription capacityDescription_;        
        ServiceModel::ServicePackageResourceGovernanceMap resourceGovernanceDescription_;
        ServiceModel::CodePackageContainersImagesMap codePackageContainersImages_;
        std::vector<std::wstring> networks_;
    };
}