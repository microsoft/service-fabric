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

ComApplicationUpgradeProgressResult::ComApplicationUpgradeProgressResult(
    IApplicationUpgradeProgressResultPtr const& impl)
    : impl_(impl)
    , cachedApplicationName_(impl_->GetApplicationName().ToString())
    , heap_()
{
}

FABRIC_URI STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::get_ApplicationName()
{
    return cachedApplicationName_.c_str();
}

FABRIC_URI STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::get_ApplicationTypeName()
{
    // This and other c_str() calls in this file were optimized to avoid copying strings, but 
    // this is also dangerous because it assumes the implementation will not return the address 
    // of a temporary string. Ensure that the implementation does not do this when modifying it.
    //
    return impl_->GetApplicationTypeName().c_str();
}

LPCWSTR STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::get_TargetApplicationTypeVersion()
{
    return impl_->GetTargetApplicationTypeVersion().c_str();
}

FABRIC_APPLICATION_UPGRADE_STATE STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::get_UpgradeState()
{
    return impl_->ToPublicUpgradeState();
}

HRESULT STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::GetUpgradeDomains(
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

HRESULT STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::GetChangedUpgradeDomains(
    __in IFabricApplicationUpgradeProgressResult * previousProgress,
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

    Common::ComPointer<ComApplicationUpgradeProgressResult> comPrevProgress(previousProgress, CLSID_ComApplicationUpgradeProgressResult); 
    IApplicationUpgradeProgressResultPtr const &prevProgressImpl = comPrevProgress->get_Impl();

    ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> referenceArray;
    auto error = impl_->ToPublicChangedUpgradeDomains(heap_, prevProgressImpl, referenceArray);

    if (error.IsSuccess())
    {
        *itemCount = static_cast<ULONG>(referenceArray.GetCount());
        *bufferedItems = referenceArray.GetRawArray();
    }

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

FABRIC_ROLLING_UPGRADE_MODE STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::get_RollingUpgradeMode()
{
    return RollingUpgradeMode::ToPublicApi(impl_->GetRollingUpgradeMode());
}

LPCWSTR STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::get_NextUpgradeDomain()
{
    return impl_->ToPublicNextUpgradeDomain();
}

FABRIC_APPLICATION_UPGRADE_PROGRESS * STDMETHODCALLTYPE ComApplicationUpgradeProgressResult::get_UpgradeProgress()
{
    auto progressPtr = heap_.AddItem<FABRIC_APPLICATION_UPGRADE_PROGRESS>();
    impl_->ToPublicApi(heap_, *progressPtr);

    return progressPtr.GetRawPointer();
}

uint64 ComApplicationUpgradeProgressResult::GetUpgradeInstance()
{
    return impl_->GetUpgradeInstance();
}
