// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class SGServiceFactory :	
        public Common::ComponentRoot
    {
        DENY_COPY(SGServiceFactory);

    public:
        SGServiceFactory(__in Federation::NodeId nodeId);
        virtual ~SGServiceFactory();

        SGStatefulServiceSPtr CreateStatefulService(
            __in LPCWSTR serviceType,
            __in LPCWSTR serviceName,
            __in ULONG initializationDataLength,
            __in const byte* initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId);

        SGStatelessServiceSPtr CreateStatelessService(
            __in LPCWSTR serviceType,
            __in LPCWSTR serviceName,
            __in ULONG initializationDataLength,
            __in const byte* initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_INSTANCE_ID instanceId);

        static std::wstring const SGStatefulServiceGroupType;
        static std::wstring const SGStatelessServiceGroupType;

    private:
        Federation::NodeId nodeId_;
        
        static Common::StringLiteral const TraceSource;
    };
};    
