// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceGroupDescriptionAdaptor : public PartitionedServiceDescWrapper
    {
        DENY_COPY(ServiceGroupDescriptionAdaptor)

    public:
        ServiceGroupDescriptionAdaptor() {}
        ~ServiceGroupDescriptionAdaptor() {}
        std::vector<ServiceGroupMemberDescriptionAdaptor> & getServiceGroupMemberDescriptions () {return serviceGroupMemberDescriptions_;}
        // This function converts the internal representation from ServiceGroupDescriptionAdaptor to PartitionedServiceDescWrapper
        // The ServiceGroup utilizes the initializationData field to store the extra fields for ServiceGroup
        // Cast to PartitionedServiceDescWrapper after calling this function
        // ie:
        // serviceGroupDescriptionAdaptor.ToPartitionedServiceDescWrapper();
        // PartitionedServiceDescWrapper &psd = serviceGroupDescriptionAdaptor;
        HRESULT ToPartitionedServiceDescWrapper();
        HRESULT FromPartitionedServiceDescWrapper(CServiceGroupDescription &cServiceGroupDescription);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceGroupMemberDescription, serviceGroupMemberDescriptions_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::vector<ServiceGroupMemberDescriptionAdaptor> serviceGroupMemberDescriptions_;

        HRESULT MemberDescriptionToPartitionedServiceDescWrapper(__in CServiceGroupMemberDescription &cServiceGroupMemberDescription, size_t index);
    };
}
