// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


using namespace std;

namespace FabricTest
{
    class SGComServiceFactory :	
        public IFabricStatefulServiceFactory, 
        public IFabricStatelessServiceFactory, 
        public Common::ComUnknownBase
    {
        DENY_COPY(SGComServiceFactory);

        BEGIN_COM_INTERFACE_LIST(SGComServiceFactory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)
        END_COM_INTERFACE_LIST()

    public:
        //
        // Constructor/Destructor.
        //
        SGComServiceFactory(SGServiceFactory & factory);
        virtual ~SGComServiceFactory();
        //
        // IFabricStatefulServiceFactory methods. 
        //
        virtual HRESULT STDMETHODCALLTYPE CreateReplica(
            __in LPCWSTR serviceType,
            __in LPCWSTR serviceName,
            __in ULONG initializationDataLength,
            __in const byte* initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __out IFabricStatefulServiceReplica** serviceReplica);

        //
        // IFabricStatelessServiceFactory methods. 
        //
        virtual HRESULT STDMETHODCALLTYPE CreateInstance(
            __in LPCWSTR serviceType,
            __in LPCWSTR serviceName,
            __in ULONG initializationDataLength,
            __in const byte * initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_INSTANCE_ID instanceId,
            __out IFabricStatelessServiceInstance ** serviceInstance);

    private:
        Common::ComponentRootSPtr root_;
        SGServiceFactory & factory_;

        static Common::StringLiteral const TraceSource;
    };
};
