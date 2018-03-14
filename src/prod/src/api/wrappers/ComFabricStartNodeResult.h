// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {39D8B85C-787D-4A2B-89B2-33CE6549F3B8}
    static const GUID CLSID_ComFabricStartNodeResult =
    { 0x39d8b85c, 0x787d, 0x4a2b, { 0x89, 0xb2, 0x33, 0xce, 0x65, 0x49, 0xf3, 0xb8 } };

    class ComFabricStartNodeResult
        : public IFabricStartNodeResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricStartNodeResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricStartNodeResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStartNodeResult)
            COM_INTERFACE_ITEM(IID_IFabricStartNodeResult, IFabricStartNodeResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricStartNodeResult, ComFabricStartNodeResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricStartNodeResult(
            IStartNodeResultPtr const &impl);

        IStartNodeResultPtr const & get_Impl() const { return impl_; }

        FABRIC_NODE_RESULT * STDMETHODCALLTYPE get_Result();

    private:
        IStartNodeResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
