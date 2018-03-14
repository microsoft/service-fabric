// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {F21C2A5F-2509-47FA-9864-B3188C882501}
    static const GUID CLSID_ComClientSettingsResult = 
        {0xf21c2a5f,0x2509,0x47fa,{0x98,0x64,0xb3,0x18,0x8c,0x88,0x25,0x1}};
    class ComClientSettingsResult :
        public IFabricClientSettingsResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComClientSettingsResult)

        BEGIN_COM_INTERFACE_LIST(ComClientSettingsResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricClientSettingsResult)
            COM_INTERFACE_ITEM(IID_IFabricClientSettingsResult, IFabricClientSettingsResult)
            COM_INTERFACE_ITEM(CLSID_ComClientSettingsResult, ComClientSettingsResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComClientSettingsResult(ServiceModel::FabricClientSettings const & settings);
        virtual ~ComClientSettingsResult();

        // 
        // IFabricClientSettingsResult methods
        // 
        const FABRIC_CLIENT_SETTINGS * STDMETHODCALLTYPE get_Settings();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_CLIENT_SETTINGS> settings_;
    };
}
