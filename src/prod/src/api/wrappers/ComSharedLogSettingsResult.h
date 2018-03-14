// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {459B5F67-7BE3-41E2-BA38-654E6CB4B07B}
    static const GUID CLSID_ComSharedLogSettingsResult= 
        {0x459B5F67,0x7BE3,0x41E2,{0xBA,0x38,0x65,0x4E,0x6C,0xB4,0xB0,0x7B}};

    class ComSharedLogSettingsResult :
        public IFabricSharedLogSettingsResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComSharedLogSettingsResult)

        BEGIN_COM_INTERFACE_LIST(ComSharedLogSettingsResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricSharedLogSettingsResult)
            COM_INTERFACE_ITEM(IID_IFabricSharedLogSettingsResult, IFabricSharedLogSettingsResult)
            COM_INTERFACE_ITEM(CLSID_ComSharedLogSettingsResult, ComSharedLogSettingsResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComSharedLogSettingsResult(ISharedLogSettingsResultPtr const & impl);
        virtual ~ComSharedLogSettingsResult();

        const KTLLOGGER_SHARED_LOG_SETTINGS * STDMETHODCALLTYPE get_Settings();

    private:
        Common::ScopedHeap heap_;
        ISharedLogSettingsResultPtr impl_;
    };
}
