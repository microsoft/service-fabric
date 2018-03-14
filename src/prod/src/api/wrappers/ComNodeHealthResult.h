// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // 0f45e218-0a6e-463c-a1ff-93a1ddd24e95 
    static const GUID CLSID_ComNodeHealthResult = 
        {0x0f45e218, 0x0a6e, 0x463c, {0xa1, 0xff, 0x93, 0xa1, 0xdd, 0xd2, 0x4e, 0x95}};

    class ComNodeHealthResult :
        public IFabricNodeHealthResult,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComNodeHealthResult)

        BEGIN_COM_INTERFACE_LIST(ComNodeHealthResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricNodeHealthResult)
            COM_INTERFACE_ITEM(IID_IFabricNodeHealthResult, IFabricNodeHealthResult)
            COM_INTERFACE_ITEM(CLSID_ComNodeHealthResult, ComNodeHealthResult)
        END_COM_INTERFACE_LIST()

    public:
        explicit ComNodeHealthResult(ServiceModel::NodeHealth const & nodeHealth);
        virtual ~ComNodeHealthResult();

        // 
        // IFabricNodeHealthResult methods
        // 
        const FABRIC_NODE_HEALTH * STDMETHODCALLTYPE get_NodeHealth();

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_NODE_HEALTH> nodeHealth_;
    };
}
