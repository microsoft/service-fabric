// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class SystemServiceFilterHeader : 
        public Transport::MessageHeader<Transport::MessageHeaderId::SystemServiceFilter>, 
        public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(SystemServiceFilterHeader)
    
    public:
        SystemServiceFilterHeader() 
            : partitionId_(Common::Guid::Empty())
            , replicaId_(0)
            , replicaInstance_(0)
        {
        }

        SystemServiceFilterHeader(SystemServiceFilterHeader && other) 
            : partitionId_(std::move(other.partitionId_))
            , replicaId_(std::move(other.replicaId_))
            , replicaInstance_(std::move(other.replicaInstance_))
        {
        }

        SystemServiceFilterHeader(SystemServiceFilterHeader const & other) 
            : partitionId_(other.partitionId_)
            , replicaId_(other.replicaId_)
            , replicaInstance_(other.replicaInstance_)
        {
        }

        SystemServiceFilterHeader(
            Common::Guid const & partitionId, 
            ::FABRIC_REPLICA_ID replicaId,
            __int64 replicaInstance) 
            : partitionId_(partitionId) 
            , replicaId_(replicaId) 
            , replicaInstance_(replicaInstance)
        { 
        }

        __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
        Common::Guid const & get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID ReplicaId;
        ::FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

        __declspec(property(get=get_ReplicaInstance)) __int64 ReplicaInstance;
        _int64 get_ReplicaInstance() const { return replicaInstance_; }

        bool operator < (SystemServiceFilterHeader const & other);

        std::wstring ToString()
        {
            return Common::wformatString("{0}", *this);
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("{0}:{1}({2})", partitionId_, replicaId_, replicaInstance_);
        }

        FABRIC_FIELDS_03(partitionId_, replicaId_, replicaInstance_);

    private:
        Common::Guid partitionId_;
        ::FABRIC_REPLICA_ID replicaId_;
        __int64 replicaInstance_;
    };
}
