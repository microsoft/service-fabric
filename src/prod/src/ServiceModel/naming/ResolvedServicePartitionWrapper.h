// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ResolvedServicePartitionWrapper
        : public Common::IFabricJsonSerializable
    {
        DENY_COPY(ResolvedServicePartitionWrapper)

    public:
        ResolvedServicePartitionWrapper() {}

        ResolvedServicePartitionWrapper(
            std::wstring &&serviceName_,
            ServiceModel::ServicePartitionInformation &&partitionInfo,
            std::vector<ServiceEndpointInformationWrapper> &&endpoints,
            std::wstring &&version);

        ResolvedServicePartitionWrapper const& operator = (ResolvedServicePartitionWrapper &&other);

        __declspec(property(get=get_Version)) std::wstring &Version;
        std::wstring & get_Version() { return version_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Name, serviceName_);
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionInformation, partitionInfo_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Endpoints, endpoints_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Version, version_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring serviceName_;
        ServiceModel::ServicePartitionInformation partitionInfo_;
        std::vector<ServiceEndpointInformationWrapper> endpoints_;
        std::wstring version_;
    };
}
