// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // {723D157A-B0C6-485F-BE3C-FB87E3A04B97}
    static const GUID CLSID_ComHostingSettingsResult =
        { 0x723d157a, 0xb0c6, 0x485f,{ 0xbe, 0x3c, 0xfb, 0x87, 0xe3, 0xa0, 0x4b, 0x97 }};
    
    class ComHostingSettingsResult :
        public IFabricHostingSettingsResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComHostingSettingsResult)

        BEGIN_COM_INTERFACE_LIST(ComHostingSettingsResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricHostingSettingsResult)
            COM_INTERFACE_ITEM(IID_IFabricHostingSettingsResult, IFabricHostingSettingsResult)
            COM_INTERFACE_ITEM(CLSID_ComHostingSettingsResult, ComHostingSettingsResult)
        END_COM_INTERFACE_LIST()

    public:
        ComHostingSettingsResult();
        virtual ~ComHostingSettingsResult();

        // 
        // IFabricClientSettingsResult methods
        // 
        const FABRIC_HOSTING_SETTINGS * STDMETHODCALLTYPE get_Result();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_HOSTING_SETTINGS> settings_;
    };
}
