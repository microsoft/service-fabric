// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {7aa04f3a-bfc2-4733-b891-485811bffdd4}
    static const GUID CLSID_ComStoreItemEnumerator = 
        {0x7aa04f3a,0xbfc2,0x4733,{0xb8,0x91,0x48,0x58,0x11,0xbf,0xfd,0xd4}};

    class ComStoreItemEnumerator :
        public IFabricKeyValueStoreItemEnumerator2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComStoreItemEnumerator)

        BEGIN_COM_INTERFACE_LIST(ComStoreItemEnumerator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreItemEnumerator2)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemEnumerator, IFabricKeyValueStoreItemEnumerator)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemEnumerator2, IFabricKeyValueStoreItemEnumerator2)
            COM_INTERFACE_ITEM(CLSID_ComStoreItemEnumerator, ComStoreItemEnumerator)
        END_COM_INTERFACE_LIST()

    public:
        ComStoreItemEnumerator(IStoreItemEnumeratorPtr const & impl);
        virtual ~ComStoreItemEnumerator();

        IStoreItemEnumeratorPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricKeyValueStoreItemEnumerator methods
        // 
        HRESULT STDMETHODCALLTYPE MoveNext(void);
        
        IFabricKeyValueStoreItemResult *STDMETHODCALLTYPE get_Current(void);

        // 
        // IFabricKeyValueStoreItemEnumerator2 methods
        // 
        HRESULT STDMETHODCALLTYPE TryMoveNext(BOOLEAN * success);
        
    private:
        IStoreItemEnumeratorPtr impl_;
    };
}

