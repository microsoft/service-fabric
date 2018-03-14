// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    //f4e1cf21-8000-4427-bda5-a83b404c27f6
    static const GUID CLSID_ComFabricUpgradeProgressResult =
    { 0xf4e1cf21, 0x8000, 0x4427, {0xbd, 0xa5, 0xa8, 0x3b, 0x40, 0x4c, 0x27, 0xf6} };

    class ComFabricUpgradeProgressResult
        : public IFabricUpgradeProgressResult3
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComFabricUpgradeProgressResult);

        BEGIN_COM_INTERFACE_LIST(ComFabricUpgradeProgressResult)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricUpgradeProgressResult3)
        COM_INTERFACE_ITEM(IID_IFabricUpgradeProgressResult, IFabricUpgradeProgressResult)
        COM_INTERFACE_ITEM(IID_IFabricUpgradeProgressResult2, IFabricUpgradeProgressResult2)
        COM_INTERFACE_ITEM(IID_IFabricUpgradeProgressResult3, IFabricUpgradeProgressResult3)
        COM_INTERFACE_ITEM(CLSID_ComFabricUpgradeProgressResult, ComFabricUpgradeProgressResult)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricUpgradeProgressResult(
            IUpgradeProgressResultPtr const &impl);

        IUpgradeProgressResultPtr const & get_Impl() const { return impl_; }

        LPCWSTR STDMETHODCALLTYPE get_TargetCodeVersion();
        
        LPCWSTR STDMETHODCALLTYPE get_TargetConfigVersion();
        
        FABRIC_UPGRADE_STATE STDMETHODCALLTYPE get_UpgradeState();

        HRESULT STDMETHODCALLTYPE GetUpgradeDomains(
            __out ULONG * itemCount,
            __out const FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION ** value);

        HRESULT STDMETHODCALLTYPE GetChangedUpgradeDomains(
            __in IFabricUpgradeProgressResult * previousProgress,
            __out ULONG * itemCount,
            __out const FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION ** bufferedItems);

        FABRIC_ROLLING_UPGRADE_MODE STDMETHODCALLTYPE get_RollingUpgradeMode();

        LPCWSTR STDMETHODCALLTYPE get_NextUpgradeDomain();

        FABRIC_UPGRADE_PROGRESS * STDMETHODCALLTYPE get_UpgradeProgress();

    private:
        IUpgradeProgressResultPtr impl_;
        std::wstring cachedCodeVersion_;
        std::wstring cachedConfigVersion_;
        Common::ScopedHeap heap_;
    };
}
