// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {8bb9ea65-089d-4f44-936e-828a70076c81}
    static const GUID CLSID_ComKeyValueStoreItemMetadataEnumerator = 
    {0x8bb9ea65,0x089d,0x4f44,{0x93,0x6e,0x82,0x8a,0x70,0x07,0x6c,0x81}};

    class ComKeyValueStoreItemMetadataEnumerator :
        public IFabricKeyValueStoreItemMetadataEnumerator2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComKeyValueStoreItemMetadataEnumerator)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreItemMetadataEnumerator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreItemMetadataEnumerator2)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemMetadataEnumerator, IFabricKeyValueStoreItemMetadataEnumerator)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemMetadataEnumerator2, IFabricKeyValueStoreItemMetadataEnumerator2)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreItemMetadataEnumerator, ComKeyValueStoreItemMetadataEnumerator)
        END_COM_INTERFACE_LIST()

    public:
        ComKeyValueStoreItemMetadataEnumerator(IKeyValueStoreItemMetadataEnumeratorPtr const & impl);
        virtual ~ComKeyValueStoreItemMetadataEnumerator();

        IKeyValueStoreItemMetadataEnumeratorPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricKeyValueStoreItemMetadataEnumerator methods
        // 
        HRESULT STDMETHODCALLTYPE MoveNext(void);
        
        // 
        // IFabricKeyValueStoreItemMetadataEnumerator2 methods
        // 
        HRESULT STDMETHODCALLTYPE TryMoveNext(BOOLEAN * success);
        
        IFabricKeyValueStoreItemMetadataResult *STDMETHODCALLTYPE get_Current( void);

    private:
        IKeyValueStoreItemMetadataEnumeratorPtr impl_;
    };
}
