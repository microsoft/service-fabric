// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {B1623D76-5186-4213-B162-DE9FB0607B59}
    static const GUID CLSID_ComFabricMoveSecondaryResult =
    { 0xb1623d76, 0x5186, 0x4213, { 0xb1, 0x62, 0xde, 0x9f, 0xb0, 0x60, 0x7b, 0x59 } };


    class ComFabricMoveSecondaryResult
        : public IFabricMoveSecondaryResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricMoveSecondaryResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricMoveSecondaryResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricMoveSecondaryResult)
            COM_INTERFACE_ITEM(IID_IFabricMoveSecondaryResult, IFabricMoveSecondaryResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricMoveSecondaryResult, ComFabricMoveSecondaryResult)
            END_COM_INTERFACE_LIST()

    public:
        ComFabricMoveSecondaryResult(
            IMoveSecondaryResultPtr const &impl);

        IMoveSecondaryResultPtr const & get_Impl() const { return impl_; }

        FABRIC_MOVE_SECONDARY_RESULT * STDMETHODCALLTYPE get_Result();

    private:
        IMoveSecondaryResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
