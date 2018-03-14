// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComTestServiceFactory : public IFabricStatelessServiceFactory, public IFabricStatefulServiceFactory, private Common::ComUnknownBase
    {
        DENY_COPY(ComTestServiceFactory);

        BEGIN_COM_INTERFACE_LIST(ComTestServiceFactory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory)
        END_COM_INTERFACE_LIST()

    public:
        ComTestServiceFactory(TestServiceFactory & factory) 
            : root_(factory.shared_from_this())
            , factory_(factory) 
        { 
        }

        virtual ~ComTestServiceFactory()
        {
        }

        HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_INSTANCE_ID instanceId,
            /* [retval][out] */ IFabricStatelessServiceInstance **service);

        HRESULT STDMETHODCALLTYPE CreateReplica( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [retval][out] */ IFabricStatefulServiceReplica **service);
        std::wstring GetInitString(const byte * initData, ULONG nBytes);
    private:
        Common::ComponentRootSPtr root_;
        TestServiceFactory & factory_;
    };
};
