// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {a7da3b8c-3bd5-429b-a0cc-d38f04429e3e}
    static const GUID CLSID_ComKeyValueStoreEnumerator = 
        {0xa7da3b8c,0x3bd5,0x429b,{0xa0,0xcc,0xd3,0x8f,0x04,0x42,0x9e,0x3e}};

    class ComKeyValueStoreEnumerator : 
        public IFabricKeyValueStoreEnumerator2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComKeyValueStoreEnumerator)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreEnumerator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreEnumerator2)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreEnumerator, IFabricKeyValueStoreEnumerator)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreEnumerator2, IFabricKeyValueStoreEnumerator2)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreEnumerator, ComKeyValueStoreEnumerator)
        END_COM_INTERFACE_LIST()

    public:
        ComKeyValueStoreEnumerator(IStoreEnumeratorPtr const & impl);
        virtual ~ComKeyValueStoreEnumerator();

        IStoreEnumeratorPtr const & get_Impl() const { return impl_; }

        //
        // IFabricKeyValueStoreEnumerator methods
        // 
        
        HRESULT STDMETHODCALLTYPE EnumerateByKey( 
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT STDMETHODCALLTYPE EnumerateMetadataByKey( 
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);

        //
        // IFabricKeyValueStoreEnumerator2 methods
        // 
        
        HRESULT STDMETHODCALLTYPE EnumerateByKey2( 
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT STDMETHODCALLTYPE EnumerateMetadataByKey2( 
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);

    private:
        IStoreEnumeratorPtr impl_;
    };
}

