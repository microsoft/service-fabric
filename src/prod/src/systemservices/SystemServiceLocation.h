// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class SystemServiceLocation :
        public Common::TextTraceComponent<Common::TraceTaskCodes::SystemServices>
    {
        DEFAULT_COPY_CONSTRUCTOR(SystemServiceLocation)
        DEFAULT_COPY_ASSIGNMENT(SystemServiceLocation)
        
        static Common::GlobalWString TokenDelimiter;

    public:
        SystemServiceLocation();

        SystemServiceLocation(
            Federation::NodeInstance const &, 
            Common::Guid const & partitionId, 
            ::FABRIC_REPLICA_ID,
            __int64 replicaInstance);

        static Common::ErrorCode Create(
            Federation::NodeInstance const &, 
            Common::Guid const & partitionId, 
            ::FABRIC_REPLICA_ID,
            __int64 replicaInstance,
            std::wstring const & hostAddress,
            __out SystemServiceLocation &);

        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
        __declspec(property(get=get_PartitionId)) Common::Guid PartitionId;
        __declspec(property(get=get_ReplicaId)) ::FABRIC_REPLICA_ID ReplicaId;
        __declspec(property(get=get_ReplicaInstance)) __int64 ReplicaInstance;
        __declspec(property(get=get_HostAddress)) std::wstring const & HostAddress;
        __declspec(property(get=get_Location)) std::wstring const & Location;

        Federation::NodeInstance const & get_NodeInstance() const { return nodeInstance_; }
        Common::Guid get_PartitionId() const { return partitionId_; }
        ::FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }
        __int64 get_ReplicaInstance() const { return replicaInstance_; }
        std::wstring const & get_HostAddress() const { return hostAddress_; }
        std::wstring const & get_Location() const { return location_; }

        SystemServiceLocation & operator = (SystemServiceLocation &&);

        bool operator == (SystemServiceLocation const &);

        bool EqualsIgnoreInstances(SystemServiceLocation const &) const;

        SystemServiceFilterHeader CreateFilterHeader() const;

        static bool TryParse(
            std::wstring const & serviceLocation, 
            __out SystemServiceLocation &);

        static bool TryParse(
            std::wstring const & serviceLocation, 
            bool isFabSrvService,
            __out SystemServiceLocation &);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        const static FABRIC_REPLICA_ID ReplicaId_AnyReplica;
        const static __int64 ReplicaInstance_AnyReplica;

    private:
        SystemServiceLocation(
            Federation::NodeInstance const &, 
            Common::Guid const & partitionId, 
            ::FABRIC_REPLICA_ID,
            __int64 replicaInstance,
            std::wstring const & hostAddress);

        void InitializeLocation();

        Federation::NodeInstance nodeInstance_;
        Common::Guid partitionId_;
        ::FABRIC_REPLICA_ID replicaId_;
        __int64 replicaInstance_;
        std::wstring hostAddress_;

        std::wstring location_;
    };
}
