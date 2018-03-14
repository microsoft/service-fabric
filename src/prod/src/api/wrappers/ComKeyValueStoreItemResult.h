// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5b92e000-83a5-4f99-9c59-0647bd079b11}
    static const GUID CLSID_ComKeyValueStoreItemResult = 
    {0x5b92e000,0x83a5,0x4f99,{0x9c,0x59,0x06,0x47,0xbd,0x07,0x9b,0x11}};

    class ComKeyValueStoreItemResult :
        public IFabricKeyValueStoreItemResult,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComKeyValueStoreItemResult)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreItemResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreItemResult)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemResult, IFabricKeyValueStoreItemResult)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreItemResult, ComKeyValueStoreItemResult)
        END_COM_INTERFACE_LIST()

    public:
        ComKeyValueStoreItemResult(IKeyValueStoreItemResultPtr const & impl);
        virtual ~ComKeyValueStoreItemResult();

        IKeyValueStoreItemResultPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricKeyValueStoreItemResult methods
        // 
        const FABRIC_KEY_VALUE_STORE_ITEM *STDMETHODCALLTYPE get_Item( void);       

    private:
        IKeyValueStoreItemResultPtr impl_;
    };
}
