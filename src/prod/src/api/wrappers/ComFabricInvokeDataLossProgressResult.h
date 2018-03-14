// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    //b2c67fc0-e55e-46e4-980a-d48082d01b35
    static const GUID CLSID_ComFabricInvokeDataLossProgressResult =
    { 0xb2c67fc0, 0xe55e, 0x46e4, {0x98, 0x0a, 0xd4, 0x80, 0x82, 0xd0, 0x1b, 0x35} };

    class ComFabricInvokeDataLossProgressResult
		: public IFabricPartitionDataLossProgressResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricInvokeDataLossProgressResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricInvokeDataLossProgressResult)
			COM_INTERFACE_ITEM(IID_IUnknown, IFabricPartitionDataLossProgressResult)
			COM_INTERFACE_ITEM(IID_IFabricPartitionDataLossProgressResult, IFabricPartitionDataLossProgressResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricInvokeDataLossProgressResult, ComFabricInvokeDataLossProgressResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricInvokeDataLossProgressResult(
            IInvokeDataLossProgressResultPtr const &impl);

        IInvokeDataLossProgressResultPtr const & get_Impl() const { return impl_; }

        FABRIC_PARTITION_DATA_LOSS_PROGRESS * STDMETHODCALLTYPE get_Progress();

    private:
        IInvokeDataLossProgressResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
