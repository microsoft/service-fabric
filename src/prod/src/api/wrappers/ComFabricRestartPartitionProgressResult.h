// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    //aaaddf05-13d0-4a88-b9fc-60550bddb36a
    static const GUID CLSID_ComFabricRestartPartitionProgressResult =
    { 0xaaaddf05, 0x13d0, 0x4a88, { 0xb9, 0xfc, 0x60, 0x55, 0x0b, 0xdd, 0xb3, 0x6a } };

    class ComFabricRestartPartitionProgressResult
		: public IFabricPartitionRestartProgressResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricRestartPartitionProgressResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricRestartPartitionProgressResult)
			COM_INTERFACE_ITEM(IID_IUnknown, IFabricPartitionRestartProgressResult)
			COM_INTERFACE_ITEM(IID_IFabricPartitionRestartProgressResult, IFabricPartitionRestartProgressResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricRestartPartitionProgressResult, ComFabricRestartPartitionProgressResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricRestartPartitionProgressResult(
            IRestartPartitionProgressResultPtr const &impl);

        IRestartPartitionProgressResultPtr const & get_Impl() const { return impl_; }

        FABRIC_PARTITION_RESTART_PROGRESS * STDMETHODCALLTYPE get_Progress();

    private:
        IRestartPartitionProgressResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
