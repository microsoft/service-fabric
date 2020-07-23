// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <Common/KtlSF.Common.cpp>

HRESULT CreateComponent(__out IKtlInteropTestComponent ** component)
{
    ErrorCode error = Common::ComUtility::CheckMTA();
    if (!error.IsSuccess())
    {
        return Common::ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    if (nullptr == component)
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<IKtlInteropTestComponent> created;

    KtlSystem * system = &Common::GetSFDefaultKtlSystem();
    ASSERT_IF(nullptr == system, "no default ktl system");
    
    HRESULT hr = KtlInteropTest::ComComponent::Create(system->GlobalPagedAllocator(), created);

    if (SUCCEEDED(hr))
    {
        *component = created.DetachNoRelease();
    }

    return Common::ComUtility::OnPublicApiReturn(hr);
}
