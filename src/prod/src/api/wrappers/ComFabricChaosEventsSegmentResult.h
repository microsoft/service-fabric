//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Api
{
    // {80D7955F-A492-48B2-92DC-D597F8481E17}
    static const GUID CLSID_ComFabricChaosEventsSegmentResult =
    { 0x80d7955f, 0xa492, 0x48b2, { 0x92, 0xdc, 0xd5, 0x97, 0xf8, 0x48, 0x1e, 0x17 } };

    // com wrapper around ChaosEventsSegmentResult (see src\prod\src\client\ChaosEventsSegmentResult.h)
    class ComFabricChaosEventsSegmentResult
        : public IFabricChaosEventsSegmentResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricChaosEventsSegmentResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricChaosEventsSegmentResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricChaosEventsSegmentResult)
            COM_INTERFACE_ITEM(IID_IFabricChaosEventsSegmentResult, IFabricChaosEventsSegmentResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricChaosEventsSegmentResult, ComFabricChaosEventsSegmentResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricChaosEventsSegmentResult(IChaosEventsSegmentResultPtr const &impl);

        IChaosEventsSegmentResultPtr const & get_Impl() const { return impl_; }

        FABRIC_CHAOS_EVENTS_SEGMENT * STDMETHODCALLTYPE get_ChaosEventsSegmentResult();

    private:
        IChaosEventsSegmentResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
