//+---------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

namespace Management
{
    namespace LocalSecretService
    {
        class LocalSecretServiceFactory :
            public IFabricStatelessServiceFactory,
            private ComUnknownBase
        {
            DENY_COPY(LocalSecretServiceFactory)

                BEGIN_COM_INTERFACE_LIST(LocalSecretServiceFactory)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceFactory)
                COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)
                END_COM_INTERFACE_LIST()

        public:
            LocalSecretServiceFactory();

            ~LocalSecretServiceFactory();

        public:
            virtual HRESULT CreateInstance(
                __in LPCWSTR serviceTypeName,
                __in FABRIC_URI serviceName,
                __in ULONG initializationDataLength,
                __in_ecount(initializationDataLength) const byte* initializationData,
                __in FABRIC_PARTITION_ID partitionId,
                __in FABRIC_INSTANCE_ID instanceId,
                __out IFabricStatelessServiceInstance** serviceInstance
            ) override;
        };
    }
}