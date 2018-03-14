// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {c99a8e72-9711-4460-b4e9-b51715178dbc}
    static const GUID CLSID_ComStoreEventHandler = 
    {0xc99a8e72,0x9711,0x4460,{0xb4,0xe9,0xb5,0x17,0x15,0x17,0x8d,0xbc}};

    class ComStoreEventHandler :
        public IFabricStoreEventHandler,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComStoreEventHandler)

        BEGIN_COM_INTERFACE_LIST(ComStoreEventHandler)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStoreEventHandler)
            COM_INTERFACE_ITEM(IID_IFabricStoreEventHandler, IFabricStoreEventHandler)
            COM_INTERFACE_ITEM(CLSID_ComStoreEventHandler, ComStoreEventHandler)
        END_COM_INTERFACE_LIST()

    public:
        ComStoreEventHandler(IStoreEventHandlerPtr const & impl);
        virtual ~ComStoreEventHandler();

        IStoreEventHandlerPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricStoreEventHandler methods
        // 
        void STDMETHODCALLTYPE OnDataLoss( void);

    private:
        IStoreEventHandlerPtr impl_;
    };
}
