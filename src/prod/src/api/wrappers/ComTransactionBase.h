// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {9B833CE9-5DDE-461F-9674-ADEB1BE77F45}
    static const GUID CLSID_ComTransactionBase = 
        { 0x9b833ce9, 0x5dde, 0x461f, { 0x96, 0x74, 0xad, 0xeb, 0x1b, 0xe7, 0x7f, 0x45 } };

    class ComTransactionBase :
        public IFabricTransactionBase,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComTransactionBase)

        BEGIN_COM_INTERFACE_LIST(ComTransactionBase)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransactionBase)
            COM_INTERFACE_ITEM(IID_IFabricTransactionBase, IFabricTransactionBase)
            COM_INTERFACE_ITEM(CLSID_ComTransactionBase, ComTransactionBase)
        END_COM_INTERFACE_LIST()

    public:
        ComTransactionBase(ITransactionBasePtr const & impl);
        virtual ~ComTransactionBase();

        ITransactionBasePtr const & get_Impl() const { return impl_; }

        // 
        // IFabricTransactionBase methods
        // 
        const FABRIC_TRANSACTION_ID *STDMETHODCALLTYPE get_Id( void);
        
        FABRIC_TRANSACTION_ISOLATION_LEVEL STDMETHODCALLTYPE get_IsolationLevel( void);

    private:
        ITransactionBasePtr impl_;
        FABRIC_TRANSACTION_ID id_;
    };
}
