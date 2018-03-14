// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {D6FBDF26-A66C-4245-ADA5-1D4FF61E551D}
    static const GUID CLSID_ComFabricStopNodeResult =
    { 0xd6fbdf26, 0xa66c, 0x4245, { 0xad, 0xa5, 0x1d, 0x4f, 0xf6, 0x1e, 0x55, 0x1d } };

    class ComFabricStopNodeResult
        : public IFabricStopNodeResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricStopNodeResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricStopNodeResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStopNodeResult)
            COM_INTERFACE_ITEM(IID_IFabricStopNodeResult, IFabricStopNodeResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricStopNodeResult, ComFabricStopNodeResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricStopNodeResult(
            IStopNodeResultPtr const &impl);

        IStopNodeResultPtr const & get_Impl() const { return impl_; }

        FABRIC_NODE_RESULT * STDMETHODCALLTYPE get_Result();

    private:
        IStopNodeResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
