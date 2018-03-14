// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class CreateApplicationServiceMessageBody : public Serialization::FabricSerializable
        {
        public:
            CreateApplicationServiceMessageBody()
            {
            }

            CreateApplicationServiceMessageBody(
                Common::NamingUri const & applicationName,
                Naming::PartitionedServiceDescriptor const & serviceDescription)
                : applicationName_(applicationName),
                serviceDescription_(serviceDescription)
            {
            }

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;

            Naming::PartitionedServiceDescriptor const & get_ServiceDescription() const { return serviceDescription_; }
            __declspec(property(get=get_ServiceDescription)) Naming::PartitionedServiceDescriptor const & ServiceDescription;

            FABRIC_FIELDS_02(applicationName_, serviceDescription_);

        private:
            Common::NamingUri applicationName_;
            Naming::PartitionedServiceDescriptor serviceDescription_;
        };
    }
}
