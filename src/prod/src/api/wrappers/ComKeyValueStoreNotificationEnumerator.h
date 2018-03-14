// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {9ae7061c-a487-46af-b1ef-4511a9913808}
    static const GUID CLSID_ComKeyValueStoreNotificationEnumerator = 
    {0x9ae7061c,0xa487,0x46af,{0xb1,0xef,0x45,0x11,0xa9,0x91,0x38,0x08}};

    class ComKeyValueStoreNotificationEnumerator :
        public IFabricKeyValueStoreNotificationEnumerator2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComKeyValueStoreNotificationEnumerator)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreNotificationEnumerator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricKeyValueStoreNotificationEnumerator2)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreNotificationEnumerator, IFabricKeyValueStoreNotificationEnumerator)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreNotificationEnumerator2, IFabricKeyValueStoreNotificationEnumerator2)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreNotificationEnumerator, ComKeyValueStoreNotificationEnumerator)
        END_COM_INTERFACE_LIST()

    public:
        ComKeyValueStoreNotificationEnumerator(IStoreNotificationEnumeratorPtr const & impl);
        virtual ~ComKeyValueStoreNotificationEnumerator();

        IStoreNotificationEnumeratorPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricKeyValueStoreNotificationEnumerator methods
        // 
        HRESULT STDMETHODCALLTYPE MoveNext(void);
        
        IFabricKeyValueStoreNotification *STDMETHODCALLTYPE get_Current(void);

        void STDMETHODCALLTYPE Reset(void);

        // 
        // IFabricKeyValueStoreNotificationEnumerator2 methods
        // 
        HRESULT STDMETHODCALLTYPE TryMoveNext(BOOLEAN * success);
        
    private:
        IStoreNotificationEnumeratorPtr impl_;
    };
}
