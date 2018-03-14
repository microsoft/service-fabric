// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceCheckReplyBody : public Serialization::FabricSerializable
    {
    public:

        ServiceCheckReplyBody()
            : serviceExists_(false)
            , description_()
            , storeVersion_(ServiceModel::Constants::UninitializedVersion)
        {
        }

        explicit ServiceCheckReplyBody(_int64 storeVersion)
            : serviceExists_(false)
            , description_()
            , storeVersion_(storeVersion)
        {
        }

        explicit ServiceCheckReplyBody(PartitionedServiceDescriptor && description)
            : serviceExists_(true)
            , description_(std::move(description))
            , storeVersion_(description_.Version)
        {
        }

        explicit ServiceCheckReplyBody(PartitionedServiceDescriptor const & description)
            : serviceExists_(true)
            , description_(description)
            , storeVersion_(description_.Version)
        {
        }

        __declspec(property(get=get_DoesExist)) bool DoesExist;
        bool get_DoesExist() const { return serviceExists_; }
                
        __declspec(property(get=get_StoreVersion)) __int64 StoreVersion;
        __int64 get_StoreVersion() const { return storeVersion_; }

        __declspec(property(get=get_PartitionedServiceDescriptor)) Naming::PartitionedServiceDescriptor const & PartitionedServiceDescriptor;
        Naming::PartitionedServiceDescriptor const & get_PartitionedServiceDescriptor() const { return description_; }

        Naming::PartitionedServiceDescriptor && TakePsd() { return std::move(description_); }

        FABRIC_FIELDS_03(serviceExists_, description_, storeVersion_);

    private:
        bool serviceExists_;
        Naming::PartitionedServiceDescriptor description_;
        __int64 storeVersion_;
    };
}
