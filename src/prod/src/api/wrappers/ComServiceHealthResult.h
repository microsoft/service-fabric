// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // f67f9d17-8b0f-4ffa-902f-0566c4aa9c4b 
    static const GUID CLSID_ComServiceHealthResult = 
        {0xf67f9d17, 0x8b0f, 0x4ffa, {0x90, 0x2f, 0x05, 0x66, 0xc4, 0xaa, 0x9c, 0x4b}};

    class ComServiceHealthResult :
        public IFabricServiceHealthResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComServiceHealthResult)

        BEGIN_COM_INTERFACE_LIST(ComServiceHealthResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricServiceHealthResult)
            COM_INTERFACE_ITEM(IID_IFabricServiceHealthResult, IFabricServiceHealthResult)
            COM_INTERFACE_ITEM(CLSID_ComServiceHealthResult, ComServiceHealthResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComServiceHealthResult(ServiceModel::ServiceHealth const & serviceHealth);
        virtual ~ComServiceHealthResult();

        // 
        // IFabricServiceHealthResult methods
        // 
        const FABRIC_SERVICE_HEALTH * STDMETHODCALLTYPE get_ServiceHealth();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_SERVICE_HEALTH> serviceHealth_;
    };
}
