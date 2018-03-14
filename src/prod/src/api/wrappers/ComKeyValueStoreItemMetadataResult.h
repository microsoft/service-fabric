// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {24e5b0e4-4915-4397-aa11-d00fb42196f8}
    static const GUID CLSID_ComKeyValueStoreItemMetadataResult = 
    {0x24e5b0e4,0x4915,0x4397,{0xaa,0x11,0xd0,0x0f,0xb4,0x21,0x96,0xf8}};

    class ComKeyValueStoreItemMetadataResult :
        public IFabricKeyValueStoreItemMetadataResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComKeyValueStoreItemMetadataResult)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreItemMetadataResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreItemMetadataResult)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreItemMetadataResult, IFabricKeyValueStoreItemMetadataResult)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreItemMetadataResult, ComKeyValueStoreItemMetadataResult)
        END_COM_INTERFACE_LIST()

    public:
        ComKeyValueStoreItemMetadataResult(IKeyValueStoreItemMetadataResultPtr const & impl);
        virtual ~ComKeyValueStoreItemMetadataResult();

        IKeyValueStoreItemMetadataResultPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricKeyValueStoreItemMetadataResult methods
        // 
        const FABRIC_KEY_VALUE_STORE_ITEM_METADATA *STDMETHODCALLTYPE get_Metadata( void);

    private:
        IKeyValueStoreItemMetadataResultPtr impl_;
    };
}
