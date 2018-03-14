// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // 5d9a642c-cbfe-47f3-88a1-3379edf0554b 
    static const GUID CLSID_ComApplicationHealthResult = 
        {0x5d9a642c, 0xcbfe, 0x47f3, {0x88, 0xa1, 0x33, 0x79, 0xed, 0xf0, 0x55, 0x4b}};

    class ComApplicationHealthResult :
        public IFabricApplicationHealthResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComApplicationHealthResult)

        BEGIN_COM_INTERFACE_LIST(ComApplicationHealthResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricApplicationHealthResult)
            COM_INTERFACE_ITEM(IID_IFabricApplicationHealthResult, IFabricApplicationHealthResult)
            COM_INTERFACE_ITEM(CLSID_ComApplicationHealthResult, ComApplicationHealthResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComApplicationHealthResult(ServiceModel::ApplicationHealth const & applicationHealth);
        virtual ~ComApplicationHealthResult();

        // 
        // IFabricApplicationHealthResult methods
        // 
        const FABRIC_APPLICATION_HEALTH * STDMETHODCALLTYPE get_ApplicationHealth();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_APPLICATION_HEALTH> applicationHealth_;
    };
}
