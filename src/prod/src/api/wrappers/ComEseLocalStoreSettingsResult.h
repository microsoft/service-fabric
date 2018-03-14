// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {836E6FE3-768F-4A04-916B-250ABE2FFCA8}
    static const GUID CLSID_ComEseLocalStoreSettingsResult = 
        {0x836E6FE3,0x768F,0x4A04,{0x91,0x6B,0x25,0x0A,0xBE,0x2F,0xFC,0xA8}};

    class ComEseLocalStoreSettingsResult :
        public IFabricEseLocalStoreSettingsResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComEseLocalStoreSettingsResult)

        BEGIN_COM_INTERFACE_LIST(ComEseLocalStoreSettingsResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricEseLocalStoreSettingsResult)
            COM_INTERFACE_ITEM(IID_IFabricEseLocalStoreSettingsResult, IFabricEseLocalStoreSettingsResult)
            COM_INTERFACE_ITEM(CLSID_ComEseLocalStoreSettingsResult, ComEseLocalStoreSettingsResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComEseLocalStoreSettingsResult(IEseLocalStoreSettingsResultPtr const & impl);
        virtual ~ComEseLocalStoreSettingsResult();

        const FABRIC_ESE_LOCAL_STORE_SETTINGS * STDMETHODCALLTYPE get_Settings();

    private:
        Common::ScopedHeap heap_;
        IEseLocalStoreSettingsResultPtr impl_;
    };
}

