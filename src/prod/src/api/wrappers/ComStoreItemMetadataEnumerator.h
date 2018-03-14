// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {a258c382-3c09-4523-a848-df18b86a9c1b}
    static const GUID CLSID_ComStoreItemMetadataEnumerator = 
        {0xa258c382,0x3c09,0x4523,{0xa8,0x48,0xdf,0x18,0xb8,0x6a,0x9c,0x1b}};

    class ComStoreItemMetadataEnumerator :
        public IFabricKeyValueStoreItemMetadataEnumerator2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComStoreItemMetadataEnumerator)

        BEGIN_COM_INTERFACE_LIST(ComStoreItemMetadataEnumerator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreItemMetadataEnumerator2)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemMetadataEnumerator, IFabricKeyValueStoreItemMetadataEnumerator)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemMetadataEnumerator2, IFabricKeyValueStoreItemMetadataEnumerator2)
            COM_INTERFACE_ITEM(CLSID_ComStoreItemMetadataEnumerator, ComStoreItemMetadataEnumerator)
        END_COM_INTERFACE_LIST()

    public:
        ComStoreItemMetadataEnumerator(IStoreItemMetadataEnumeratorPtr const & impl);
        virtual ~ComStoreItemMetadataEnumerator();

        IStoreItemMetadataEnumeratorPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricKeyValueStoreItemMetadataEnumerator methods
        // 
        HRESULT STDMETHODCALLTYPE MoveNext(void);
        
        IFabricKeyValueStoreItemMetadataResult *STDMETHODCALLTYPE get_Current(void);

        // 
        // IFabricKeyValueStoreItemMetadataEnumerator2 methods
        // 
        HRESULT STDMETHODCALLTYPE TryMoveNext(BOOLEAN * success);
        
    private:
        IStoreItemMetadataEnumeratorPtr impl_;
    };
}

