// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTableEntry 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
    public:
        ServiceTableEntry();

        explicit ServiceTableEntry(ConsistencyUnitId consistencyUnitId);

        ServiceTableEntry(
            ConsistencyUnitId consistencyUnitId,
            std::wstring const& serviceName,
            ServiceReplicaSet && serviceReplicaSet,
            bool isFound = true);

        ServiceTableEntry(ServiceTableEntry const & other);
        ServiceTableEntry & operator=(ServiceTableEntry const & other);

        ServiceTableEntry(ServiceTableEntry && other);
        ServiceTableEntry & operator=(ServiceTableEntry && other);

        __declspec(property(get=get_ConsistencyUnitId)) Reliability::ConsistencyUnitId ConsistencyUnitId;
        Reliability::ConsistencyUnitId get_ConsistencyUnitId() const { return consistencyUnitId_; }

        __declspec(property(get=get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_ServiceReplicaSet)) Reliability::ServiceReplicaSet const& ServiceReplicaSet;
        Reliability::ServiceReplicaSet const& get_ServiceReplicaSet() const { return serviceReplicaSet_; }

        __declspec(property(get=get_Version)) int64 Version;
        int64 get_Version() const { return serviceReplicaSet_.LookupVersion; }

        __declspec(property(get=get_IsFound)) bool IsFound;
        bool get_IsFound() const { return isFound_; }

        void EnsureEmpty(int64 version);

        bool operator == (ServiceTableEntry const &) const;
        bool operator != (ServiceTableEntry const &) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        void WriteToEtw(uint16 contextSequenceId) const;

        std::wstring ToString() const;

        FABRIC_FIELDS_04(consistencyUnitId_, serviceName_, serviceReplicaSet_, isFound_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceReplicaSet_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(consistencyUnitId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Reliability::ConsistencyUnitId consistencyUnitId_;
        std::wstring serviceName_;
        Reliability::ServiceReplicaSet serviceReplicaSet_;
        bool isFound_;
    };

    typedef std::shared_ptr<ServiceTableEntry> ServiceTableEntrySPtr;
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ServiceTableEntry);
DEFINE_USER_MAP_UTILITY(int64, Reliability::ServiceTableEntrySPtr);
