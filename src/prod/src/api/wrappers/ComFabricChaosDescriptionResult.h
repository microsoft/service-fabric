//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Api
{
    // {f06b557a-acb8-483e-8ee2-b571d8ff517a}
    static const GUID CLSID_ComFabricChaosDescriptionResult =
    { 0xf06b557a, 0xacb8, 0x483e, { 0x8e, 0xe2, 0xb5, 0x71, 0xd8, 0xff, 0x51, 0x7a } };

    // com wrapper around implementation of ChaosDescriptionResultDescription (see src\prod\src\client\ChaosDescriptionResult.h)
    class ComFabricChaosDescriptionResult
        : public IFabricChaosDescriptionResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricChaosDescriptionResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricChaosDescriptionResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricChaosDescriptionResult)
            COM_INTERFACE_ITEM(IID_IFabricChaosDescriptionResult, IFabricChaosDescriptionResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricChaosDescriptionResult, ComFabricChaosDescriptionResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricChaosDescriptionResult(IChaosDescriptionResultPtr const &impl);

        IChaosDescriptionResultPtr const & get_Impl() const { return impl_; }

        FABRIC_CHAOS_DESCRIPTION * STDMETHODCALLTYPE get_ChaosDescriptionResult();

    private:
        IChaosDescriptionResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
