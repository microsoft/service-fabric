// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {aa8e60e5-95f8-46b5-9f8a-8f4ff273d410}
    static const GUID CLSID_ComStatefulServiceFactory = 
        { 0xaa8e60e5, 0x95f8, 0x46b5, { 0x9f, 0x8a, 0x8f, 0x4f, 0xf2, 0x73, 0xd4, 0x10 } };

    class ComStatefulServiceFactory :
        public IFabricStatefulServiceFactory,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComStatefulServiceFactory)

        BEGIN_COM_INTERFACE_LIST(ComStatefulServiceFactory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceFactory, IFabricStatefulServiceFactory)
            COM_INTERFACE_ITEM(CLSID_ComStatefulServiceFactory, ComStatefulServiceFactory)
        END_COM_INTERFACE_LIST()

    public:
        ComStatefulServiceFactory(IStatefulServiceFactoryPtr const & impl);
        virtual ~ComStatefulServiceFactory();

        inline IStatefulServiceFactoryPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricStatefulServiceFactory methods
        // 
        HRESULT STDMETHODCALLTYPE CreateReplica( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [retval][out] */ IFabricStatefulServiceReplica **serviceReplica);

    private:
        IStatefulServiceFactoryPtr impl_;
    };
}
