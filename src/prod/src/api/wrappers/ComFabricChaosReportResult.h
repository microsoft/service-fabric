// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {2A202C83-C907-4958-A9BF-41FC022C8288}
    static const GUID CLSID_ComFabricChaosReportResult = 
    { 0x2a202c83, 0xc907, 0x4958, { 0xa9, 0xbf, 0x41, 0xfc, 0x2, 0x2c, 0x82, 0x88 } };

    class ComFabricChaosReportResult
        : public IFabricChaosReportResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricChaosReportResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricChaosReportResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricChaosReportResult)
            COM_INTERFACE_ITEM(IID_IFabricChaosReportResult, IFabricChaosReportResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricChaosReportResult, ComFabricChaosReportResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricChaosReportResult(
            IChaosReportResultPtr const &impl);

        IChaosReportResultPtr const & get_Impl() const { return impl_; }

        FABRIC_CHAOS_REPORT * STDMETHODCALLTYPE get_ChaosReportResult();

    private:
        IChaosReportResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
