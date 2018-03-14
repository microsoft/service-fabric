// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5E5FAB15-FE91-401D-AE5B-FC5DDB6E3D2D}
    static const GUID CLSID_ComFabricRestartNodeResult = 
    { 0x5e5fab15, 0xfe91, 0x401d, { 0xae, 0x5b, 0xfc, 0x5d, 0xdb, 0x6e, 0x3d, 0x2d } };

    class ComFabricRestartNodeResult
        : public IFabricRestartNodeResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricRestartNodeResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricRestartNodeResult)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricRestartNodeResult)
        COM_INTERFACE_ITEM(IID_IFabricRestartNodeResult, IFabricRestartNodeResult)
        COM_INTERFACE_ITEM(CLSID_ComFabricRestartNodeResult, ComFabricRestartNodeResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricRestartNodeResult(
            IRestartNodeResultPtr const &impl);

        IRestartNodeResultPtr const & get_Impl() const { return impl_; }

        FABRIC_NODE_RESULT * STDMETHODCALLTYPE get_Result();

    private:
        IRestartNodeResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
