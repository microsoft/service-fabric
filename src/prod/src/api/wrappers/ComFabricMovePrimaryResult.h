// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {F0BFAEF0-54AE-4D90-BCF8-ADC100F01BE7}
    static const GUID CLSID_ComFabricMovePrimaryResult =
    { 0xf0bfaef0, 0x54ae, 0x4d90, { 0xbc, 0xf8, 0xad, 0xc1, 0x0, 0xf0, 0x1b, 0xe7 } };


    class ComFabricMovePrimaryResult
        : public IFabricMovePrimaryResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricMovePrimaryResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricMovePrimaryResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricMovePrimaryResult)
            COM_INTERFACE_ITEM(IID_IFabricMovePrimaryResult, IFabricMovePrimaryResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricMovePrimaryResult, ComFabricMovePrimaryResult)
            END_COM_INTERFACE_LIST()

    public:
        ComFabricMovePrimaryResult(
            IMovePrimaryResultPtr const &impl);

        IMovePrimaryResultPtr const & get_Impl() const { return impl_; }

        FABRIC_MOVE_PRIMARY_RESULT * STDMETHODCALLTYPE get_Result();

    private:
        IMovePrimaryResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
