// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {CEA695BD-281E-4198-8789-D0485CE59B41}
    static const GUID CLSID_ComFabricRestartDeployedCodePackageResult =
    { 0xcea695bd, 0x281e, 0x4198, { 0x87, 0x89, 0xd0, 0x48, 0x5c, 0xe5, 0x9b, 0x41 } };

    class ComFabricRestartDeployedCodePackageResult
        : public IFabricRestartDeployedCodePackageResult
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricRestartDeployedCodePackageResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricRestartDeployedCodePackageResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricRestartDeployedCodePackageResult)
            COM_INTERFACE_ITEM(IID_IFabricRestartDeployedCodePackageResult, IFabricRestartDeployedCodePackageResult)
            COM_INTERFACE_ITEM(CLSID_ComFabricRestartDeployedCodePackageResult, ComFabricRestartDeployedCodePackageResult)
            END_COM_INTERFACE_LIST()

    public:
        ComFabricRestartDeployedCodePackageResult(
            IRestartDeployedCodePackageResultPtr const &impl);

        IRestartDeployedCodePackageResultPtr const & get_Impl() const { return impl_; }

        FABRIC_DEPLOYED_CODE_PACKAGE_RESULT * STDMETHODCALLTYPE get_Result();

    private:
        IRestartDeployedCodePackageResultPtr impl_;
        Common::ScopedHeap heap_;
    };
}
