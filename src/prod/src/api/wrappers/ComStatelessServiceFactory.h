// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {C54303A6-1EA8-40BA-B189-B92B11387B4E}
    static const GUID CLSID_ComStatelessServiceFactory = 
        { 0xc54303a6, 0x1ea8, 0x40ba, { 0xb1, 0x89, 0xb9, 0x2b, 0x11, 0x38, 0x7b, 0x4e } };

    class ComStatelessServiceFactory :
        public IFabricStatelessServiceFactory,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComStatelessServiceFactory)

        BEGIN_COM_INTERFACE_LIST(ComStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceFactory, IFabricStatelessServiceFactory)
            COM_INTERFACE_ITEM(CLSID_ComStatelessServiceFactory, ComStatelessServiceFactory)
        END_COM_INTERFACE_LIST()

    public:
        ComStatelessServiceFactory(IStatelessServiceFactoryPtr const & impl);
        virtual ~ComStatelessServiceFactory();

        inline IStatelessServiceFactoryPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricStatelessServiceFactory methods
        // 
        HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_INSTANCE_ID instanceId,
            /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance);

    private:
        IStatelessServiceFactoryPtr impl_;
    };
}
