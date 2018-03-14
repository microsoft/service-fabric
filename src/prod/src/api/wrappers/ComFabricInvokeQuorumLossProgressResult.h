// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    //7a7f96c4-e8e1-40fa-9c36-32fc0fc82464
    static const GUID CLSID_ComFabricInvokeQuorumLossProgressResult =
    { 0x7a7f96c4, 0xe8e1, 0x40fa, {0x9c, 0x36, 0x32, 0xfc, 0x0f, 0xc8, 0x24, 0x64} };

    class ComFabricInvokeQuorumLossProgressResult
		: public IFabricPartitionQuorumLossProgressResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricInvokeQuorumLossProgressResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricInvokeQuorumLossProgressResult)
			COM_INTERFACE_ITEM(IID_IUnknown, IFabricPartitionQuorumLossProgressResult)
			COM_INTERFACE_ITEM(IID_IFabricPartitionQuorumLossProgressResult, IFabricPartitionQuorumLossProgressResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricInvokeQuorumLossProgressResult, ComFabricInvokeQuorumLossProgressResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricInvokeQuorumLossProgressResult(
            IInvokeQuorumLossProgressResultPtr const &impl);

        IInvokeQuorumLossProgressResultPtr const & get_Impl() const { return impl_; }

        FABRIC_PARTITION_QUORUM_LOSS_PROGRESS * STDMETHODCALLTYPE get_Progress();

    private:
        IInvokeQuorumLossProgressResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
