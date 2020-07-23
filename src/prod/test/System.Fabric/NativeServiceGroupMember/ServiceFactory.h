// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace NativeServiceGroupMember
{
    class ServiceFactory : 
        public IFabricStatelessServiceFactory,
        public IFabricStatefulServiceFactory,
        private ComUnknownBase
    {
        DENY_COPY(ServiceFactory)

        BEGIN_COM_INTERFACE_LIST(ServiceFactory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory)
        END_COM_INTERFACE_LIST()

    public:

        ServiceFactory();
        ~ServiceFactory();

        STDMETHODIMP CreateInstance( 
            __in LPCWSTR serviceType,
            __in FABRIC_URI serviceName,
            __in ULONG initializationDataLength,
            __in const byte *initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_INSTANCE_ID instanceId,
            __out IFabricStatelessServiceInstance **serviceInstance);

        STDMETHODIMP CreateReplica( 
            __in LPCWSTR serviceType,
            __in FABRIC_URI serviceName,
            __in ULONG initializationDataLength,
            __in const byte *initializationData,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __out IFabricStatefulServiceReplica **serviceReplica);

    private:

        template <class ComImplementation>
        friend Common::ComPointer<ComImplementation> Common::make_com();
    };
}
