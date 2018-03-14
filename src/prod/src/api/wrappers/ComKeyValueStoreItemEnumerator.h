// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {0579db29-1330-4112-a4a2-683d0d99e993}
    static const GUID CLSID_ComKeyValueStoreItemEnumerator = 
    {0x0579db29,0x1330,0x4112,{0xa4,0xa2,0x68,0x3d,0x0d,0x99,0xe9,0x93}};

    class ComKeyValueStoreItemEnumerator :
        public IFabricKeyValueStoreItemEnumerator2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComKeyValueStoreItemEnumerator)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreItemEnumerator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreItemEnumerator2)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemEnumerator, IFabricKeyValueStoreItemEnumerator)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemEnumerator2, IFabricKeyValueStoreItemEnumerator2)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreItemEnumerator, ComKeyValueStoreItemEnumerator)
        END_COM_INTERFACE_LIST()

    public:
        ComKeyValueStoreItemEnumerator(IKeyValueStoreItemEnumeratorPtr const & impl);
        virtual ~ComKeyValueStoreItemEnumerator();

        IKeyValueStoreItemEnumeratorPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricKeyValueStoreItemEnumerator methods
        // 
        HRESULT STDMETHODCALLTYPE MoveNext(void);
        
        // 
        // IFabricKeyValueStoreItemEnumerator2 methods
        // 
        HRESULT STDMETHODCALLTYPE TryMoveNext(BOOLEAN * success);
        
        IFabricKeyValueStoreItemResult *STDMETHODCALLTYPE get_Current( void);

    private:
        IKeyValueStoreItemEnumeratorPtr impl_;
    };
}
