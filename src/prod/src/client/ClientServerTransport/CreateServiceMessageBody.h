// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class CreateServiceMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            CreateServiceMessageBody()
            {
            }

            explicit CreateServiceMessageBody(
                Naming::PartitionedServiceDescriptor const & serviceDescription)
                : serviceDescription_(serviceDescription)
            {
            }

            __declspec(property(get=get_ServiceDescription)) Naming::PartitionedServiceDescriptor & ServiceDescription;
            Naming::PartitionedServiceDescriptor & get_ServiceDescription() { return serviceDescription_; }

            FABRIC_FIELDS_01(serviceDescription_);

        private:
            Naming::PartitionedServiceDescriptor serviceDescription_;
        };
    }
}
