// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define LOCK_MODE_TAG 'mmLI'

class LockModeComparer
    : public KObject<LockModeComparer>
    , public KShared<LockModeComparer>
    , public Data::IComparer<Data::TStore::LockMode::Enum>
{
    K_FORCE_SHARED(LockModeComparer)
        K_SHARED_INTERFACE_IMP(IComparer)

public:
    static NTSTATUS Create(
        __in KAllocator & allocator,
        __out IComparer<Data::TStore::LockMode::Enum>::SPtr & result);

    int Compare(__in const Data::TStore::LockMode::Enum& x, __in const Data::TStore::LockMode::Enum& y) const override;
};
