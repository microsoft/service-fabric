// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using Common::ComUtility;

using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

ComFabricUpgradeProgressResult::ComFabricUpgradeProgressResult(
    IUpgradeProgressResultPtr const& impl)
    : impl_(impl)
    , cachedCodeVersion_(impl_->GetTargetCodeVersion().ToString())
    , cachedConfigVersion_(impl_->GetTargetConfigVersion().ToString())
    , heap_()
{
}

FABRIC_URI STDMETHODCALLTYPE ComFabricUpgradeProgressResult::get_TargetCodeVersion()
{
    return cachedCodeVersion_.c_str();
}

FABRIC_URI STDMETHODCALLTYPE ComFabricUpgradeProgressResult::get_TargetConfigVersion()
{
    return cachedConfigVersion_.c_str();
}

FABRIC_UPGRADE_STATE STDMETHODCALLTYPE ComFabricUpgradeProgressResult::get_UpgradeState()
{
    return impl_->ToPublicUpgradeState();
}

HRESULT STDMETHODCALLTYPE ComFabricUpgradeProgressResult::GetUpgradeDomains(
    __out ULONG * itemCount,
    __out const FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION ** value)
{
    if (itemCount == NULL || value == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> referenceArray;
    impl_->ToPublicUpgradeDomains(heap_, referenceArray);

    *itemCount = static_cast<ULONG>(referenceArray.GetCount());
    *value = referenceArray.GetRawArray();

    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComFabricUpgradeProgressResult::GetChangedUpgradeDomains(
    __in IFabricUpgradeProgressResult * previousProgress,
    __out ULONG * itemCount,
    __out const FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION ** bufferedItems)
{
    if (itemCount == NULL || bufferedItems == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (previousProgress == NULL)
    {
        return this->GetUpgradeDomains(itemCount, bufferedItems);
    }

    Common::ComPointer<ComFabricUpgradeProgressResult> comPrevProgress(previousProgress, CLSID_ComFabricUpgradeProgressResult); 
    IUpgradeProgressResultPtr const &prevProgressImpl = comPrevProgress->get_Impl();

    ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> referenceArray;
    auto error = impl_->ToPublicChangedUpgradeDomains(heap_, prevProgressImpl, referenceArray);

    if (error.IsSuccess())
    {
        *itemCount = static_cast<ULONG>(referenceArray.GetCount());
        *bufferedItems = referenceArray.GetRawArray();
    }

    return ComUtility::OnPublicApiReturn(S_OK);
}

FABRIC_ROLLING_UPGRADE_MODE STDMETHODCALLTYPE ComFabricUpgradeProgressResult::get_RollingUpgradeMode()
{
    return RollingUpgradeMode::ToPublicApi(impl_->GetRollingUpgradeMode());
}

LPCWSTR STDMETHODCALLTYPE ComFabricUpgradeProgressResult::get_NextUpgradeDomain()
{
    return impl_->ToPublicNextUpgradeDomain();
}

FABRIC_UPGRADE_PROGRESS * STDMETHODCALLTYPE ComFabricUpgradeProgressResult::get_UpgradeProgress()
{
    auto progressPtr = heap_.AddItem<FABRIC_UPGRADE_PROGRESS>();
    impl_->ToPublicApi(heap_, *progressPtr);

    return progressPtr.GetRawPointer();
}
