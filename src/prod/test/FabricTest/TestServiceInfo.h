// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    struct TestServiceInfo: public Serialization::FabricSerializable
    {
    public:
        TestServiceInfo(
            std::wstring applicationId,
            std::wstring serviceType,
            std::wstring serviceName,
            std::wstring partitionId,
            std::wstring serviceLocation,
            bool isStateful,
            FABRIC_REPLICA_ROLE replicaRole)
            : applicationId_(applicationId),
            serviceType_(serviceType),
            serviceName_(serviceName),
            partitionId_(partitionId),
            serviceLocation_(serviceLocation),
            isStateful_(isStateful),
            replicaRole_(replicaRole)
        {
        }

        TestServiceInfo()
        {
        }

        __declspec (property(get=getApplicationId))std::wstring const & ApplicationId;
        __declspec (property(get=getServiceName))std::wstring const & ServiceName;
        __declspec (property(get=getServiceType))std::wstring const & ServiceType;
        __declspec (property(get=getPartitionId))std::wstring const & PartitionId;
        __declspec (property(get=getServiceLocation, put=putServiceLocation))std::wstring const & ServiceLocation;
        __declspec (property(get=getIsStateful))bool IsStateful;
        __declspec (property(get=getReplicaRole, put=putReplicaRole))FABRIC_REPLICA_ROLE StatefulServiceRole;

        std::wstring const & getApplicationId() const { return applicationId_; }
        std::wstring const & getServiceName() const { return serviceName_; }
        std::wstring const & getServiceType() const { return serviceType_; }
        std::wstring const & getPartitionId() const { return partitionId_; }
        std::wstring const & getServiceLocation() const { return serviceLocation_; }
        bool getIsStateful() const { return isStateful_; }
        FABRIC_REPLICA_ROLE getReplicaRole() const { return replicaRole_; }
        void putReplicaRole(FABRIC_REPLICA_ROLE replicaRole) { replicaRole_ = replicaRole; }
        void putServiceLocation(std::wstring const & serviceLocation) { serviceLocation_ = serviceLocation; }

        bool operator == ( const TestServiceInfo& other ) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_07(applicationId_, serviceType_, serviceName_, partitionId_, serviceLocation_, isStateful_, replicaRole_);

    private:
        std::wstring applicationId_;
        std::wstring serviceType_; 
        std::wstring serviceName_;
        std::wstring partitionId_; 
        std::wstring serviceLocation_;
        bool isStateful_;
        FABRIC_REPLICA_ROLE replicaRole_;
    };
};

DEFINE_USER_ARRAY_UTILITY(FabricTest::TestServiceInfo);
