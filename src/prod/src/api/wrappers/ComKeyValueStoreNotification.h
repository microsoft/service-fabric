// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {ad8dc553-d57d-43f8-8b1c-83d4738825f5}
    static const GUID CLSID_ComKeyValueStoreNotification = 
    {0xad8dc553,0xd57d,0x43f8,{0x8b,0x1c,0x83,0xd4,0x73,0x88,0x25,0xf5}};

    class ComKeyValueStoreNotification :
        public IFabricKeyValueStoreNotification,
        private ComKeyValueStoreItemResult
    {
        DENY_COPY(ComKeyValueStoreNotification)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreNotification)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreNotification)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreNotification, IFabricKeyValueStoreNotification)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreNotification, ComKeyValueStoreNotification)
            COM_DELEGATE_TO_BASE_ITEM(ComKeyValueStoreItemResult)
        END_COM_INTERFACE_LIST()

    public:
        ComKeyValueStoreNotification(IStoreItemPtr const & impl);
        virtual ~ComKeyValueStoreNotification();

        IStoreItemPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricKeyValueStoreNotification methods
        // 
        const FABRIC_KEY_VALUE_STORE_ITEM *STDMETHODCALLTYPE get_Item( void);       

        BOOLEAN STDMETHODCALLTYPE IsDelete();

    private:
        IStoreItemPtr impl_;
    };
}

