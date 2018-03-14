// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class CreateServiceMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        CreateServiceMessageBody()
            : name_()
            , PartitionedServiceDescriptor_()
        {
        }

        CreateServiceMessageBody(
            Common::NamingUri const & name,
            PartitionedServiceDescriptor const & PartitionedServiceDescriptor)
            : name_(name)
            , PartitionedServiceDescriptor_(PartitionedServiceDescriptor)
        {
        }

        __declspec(property(get=get_NamingUri)) Common::NamingUri const & NamingUri;
        __declspec(property(get=get_PartitionedServiceDescriptor)) Naming::PartitionedServiceDescriptor const & PartitionedServiceDescriptor;

        Common::NamingUri const & get_NamingUri() const { return name_; }
        Naming::PartitionedServiceDescriptor const & get_PartitionedServiceDescriptor() const { return PartitionedServiceDescriptor_; }

        FABRIC_FIELDS_02(name_, PartitionedServiceDescriptor_);

    private:
        Common::NamingUri name_;
        Naming::PartitionedServiceDescriptor PartitionedServiceDescriptor_;
    };
}
