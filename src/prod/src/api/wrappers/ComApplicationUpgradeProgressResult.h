// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    //81b4e3e4-ec56-4f75-99aa-f0eb96c967f6
    static const GUID CLSID_ComApplicationUpgradeProgressResult =
    {0x81b4e3e4,0xec56,0x4f75,{0x99,0xaa,0xf0,0xeb,0x96,0xc9,0x67,0xf6}};

    class ComApplicationUpgradeProgressResult
        : public IFabricApplicationUpgradeProgressResult3
        , public Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComApplicationUpgradeProgressResult);

        BEGIN_COM_INTERFACE_LIST(ComApplicationUpgradeProgressResult)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricApplicationUpgradeProgressResult3)
        COM_INTERFACE_ITEM(IID_IFabricApplicationUpgradeProgressResult, IFabricApplicationUpgradeProgressResult)
        COM_INTERFACE_ITEM(IID_IFabricApplicationUpgradeProgressResult2, IFabricApplicationUpgradeProgressResult2)
        COM_INTERFACE_ITEM(IID_IFabricApplicationUpgradeProgressResult3, IFabricApplicationUpgradeProgressResult3)
        COM_INTERFACE_ITEM(CLSID_ComApplicationUpgradeProgressResult, ComApplicationUpgradeProgressResult)
        END_COM_INTERFACE_LIST()

    public:
        ComApplicationUpgradeProgressResult(
            IApplicationUpgradeProgressResultPtr const &impl);

        IApplicationUpgradeProgressResultPtr const & get_Impl() const { return impl_; }

        FABRIC_URI STDMETHODCALLTYPE get_ApplicationName();

        FABRIC_URI STDMETHODCALLTYPE get_ApplicationTypeName();

        LPCWSTR STDMETHODCALLTYPE get_TargetApplicationTypeVersion();

        FABRIC_APPLICATION_UPGRADE_STATE STDMETHODCALLTYPE get_UpgradeState();

        HRESULT STDMETHODCALLTYPE GetUpgradeDomains(
            __out ULONG * itemCount,
            __out const FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION ** value);

        HRESULT STDMETHODCALLTYPE GetChangedUpgradeDomains(
            __in IFabricApplicationUpgradeProgressResult * previousProgress,
            __out ULONG * itemCount,
            __out const FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION ** bufferedItems);

        FABRIC_ROLLING_UPGRADE_MODE STDMETHODCALLTYPE get_RollingUpgradeMode();

        LPCWSTR STDMETHODCALLTYPE get_NextUpgradeDomain();

        FABRIC_APPLICATION_UPGRADE_PROGRESS * STDMETHODCALLTYPE get_UpgradeProgress();

        uint64 GetUpgradeInstance();

    private:
        IApplicationUpgradeProgressResultPtr impl_;
        std::wstring cachedApplicationName_;
        Common::ScopedHeap heap_;
    };
}
