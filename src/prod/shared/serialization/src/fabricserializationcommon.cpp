// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <Serialization.h>

using namespace Serialization;

KAllocator& FabricSerializationCommon::GetPagedKtlAllocator()
{
    static KAllocator* volatile alloc = nullptr;

    if (alloc != nullptr)
    {
        return *alloc;
    }

    alloc = &KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->PagedAllocator();

#if KTL_USER_MODE
    KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->SetEnableAllocationCounters(false);
#endif

    return *alloc;
}

KAllocator& FabricSerializationCommon::GetNonPagedKtlAllocator()
{
    static KAllocator* volatile alloc = nullptr;

    if (alloc != nullptr)
    {
        return *alloc;
    }

    alloc = &KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->NonPagedAllocator();

#if KTL_USER_MODE
    KtlSystemCoreImp::TryGetDefaultKtlSystemCore()->SetEnableAllocationCounters(false);
#endif

    return *alloc;
}
