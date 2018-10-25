//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Api
{
    // {f6a0e641-3c7f-46b2-a918-6703ddc40129}
    static const GUID CLSID_ComFabricChaosScheduleDescriptionResult =
    { 0xf6a0e641, 0x3c7f, 0x46b2, { 0xa9, 0x18, 0x67, 0x03, 0xdd, 0xc4, 0x01, 0x29 } };

    // com wrapper around ChaosScheduleDescriptionResult see (src\prod\src\client\ChaosScheduleDescriptionResult.h)
    class ComFabricChaosScheduleDescriptionResult
        : public IFabricChaosScheduleDescriptionResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricChaosScheduleDescriptionResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricChaosScheduleDescriptionResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricChaosScheduleDescriptionResult)
            COM_INTERFACE_ITEM(IID_IFabricChaosScheduleDescriptionResult, IFabricChaosScheduleDescriptionResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricChaosScheduleDescriptionResult, ComFabricChaosScheduleDescriptionResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricChaosScheduleDescriptionResult(IChaosScheduleDescriptionResultPtr const &impl);

        IChaosScheduleDescriptionResultPtr const & get_Impl() const { return impl_; }

        FABRIC_CHAOS_SCHEDULE_DESCRIPTION * STDMETHODCALLTYPE get_ChaosScheduleDescriptionResult();

    private:
        IChaosScheduleDescriptionResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
