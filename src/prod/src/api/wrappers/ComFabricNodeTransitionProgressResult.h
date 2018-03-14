// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {E702B1D6-2E48-4897-8418-DA53A5E9AE7E}
    static const GUID CLSID_ComFabricNodeTransitionProgressResult = 
    { 0xe702b1d6, 0x2e48, 0x4897, { 0x84, 0x18, 0xda, 0x53, 0xa5, 0xe9, 0xae, 0x7e } };

    class ComFabricNodeTransitionProgressResult
    : public IFabricNodeTransitionProgressResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricNodeTransitionProgressResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricNodeTransitionProgressResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricNodeTransitionProgressResult)
            COM_INTERFACE_ITEM(IID_IFabricNodeTransitionProgressResult, IFabricNodeTransitionProgressResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricNodeTransitionProgressResult, ComFabricNodeTransitionProgressResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricNodeTransitionProgressResult(
            INodeTransitionProgressResultPtr const &impl);

        INodeTransitionProgressResultPtr const & get_Impl() const { return impl_; }

        FABRIC_NODE_TRANSITION_PROGRESS * STDMETHODCALLTYPE get_Progress();

    private:
        INodeTransitionProgressResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
