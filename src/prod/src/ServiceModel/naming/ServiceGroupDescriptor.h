// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Naming
{
    class ServiceGroupDescriptor 
    {
    public:

        ServiceGroupDescriptor();

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SERVICE_GROUP_DESCRIPTION & serviceGroupDescription) const;

        static Common::ErrorCode Create(__in PartitionedServiceDescriptor const & serviceDescription, __out ServiceGroupDescriptor &);
        Common::ErrorCode GetServiceDescriptorForMember(__in Common::NamingUri const & name, __out PartitionedServiceDescriptor & memberServiceDescription);
        Naming::PartitionedServiceDescriptor get_PartitionedServiceDescriptor() { return serviceDescriptor_; }
        ServiceModel::CServiceGroupDescription get_CServiceGroupDescription() { return serviceGroupDescription_; }

    private:
        ServiceModel::CServiceGroupDescription serviceGroupDescription_;
        PartitionedServiceDescriptor serviceDescriptor_;
    };

}
